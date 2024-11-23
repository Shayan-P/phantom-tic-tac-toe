#ifndef IO_HPP
#define IO_HPP

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
    // 1 dim case
    template <typename T, typename Iterator>
    static void save_to_numpy(const std::string &filename, Iterator itl, Iterator itr) {
        std::cout << "save " << filename << std::endl;
        int N = itr - itl;
        xt::xarray<T> arr = xt::zeros<T>({N});
        for(int i = 0; i < N; i++) {
            arr(i) = *(itl + i);
        }
        xt::dump_npy(filename, arr);
        std::cout << "done " << filename << std::endl;
    }

    template <typename T, int DIM, typename Iterator>
    static void save_to_numpy(const std::string &filename, Iterator itl, Iterator itr) {
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
    static void load_from_numpy(const std::string &filename, Iterator itl) {
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

#endif