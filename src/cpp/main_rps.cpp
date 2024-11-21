// #define NO_PRECOMPUTE

#include "rps.hpp"
#include "mccfr.hpp"

using rps::RPS;
using namespace std;
using mccfr::MCCFR;

int main() {
    ios_base::sync_with_stdio(0); cin.tie(0); cout.tie(0);

    MCCFR<RPS> mccfr;    

    for(int i = 0; i < 1000; i++) {
        mccfr.iteration();
    }

    mccfr.debug_print();
}