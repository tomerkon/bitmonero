#include <atomic>
#include <cstdio>
#include <algorithm>
#include <fstream>

class bootfileloader {
public:
	 static bool load_from_raw_file(void* bcs, const std::string& raw_file_name);
};

