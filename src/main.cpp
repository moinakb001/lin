#define _FORTIFY_SOURCE 0
#include "blake3.h"
#include "types.hpp"
#include <cstring>
#include <fcntl.h> 
#include <sys/types.h>
#include <dirent.h>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <cstdio>

struct blake_hash_t
{
    u8 hash[BLAKE3_OUT_LEN];
};
struct store_path_t
{
    u8 path[ 5 /*sizeof("/lin/")*/ + (2 * BLAKE3_OUT_LEN) + 1 /* NULL terminator */];
};

store_path_t create_store_path(blake_hash_t hash)
{
    store_path_t ret;
    ret.path[0] = '/';
    ret.path[1] = 'l';
    ret.path[2] = 'i';
    ret.path[3] = 'n';
    ret.path[4] = '/';

    for(u64 i = 0; i < BLAKE3_OUT_LEN; i++)
    {
        const char *ca = "0123456789abcdef";
        ret.path[5 + (i << 1)] = ca[hash.hash[i] & 0xf];
        ret.path[6 + (i << 1)] = ca[(hash.hash[i] >> 4) & 0xf];
    }
    ret.path[sizeof(ret.path) - 1] = 0;
    return ret;
}

struct filesort_node_t;

struct filesort_node_t
{
    filesort_node_t *left, *right, *parent;
    blake_hash_t name;
    blake_hash_t contents;
};

extern "C" {
    void *__memcpy_chk(void *dest, const void *src,
              size_t copy_amount, size_t dest_len)
    {
        u8 *d = (u8*) dest;
        u8 *c = (u8*) src;
        for(u64 i = 0; i < copy_amount; i++)
        {
            d[i] = c[i];
        }
    }
    void *__strcat_chk(char *a, char *b, u64 len)
    {
        return strcat(a, b);
    }
};

void outhash(blake_hash_t hash)
{
    for(u64 i = 0; i < sizeof(hash.hash); i++)
    {
        printf("%02x", hash.hash[i]);
    }
    printf("\n");
}

blake_hash_t hash_string(u8 *x, u64 num)
{
    blake_hash_t ret;
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    blake3_hasher_update(&hasher, x, num);
    blake3_hasher_finalize(&hasher, (uint8_t *) ret.hash, sizeof(ret.hash));
    return ret;

}
blake_hash_t hash_char(char x)
{
    return hash_string((u8*)&x, 1);
}

u8 translate_char(char x)
{
    if(x >='a' && x <= 'z')
    {
        return (u8)(x - 'a');
    }
    if(x >='0' && x <= '9')
    {
        return (u8)(x - '0');
    }
    return 0;
}

blake_hash_t hash_file(char *name)
{
    blake_hash_t ret;
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    auto ftag = hash_char('f');
    blake3_hasher_update(&hasher, ftag.hash, sizeof(ftag.hash));
    struct stat statst;
    stat(name, &statst);
    auto fd = open(name, 0);
    u8 * buf = (u8*)mmap(0, statst.st_size, PROT_READ, MAP_PRIVATE ,fd , 0);
    blake3_hasher_update(&hasher, buf, statst.st_size);
    blake3_hasher_finalize(&hasher, (uint8_t *) ret.hash, sizeof(ret.hash));
    close(fd);
    return ret;
}

s8 hash_cmp(blake_hash_t s1, blake_hash_t s2)
{
    u64 *ptr1 = (u64*) &s1.hash;
    u64 *ptr2 = (u64*) &s2.hash;
    for(u64 i = 0; i < sizeof(s1.hash); i += 8)
    {
        if(ptr1[i] < ptr2[i]) return -1;
        if(ptr1[i] > ptr2[i]) return 1;
    }
    return 0;
}

