#ifndef IO_MCCFR_HPP
#define IO_MCCFR_HPP

#include <array>
#include <cassert>
#include <cstring>
#include <vector>
#include <random>
#include <mutex>
#include "strategy.hpp"


namespace mccfr {
    using T = double;

    template<int MAX_DIM>

    class RegretMinimizer {
        using Utility = std::array<T, MAX_DIM>;
        using Policy = std::array<T, MAX_DIM>;

        T regret[MAX_DIM]; // todo: made this public so that we can load and save from file but later replace with friend functions
        T average_policy[MAX_DIM];

        int dim = -1;
        std::mutex mtx_regret, mtx_policy; // mutex for locking

    public:
        // note that this is indexed on the action indices and not the actions themselves!
        RegretMinimizer() {
            memset(regret, 0, sizeof(regret));
            memset(average_policy, 0, sizeof(average_policy));
        }

        void set_dim(int dim_) {
            assert(dim == -1 || dim == dim_);
            dim = dim_;
            // memset(regret, 0, sizeof(regret)); // otherwise we override the saved version...
        }

        void observe_utility(const Utility& utility, const Policy &last_policy) {
            std::lock_guard<std::mutex> lock(mtx_regret); // lock the mutex
            T avg = 0;
            for(int i = 0; i < dim; i++) {
                avg += last_policy[i] * utility[i];
            }
            for(int i = 0; i < dim; i++) {
                regret[i] += utility[i] - avg;
            }
        }

        void next_policy(Policy &policy) {
            std::lock_guard<std::mutex> lock(mtx_regret); // lock the mutex
            T sum = 0;
            for(int i = 0; i < dim; i++) {
                policy[i] = std::max(regret[i], 0.0);
                sum += policy[i];
            }
            if(sum <= 1e-9) { // epsilon error
                for(int i = 0; i < dim; i++) {
                    policy[i] = 1.0;
                }
                sum = dim;
            }
            for(int i = 0; i < dim; i++) {
                policy[i] /= sum;
            }
        }

        void set_average_policy(const Policy &policy_values) {
            std::lock_guard<std::mutex> lock(mtx_policy); // lock the mutex
            for(int i = 0; i < MAX_DIM; i++)
                average_policy[i] = policy_values[i];
        }

        void get_average_policy(Policy &policy_values) {
            std::lock_guard<std::mutex> lock(mtx_policy); // lock the mutex
            for(int i = 0; i < MAX_DIM; i++)
                policy_values[i] = average_policy[i];
        }

        void set_regret(const Utility &regret_values) {
            std::lock_guard<std::mutex> lock(mtx_regret); // lock the mutex
            for(int i = 0; i < MAX_DIM; i++)
                regret[i] = regret_values[i];
        }

        void get_regret(Utility &regret_values) {
            std::lock_guard<std::mutex> lock(mtx_regret); // lock the mutex
            for(int i = 0; i < MAX_DIM; i++)
                regret_values[i] = regret[i];
        }

        void increment_avg_policy(int action, T increment) {
            std::lock_guard<std::mutex> lock(mtx_policy); // lock the mutex
            average_policy[action] += increment;
        }
    };;

////////////////////////////////////////

    template<class Game>
    class MCCFR {
        using Player = typename Game::Player;

        static constexpr T EXPLORATION = 0.6;

        // regret minimizers are saved in action index space
        // average policy is saved in **action** space
        std::vector<RegretMinimizer<Game::ACTION_MAX_DIM>> regret_minimizers; // size Game::NUM_INFO_SETS
        
        using Buffer = std::array<T, Game::ACTION_MAX_DIM>;
        using BufferInt = std::array<int, Game::ACTION_MAX_DIM>;

        // compute memo is a scratch pad for the computation that needs to happen in each node...
        struct ComputeMemo {
            Buffer utility;

            int sample_index(const Buffer &probs, int size) {
                T sum = 0;
                for (int i = 0; i < size; i++) {
                    sum += probs[i];
                }
                if (sum <= 1e-9) { // epsilon error
                    return int(dis(gen) * size);
                }

                T r = dis(gen) * sum;
                T cumulative = 0.0;
                for (int i = 0; i < size; i++) {
                    cumulative += probs[i];
                    if (r < cumulative) {
                        return i;
                    }
                }
                return size - 1; // should not reach here
            }

