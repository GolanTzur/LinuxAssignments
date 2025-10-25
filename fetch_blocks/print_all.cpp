#include "blockchain.h"
#include <iostream>

int main() {
    load_db("./blocks");
    for (const auto& blk : get_all_blocks()) {
        print_block(blk);
    }
    return 0;
}