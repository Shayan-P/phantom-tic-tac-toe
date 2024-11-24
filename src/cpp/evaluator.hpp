#ifndef EVALUATOR_HPP
#define EVALUATOR_HPP

#include "pttt.hpp"
#include "strategy.hpp"

namespace eval {
    template<typename Game>
    class Eval {
        using T = double;
        using Strategy = strategy::Strategy<Game>;

        struct History {
            const Game state;
            int infoset_idx;
            std::vector<int> actions;
            std::vector<History> children;

            // the order of actions should be the same across all the information sets
            static std::vector<int> get_actions(const Game &state) {
                if(state.is_terminal())
                    return {};
                int num_actions = state.num_actions();
                std::array<int, Game::ACTION_MAX_DIM> actions;
                state.actions(actions);
                return {actions.begin(), actions.begin() + num_actions};
            }

            static std::vector<History> get_children(const Game &state) {
                if(state.is_terminal())
                    return {};
                std::vector<History> res;  
                int num_actions = state.num_actions();
                std::array<int, Game::ACTION_MAX_DIM> actions;
                state.actions(actions);
                for(int i = 0; i < num_actions; i++) {
                    Game new_state = state;
                    new_state.step(actions[i]);
                    res.push_back(History(new_state));
                }
                return res;
            }

            History(const Game &state): 
                state(state),
                infoset_idx((state.is_terminal() || state.is_chance()) ? -1 : state.info_set_idx()),
                actions(get_actions(state)),
                children(get_children(state))
            {}

            T value;
            T p_reach_others;
        };
        struct Infoset {
            std::vector<History*> histories;

            mutable bool visited;
        };

        using Player = typename Game::Player;

        History root;
        std::vector<Infoset> infosets;

        void create_rec(History &history) {
            if(history.state.is_terminal()) {
                return;
            }
            if(!history.state.is_chance()) {
                infosets[history.infoset_idx].histories.push_back(&history);
            }
        for(auto &child: history.children) {
                create_rec(child);
            }
        }

    public:
        Eval(): root(Game()), infosets(Game::NUM_INFO_SETS) {
            create_rec(root);
        }

        Strategy best_response(const Strategy &strategy, Player player) {
            Strategy copy_strategy(strategy.strat);
            for(auto &infoset: infosets) {
                infoset.visited = false;
            }
            best_response_rec_p_reach_calc(root, player, copy_strategy, 1.0);
            best_response_rec(root, player, copy_strategy);
            return copy_strategy;
        }

        void best_response_rec_p_reach_calc(History &hist, const Player &player, Strategy &strategy, const double p_reach_other) {
            hist.p_reach_others = p_reach_other;
            if(hist.state.is_terminal())
                return;
            std::array<T, Game::ACTION_MAX_DIM> probs;
            if(hist.state.is_chance()) {
                hist.state.action_probs(probs);
            } else {
                for(int i = 0; i < hist.actions.size(); i++) {
                    probs[i] = hist.state.current_player() == player ? 1 : strategy.strat[hist.infoset_idx][hist.actions[i]];
                }
            }
            for(int i = 0; i < hist.actions.size(); i++) {
                best_response_rec_p_reach_calc(hist.children[i], player, strategy, p_reach_other * probs[i]);
            }
        }

        void best_response_rec(History &hist, const Player &player, Strategy &strategy) {
            if(hist.state.is_terminal()) {
                hist.value = hist.state.utility(player) * hist.p_reach_others;
            } else if (hist.state.is_chance() || hist.state.current_player() != player) {
                int num_actions = hist.actions.size();
                std::array<T, Game::ACTION_MAX_DIM> probs;
                if(hist.state.is_chance()) {
                    hist.state.action_probs(probs);
                } else {
                    for(int i = 0; i < num_actions; i++)
                        probs[i] = strategy.strat[hist.infoset_idx][hist.actions[i]];
                }
                hist.value = 0;
                for(int i = 0; i < num_actions; i++) {
                    best_response_rec(hist.children[i], player, strategy);
                    hist.value += hist.children[i].value;
                }
            } else { // cur_player == player
                assert(hist.state.current_player() == player);
                int idx = hist.infoset_idx;
                if(infosets[idx].visited) {
                    return;
                }
                infosets[idx].visited = true;
                std::array<T, Game::ACTION_MAX_DIM> vals;
                vals.fill(0);

                for(History *h: infosets[idx].histories) { // do all at once
                    for(int i = 0; i < hist.actions.size(); i++) {
                        best_response_rec(h->children[i], player, strategy);
                        vals[i] += h->children[i].value;
                    }
                }

                int argmaxi = 0;

                for(int i = 0; i < hist.actions.size(); i++) {
                    if(vals[i] > vals[argmaxi]) {
                        argmaxi = i;
                    }
                }

                for(int i = 0; i < hist.actions.size(); i++) {
                    strategy.strat[idx][hist.actions[i]] = i == argmaxi ? 1 : 0;
                }
                for(History *h: infosets[idx].histories) {
                    h->value = h->children[argmaxi].value;
                }
            }
        }

        T eval_for(const Strategy &strategy, Player player) {
            return eval_for_rec(root, strategy, player);
        }

        T eval_for_rec(History &hist, const Strategy &strategy, const Player &player) {
            if(hist.state.is_terminal()) {
                return hist.state.utility(player);
            }
            int num_actions = hist.actions.size();
            std::array<T, Game::ACTION_MAX_DIM> probs;
            if(hist.state.is_chance()) {
                hist.state.action_probs(probs);
            } else {
                for(int i = 0; i < num_actions; i++)
                    probs[i] = strategy.strat[hist.infoset_idx][hist.actions[i]];
            }
            T res = 0;
            for(int i = 0; i < num_actions; i++) {
                res += probs[i] * eval_for_rec(hist.children[i], strategy, player);
            }
            return res;
        }

