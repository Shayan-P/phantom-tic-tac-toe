#include "pttt.hpp"

using namespace std;

// template<Game game>
using Game = pttt::PTTT;

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
    return res;
}

int main() {
    cout << count_histories(Game()) << endl;
}
