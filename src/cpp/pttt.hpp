#ifndef PTTT_HPP
#define PTTT_HPP

#include <assert.h>
#include <cstdint>
#include <iostream>
#include <cstring>
#include <vector>
#include <array>
#include "io.hpp"
#include <filesystem>
#include <string>
#include "paths.hpp"
#include "pttt_game_dynamics.hpp"

namespace pttt {
    using PTTT_Infoset = std::pair<std::string, uint32_t>; // name, valid mask

    class PTTT {
    public:
        using Player = pttt::Player;
    private:
        PTTTDynamics game_;
        bool done = false;
        Player winner;
        bool tie = false;
        PTTT_Infoset info_set_repr[2] = {PTTT_Infoset("|", (1<<PTTT_NUM_ACTIONS)-1), PTTT_Infoset("|", (1<<PTTT_NUM_ACTIONS)-1)}; // todo later we can replace this with a better representation...

    public:
        static constexpr int ACTION_MAX_DIM = 9;
        static constexpr int NUM_INFO_SETS = 14482810 + 8827459; // later figure out a way to compute this, or at least confirm that this is correct...
        static constexpr int NUM_PLAYERS = 2;

        using T = double;
        // using Player = pttt::Player;
        using Buffer = std::array<T, ACTION_MAX_DIM>;
        using ActionInts = std::array<ActionInt, ACTION_MAX_DIM>;
        static const std::array<Player, NUM_PLAYERS> players;

        PTTT() {
            precompute_if_needed();
        }

        bool is_terminal() const {
            return done;
        }

        T utility(Player player) const {
            assert(done);
            // todo does the player want to maximize it's win or also the player doesn't like it when the opponent wins?   
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
            auto player = game_.current_player();
            return valid_action_count(player, game_.player_observation(player));
        }

        void step(ActionInt action_) {
            assert(!done);
            auto cur_player = game_.current_player();
            auto opponent = OtherPlayer(cur_player);
            Action action = ACTION_INT_TO_MASK(cur_player, action_);
            bool succ_move = game_.step(action);
            bool won = game_.has_won(cur_player);
            if(won) {
                done = true;
                winner = cur_player;
            }
            else if(game_.board_fully_occupied()) {
                done = true;
                tie = true;
            }
            char cell = '0' + action_;
            char succ = succ_move ? '*' : '.';
            info_set_repr[PlayerIdx(cur_player)].first += cell;
            info_set_repr[PlayerIdx(cur_player)].first += succ;
            info_set_repr[PlayerIdx(cur_player)].second &= ~(1 << action_);
        }

        // the order of actions should be the same in all information sets
        void actions(ActionInts &buffer) const { // don't use vector or dynamic memory
            Player player = game_.current_player();
            Actions mask = valid_action_mask(player, game_.player_observation(player));
            int cnt = 0;
            while(mask) {
                Action action = mask & -mask;
                mask-= action;
                buffer[cnt] = ACTION_MASK_TO_INT(action);
                cnt++;
            }
        }

        inline Player current_player() const {
            return game_.current_player();
        }

        int info_set_idx() const {
            auto player = game_.current_player();
            auto idx = PlayerIdx(player);
            auto it = info_set_to_idx[idx].find(info_set_repr[idx]);
            // std::cout << idx << std::endl;
            // std::cout << info_set_repr[idx].first << std::endl;
            // std::cout << info_set_repr[1-idx].first << std::endl;
            // std::cout << game_ << std::endl;
            assert(it != info_set_to_idx[idx].end());
            return it->second;
        }

        friend std::ostream& operator<<(std::ostream& os, const PTTT& game);

    private:

        static void load_information_sets(const std::string &filename, std::vector<PTTT_Infoset> &info_sets_reprs) {
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
                mask = ((1<<PTTT_NUM_ACTIONS)-1) ^ mask; // invert the mask to keep the valid ones...
                info_sets_reprs.push_back({line, mask});
            }
            file.close();
            std::cout << "done " << filename << std::endl;
        }

        static void precompute() {
            load_information_sets(pttt::get_player0_infoset_path(), info_sets_reprs_p[0]);
            load_information_sets(pttt::get_player1_infoset_path(), info_sets_reprs_p[1]);

            for(int i = 0; i < info_sets_reprs_p[0].size(); i++) {
                info_set_to_idx[0][info_sets_reprs_p[0][i]] = i;
            }
            for(int i = 0; i < info_sets_reprs_p[1].size(); i++) {
                info_set_to_idx[1][info_sets_reprs_p[1][i]] = i + info_sets_reprs_p[0].size();
            }

            assert(info_sets_reprs_p[0].size() + info_sets_reprs_p[1].size() == NUM_INFO_SETS);
        }

