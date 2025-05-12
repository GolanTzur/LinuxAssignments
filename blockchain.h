// blockchain.h
#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <string>
#include <vector>

struct Block {
    std::string hash;
    int height;
    long long total;   // <-- corrected to long long
    std::string time;
    std::string relayed_by;
    std::string prev_block;
};

void load_db(const std::string& directory);
Block find_by_hash(const std::string& hash);
Block find_by_height(int height);
std::vector<Block> get_all_blocks();
void print_block(const Block& blk);
void export_to_csv(const std::string& filename);

#endif // BLOCKCHAIN_H

