#ifndef STRATEGY_HPP
#define STRATEGY_HPP

#include <vector>
#include <array>
#include <cassert>
#include <random>

namespace strategy
{
    template <class Game>
    class Strategy
    {
    public:
        using Player = typename Game::Player;
        using T = typename Game::T;
        using Action = int; // Game::Action;
        using Buffer = std::array<T, Game::ACTION_MAX_DIM>;
        using BufferInt = std::array<int, Game::ACTION_MAX_DIM>;
        using Strat = std::vector<Buffer>;

        Strat strat;

        Strategy(const Strat &strat) : strat(strat) {
            assert(strat.size() == Game::NUM_INFO_SETS);

            gen = std::mt19937(rd());
            dis = std::uniform_real_distribution<T>(0.0, 1.0);
        }

        Strategy(Strategy &&other) : strat(std::move(other.strat)) {
            gen = std::mt19937(rd());
            dis = std::uniform_real_distribution<T>(0.0, 1.0);
        }

        Action sample_action(const Game &state) {
            std::array<int, Game::ACTION_MAX_DIM> actions;
            state.actions(actions);
            int num_actions = state.num_actions();
            if(state.is_chance()) {
                Buffer policy;
                state.action_probs(policy);
                return actions[sample_index(policy, num_actions)];
            }
            int info_set_idx = state.info_set_idx();
            Action action = sample_index(strat[info_set_idx], Game::ACTION_MAX_DIM);
            for(int i = 0; i < num_actions; i++) {
                if(actions[i] == action) {
                    return action;
                }
            }
            // asserts that the action is valid
            assert(false);
        }

        T evaluate(Player p, int iters=1000) {
            T sum = 0;
            for(int _ = 0; _ < iters; _++) {
                Game state;
                while(!state.is_terminal()) {
                    Action action = sample_action(state);
                    state.step(action);
                }
                sum += state.utility(p);
            }
            return sum / iters;
        }

        T evaluate_against_uniform(Player p, int iters=1000) {
            T sum = 0;
            for(int _ = 0; _ < iters; _++) {
                Game state;
                BufferInt actions;
                state.actions(actions);
                Buffer policy;

                while(!state.is_terminal()) {
                    int num_actions = state.num_actions();
                    state.actions(actions);
                    Action action;

                    if(state.is_chance()) {
                        state.action_probs(policy);
                        action = actions[sample_index(policy, num_actions)];
                    } else if(state.current_player() == p) {
                        action = sample_index(strat[state.info_set_idx()], Game::ACTION_MAX_DIM);
                    } else {
                        action = actions[int(dis(gen) * num_actions)];
                    }
                    state.step(action);
                }
                sum += state.utility(p);
            }
            return sum / iters;
        }


        // using Utils = std::vector<std::array<T, Game::ACTION_MAX_DIM>>;
        
        // struct InfosetInfo {
        //     Buffer util_best_response;
        //     std::vector<Game> states; // states that belong to this infoset...
        //     bool traversed;

        //     InfosetInfo() {
        //         util_best_response.fill(0);
        //         traversed = false;
        //     }
        // };

        // T best_response(Player player) {
            // Strat best_response = strat;
            // std::vector<CalculationBuffer> memo(Game::NUM_INFO_SETS);
            // recuse_and_update_best_response(Game(), best_response, memo, player);
            // do something here...
        // }

        // T recuse_and_update_best_response(Game state, Strat &best_response, std::vector<CalculationBuffer> &memo, Player player) {
        //     // not strategies are mapped via actions but anything else is mapped via action index...
        //     if(state.is_terminal()) {
        //         return state.utility(player);
        //     }

        //     int num_actions = state.num_actions();
        //     BufferInt actions;
        //     Buffer policy;

        //     state.actions(actions);
        //     int info_set_idx = state.info_set_idx();

        //     bool cur_player_turn = false;

        //     if(state.is_chance()) {
        //         Buffer policy_idx;
        //         state.action_probs(policy_idx);
        //         for(int i = 0; i < num_actions; i++) {
        //             policy[actions[i]] = policy_idx[i];
        //         }
        //     } else {
        //         for(int i = 0; i < Game::ACTION_MAX_DIM; i++) {
        //             policy[i] = strat[info_set_idx][i];
        //         }
        //         cur_player_turn = state.current_player() == player;
        //     }

        //     for(int i = 0; i < num_actions; i++) {
        //         Game new_state = state;
        //         Action action = actions[i];
        //         new_state.step(action);
        //         T res = recuse_and_update_best_response(new_state, best_response, memo, player);

        //         memo[info_set_idx].util_best_response[action] += res;
        //     }

        // }

        void debug_print() {
            for(int i = 0; i < Game::NUM_INFO_SETS; i++) {
                std::cout << "info set " << i << std::endl;
                for(int j = 0; j < Game::ACTION_MAX_DIM; j++) {
                    std::cout << strat[i][j] << " ";
                }
                std::cout << std::endl;
            }
        }

    private:
        int sample_index(const Buffer &probs, int size) {
            T sum = 0;
            for (int i = 0; i < size; i++) {
                sum += probs[i];
            }
            assert(abs(sum-1) <= 1e-5); // this is a probability distribution

            T r = dis(gen);
            T cumulative = 0.0;
            for (int i = 0; i < size; i++) {
                cumulative += probs[i];
                if (r < cumulative) {
                    return i;
                }
            }
            return size - 1; // should not reach here
        }

        std::random_device rd;
        std::mt19937 gen;
        std::uniform_real_distribution<T> dis;
    };
} // namespace strat

#endif
