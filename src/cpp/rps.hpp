
#pragma once

#include <assert.h>
#include <cstdint>
#include <iostream>
#include <cstring>
#include <vector>
#include <array>
#include "io.hpp"
#include <filesystem>
#include <string>
#include "pttt_paths.hpp"
#include "pttt_game_dynamics.hpp"

namespace rps {
    class RPS {
    public:
        enum Player { P1, P2 };
    private:
        bool done = false;
        Player winner;
        bool tie = false;
        int p1_move = -1;
        int p2_move = -1;

    public:
        static constexpr int ACTION_MAX_DIM = 3;
        static constexpr int NUM_INFO_SETS = 2;
        static constexpr int NUM_PLAYERS = 2;

        static const std::array<Player, NUM_PLAYERS> players;

        using ActionInt = int;

        using T = double;
        // using Player = pttt::Player;
        using Buffer = std::array<T, ACTION_MAX_DIM>;
        using ActionInts = std::array<ActionInt, ACTION_MAX_DIM>;

        bool is_terminal() const {
            return p1_move != -1 && p2_move != -1;
        }

        T utility(Player player) const {
            assert(done);
            if(tie)
                return 0;
            return winner == player ? 1 : -1;
        }

        bool is_chance() const {
            return false; // no chance node here...
        }

        void action_probs(Buffer& buffer) const {
            assert(false); // since we don't have chance nodes, this should not be called...
        }

        int num_actions() const {
            return 3;
        }

        void step(ActionInt action_) {
            assert(!done);
            if(p1_move == -1) {
                p1_move = action_;
            } else {
                p2_move = action_;
            }
            if(p1_move != -1 && p2_move != -1) {
                done = true;
                if(p1_move == p2_move) {
                    tie = true;
                } else if((p1_move + 1) % 3 == p2_move) {
                    winner = Player::P2;
                } else {
                    winner = Player::P1;
                }
            }
        }

        void actions(ActionInts &buffer) const { // don't use vector or dynamic memory
            buffer[0] = 0;
            buffer[1] = 1;
            buffer[2] = 2;
        }

        inline Player current_player() const {
            return p1_move == -1 ? Player::P1 : Player::P2;
        }

        int info_set_idx() const {
            return p1_move == -1 ? 0 : 1;
        }

        friend std::ostream& operator<<(std::ostream& os, const RPS& game);

    private:

    public:
        static std::vector<std::array<T, ACTION_MAX_DIM>> get_strategy(const std::vector<std::array<T, ACTION_MAX_DIM>> &average_policy) {
            std::vector<std::array<T, ACTION_MAX_DIM>> result(NUM_INFO_SETS);
            for(int idx = 0; idx < NUM_INFO_SETS; idx++) {
                result[idx] = average_policy[idx];
                T sm = 0;
                for(int j = 0; j < ACTION_MAX_DIM; j++) {
                    sm += average_policy[idx][j];
                }
                if (sm <= 1e-9) {
                    // assign uniform distribution
                    int num_valid_actions = ACTION_MAX_DIM;
                    sm = num_valid_actions;
                    for(int j = 0; j < ACTION_MAX_DIM; j++) {
                        result[idx][j] = 1.0;
                    }
                }
                for(int j = 0; j < ACTION_MAX_DIM; j++) {
                    result[idx][j] /= sm;
                }
            }
            return result;
        }

        static void save_strategy_to_file(const std::string &name, std::vector<std::array<T, ACTION_MAX_DIM>> &average_policy) {
            std::cout << "not implemented yet" << std::endl;
        }

        static void load_strategy_from_file(const std::string &name, std::vector<std::array<T, ACTION_MAX_DIM>> &average_policy) {
            std::cout << "not implemented yet" << std::endl;
        }

        template<typename T>
        static void save_state_from_file(const std::string &name, std::vector<std::array<T, ACTION_MAX_DIM>> &state) {
            std::cout << "not implemented yet" << std::endl;
        }

        static void load_state_from_file(const std::string &name, std::vector<std::array<T, ACTION_MAX_DIM>> &state) {
            std::cout << "not implemented yet" << std::endl;
        }
    };

    const std::array<RPS::Player, RPS::NUM_PLAYERS> RPS::players = {RPS::Player::P1, RPS::Player::P2};

    std::ostream& operator<<(std::ostream& os, const RPS& game) {
        os << "Player 1: " << game.p1_move;
        os << " ";
        os << "Player 2: " << game.p2_move << std::endl;
        return os;
    }
}