        T nash_gap(const Strategy &strategy) {
            // only for two player games...
            assert(Game::NUM_PLAYERS == 2);
            auto p1 = Game::players[0];
            auto p2 = Game::players[1];
            Strategy strategy_p1 = best_response(strategy, p1);
            Strategy strategy_p2 = best_response(strategy, p2);

            return eval_for(strategy_p1, p1) - eval_for(strategy_p2, p1);
        }
    };

    // hopefully fast
    template<typename Game>
    class Treeplex {
        enum NodeType {
            DECISION, OBSERVATION
        };
        using T = double;
        using Strategy = typename strategy::Strategy<Game>;

        struct Node {
            std::map<int, Node*> children;
            NodeType type;
            int info_set_idx;
            int num_actions;
            std::array<int, Game::ACTION_MAX_DIM> actions;

            double value;

            Node* get_or_create_child(int idx) {
                if(children.find(idx) == children.end()) {
                    children[idx] = new Node();
                }
                return children[idx];
            }
        };

        using Player = typename Game::Player;

        Node root;
        Player player;

    public:
        Treeplex(Player player): player(player) {}

        void update_from_root(const Strategy &strategy) {
            reset(root);
            update(Game(), root, strategy, 1.0);
        }

        void reset(Node &node) {
            node.value = 0;
            node.info_set_idx = -1;
            for(auto [_, child]: node.children) {
                reset(*child);
            }
        }

        void update(const Game& state, Node& node, const Strategy &strategy, double p_reach) {
            if(state.is_terminal()) {
                node.type = OBSERVATION;
                node.value += state.utility(player) * p_reach;
                return;
            }

            std::array<double, Game::ACTION_MAX_DIM> probs;
            std::array<int, Game::ACTION_MAX_DIM> actions;
            state.actions(actions);
            int num_actions = state.num_actions();
            int info_set_idx = state.info_set_idx();
            if(state.is_chance()) {
                state.action_probs(probs);
            } else {
                for(int i = 0; i < num_actions; i++) {
                    probs[i] = strategy.strat[info_set_idx][actions[i]];
                }
            }
            
            if(state.is_chance() || state.current_player() != player) {
                node.type = OBSERVATION;

                for(int i = 0; i < num_actions; i++) {
                    Game new_state = state;
                    new_state.step(actions[i]);
                    if(new_state.is_terminal() || new_state.is_chance() || new_state.current_player() != player) {
                        update(new_state, node, strategy, p_reach * probs[i]);
                    } else {
                        update(new_state, *node.get_or_create_child(new_state.info_set_idx()), strategy, p_reach * probs[i]);
                    }
                }
            } else {
                assert(state.current_player() == player);

                node.type = DECISION;
                node.info_set_idx = state.info_set_idx();
                node.num_actions = num_actions;
                node.actions = actions;
                // put some assert that the order of actions are the same: TODO :D                
                for(int i = 0; i < num_actions; i++) {
                    Game new_state = state;
                    new_state.step(actions[i]);
                    update(new_state, *node.get_or_create_child(i), strategy, p_reach);
                }
            }
        }

        Strategy best_response(const Strategy &strategy) {
            update_from_root(strategy);
            Strategy new_strategy(strategy.strat);
            best_response_rec(root, new_strategy);
            return new_strategy;
        }

        void best_response_rec(Node& node, Strategy &strategy) {
            if(node.type == OBSERVATION) {
                for(auto [_, child]: node.children) {
                    best_response_rec(*child, strategy);
                    node.value += child->value;
                }
            } else {
                int best_action = -1;
                T prev_best = 0;
                for(auto [action_idx, child]: node.children) {
                    best_response_rec(*child, strategy);
                    if(best_action == -1 || prev_best < (child->value)) {
                        best_action = action_idx;
                        prev_best = child->value;
                    }
                }
                node.value = prev_best;
                for(int i = 0; i < Game::ACTION_MAX_DIM; i++) {
                    strategy.strat[node.info_set_idx][i] = 0;
                }
                for(int i = 0; i < node.num_actions; i++) {
                    strategy.strat[node.info_set_idx][node.actions[i]] = (i == best_action);
                }
            }
        }

        T get_last_eval() {
            return root.value;
        }
    };

    template<typename Game>
    class EvalFast {
        using Player = typename Game::Player;
        using Strategy = typename strategy::Strategy<Game>;
        using T = double;

        std::map<Player, Treeplex<Game>*> treeplexes;

    public:

        EvalFast() {
            for(auto player: Game::players) {
                treeplexes[player] = new Treeplex<Game>(player);
            }
        }

        Strategy best_response(const Strategy &strategy, Player player) {
            return treeplexes[player]->best_response(strategy);
        }

        T nash_gap(const Strategy &strategy) {
            // only for two player games...
            assert(Game::NUM_PLAYERS == 2);
            auto p1 = Game::players[0];
            auto p2 = Game::players[1];
            Strategy strategy_p1 = best_response(strategy, p1);
            Strategy strategy_p2 = best_response(strategy, p2);
            return treeplexes[p1]->get_last_eval() + treeplexes[p2]->get_last_eval();
            // return eval_for(strategy_p1, p1) - eval_for(strategy_p2, p1);
        }
    };
}

#endif