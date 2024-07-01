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

int main()
{
    auto pp = relocate_dir("/alc");
    printf("%s\n", pp.path);
    return 0;
}