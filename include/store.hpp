#pragma once
#include "blake3.h"
#include "types.hpp"

namespace store
{
    struct blake_hash_t
    {
        u8 hash[BLAKE3_OUT_LEN];
    };
    struct store_path_t
    {
        u8 path[ 5 /*sizeof("/lin/")*/ + (2 * BLAKE3_OUT_LEN) + 1 /* NULL terminator */];
    };
    struct filehash_metadata_t
    {
        blake_hash_t guid;
        s64          uid;
        s64          gid;
        s64          mode;
    }
    constexpr u8 translate_path_char(char x)
    {
        if(x >='a' && x <= 'z')name
        {
            return (u8)(x - 'a');
        }
        if(x >='0' && x <= '9')
        {
            return (u8)(x - '0');
        }
        return 0;
    }
    constexpr blake_hash_t path_to_hash(store_path_t path)
    {
        blake_hash_t ret;
        for(u64 i = 0; i < BLAKE3_OUT_LEN; i++)
        {
            u8 *dat = (u8 *) &path.path[5];
            ret.hash[i] = translate_path_char(dat[i << 1]) | (translate_path_char(dat[(i << 1) + 1]) << 4);
        }
        return ret;
    }
    constexpr store_path_t hash_to_path(blake_hash_t hash)
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

    constexpr s8 hash_cmp(blake_hash_t s1, blake_hash_t s2)
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

    static inline blake_hash_t hash_string(buf_t<u8> buf)
    {
        blake_hash_t ret;
        blake3_hasher hasher;
        blake3_hasher_init(&hasher);
        blake3_hasher_update(&hasher, x, num);
        blake3_hasher_finalize(&hasher, (uint8_t *) ret.hash, sizeof(ret.hash));
        return ret;

    }

    static inline blake_hash_t hash_char(char x)
    {
        return hash_string(buf_t<u8>{(u8*)&x, 1});
    }

    struct {
        blake_hash_t file;
        blake_hash_t dir;
    } guids;

    static inline void initialize_guids()
    {
        guids.file = hash_char('f');
        guids.dir = hash_char('d');
    }
    store_path_t relocate_file(s64 dirfd, buf_t<u8> path);
    store_path_t relocate_dir(s64 dirfd, buf_t<u8> path);
};
