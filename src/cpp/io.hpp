#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <assert.h>
#include <iostream>
#include <iomanip>
#include "xtensor/xarray.hpp"
#include "xtensor-io/xnpz.hpp"

namespace io {
    // todo later if you want to make it faster create a trie data structure...
    template<typename PTTT_Infoset>
    void load_information_sets(const std::string &filename, std::vector<PTTT_Infoset> &info_sets_reprs) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file " + filename);
        }
        std::cout << "reading from file " << filename << std::endl;
        std::string line;
        while (std::getline(file, line)) {
            assert(line[0] == '|');
            assert(line.back() == '|' || line.back() == '.' || line.back() == '*');
            uint32_t mask = 0;
            for (size_t idx = 1; idx < line.size() - 1; idx += 2) {
                mask |= 1 << (line[idx] - '0');
            }
            info_sets_reprs.push_back({line, mask});
        }
        file.close();
        std::cout << "done " << filename << std::endl;
    }

    template <typename T, int DIM, typename Iterator>
    void save_to_numpy(const std::string &filename, Iterator itl, Iterator itr) {
        std::cout << "save " << filename << std::endl;
        int N = itr - itl;
        xt::xarray<T> arr = xt::zeros<T>({N, DIM});
        for(int i = 0; i < N; i++) {
            const auto& inner_arr = *(itl + i);
            assert(DIM == inner_arr.size());
            for (size_t j = 0; j < inner_arr.size(); ++j) {
                arr(i, j) = inner_arr[j];
            }
        }
        xt::dump_npy(filename, arr);
        std::cout << "done " << filename << std::endl;
    }

    template <typename T, int DIM, typename Iterator>
    void load_from_numpy(const std::string &filename, Iterator itl) {
        std::cout << "load " << filename << std::endl;
        auto loaded_arr = xt::load_npy<T>(filename);
        for(int i = 0; i < loaded_arr.shape()[0]; i++) {
            auto& inner_arr = *(itl + i);
            for(int j = 0; j < loaded_arr.shape()[1]; j++) {
                inner_arr[j] = loaded_arr(i, j);
            }
        }
        std::cout << "done " << filename << std::endl;
    }
};
