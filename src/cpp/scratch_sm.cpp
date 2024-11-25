#include <array>
#include <iostream>

template<typename T, std::size_t N>
std::ostream& operator<<(std::ostream& os, const std::array<T, N>& arr) {
    os << "[";
    for (std::size_t i = 0; i < N; ++i) {
        os << arr[i];
        if (i < N - 1) {
            os << ", ";
        }
    }
    os << "]";
    return os;
}


#include "loaded_game.hpp"
#include "rps.hpp"
#include "paths.h"
#include "mccfr_es.hpp"
#include "evaluator.hpp"
#include "io.hpp"

using namespace std;
using mccfr_es::MCCFR;


template<typename Game>
int count_histories(Game state) {
    if(state.is_terminal()) {
        return 1;
    }
    int num_actions = state.num_actions();
    std::array<int, Game::ACTION_MAX_DIM> actions;
    state.actions(actions);
    int res = 0;
    for(int i = 0; i < num_actions; i++) {
        Game new_state = state;
        new_state.step(actions[i]);
        res += count_histories(new_state);
    }
    return res + 1;
}

void nash_gap_check() {
    using Game = loaded_game::Kuhn;
    Game game;
    MCCFR<Game> mccfr;
    strategy::Strategy<Game> strategy = game.get_strategy(mccfr.get_strategy_data());
    
    std::cout << "P1 " << strategy.evaluate(Game::Player::P1, 100000) << std::endl;
    std::cout << "P2 " << strategy.evaluate(Game::Player::P2, 100000) << std::endl;
    std::cout << "result from evaluator " << std::endl;
    eval::Eval<Game> evaluator;

    std::cout << "created eval " << std::endl;

    std::cout << "P1 " << evaluator.eval_for(strategy, Game::Player::P1) << std::endl;
    std::cout << "P2 " << evaluator.eval_for(strategy, Game::Player::P2) << std::endl;

    strategy::Strategy<Game> strategy_br1 = evaluator.best_response(strategy, Game::Player::P1);

    std::cout << "after best response player 1 " << std::endl;
    std::cout << "P1 " << strategy_br1.evaluate(Game::Player::P1, 10000) << std::endl;
    std::cout << "P2 " << strategy_br1.evaluate(Game::Player::P2, 10000) << std::endl;

    std::cout << "P1 " << evaluator.eval_for(strategy_br1, Game::Player::P1) << std::endl;
    std::cout << "P2 " << evaluator.eval_for(strategy_br1, Game::Player::P2) << std::endl;

    std::cout << "nash gap: " << evaluator.nash_gap(strategy) << std::endl;

    std::cout << "nash gap: " << evaluator.nash_gap(strategy_br1) << std::endl;
}


template<typename Game>
std::ostream& operator<<(std::ostream& os, const strategy::Strategy<Game>& strat) {
    os << "Strategy: [";
    for (std::size_t i = 0; i < strat.strat.size(); ++i) {
        os << strat.strat[i];
        if (i < strat.strat.size() - 1) {
            os << ", ";
        }
    }
    os << "]";
    return os;
}

int main() {
    using Game = loaded_game::Leduc;

    MCCFR<Game> mccfr;
    eval::Eval<Game> evaluator;

    std::vector<double> gaps;
    for(int i = 0; i < 50000; i++) {
        mccfr.iteration();
        strategy::Strategy<Game> strategy = mccfr.get_strategy();
        auto gap = evaluator.nash_gap(strategy);
        gaps.push_back(gap);
        std::cout << "nash gap: " << gap << std::endl;
        assert(gap >= 0);
    }
    io::save_to_numpy<double>("./gaps_baseline.npy", gaps.begin(), gaps.end());

    // cout << count_histories(Game()) << endl;
    // loaded_game::LoadedGame(paths::get_kuhn_descriptor());

    // cout << count_histories(kuhn) << endl;
    // while(true) {
    //     mccfr.iteration();    
    //     strategy::Strategy<loaded_game::Kuhn> strategy = kuhn.get_strategy(mccfr.get_strategy_data());
    //     cout << "P1 " << strategy.evaluate(loaded_game::Kuhn::Player::P1) << endl;
    //     cout << "P2 " << strategy.evaluate(loaded_game::Kuhn::Player::P2) << endl;
    // }
}
