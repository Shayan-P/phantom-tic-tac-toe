#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <assert.h>
#include <iostream>
#include <iomanip>

namespace io {
    // todo later if you want to make it faster create a trie data structure...
    void load_information_sets(const std::string &filename, std::vector<std::string> &info_sets_reprs) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file " + filename);
        }
        std::cout << "reading from file " << filename << std::endl;
        std::string line;
        while (std::getline(file, line)) {
            assert(line[0] == '|');
            assert(line.back() == '|' || line.back() == '.' || line.back() == '*');
            info_sets_reprs.push_back(line);
        }
        file.close();
        std::cout << "done " << filename << std::endl;
    }

    template <typename Iterator>
    void save_information_sets(const std::string &filename, Iterator itl, Iterator itr) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file " + filename);
        }

        std::cout << "writing to file " << filename << std::endl;

        for(auto it = itl; it != itr; ++it) {
            file << std::fixed << std::setprecision(12) << (*it) << "\n"; // adjust the precision
        }

        std::cout << "done " << filename << std::endl;
    }
};