    public:
        static std::vector<std::array<T, ACTION_MAX_DIM>> get_strategy(const std::vector<std::array<T, ACTION_MAX_DIM>> &average_policy) {
            std::vector<std::array<T, ACTION_MAX_DIM>> result(NUM_INFO_SETS);

            // TODO: note one needs to assign average strategy to the nodes have value 0!
            // ugly ugly code... :D
            int idx = 0;
            for(int p = 0; p < 2; p++) {
                for(int i = 0; i < info_sets_reprs_p[p].size(); i++, idx++) {
                    result[idx] = average_policy[idx];
                    T sm = 0;
                    for(int j = 0; j < ACTION_MAX_DIM; j++) {
                        sm += average_policy[idx][j];
                    }
                    if (sm <= 1e-9) {
                        // assign uniform distribution
                        auto mask = info_sets_reprs_p[p][i].second;
                        int num_valid_actions = __builtin_popcount(mask);
                        sm = num_valid_actions;
                        for(int j = 0; j < ACTION_MAX_DIM; j++) {
                            if(mask & (1<<j))
                                result[idx][j] = 1.0;
                        }
                    }
                    for(int j = 0; j < ACTION_MAX_DIM; j++) {
                        result[idx][j] /= sm;
                    }
                }
            }
            return result;
        }

        static void save_strategy_to_file(const std::string &name, const std::vector<std::array<T, ACTION_MAX_DIM>> &average_policy) {
            precompute_if_needed();

            std::vector<std::array<T, ACTION_MAX_DIM>> policy = get_strategy(average_policy);
            
            assert(policy.size() == NUM_INFO_SETS);

            std::vector<std::array<T, ACTION_MAX_DIM>>::iterator it1 = policy.begin();
            std::vector<std::array<T, ACTION_MAX_DIM>>::iterator it2 = it1 + info_sets_reprs_p[0].size();
            std::vector<std::array<T, ACTION_MAX_DIM>>::iterator it3 = policy.end();
            assert(info_sets_reprs_p[0].size() + info_sets_reprs_p[1].size() == policy.size());

            io::save_to_numpy<T, ACTION_MAX_DIM>(paths::get_checkpoints_dir() / (name + "_p0.npy"), it1, it2);
            io::save_to_numpy<T, ACTION_MAX_DIM>(paths::get_checkpoints_dir() / (name + "_p1.npy"), it2, it3);
        }

        static void load_strategy_from_file(const std::string &name, std::vector<std::array<T, ACTION_MAX_DIM>> &average_policy) {
            precompute_if_needed();

            assert(average_policy.size() == NUM_INFO_SETS);

            std::vector<std::array<T, ACTION_MAX_DIM>>::iterator it1 = average_policy.begin();
            std::vector<std::array<T, ACTION_MAX_DIM>>::iterator it2 = it1 + info_sets_reprs_p[0].size();

            io::load_from_numpy<T, ACTION_MAX_DIM>(paths::get_checkpoints_dir() / (name + "_p0.npy"), it1);
            io::load_from_numpy<T, ACTION_MAX_DIM>(paths::get_checkpoints_dir() / (name + "_p1.npy"), it2);
        }

        // todo later make the type generic
        template<typename T>
        static void save_state_from_file(const std::string &name, std::vector<std::array<T, ACTION_MAX_DIM>> &state) {
            precompute_if_needed();
            io::save_to_numpy<T, ACTION_MAX_DIM>(paths::get_checkpoints_dir() / (name + "_state.npy"), state.begin(), state.end());
        }

        static void load_state_from_file(const std::string &name, std::vector<std::array<T, ACTION_MAX_DIM>> &state) {
            precompute_if_needed();

            io::load_from_numpy<T, ACTION_MAX_DIM>(paths::get_checkpoints_dir() / (name + "_state.npy"), state.begin());
        }

        static void precompute_if_needed() {
            if(precomputed)
                return;
            precompute();
            precomputed = true;
        }

    private:
        
        static std::vector<PTTT_Infoset> info_sets_reprs_p[2];
        static std::map<PTTT_Infoset, int> info_set_to_idx[2];

        // todo later add the ability to load from the last checkpoint...
        // warmstart the regret minimizers...

        static bool precomputed;
    };
#ifdef NO_PRECOMPUTE
    bool PTTT::precomputed = true;
#else
    bool PTTT::precomputed = false;
#endif

    std::vector<PTTT_Infoset> PTTT::info_sets_reprs_p[PTTT::NUM_PLAYERS] = {{}, {}};
    std::map<PTTT_Infoset, int> PTTT::info_set_to_idx[PTTT::NUM_PLAYERS] = {{}, {}};

    const std::array<Player, PTTT::NUM_PLAYERS> PTTT::players = {Player::P1, Player::P2};

    std::ostream& operator<<(std::ostream& os, const PTTT& game) {
        os << game.game_;
        return os;
    }
}

#endif
