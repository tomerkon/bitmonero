#include <atomic>
#include <cstdio>
#include <algorithm>
#include <fstream>

class bootfileloader {
public:
	 static bool load_from_raw_file(cryptonote::blockchain_storage* bcs, cryptonote::tx_memory_pool* _tx_pool, const std::string& raw_file_name);
};

