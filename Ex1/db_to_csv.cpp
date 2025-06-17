#include "blockchain.h"
#include <iostream>

int main() {
    load_db("./blocks");
    export_to_csv("a.csv");
    std::cout << "Exported to a.csv\n";
    return 0;
}
