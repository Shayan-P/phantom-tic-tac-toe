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
            const auto& arr = *it;
            for (size_t i = 0; i < arr.size(); ++i) {
                file << arr[i];
                if (i < arr.size() - 1) {
                    file << " "; // separate elements with a space
                }
            }
            file << "\n"; // new line after each array
        }

        std::cout << "done " << filename << std::endl;
    }
};
