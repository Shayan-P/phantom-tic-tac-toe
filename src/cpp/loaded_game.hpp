#ifndef LOADED_GAME_HPP
#define LOADED_GAME_HPP

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <iterator>
#include <iostream>
#include <cassert>
#include <map>
#include "paths.hpp"
#include <array>

namespace loaded_game {
    class LoadedGame {        
    public:
        enum Type {
            TERMINAL, CHANCE, PLAYER
        };

        struct History {
            std::string hist;
            Type type;
            std::string player_name;
            std::map<std::string, double> action_probs;
            std::map<std::string, double> payoffs;
            std::vector<std::string> actions;
            int infoset_idx = -1;
        };

        struct Infoset {
            std::string name;
            std::vector<std::string> histories;
            std::vector<std::string> actions;
        };

        std::map<std::string, History> histories;
        std::vector<Infoset> infosets;

    public:
        LoadedGame(const std::string &filename) {
            std::ifstream file(filename);
            std::string line;

            while (std::getline(file, line)) {
                line.erase(line.find_last_not_of(" \n\r\t") + 1); // Strip trailing whitespace
                line.erase(0, line.find_first_not_of(" \n\r\t")); // Strip leading whitespace
                std::istringstream iss(line);
                std::vector<std::string> words((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());
                if(words.size() == 0) {
                    continue;
                }

                if(words[0] == "node") {
                    History history;
                    history.hist = words[1];
                    auto type = words[2];
                    if(type == "chance") {
                        history.type = CHANCE;
                        history.action_probs = read_dict(std::vector<std::string>(words.begin() + 4, words.end()));
                        for (const auto &pair : history.action_probs) {
                            history.actions.push_back(pair.first);
                        }
                    } else if(type == "terminal") {
                        history.type = TERMINAL;
                        history.payoffs = read_dict(std::vector<std::string>(words.begin() + 4, words.end()));
                    } else if (type == "player") {
                        history.type = PLAYER;
                        history.player_name = words[3];
                        history.actions = std::vector<std::string>(words.begin() + 5, words.end());
                    } else {
                        assert(false);
                    }
                    histories[history.hist] = history;
                } else if(words[0] == "infoset") {
                    Infoset infoset;
                    infoset.name = words[1];
                    infoset.histories = std::vector<std::string>(words.begin() + 3, words.end());
                    for(auto hist: infoset.histories) {
                        assert(histories.count(hist) > 0);
                        histories[hist].infoset_idx = infosets.size();
                        if(infoset.actions.size() == 0) {
                            infoset.actions = histories[hist].actions;
                        } else {
                            assert(infoset.actions.size() == histories[hist].actions.size()); 
                        }
                    }
                    infosets.push_back(infoset);
                } else {
                    assert(false);
                }
            }
            file.close();

            for(auto &[name, hist]: histories) {
                if(hist.type == PLAYER) {
                    assert(hist.infoset_idx != -1);
                }
            }
        }
    private:
        std::map<std::string, double> read_dict(const std::vector<std::string> &words) {
            std::map<std::string, double> dict;
            for(auto word: words) {
                auto pos = word.find('=');
                assert(pos != std::string::npos);
                dict[word.substr(0, pos)] = std::stod(word.substr(pos + 1));
            }
            return dict;
        }
    };

    template<int ACTION_MAX_DIM, typename Player>
    class LoadedState {
        const LoadedGame &game;
        std::map<Player, std::string> &player_to_name;
        std::map<std::string, Player> &name_to_player;

        std::string current_hist = "/";

        public:
            LoadedState(const LoadedGame &game, std::map<Player, std::string> &player_to_name, std::map<std::string, Player> &name_to_player):
                game(game), player_to_name(player_to_name), name_to_player(name_to_player) {}

        using T = double;
        using Buffer = std::array<T, ACTION_MAX_DIM>;
        using BufferInt = std::array<int, ACTION_MAX_DIM>;

        bool is_terminal() const {
            return game.histories.at(current_hist).type == LoadedGame::TERMINAL;
        }

        T utility(Player player) const {
            auto player_name = player_to_name.at(player);
            assert(is_terminal());
            return game.histories.at(current_hist).payoffs.at(player_name);
        }

        bool is_chance() const {
            return game.histories.at(current_hist).type == LoadedGame::CHANCE;
        }

        bool is_player() const {
            return game.histories.at(current_hist).type == LoadedGame::PLAYER;
        }

        std::vector<std::string> actions() const {
            assert(!is_terminal());
            if(is_chance()) {
                return game.histories.at(current_hist).actions;
            } else {
                int infoset_idx = game.histories.at(current_hist).infoset_idx;
                return game.infosets[infoset_idx].actions;
            }
        }

        void action_probs(Buffer& buffer) const {
            assert(is_chance());
            int i = 0;
            for(auto action: actions()) {
                buffer[i] = game.histories.at(current_hist).action_probs.at(action);
                i++;
            }
        }

