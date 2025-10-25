#include "blockchain.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cctype>    // for isdigit, isspace
#include <algorithm> // for trimming

namespace fs = std::filesystem;

static std::vector<Block> blocks_db;

// Helper function to check if a string is a number
bool is_number(const std::string& s) {
    for (char c : s) {
        if (!std::isdigit(c)) return false;
    }
    return !s.empty();
}

// Helper function to trim leading/trailing spaces
std::string trim(const std::string& s) {
    std::string result = s;
    result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    result.erase(std::find_if(result.rbegin(), result.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), result.end());
    return result;
}

void load_db(const std::string& directory) {
    blocks_db.clear();
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            std::ifstream file(entry.path());
            if (file) {
                Block blk;
                std::string line;
                while (std::getline(file, line)) {
                    if (line.find("Hash:") == 0)
                        blk.hash = trim(line.substr(5));
                    else if (line.find("Height:") == 0) {
                        std::string height_str = trim(line.substr(7));
                        if (is_number(height_str))
                            blk.height = std::stoi(height_str);  // Height is int
                        else
                            blk.height = -1;
                    }
                    else if (line.find("Total:") == 0) {
                        std::string total_str = trim(line.substr(6));
                        if (is_number(total_str))
                            blk.total = std::stoll(total_str);    // <-- USE STOLL for large totals
                        else
                            blk.total = -1;
                    }
                    else if (line.find("Time:") == 0)
                        blk.time = trim(line.substr(5));
                    else if (line.find("Relayed by:") == 0)
                        blk.relayed_by = trim(line.substr(11));
                    else if (line.find("Previous Block:") == 0)
                        blk.prev_block = trim(line.substr(15));
                }
                blocks_db.push_back(blk);
            }
        }
    }
}

Block find_by_hash(const std::string& hash) {
    for (const auto& blk : blocks_db) {
        if (blk.hash == hash)
            return blk;
    }
    throw std::runtime_error("Block not found by hash!");
}

Block find_by_height(int height) {
    for (const auto& blk : blocks_db) {
        if (blk.height == height)
            return blk;
    }
    throw std::runtime_error("Block not found by height!");
}

std::vector<Block> get_all_blocks() {
    return blocks_db;
}

void print_block(const Block& blk) {
    std::cout << "Hash: " << blk.hash << "\n";
    std::cout << "Height: " << blk.height << "\n";
    std::cout << "Total: " << blk.total << "\n";  // <-- Total is now long long
    std::cout << "Time: " << blk.time << "\n";
    std::cout << "Relayed By: " << blk.relayed_by << "\n";
    std::cout << "Previous Block: " << blk.prev_block << "\n";
    std::cout << "--------------------------\n";
}

void export_to_csv(const std::string& filename) {
    std::ofstream file(filename);
    if (!file) {
        std::cerr << "Failed to open CSV file.\n";
        return;
    }
    file << "Hash,Height,Total,Time,Relayed By,Previous Block\n";
    for (const auto& blk : blocks_db) {
        file << blk.hash << "," << blk.height << "," << blk.total << ","
             << blk.time << "," << blk.relayed_by << "," << blk.prev_block << "\n";
    }
}

