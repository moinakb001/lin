#include "blake3.h"
#include "store.hpp"
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

#include "store.cpp"

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
        return dest;
    }
    void *__strcat_chk(char *a, char *b, u64 len)
    {
        return strcat(a, b);
    }
};


int main( int argc, char ** argv)
{
    if (argc < 2)
    {
        return -1;
    }
    buf_t<u8> str = {(u8*) argv[1], store::strlen(argv[1])};
    auto pp = store::relocate_dir(AT_FDCWD, str);
    printf("%s\n", pp.path);
    return 0;
}