blake_hash_t hash_dir(char *name)
{
    blake_hash_t ret;
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    auto dtag = hash_char('d');
    blake3_hasher_update(&hasher, dtag.hash, sizeof(dtag.hash));
    auto de = opendir(name);
    auto dent = readdir64(de);
    blake_hash_t blh;
    char xal[sizeof(blh) * 2 + 5 /*sizeof("/lin/")*/ + 1];
    filesort_node_t *node = NULL;
    u64 mdepth = 0;
    while (dent != NULL)
    {
        if((strcmp(dent->d_name, ".") == 0) || (strcmp(dent->d_name, "..") == 0))
        {
            dent = readdir64(de);
            continue;
        }
        filesort_node_t *next = (filesort_node_t *) malloc(sizeof(filesort_node_t));
        auto namehash = hash_string((u8*)dent->d_name, strlen(dent->d_name));
        
        readlinkat(dirfd(de), dent->d_name, xal, sizeof(xal));
        for(u64 i = 0; i < sizeof(blh); i++)
        {
            u8 *dat = (u8 *) &xal[5];
            blh.hash[i] = translate_char(dat[i << 1]) | (translate_char(dat[(i << 1) + 1]) << 4);
        }
        next->name = namehash;
        next->contents = blh;
        u64 depth = 1;
        filesort_node_t **aln = &node;
        filesort_node_t *pln = NULL;;
        while((*aln) != NULL)
        {
            pln = (*aln);
            depth++;
            s8 c1 = hash_cmp((*aln)->name, next->name);
            if(c1 == 0) c1 = hash_cmp((*aln)->contents, next->contents);
            aln = (c1 < 0) ? &((*aln)->left) : &((*aln)->right);
        }
        next->parent = pln;
        if (depth > mdepth) mdepth = depth;
        *aln = next;
        dent = readdir64(de);
    }
    u64 curptr = 0;
    filesort_node_t *pln = node;
    filesort_node_t * last = NULL;
    while (pln != NULL)
    {
        if(pln->left != NULL && last != pln->left && last != pln->right)
        {
            last = pln;
            pln = pln->left;
            continue;
        }
        if (last != pln->right || pln->right == NULL)
        {
            blake3_hasher_update(&hasher, pln->name.hash, sizeof(pln->name.hash));
            blake3_hasher_update(&hasher, pln->contents.hash, sizeof(pln->contents.hash));
        }
        if(pln->right != NULL && last != pln->right)
        {
            last = pln;
            pln = pln->right;
            continue;
        }
        auto tmp = pln->parent;
        last = pln;
        pln = tmp;
    }
exit:
    closedir(de);
    blake3_hasher_finalize(&hasher, (uint8_t *) ret.hash, sizeof(ret.hash));
    return ret;
}

store_path_t relocate_file(char *name)
{
    auto hash = hash_file(name);
    auto path = create_store_path(hash);
    rename(name, (char *)path.path);
    return path;
}
store_path_t relocate_dir(char *name)
{
    auto de = opendir(name);
    auto dent = readdir64(de);
    u64 clen = strlen(name);
    u64 csz = 10;
    char *arr = (char *) malloc(clen + 10 + 2);
    memcpy(arr, name, clen);
    arr[clen] = '/';
    arr[clen + 1] = 0;

    while(dent != NULL)
    {
        // TODO replace strcat
        if((strcmp(dent->d_name, ".") == 0) || (strcmp(dent->d_name, "..") == 0))
        {
            dent = readdir64(de);
            continue;
        }
        
        u64 curna = strlen(dent->d_name);
        if(curna > csz)
        {
            arr = (char *) realloc(arr, clen + curna + 2);
            csz = curna;
        }
        memcpy(&arr[clen + 1], dent->d_name, curna);
        arr[clen + 1 + curna] = 0;
        printf("%s\n", arr);
        switch(dent->d_type)
        {
            case DT_REG:
            {
                auto pathss = relocate_file(arr);
                symlink((char *) pathss.path, arr);
                break;
            }
            case DT_DIR:
            {
                auto pathss = relocate_dir(arr);
                symlink((char *) pathss.path, arr);
                break;
            }
        }
        arr[clen + 1] = 0;
        dent = readdir64(de);
    }

    auto hash = hash_dir(name);
    auto path = create_store_path(hash);
    rename(name, (char *)path.path);
    return path;
}

int main()
{
    auto pp = relocate_dir("/alc");
    printf("%s\n", pp.path);
    return 0;
}