
#pragma once

#include <assert.h>
#include <cstdint>
#include <iostream>
#include <cstring>
#include <vector>
#include <array>
#include "io.hpp"

namespace phantom_ttt {
    #define GRID_SIZE 3
    #define CELL(player, x, y) ((player) << (BITS_PER_CELL * (x * GRID_SIZE + y)))
    #define BITS_PER_CELL    2
    #define NUM_CELLS        (GRID_SIZE * GRID_SIZE)
    #define CELL_MASK        0b11
    #define CELL_EMPTY       0b00
    #define CELL_P1          0b01
    #define CELL_P2          0b10
    #define ALL_P1           0b010101010101010101
    #define ALL_P2           0b101010101010101010

    enum Player { P1, P2 };

    #define OtherPlayer(player) (player == Player::P1 ? Player::P2 : Player::P1)
    #define PlayerIdx(player) (player == Player::P1 ? 0 : 1)
    #define PlayerMask(player) (player == Player::P1 ? CELL_P1 : CELL_P2)
    #define OppIdx(player) (player == Player::P1 ? 1 : 0)
    #define OppMask(player) (player == Player::P1 ? CELL_P2 : CELL_P1)

    #define ACTION_MASK_TO_INT(action) (__builtin_ctz(action) / BITS_PER_CELL)
    #define ACTION_INT_TO_MASK(player, action) (PlayerMask(player) << (action * BITS_PER_CELL))

    // 2 bits for each cell    
    using StateObservation = uint32_t;
    using Action = uint32_t; // Action mask
    using Actions = uint32_t; // Multiple Action mask
    using ActionInt = int; // Action index
    using OccupiedMask = uint16_t;

    Actions valid_action_mask(Player player, StateObservation state) {
        uint32_t mask = (player == Player::P1) ? ALL_P1 : ALL_P2;
        uint32_t occupied = state | ((state & ALL_P1) << 1) | ((state & ALL_P2) >> 1);
        return ~occupied & mask;
    }

    int valid_action_count(Player player, StateObservation state) {
        return __builtin_popcount(valid_action_mask(player, state));
    }

    std::vector<Action> valid_actions(Player player, StateObservation state) {
        Actions mask = valid_action_mask(player, state);
        std::vector<Action> actions;
        while(mask) {
            Action action = mask & -mask;
            mask-= action;
            actions.push_back(action);
        }
        return actions;
    }

    Action action(Player player, int x, int y) {
        return CELL(PlayerMask(player), x, y);
    }
    
    ////////////////////////////////////////////////////////////////
    class GameDynamics {
        // todo: later, very simple optimization: consider the symmetry of the board... observation would be the minimum of observation among all symmetries...
        StateObservation obs_player[2] = {0, 0};
        OccupiedMask player_occupied[2] = {0, 0};
        Player cur_player = Player::P1;

        static bool win_mask[(1<<(GRID_SIZE*GRID_SIZE))];
        static bool precomputed;

        #define CELL_TO_MASK(x, y) (1 << (x * GRID_SIZE + y))

        static void generate_win_masks() {
            memset(win_mask, 0, sizeof(win_mask));

            for(int i = 0; i < 3; i++) {
                win_mask[CELL_TO_MASK(i, 0) | CELL_TO_MASK(i, 1) | CELL_TO_MASK(i, 2)] = true;
            }
            for(int i = 0; i < 3; i++) {
                win_mask[CELL_TO_MASK(0, i) | CELL_TO_MASK(1, i) | CELL_TO_MASK(2, i)] = true;
            }
            win_mask[CELL_TO_MASK(0, 0) | CELL_TO_MASK(1, 1) | CELL_TO_MASK(2, 2)] = true;
            win_mask[CELL_TO_MASK(0, 2) | CELL_TO_MASK(1, 1) | CELL_TO_MASK(2, 0)] = true;
            for(int mask = 0; mask < (1<<(GRID_SIZE*GRID_SIZE)); mask++) {
                for(int i = 0; i < (GRID_SIZE*GRID_SIZE); i++) {
                    if(mask & (1 << i))
                        win_mask[mask] |= win_mask[mask ^ (1 << i)];
                }
            }
        }

