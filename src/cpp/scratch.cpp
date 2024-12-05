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
#include "mccfr.hpp"
#include "mccfr_es.hpp"
#include "evaluator.hpp"
#include "io.hpp"

using namespace std;


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

struct Stat {
    double seconds;
    long long iterations;
    double nash_gap;
};

using Game = rps::RPS;
const std::string game_name = "rps";

template<class MCCFR>
void gather_data(MCCFR &mccfr, std::string name, int seconds_to_run) {
    eval::EvalFast<Game> evaluator_fast;
    std::vector<Stat> stats;

    auto start = std::chrono::steady_clock::now();

    long long iterations = 0;

    while(true) {
        mccfr.iteration();
        strategy::Strategy<Game> strategy = mccfr.get_strategy();
        auto gap_fast = evaluator_fast.nash_gap(strategy);
        iterations++;

        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_seconds = now - start;
        double secs = elapsed_seconds.count();

        stats.push_back({secs, iterations, gap_fast});

        if(secs >= seconds_to_run) {
            break;
        }
    }

    std::vector<double> nash_gap_data;
    std::vector<long long> iters_data;
    std::vector<double> seconds_data;

    for (const auto& stat : stats) {
        nash_gap_data.push_back(stat.nash_gap);
        iters_data.push_back(stat.iterations);
        seconds_data.push_back(stat.seconds);
    }

    io::save_to_numpy<int>("./" + name + "iters.npy", iters_data.begin(), iters_data.end());
    io::save_to_numpy<int>("./" + name + "seconds.npy", seconds_data.begin(), seconds_data.end());
    io::save_to_numpy<double>("./" + name + "nash_gaps.npy", nash_gap_data.begin(), nash_gap_data.end());
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

#include <thread>

int main() {
    mccfr::MCCFR<Game> mccfr;
    mccfr_es::MCCFR<Game> mccfr_es;
    eval::EvalFast<Game> evaluator_fast;

    int time_limit = 60 * 5;

    std::thread thread1(gather_data<mccfr::MCCFR<Game>>, std::ref(mccfr), game_name + "_os_", time_limit);
    std::thread thread2(gather_data<mccfr_es::MCCFR<Game>>, std::ref(mccfr_es), game_name + "_es_", time_limit);

    thread1.join();
    thread2.join();
}
