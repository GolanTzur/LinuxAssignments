#include "blockchain.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage:\n";
        std::cout << "./block_finder.out --hash <hash_value>\n";
        std::cout << "./block_finder.out --height <height_value>\n";
        return 1;
    }

    std::string option = argv[1];
    std::string value = argv[2];

    load_db("./blocks");

    try {
        if (option == "--hash") {
            Block blk = find_by_hash(value);
            print_block(blk);
        } else if (option == "--height") {
            int height = std::stoi(value);
            Block blk = find_by_height(height);
            print_block(blk);
        } else {
            std::cout << "Invalid option. Use --hash or --height.\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
    }

    return 0;
}

