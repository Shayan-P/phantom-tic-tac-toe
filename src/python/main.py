import matplotlib.pyplot as plt
import numpy as np
from game_description import GameDescriptor
from game import GameTree, Treeplex, CFR, MCCFR


games = []

with open("./kuhn.txt") as f:
    games.append(("kuhn", GameDescriptor.parse(f.read())))

with open("./leduc2.txt") as f:
    games.append(("leduc", GameDescriptor.parse(f.read())))



def check1(game_name, game_desc, CFR_CLASS):
    print('--------------------------------')
    print("processing game: ", game_name)
    game_tree = GameTree(game_desc)
    player1 = '1'
    player2 = '2'
    treeplex1 = Treeplex(game_tree=game_tree, player=player1)
    treeplex2 = Treeplex(game_tree=game_tree, player=player2)

    uniform_strategies = {p: game_tree.get_player_strategy_uniform_behavioral(p) for p in game_desc.players}

    cfr = CFR_CLASS(treeplex1)
    T = 1000
    average_utilities_p1 = []
    average_reach_prob_player1 = np.zeros_like(uniform_strategies[player1])
    average_reach_prob_player2 = np.zeros_like(uniform_strategies[player2])
    for t in range(T):
        strat1 = cfr.next_strategy_behavioral()
        strat2 = uniform_strategies[player2]

        average_reach_prob_player1 += (1.0 / (t+1)) * (game_tree.behavioral_to_reach_probability(strat1) - average_reach_prob_player1)
        average_reach_prob_player2 += (1.0 / (t+1)) * (game_tree.behavioral_to_reach_probability(strat2) - average_reach_prob_player2)

        u1 = game_tree.eval_utility_by_reach_probability({player1: average_reach_prob_player1, player2: average_reach_prob_player2}, player1)
        average_utilities_p1.append(u1)

        cfr.improve({player2: uniform_strategies[player2]})

    print(f"final utility for player {player1}", average_utilities_p1[-1])
    plt.plot(average_utilities_p1)
    plt.xlabel('iterations')
    plt.ylabel('average utility player1')
    plt.title("game: " + game_name)
    plt.show()


def check2(game_name, game_desc, CFR_CLASS):
    print('--------------------------------')
    print("processing game: ", game_name)
    game_tree = GameTree(game_desc)
    player1 = '1'
    player2 = '2'
    treeplex1 = Treeplex(game_tree=game_tree, player=player1)
    treeplex2 = Treeplex(game_tree=game_tree, player=player2)

    cfr1 = CFR_CLASS(treeplex1)
    cfr2 = CFR_CLASS(treeplex2)
    
    T = 1000
    average_utilities_p1 = []
    strat1 = cfr1.next_strategy_behavioral()
    strat2 = cfr2.next_strategy_behavioral()
    average_reach_prob_player1 = strat1.copy()
    average_reach_prob_player2 = strat2.copy()
    ne_gaps = []

    for t in range(1, T):
        cfr1.improve({player2: strat2})
        strat1 = cfr1.next_strategy_behavioral()
        cfr2.improve({player1: strat1})
        strat2 = cfr2.next_strategy_behavioral()

        average_reach_prob_player1 += (1.0 / (t+1)) * (game_tree.behavioral_to_reach_probability(strat1) - average_reach_prob_player1)
        average_reach_prob_player2 += (1.0 / (t+1)) * (game_tree.behavioral_to_reach_probability(strat2) - average_reach_prob_player2)

        u1 = game_tree.eval_utility_by_reach_probability({player1: average_reach_prob_player1, player2: average_reach_prob_player2}, player1)
        average_utilities_p1.append(u1)

        best_response_reach1 = game_tree.behavioral_to_reach_probability(treeplex1.best_response({player2: average_reach_prob_player2}, reach_probability=True))
        best_response_reach2 = game_tree.behavioral_to_reach_probability(treeplex2.best_response({player1: average_reach_prob_player1}, reach_probability=True))

        chk1 = (game_tree.eval_utility_by_reach_probability({player1: best_response_reach1, player2: average_reach_prob_player2}, player1) - game_tree.eval_utility_by_reach_probability({player1: average_reach_prob_player1, player2: average_reach_prob_player2}, player1))
        chk2 = (game_tree.eval_utility_by_reach_probability({player1: average_reach_prob_player1, player2: best_response_reach2}, player2) - game_tree.eval_utility_by_reach_probability({player1: average_reach_prob_player1, player2: average_reach_prob_player2}, player2))
        if(chk1 < 0 or chk2 < 0):
            print("error")
            print("iter ", t)
            print(chk1)
            print(chk2)
        assert chk1 >= 0
        assert chk2 >= 0

        ne_gap = (game_tree.eval_utility_by_reach_probability({player1: best_response_reach1, player2: average_reach_prob_player2}, player1) -
                    game_tree.eval_utility_by_reach_probability({player1: average_reach_prob_player1, player2: best_response_reach2}, player1))                    
        ne_gaps.append(ne_gap)

    print(f"final utility for player {player1}", average_utilities_p1[-1])
    print(f"final ne gap", ne_gaps[-1])

    plt.plot(average_utilities_p1)
    plt.xlabel('iterations')
    plt.ylabel('average utility player1')
    plt.title("game: " + game_name)
    plt.show()

    plt.plot(ne_gaps)
    plt.xlabel('iterations')
    plt.ylabel('ne gap')
    plt.title("game: " + game_name)
    plt.show()


def main():
    check1(*games[0], CFR)
    check1(*games[0], MCCFR)

    # check2(*games[1], CFR)
    # check2(*games[1], MCCFR)

if __name__ == "__main__":
    main()
