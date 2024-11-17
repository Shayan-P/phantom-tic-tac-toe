
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

namespace pttt {
    class Game {
        GameDynamics game_;
        bool done = false;
        Player winner;
        bool tie = false;

    public:
        static const int ACTION_MAX_DIM = 9;
        static const int NUM_INFO_SETS = 14482810 + 8827459; // later figure out a way to compute this, or at least confirm that this is correct...
        static const int NUM_PLAYERS = 2;

        using T = double;
        // using Player = pttt::Player;
        using Buffer = std::array<T, ACTION_MAX_DIM>;
        using BufferInt = std::array<int, ACTION_MAX_DIM>;
        static const std::array<Player, NUM_PLAYERS> players;
        using Player = pttt::Player;

        Game() {
            if(!precomputed) {
                precomputed = true;
                Game::precompute();
            }
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
            Action action = ACTION_INT_TO_MASK(cur_player, action_);
            bool res = game_.step(action);
            if(res) {
                done = true;
                winner = OtherPlayer(cur_player);
            }
            if(valid_action_count(cur_player, game_.player_observation(cur_player)) == 0) {
                done = true;
                tie = true;
            }
        }

        void actions(BufferInt &buffer) { // don't use vector or dynamic memory
            Player player = game_.current_player();
            Actions mask = valid_action_mask(player, game_.player_observation(player));
            int cnt = 0;
            while(mask) {
                Action action = mask & -mask;
                mask-= action;
                buffer[cnt] = ACTION_MASK_TO_INT(action);
                cnt++;
            }
            assert(false);
        }

        inline Player current_player() const {
            return game_.current_player();
        }

        int info_set_idx() const {
            // Implement information set index retrieval
            return 0;
        }

        friend std::ostream& operator<<(std::ostream& os, const Game& game);

    private:
        static void precompute() {
            io::load_information_sets(pttt::get_player0_infoset_path(), info_sets_reprs_p0);
            io::load_information_sets(pttt::get_player1_infoset_path(), info_sets_reprs_p1);

            assert(info_sets_reprs_p0.size() + info_sets_reprs_p1.size() == NUM_INFO_SETS);
        }

        static void save_to_file(const std::string &name, std::vector<std::array<T, ACTION_MAX_DIM>> &average_policy) {
            auto it1 = average_policy.begin();
            auto it2 = it1 + info_sets_reprs_p0.size();
            auto it3 = average_policy.end();
            assert(info_sets_reprs_p0.size() + info_sets_reprs_p1.size() == average_policy.size());

            io::save_information_sets(pttt::get_player0_infoset_path(), it1, it2);
            io::save_information_sets(pttt::get_player1_infoset_path(), it2, it3);
        }

        static std::vector<std::string> info_sets_reprs_p0;
        static std::vector<std::string> info_sets_reprs_p1;

        // todo later add the ability to load from the last checkpoint...
        // warmstart the regret minimizers...

        static bool precomputed;
    };
    bool Game::precomputed = false;
    std::vector<std::string> Game::info_sets_reprs_p0 = {};
    std::vector<std::string> Game::info_sets_reprs_p1 = {};
    const std::array<Player, Game::NUM_PLAYERS> Game::players = {Player::P1, Player::P2};

    std::ostream& operator<<(std::ostream& os, const Game& game) {
        os << game.game_;
        return os;
    }
}