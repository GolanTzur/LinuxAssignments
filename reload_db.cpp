#include <cstdlib> // for system()
#include <iostream>

int main(int argc, char* argv[]) {
    int num_blocks = 5; // Default value

    if (argc == 2) {
        try {
            num_blocks = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "Invalid number provided, using default (5 blocks).\n";
        }
    } else if (argc > 2) {
        std::cout << "Usage: ./reload_db [number_of_blocks]\n";
        return 1;
    }

    std::cout << "Reloading database with " << num_blocks << " blocks...\n";

    std::string command = "./fetch_blocks.sh " + std::to_string(num_blocks);
    int ret = system(command.c_str());

    if (ret != 0)
        std::cout << "Failed to reload blocks.\n";
    else
        std::cout << "Reload completed.\n";

    return 0;
}