        static void precompute() {
            generate_win_masks();
        }

    public:
        GameDynamics() {
            if(!precomputed) {
                precomputed = true;
                precompute();
            }
            obs_player[0] = obs_player[1] = 0;
            player_occupied[0] = player_occupied[1] = 0;
            cur_player = Player::P1;
        }

        StateObservation player_observation(Player player) const {
            return obs_player[PlayerIdx(player)];
        }

        bool step(Action action) { // returns whether the player has won
            assert(__builtin_popcount(action) == 1);
            int ctz = __builtin_ctz(action);
            assert(ctz < NUM_CELLS * BITS_PER_CELL);
            int cellidx = ctz / BITS_PER_CELL;
            int player_idx = ctz % BITS_PER_CELL;
            std::cout << player_idx << " " << PlayerIdx(cur_player) << std::endl;
            printf("%b\n", action);
            assert(PlayerIdx(cur_player) == player_idx);
            int opp_idx = 1 - player_idx;

            Action changed_player_action = action ^ (CELL_MASK << (cellidx * BITS_PER_CELL));

            if(obs_player[opp_idx] & changed_player_action) {
                // player finds out that this cell is occupied
                obs_player[player_idx] |= changed_player_action;
            } else {
                obs_player[player_idx] |= action;
                player_occupied[player_idx] |= (1 << cellidx);
            }
            cur_player = (cur_player == Player::P1) ? Player::P2 : Player::P1;
            return win_mask[player_occupied[player_idx]];
        }

        inline Player current_player() const {
            return cur_player;
        }

        friend std::ostream& operator<<(std::ostream& os, const GameDynamics& game);
    };
    bool GameDynamics::precomputed = false;
    bool GameDynamics::win_mask[(1<<(GRID_SIZE*GRID_SIZE))];
    ////////////////////////////////////////////////////////////////

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
        // using Player = phantom_ttt::Player;
        using Buffer = std::array<T, ACTION_MAX_DIM>;
        using BufferInt = std::array<int, ACTION_MAX_DIM>;
        const std::array<Player, NUM_PLAYERS> players = {Player::P1, Player::P2};

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
            io::load_information_sets(PLAYER0_INFOSET_PATH, info_sets_reprs_p0);
            io::load_information_sets(PLAYER1_INFOSET_PATH, info_sets_reprs_p1);
        }

        static void save_to_file(const std::string &name, std::vector<std::array<T, ACTION_MAX_DIM>> &average_policy) {
            auto it1 = average_policy.begin();
            auto it2 = it1 + info_sets_reprs_p0.size();
            auto it3 = average_policy.end();
            assert(info_sets_reprs_p0.size() + info_sets_reprs_p1.size() == average_policy.size());

            io::save_information_sets(PLAYER0_INFOSET_PATH, info_sets_reprs_p0, it1, it2);
            io::save_information_sets(PLAYER1_INFOSET_PATH, info_sets_reprs_p1, it2, it3);
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


    std::ostream& operator<<(std::ostream& os, const Game& game) {
        os << game.game_;
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const GameDynamics& game) {
        os << "Game (Turn=" << (game.cur_player == Player::P1 ? "X" : "O") << ")\n";
        for(int i = 0; i < GRID_SIZE; i++) {
            for(int j = 0; j < GRID_SIZE; j++) {
                bool p1_1 = game.obs_player[0] & CELL(CELL_P1, i, j);
                bool p1_2 = game.obs_player[0] & CELL(CELL_P2, i, j);
                bool p2_1 = game.obs_player[1] & CELL(CELL_P1, i, j);
                bool p2_2 = game.obs_player[1] & CELL(CELL_P2, i, j);
                if(p1_1 && p2_1) {
                    os << "X ";
                } else if(p1_2 && p2_2) {
                    os << "O ";
                } else if(p1_1) {
                    os << "X?";
                } else if(p2_2) {
                    os << "O?";
                } else {
                    os << ". ";
                }
            }
            os << "\n";
        }
        return os;
    }
}