// #define NO_PRECOMPUTE

#include "pttt.hpp"
#include "mccfr.hpp"
#include <thread>
#include <atomic>

using pttt::PTTT;
using namespace std;
using mccfr::MCCFR;

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

    MCCFR<PTTT> mccfr;    
    PTTT::precompute_if_needed(); // do this before starting the threads...

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
        auto start = chrono::steady_clock::now();
        auto last_checkpoint = start;
        auto elapsed_since_start_prev = -1;
        int minute_count = 0;

        while (true) {
            cout << "minute_count=" << minute_count << " iters=" << iters.load() << endl;
            if(minute_count % 60 == 0) { // every hour
                time_t t = time(nullptr);
                tm* timePtr = localtime(&t);
                char buffer[80];
                strftime(buffer, sizeof(buffer), "parallel_checkpoint__%m_%d_%Y_%H_%M_%S", timePtr);
                mccfr.save_checkpoint(buffer);
                mccfr.save_checkpoint("latest");
            }
            minute_count++;
            this_thread::sleep_for(chrono::minutes(1));
        }
    });

    for (auto& t : threads) {
        t.join();
    }
}