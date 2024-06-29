#include "blake3.h"
#include "types.hpp"

struct blake_hash
{
    u8 hash[BLAKE3_BLOCK_LEN];
};

int main()
{
    return 0;
}