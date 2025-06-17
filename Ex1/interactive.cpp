#include "blockchain.h"
#include <iostream>
#include <cstdlib>

int main() {
    load_db("./blocks");
    int choice;
    do {
        std::cout << "==== Blockchain Manager ====\n";
        std::cout << "1. Print DB\n";
        std::cout << "2. Print Block by Hash\n";
        std::cout << "3. Print Block by Height\n";
        std::cout << "4. Export to CSV\n";
        std::cout << "5. Refresh\n";
        std::cout << "0. Exit\n";
        std::cout << "Choice: ";
        std::cin >> choice;
        std::cin.ignore();

        try {
            if (choice == 1) {
                for (const auto& blk : get_all_blocks()) {
                    print_block(blk);
                }
            } else if (choice == 2) {
                std::string hash;
                std::cout << "Enter Hash: ";
                std::getline(std::cin, hash);
                print_block(find_by_hash(hash));
            } else if (choice == 3) {
                int height;
                std::cout << "Enter Height: ";
                std::cin >> height;
                print_block(find_by_height(height));
            } else if (choice == 4) {
                export_to_csv("blocks.csv");
                std::cout << "Exported to blocks.csv\n";
            } else if (choice == 5) {
                int num_blocks;
                 std::cout << "How many blocks to fetch? ";
                 std::cin >> num_blocks;
                std::cin.ignore();

                std::string command = "./fetch_blocks.sh " + std::to_string(num_blocks);
                 int ret = system(command.c_str());

                if (ret == 0)
                    std::cout << "Refresh successful!\n";
                else
                    std::cout << "Refresh failed!\n";
                load_db("./blocks");
            }
        } catch (const std::exception& e) {
            std::cerr << e.what() << "\n";
        }
    } while (choice != 0);

    return 0;
}