        int num_actions() const {
            int res = actions().size();
            assert(res <= ACTION_MAX_DIM);
            return res;
        }

        std::string current_player_name() const {
            assert(is_player());
            return game.histories.at(current_hist).player_name;
        }

        void step(int action_) {
            assert(action_ < num_actions());
            assert(!is_terminal());
            std::string action = actions()[action_];
            std::string player = is_chance() ? "C" : ("P" + current_player_name());
            current_hist += player + ":" + action + "/";
        }

        // legacy
        void actions(BufferInt &buffer) const { // don't use vector or dynamic memory
            int n = num_actions();
            for(int i = 0; i < n; i++) {
                buffer[i] = i;
            }
        }

        inline Player current_player() const {
            assert(is_player());
            auto player_name = game.histories.at(current_hist).player_name;
            return name_to_player.at(player_name);
        }

        int info_set_idx() const {
            return game.histories.at(current_hist).infoset_idx;
        }

        static std::vector<std::array<T, ACTION_MAX_DIM>> get_strategy(const LoadedGame& game, const std::vector<std::array<T, ACTION_MAX_DIM>> &average_policy) {
            std::vector<std::array<T, ACTION_MAX_DIM>> result(game.infosets.size());

            assert(game.infosets.size() == average_policy.size());

            for(int idx = 0; idx < game.infosets.size(); idx++) {
                result[idx] = average_policy[idx];
                T sm = 0;
                for(int j = 0; j < ACTION_MAX_DIM; j++) {
                    sm += average_policy[idx][j];
                }
                if (sm <= 1e-9) {
                    int sz = game.infosets[idx].actions.size();
                    for(int j = 0; j < sz; j++) {
                        result[idx][j] = 1.0;
                    }
                    sm = sz;
                }
                for(int j = 0; j < ACTION_MAX_DIM; j++) {
                    result[idx][j] /= sm;
                }

                double assert_sm = 0;
                for(int j = 0; j < ACTION_MAX_DIM; j++) {
                    assert_sm += result[idx][j];
                }
                assert(abs(assert_sm - 1) <= 1e-5);
            }
            return result;
        }

    };

    enum Player2PG {
        P1, P2
    };
    static std::map<Player2PG, std::string> player_to_name_2pg = {
        {P1, "1"},
        {P2, "2"}
    };
    static std::map<std::string, Player2PG> name_to_player_2pg = {
        {"1", P1},
        {"2", P2}
    };

    static LoadedGame kuhn(paths::get_kuhn_descriptor());
    class Kuhn: public LoadedState<6, Player2PG> {
    public:
        using Player = Player2PG;

        static constexpr int ACTION_MAX_DIM = 6; // 6 because of the chance nodes... bad architecture...
        static constexpr int NUM_PLAYERS = 2;
        static constexpr int NUM_INFO_SETS = 68 - 56;
        static const LoadedGame &my_game;
        static const std::array<Player, NUM_PLAYERS> players;

        Kuhn(): LoadedState<ACTION_MAX_DIM, Player2PG>(Kuhn::my_game, loaded_game::player_to_name_2pg, loaded_game::name_to_player_2pg) {}

        static std::vector<std::array<T, ACTION_MAX_DIM>> get_strategy(const std::vector<std::array<T, ACTION_MAX_DIM>> &average_policy) {
            return LoadedState::get_strategy(Kuhn::my_game, average_policy);
        }
    };
    const LoadedGame& Kuhn::my_game = kuhn;
    const std::array<Kuhn::Player, Kuhn::NUM_PLAYERS> Kuhn::players = {Kuhn::Player::P1, Kuhn::Player::P2};


    static LoadedGame leduc(paths::get_leduc_descriptor());
    class Leduc: public LoadedState<6, Player2PG> {
    public:
        using Player = Player2PG;

        static constexpr int ACTION_MAX_DIM = 6; // 6 because of the chance nodes... bad architecture...
        static constexpr int NUM_PLAYERS = 2;
        static constexpr int NUM_INFO_SETS = 2225 - 1937;
        static const LoadedGame &my_game;
        static const std::array<Player, NUM_PLAYERS> players;

        Leduc(): LoadedState<ACTION_MAX_DIM, Player2PG>(kuhn, loaded_game::player_to_name_2pg, loaded_game::name_to_player_2pg) {}

        static std::vector<std::array<T, ACTION_MAX_DIM>> get_strategy(const std::vector<std::array<T, ACTION_MAX_DIM>> &average_policy) {
            return LoadedState::get_strategy(Leduc::my_game, average_policy);
        }
    };
    const LoadedGame& Leduc::my_game = leduc;
    const std::array<Leduc::Player, Leduc::NUM_PLAYERS> Leduc::players = {Leduc::Player::P1, Leduc::Player::P2};

}

#endif