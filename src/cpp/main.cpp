// #define NO_PRECOMPUTE

#include "pttt.hpp"
#include "mccfr.hpp"

using pttt::Game;
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

    MCCFR<Game> mccfr;    

    auto start = chrono::steady_clock::now();
    auto last_checkpoint = start;
    auto elapsed_since_start_prev = -1;
    int iters = 0;
    
    while(true) {
        mccfr.iteration();   
        iters++;

        auto now = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::hours>(now - last_checkpoint).count();
        auto elapsed_since_start = chrono::duration_cast<chrono::minutes>(now - last_checkpoint).count();

        if(elapsed_since_start != elapsed_since_start_prev) {
            cout << "Elapsed time: " << elapsed_since_start << " minutes" << endl;
            cout << iters << " iterations" << endl;
            elapsed_since_start_prev = elapsed_since_start;
        }

        if (elapsed >= 1) {
            time_t t = time(nullptr);
            tm* timePtr = localtime(&t);
            char buffer[80];
            strftime(buffer, sizeof(buffer), "checkpoint__%m_%d_%Y_%H_%M_%S", timePtr);
            mccfr.save_checkpoint(buffer);
            mccfr.save_checkpoint("latest");
            last_checkpoint = now;
        }

        // todo checkpoint after a bunch of iterations
        
        // todo log and keep track of progress

        // plot? some metric so that we can see this making process
        // e.g. the nash gap?
    }
}