            ComputeMemo() {
                gen = std::mt19937(rd());
                dis = std::uniform_real_distribution<T>(0.0, 1.0);
            }

        private:
            std::random_device rd;
            std::mt19937 gen;
            std::uniform_real_distribution<T> dis;
        };

    public:
        void iteration() {
            ComputeMemo memo; // if you don't want to recreate this within the loop, you can also pass it from outside...
            for(auto player: Game::players) {
                Game state;
                // we pass the reference of these two... they do not change throughout the sampling process...
                episode(memo, state, player);
            }
        }

        void iteration(Player player) {
            ComputeMemo memo;
            Game state;
            episode(memo, state, player);
        }

        MCCFR(): regret_minimizers(Game::NUM_INFO_SETS) { }

        void save_checkpoint(const std::string &name) {
            std::vector<std::array<T, Game::ACTION_MAX_DIM>> average_policy_data;
            average_policy_data.reserve(Game::NUM_INFO_SETS);
            average_policy_data.resize(Game::NUM_INFO_SETS);
            for(int i = 0; i < Game::NUM_INFO_SETS; i++) {
                regret_minimizers[i].get_average_policy(average_policy_data[i]);
            }
            Game::save_strategy_to_file(name, average_policy_data);

            std::vector<std::array<T, Game::ACTION_MAX_DIM>> regret_minimizers_data;
            regret_minimizers_data.reserve(Game::NUM_INFO_SETS);
            regret_minimizers_data.resize(Game::NUM_INFO_SETS);
            for(int i = 0; i < Game::NUM_INFO_SETS; i++) {
                regret_minimizers[i].get_regret(regret_minimizers_data[i]);
            }
            Game::save_state_from_file(name, regret_minimizers_data);
        }

        void load_from_checkpoint(const std::string &name) {
            std::vector<std::array<T, Game::ACTION_MAX_DIM>> average_policy_data;
            average_policy_data.reserve(Game::NUM_INFO_SETS);
            average_policy_data.resize(Game::NUM_INFO_SETS);
            Game::load_strategy_from_file(name, average_policy_data);
            for(int i = 0; i < Game::NUM_INFO_SETS; i++) {
                regret_minimizers[i].set_average_policy(average_policy_data[i]);
            }

            std::vector<std::array<T, Game::ACTION_MAX_DIM>> regret_minimizers_data;
            regret_minimizers_data.reserve(Game::NUM_INFO_SETS);
            regret_minimizers_data.resize(Game::NUM_INFO_SETS);
            Game::load_state_from_file(name, regret_minimizers_data);
            for(int i = 0; i < Game::NUM_INFO_SETS; i++) {
                regret_minimizers[i].set_regret(regret_minimizers_data[i]);
            }
        }

        std::vector<std::array<T, Game::ACTION_MAX_DIM>> get_strategy_data() {
            std::vector<std::array<T, Game::ACTION_MAX_DIM>> average_policy_data;
            average_policy_data.reserve(Game::NUM_INFO_SETS);
            average_policy_data.resize(Game::NUM_INFO_SETS);
            for(int i = 0; i < Game::NUM_INFO_SETS; i++) {
                regret_minimizers[i].get_average_policy(average_policy_data[i]);
            }
            return average_policy_data;
        }

        strategy::Strategy<Game> get_strategy() {
            std::vector<std::array<T, Game::ACTION_MAX_DIM>> average_policy_data;
            average_policy_data.reserve(Game::NUM_INFO_SETS);
            average_policy_data.resize(Game::NUM_INFO_SETS);
            for(int i = 0; i < Game::NUM_INFO_SETS; i++) {
                regret_minimizers[i].get_average_policy(average_policy_data[i]);
            }
            return Game::get_strategy(average_policy_data);
        }

        void set_strategy(const strategy::Strategy<Game> &strategy) {
            for(int i = 0; i < Game::NUM_INFO_SETS; i++) {
                regret_minimizers[i].set_average_policy(strategy.strat[i]);
            }
        }

