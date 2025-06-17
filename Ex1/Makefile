CXX = g++
CXXFLAGS = -Wall -fPIC -Iinclude
LDFLAGS = -L. -lblockchain

SRC_DIR ?= .
INCLUDE_DIR = include

BLOCKCHAIN_LIB = libblockchain.so

PROGRAMS = print_all db_to_csv.out reload_db interactive block_finder.out

all: $(BLOCKCHAIN_LIB) $(PROGRAMS)

$(BLOCKCHAIN_LIB): $(SRC_DIR)/blockchain.cpp
	$(CXX) $(CXXFLAGS) -shared -o $(BLOCKCHAIN_LIB) $<

print_all: $(SRC_DIR)/print_all.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

db_to_csv.out: $(SRC_DIR)/db_to_csv.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

reload_db: $(SRC_DIR)/reload_db.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

interactive: $(SRC_DIR)/interactive.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

block_finder.out: $(SRC_DIR)/block_finder.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f *.o *.so $(PROGRAMS)
