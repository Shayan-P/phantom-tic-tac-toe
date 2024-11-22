// #define NO_PRECOMPUTE

#include "rps.hpp"
#include "mccfr.hpp"

using rps::RPS;
using namespace std;
using mccfr::MCCFR;
using strategy::Strategy;


int main() {
    ios_base::sync_with_stdio(0); cin.tie(0); cout.tie(0);

    MCCFR<RPS> mccfr;    

    for(int i = 0; i < 10000; i++) {
        mccfr.iteration();
    }
    Strategy strategy = mccfr.get_strategy();
    std::cout << strategy.evaluate_against_uniform(RPS::P1) << std::endl;
    std::cout << strategy.evaluate_against_uniform(RPS::P1) << std::endl;

    // mccfr.regret_minimizers[0].set_dim(3);
    // mccfr.regret_minimizers[1].set_dim(3);

    // mccfr.debug_print();
    // for(int i = 0; i < 10000; i++) {
    //     // mccfr.debug_print();
    //     cout << "--------------------------------------\n";
    //     mccfr.iteration();
    // }
    // mccfr.debug_print();

    // cout << "evaluating: \n";

    // Strategy<RPS> strategy({
    //     {1, 0, 0},
    //     {0, 0, 1}
    // });

    // Strategy<RPS> strategy = mccfr.get_strategy();

    // strategy.debug_print();

    // cout << "P1: " << strategy.evaluate(RPS::P1) << endl;
    // cout << "P2: " << strategy.evaluate(RPS::P2) << endl;
}