    private:
        // later put & back for game state... for now we remove it to make simplification 
        T episode(ComputeMemo &memo, const Game state, const Player player, const T reach_me=1.0, const T reach_other=1.0, const T reach_sample=1.0) {
            // std::cout << "entering " << " cur player is " << state.current_player() << std::endl;
            // std::cout << state << std::endl;

            if(state.is_terminal()) {
                // std::cout << "terminal " << " " << "utility=" << state.utility(player) << std::endl;
                return state.utility(player);
            }

            // memory creation
            Buffer policy;
            Buffer sample_policy;
            BufferInt actions;
            // end memo creation

            int num_actions = state.num_actions();
            state.actions(actions);

            if(state.is_chance()) {
                // state should also carry a dynamic memory so that we can do this calculations on the fly without the need to allocate dynamic memory
                state.action_probs(policy);
                int action_idx = memo.sample_index(policy, num_actions);
                auto p = policy[action_idx];
                // be careful that after stepping the memo is changed because it is shared...
                Game new_state = state;
                new_state.step(actions[action_idx]); // todo later do this on state and add reference to state
                // gets multiplied by p for the probability of the path and then gets divided by p for importance sampling
                return episode(memo, new_state, player, reach_me, reach_other * p, reach_sample * p);
            }

            auto cur_player = state.current_player();
            int info_set_idx = state.info_set_idx();
            // later put locks here if we are doing parallelism
            regret_minimizers[info_set_idx].set_dim(num_actions); // sets the dimension if you are visiting the regret minimizer for the first time
            regret_minimizers[info_set_idx].next_policy(policy); // gets the policy

            if(cur_player == player) {
                for(int i = 0; i < num_actions; i++) {
                    sample_policy[i] = EXPLORATION / num_actions + (1.0 - EXPLORATION) * policy[i];
                } // exploration + policy
            } else {
                for(int i = 0; i < num_actions; i++) {
                    sample_policy[i] = policy[i];
                }
            }

            int action_idx = memo.sample_index(sample_policy, num_actions);

            T new_reach_me = reach_me;
            T new_reach_other = reach_other;
            T new_reach_sample = reach_sample * sample_policy[action_idx];
            if(cur_player == player) {
                new_reach_me *= policy[action_idx];
            } else {
                new_reach_other *= policy[action_idx];
            }


            // be careful that after stepping the memo is changed because it is shared...
            Game new_state = state;
            new_state.step(actions[action_idx]); // todo later do this on state and add reference to state

            T rec_child_value = episode(memo, new_state, player, new_reach_me, new_reach_other, new_reach_sample);

            T value_estimate = 0;

            for(int i = 0; i < num_actions; i++) {
                const T baseline = 0; // we can change later to improve the variance
                T child_value = (
                    (action_idx == i)
                    ? (baseline + (rec_child_value - baseline) / sample_policy[action_idx])
                    : (baseline)
                );
                memo.utility[i] = child_value * reach_other / reach_sample;
                value_estimate += child_value * policy[i];
            }

            if(cur_player == player) {
                regret_minimizers[info_set_idx].observe_utility(memo.utility, policy);

                // update average policy
                for(int i = 0; i < num_actions; i++) {
                    // why not update with new policy?
                    T increment = reach_me * policy[i] / reach_sample;
                    regret_minimizers[info_set_idx].increment_avg_policy(actions[i], increment);
                }
            }
            return value_estimate;
        }

    public:
        void debug_print() {
            std::cout << "printing average strategy: ----------------------" << std::endl;
            for(int i = 0; i < Game::NUM_INFO_SETS; i++) {
                std::cout << "info set " << i << std::endl;
                Buffer policy;
                regret_minimizers[i].get_average_policy(policy);
                for(int j = 0; j < Game::ACTION_MAX_DIM; j++) {
                    std::cout << policy[j] << " ";
                }
                std::cout << std::endl;
            }
            std::cout << "printing current strategy: ----------------------" << std::endl;
            for(int i = 0; i < Game::NUM_INFO_SETS; i++) {
                std::cout << "info set " << i << std::endl;
                Buffer policy;
                regret_minimizers[i].next_policy(policy);
                for(int j = 0; j < Game::ACTION_MAX_DIM; j++) {
                    std::cout << policy[j] << " ";
                }
                std::cout << std::endl;
            }
        }
    };
} // namespace mccfr

#endif
