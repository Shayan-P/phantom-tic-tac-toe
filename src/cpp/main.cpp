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

int train() {
    MCCFR<Game> mccfr;

    while(true) {
        mccfr.iteration();   

        // todo checkpoint after a bunch of iteratios
        
        // todo log and keep track of progress

        // plot? some metric so that we can see this making process
        // e.g. the nash gap?
    }
    return 0;
}

int main() {
    ios_base::sync_with_stdio(0); cin.tie(0); cout.tie(0);

    Game game;
    cout << game << endl;
    
    // for(int i = 0; i < Game::NUM_INFO_SETS; i++) {
    //     for(int j = 0; j < Game::ACTION_MAX_DIM; j++) {
    //         buffer[i][j] = 0.0;
    //     }
    // }

    // RegretMinimizer<Game::ACTION_MAX_DIM> regret_minimizers[Game::NUM_INFO_SETS];
    // std::array<T, Game::ACTION_MAX_DIM> average_policy[Game::NUM_INFO_SETS];

    MCCFR<Game> mccfr;

    mccfr.iteration();
}