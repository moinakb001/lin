#include "store.hpp"
#include <fcntl.h> /* Definition of AT_* constants */
#include <sys/stat.h>

namespace store {
    struct file_hash_itm_t
    {
        s64 fd;
        u64 size;
        filehash_metadata_t meta;
    };
    struct filehash_node_t;
    struct filehash_node_t
    {
        filehash_node_t *left, *right;
        blake_hash_t name;
        blake_hash_t content;
    };
    static inline file_hash_itm_t get_file_header(s64 dirfd, buf_t<u8> path)
    {
        file_hash_itm_t ret;
        auto cstr = (char *) __builtin_alloca(path.num + 1);
        memcpy(cstr, path.arr, path.num);
        cstr[path.num] = 0;
        auto fd = openat((int)dirfd, cstr, 0);
        struct stat stats;
        fstatat(fd, "", &stats, AT_EMPTY_PATH);
        filehash_metadata_t meta;
        meta.gid = stats.st_gid;
        meta.uid = stats.st_uid;
        meta.mode = stats.st_mode;
        meta.guid = guids.file;
        ret.fd = fd;
        ret.meta = meta;
        ret.size = stats.st_size;
        return ret;
    }
    blake_hash_t hash_file(s64 dirfd, buf_t<u8> path)
    {
        blake_hash_t ret;
        blake3_hasher hasher;
        blake3_hasher_init(&hasher);
        auto head = get_file_header(dirfd, path);
        blake3_hasher_update(&hasher, &head.meta, sizeof(head.meta));
        u8 * buf = (u8*)mmap(0, head.size, PROT_READ, MAP_PRIVATE, head.fd, 0);
        blake3_hasher_update(&hasher, buf, head.size);
        munmap(buf, head.size);
        close(head.fd);
        blake3_hasher_finalize(&hasher, (uint8_t *) ret.hash, sizeof(ret.hash));
        return ret;
    }
    blake_hash_t hash_dir(s64 dirfd, buf_t<u8> path);
    store_path_t relocate_file(s64 dirfd, buf_t<u8> path)
    {
        auto cstr = (char *) __builtin_alloca(path.num + 1);
        memcpy(cstr, path.arr, path.num);
        cstr[path.num] = 0;
        auto hash = hash_file(dirfd, path);
        auto spath = hash_to_path(hash);
        renameat(dirfd, cstr, -1, (char *) spath.path);
        return spath;
    }
    u8 is_useless_path(char *path)
    {
        if(path[0] == 0) return 1;
        if(path[1] == 0){
            return path[0] == '.';
        }
        if(path[2] == 0)
        {
            return path[0] == '.' && path[1] == '.';
        }
        return 0;
    }
    u64 strlen(char *s)
    {
        u64 ret = 0;
        while(s[ret] != 0) ret++;
        return ret;
    }
    u64 insert_into_tree(filehash_node_t **tree, filehash_node_t node)
    {
        filehash_node_t *anode = (filehash_node_t *)malloc(sizeof(filehash_node_t));
        memcpy(anode, &node, sizeof(filehash_node_t));
        u64 inx = 1;
        while ((*tree) != NULL)
        {
            auto h1 = hash_cmp((*tree)->name, anode->name);
            if(h1 == 0) h1 = hash_cmp((*tree)->content, anode->content);
            if(h1 > 0) tree = &((*tree)->left);
            else tree = &((*tree)->right);
            inx++;
        }
        *tree = anode;
        return inx;
    }
    store_path_t relocate_dir(s64 dirfd, buf_t<u8> path)
    {
        blake_hash_t sshash;
        auto hdr = get_file_header(dirfd, path);
        auto dir = fdopendir(hdr.fd);
        auto dent = readdir64(dir);
        filehash_node_t *node = NULL;
        u64 height = 1;
        while(dent != NULL)
        {
            buf_t<u8> str;
            if(is_useless_path(dent->d_name)){
                dent = readdir64(dir);
                continue;
            }
            str.arr = (u8*)dent->d_name;
            str.num = strlen(dent->d_name);
            filehash_node_t fn{};
            fn.name = hash_string(str);
            switch(dent->d_type)
            {
                case DT_REG:
                {
                    auto path = relocate_file(hdr.fd, str);
                    symlinkat((char*)path.path, hdr.fd, (char *) str.arr);
                    fn.content = path_to_hash(path);
                    u64 ht = insert_into_tree(&node, fn);
                    height = height > ht ? height : ht;
                    break;
                }
                case DT_DIR:
                {
                    auto path = relocate_dir(hdr.fd, str);
                    symlinkat((char*)path.path, hdr.fd, (char*) str.arr);
                    fn.content = path_to_hash(path);
                    u64 ht = insert_into_tree(&node, fn);
                    height = height > ht ? height : ht;
                    break;
                }
            }
            dent = readdir64(dir);
        }
        blake3_hasher hasher;
        blake3_hasher_init(&hasher);
        blake3_hasher_update(&hasher, &hdr.meta, sizeof(hdr.meta));
        filehash_node_t *stack[height];
        u64 ptr = 1;
        stack[0] = node;
        filehash_node_t *prev = NULL;
        while(ptr != 0)
        {
            auto cur = stack[ptr - 1];
            u8 nl = (prev != cur->left) || (!cur->left);
            u8 nr = (prev != cur->right) || (!cur->right);
            if(cur->left && nr && nl)
            {
                stack[ptr] = cur->left;
                ptr++;
                prev = cur;
                continue;
            }
            if (nr)
            {
                blake3_hasher_update(&hasher, cur->name.hash, sizeof(cur->name.hash));
                blake3_hasher_update(&hasher, cur->content.hash, sizeof(cur->content.hash));
                if(cur->right)
                {
                    stack[ptr] = cur->right;
                    ptr++;
                    prev = cur;
                    continue;
                }
            }
            prev = cur;
            ptr--;
            free(prev);
        }
        blake3_hasher_finalize(&hasher, (uint8_t *) sshash.hash, sizeof(sshash));
        auto cstr = (char *) __builtin_alloca(path.num + 1);
        memcpy(cstr, path.arr, path.num);
        cstr[path.num] = 0;
        auto sspath = hash_to_path(sshash);
        renameat(dirfd, cstr, -1, (char *) sspath.path);
        return sspath;
    }
}