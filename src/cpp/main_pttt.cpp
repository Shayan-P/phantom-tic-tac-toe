// #define NO_PRECOMPUTE

#include "pttt.hpp"
#include "mccfr.hpp"
#include "evaluator.hpp"
#include <thread>
#include <atomic>
// #include "loaded_game.hpp"

// using Game = loaded_game::Kuhn;
using Game = pttt::PTTT;
using MCCFR = mccfr::MCCFR<Game>;
using Strategy = strategy::Strategy<Game>;
using Eval = eval::Eval<Game>;
using namespace std;

// void sample_rollout() {
//     Game game;

//     cout << game << endl;
    
//     srand(time(NULL));
//     while(true) {
//         Player player = game.current_player();
//         std::vector<Action> actions = valid_actions(player, game.player_observation(player));
//         cout << "Player " << player << " has " << actions.size() << " valid actions." << endl;
//         Action action = actions[rand() % actions.size()];
//         bool done = game.step(action);
//         cout << game << endl;
//         if(done) {
//             cout << "Player " << player << " wins!" << endl;
//             break;
//         }
//         if(actions.size() == 0) {
//             cout << "No more valid actions. Game over." << endl;
//             break;
//         }
//     }
// }

int main() {
    ios_base::sync_with_stdio(0); cin.tie(0); cout.tie(0);

    #ifdef NO_PRECOMPUTE
    cout << "NO_PRECOMPUTE MODE" << endl;
    #else
    cout << "PRECOMPUTE MODE" << endl;
    #endif

    MCCFR mccfr;
    Game::precompute_if_needed(); // do this before starting the threads...

    int num_threads = thread::hardware_concurrency();
    cout << "Number of available cores: " << num_threads << endl; // I think this might not be the number of cores you have access to... maybe set this manually?

    atomic<int> iters(0);
    vector<thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&mccfr, &iters]() {
            while (true) {
                mccfr.iteration();
                iters++;
            }
        });
    }

    threads.emplace_back([&mccfr, &iters]() {
        // logging thread 
        Eval eval;

        auto start = chrono::steady_clock::now();
        auto last_checkpoint = start;
        auto elapsed_since_start_prev = -1;
        
        struct Stat {
            int minutes;
            int iters;
            double nash_gap;
        };
        std::vector<Stat> stats;

        while (true) {
            auto now = chrono::steady_clock::now();
            auto elapsed_since_start = chrono::duration_cast<chrono::minutes>(now - start).count();
            auto elapsed_since_check = chrono::duration_cast<chrono::minutes>(now - last_checkpoint).count();

            cout << "minute_count=" << elapsed_since_start << " iters=" << iters.load() << endl;
            if(elapsed_since_check > 60) { // every hour
                last_checkpoint = now;
                time_t t = time(nullptr);
                tm* timePtr = localtime(&t);
                char buffer[80];
                strftime(buffer, sizeof(buffer), "parallel_checkpoint__%m_%d_%Y_%H_%M_%S", timePtr);
                mccfr.save_checkpoint(buffer);
                mccfr.save_checkpoint("latest");
            }
            if(true) { // define the frequency later...
                Strategy strategy = mccfr.get_strategy(); // loads a huge data structure. try to avoid :D 
                std::cout << "P1 against unifrom: " << strategy.evaluate_against_uniform(Game::Player::P1, 5000) << std::endl;
                std::cout << "P2 against uniform: " << strategy.evaluate_against_uniform(Game::Player::P2, 5000) << std::endl;
                auto nash_gap = eval.nash_gap(strategy);
                stats.push_back({int(elapsed_since_start), iters.load(), nash_gap});
                std::cout << "nash gap " << nash_gap << std::endl;

                // plot
                std::vector<double> nash_gap_data;
                std::vector<int> iters_data;
                std::vector<int> minutes_data;
                for(auto &stat: stats) {
                    nash_gap_data.push_back(stat.nash_gap);
                    iters_data.push_back(stat.iters);
                    minutes_data.push_back(stat.minutes);
                }
                io::save_to_numpy<double>("./nash_gaps.npy", nash_gap_data.begin(), nash_gap_data.end());
                io::save_to_numpy<int>("./iters.npy", iters_data.begin(), iters_data.end());
                io::save_to_numpy<int>("./minutes.npy", minutes_data.begin(), minutes_data.end());
            }
            this_thread::sleep_for(chrono::minutes(1));
        }
    });

    for (auto& t : threads) {
        t.join();
    }
}