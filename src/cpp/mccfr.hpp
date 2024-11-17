#pragma once

#include <array>
#include <cassert>
#include <cstring>
#include <vector>
#include <random>

namespace mccfr {
    using T = double;

    template<int MAX_DIM>
    class RegretMinimizer {
        using Utility = std::array<T, MAX_DIM>;
        using Policy = std::array<T, MAX_DIM>;

        T regret[MAX_DIM];
        int dim = -1;
    public:
        void set_dim(int dim_) {
            assert(dim == -1 || dim == dim_);
            dim = dim_;
            memset(regret, 0, sizeof(regret));
        }

        void observe_utility(const Utility& utility, const Policy &last_policy) {
            T avg = 0;
            for(int i = 0; i < dim; i++) {
                avg += last_policy[i] * utility[i];
            }
            for(int i = 0; i < dim; i++) {
                regret[i] += utility[i] - avg;
            }
        }
        void next_policy(Policy &policy) {
            T sum = 0;
            for(int i = 0; i < dim; i++) {
                policy[i] += std::max(regret[i], 0.0);
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
    };

////////////////////////////////////////

    template<class Game>
    class MCCFR {
        using Player = typename Game::Player;

        static constexpr T EXPLORATION = 0.6;

        std::vector<RegretMinimizer<Game::ACTION_MAX_DIM>> regret_minimizers; // size Game::NUM_INFO_SETS
        std::vector<std::array<T, Game::ACTION_MAX_DIM>> average_policy; // size Game::NUM_INFO_SETS

        struct ComputeMemo {
            int size;

            using Buffer = std::array<T, Game::ACTION_MAX_DIM>;
            using BufferInt = std::array<int, Game::ACTION_MAX_DIM>;

            Buffer policy;
            Buffer sample_policy;
            Buffer utility;
            BufferInt actions;

            int sample(Buffer &probs) {
                T sum = 0;
                for (int i = 0; i < size; i++) {
                    sum += probs[i];
                }
                if (sum <= 1e-9) { // epsilon error
                    for (int i = 0; i < size; i++) {
                        probs[i] = 1.0;
                    }
                    sum = size;
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

        MCCFR() {
            regret_minimizers.reserve(Game::NUM_INFO_SETS);
            average_policy.reserve(Game::NUM_INFO_SETS);
            regret_minimizers.resize(Game::NUM_INFO_SETS);
            average_policy.resize(Game::NUM_INFO_SETS);
            for(auto& policy : average_policy) {
                policy.fill(0);
            }
        }

    private:
        T episode(ComputeMemo &memo, Game &state, Player player, T reach_me=1.0, T reach_other=1.0, T reach_sample=1.0) {
            if(state.is_terminal()) {
                return state.utility(player);
            }

            int num_actions = state.num_actions();
            memo.size = state.num_actions();

            if(state.is_chance()) {
                // state should also carry a dynamic memory so that we can do this calculations on the fly without the need to allocate dynamic memory
                state.action_probs(memo.policy);
                int action_idx = memo.sample(memo.policy);
                auto p = memo.policy[action_idx];
                state.step(memo.actions[action_idx]);
                return episode(memo, state, player, reach_me, reach_other * p, reach_sample * p);
            }

            auto cur_player = state.current_player();
            int info_set_idx = state.info_set_idx();
            // later put locks here if we are doing parallelism
            regret_minimizers[info_set_idx].set_dim(num_actions); // sets the dimension if you are visiting the regret minimizer for the first time
            regret_minimizers[info_set_idx].next_policy(memo.policy); // gets the policy

            if(cur_player == player) {
                for(int i = 0; i < num_actions; i++) {
                    memo.sample_policy[i] = EXPLORATION / num_actions + (1.0 - EXPLORATION) * memo.policy[i];
                } // exploration + policy
            } else {
                for(int i = 0; i < num_actions; i++) {
                    memo.sample_policy[i] = memo.policy[i];
                }
            }

            state.actions(memo.actions);
            int action_idx = memo.sample(memo.sample_policy);
            state.step(memo.actions[action_idx]);

            double new_reach_me = reach_me;
            double new_reach_other = reach_other;
            double new_reach_sample = reach_sample * memo.sample_policy[action_idx];
            if(cur_player == player) {
                new_reach_me *= memo.policy[action_idx];
            } else {
                new_reach_other *= memo.policy[action_idx];
            }

            auto child_value = episode(memo, state, player, new_reach_me, new_reach_other, new_reach_sample);

            T value = 0;
            if(cur_player == player) {
                for(int i = 0; i < num_actions; i++) {

                    const double baseline = 0; // we can change later to improve the variance
                    T child_value = (
                        baseline + (child_value - baseline) / memo.sample_policy[action_idx]
                        ? action_idx == i
                        : baseline
                    );
                    memo.utility[i] = child_value * reach_other / reach_sample;
                    value += child_value * memo.policy[i];
                }
                regret_minimizers[info_set_idx].observe_utility(memo.utility, memo.policy);

                // update average policy
                for(int i = 0; i < num_actions; i++) {
                    // why not update with new policy?
                    T increment = reach_me * memo.policy[i] / reach_sample;
                    average_policy[info_set_idx][memo.actions[i]] += increment;
                }
                // later put locks here if we are doing parallelism
            }
            return value;
        }
    };
} // namespace mccfr
