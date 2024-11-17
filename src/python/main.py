#%%

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


def debug(game_name, game_desc, CFR_CLASS):
    game_tree = GameTree(game_desc)
    player1 = '1'
    player2 = '2'
    treeplex1 = Treeplex(game_tree=game_tree, player=player1)
    treeplex2 = Treeplex(game_tree=game_tree, player=player2)

    uniform_strategies = {p: game_tree.get_player_strategy_uniform_behavioral(p) for p in game_desc.players}

    cfr = CFR_CLASS(treeplex1)
    T = 2
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


def main():
    # debug(*games[0], MCCFR)
    # debug(*games[0], CFR)

    check2(*games[1], MCCFR)
    check2(*games[1], CFR)
    # check1(*games[1], CFR)

    # check2(*games[1], MCCFR)
    # check2(*games[1], CFR)

    # check2(*games[1], CFR)
    # check2(*games[1], MCCFR)

if __name__ == "__main__":
    main()

#%%

# from game import VALUE_DEBUG_CFR, VALUE_DEBUG_MCCFR

# game_name, game_desc = games[1]
# game_tree = GameTree(game_desc)
# player1 = '1'
# player2 = '2'
# treeplex1 = Treeplex(game_tree=game_tree, player=player1)

# uniform_strategies = {p: game_tree.get_player_strategy_uniform_behavioral(p) for p in game_desc.players}

# cfr = CFR(treeplex1)
# mccfr = MCCFR(treeplex1)

# strat_cfr = cfr.next_strategy_behavioral()
# strat_mccfr = mccfr.next_strategy_behavioral()
# print('distribution difference', np.abs(strat_cfr - strat_mccfr).max())

# cfr.improve({player2: uniform_strategies[player2]})
# mccfr.improve({player2: uniform_strategies[player2]})

# from game import GameNode, DecisionTreeplexNode


# from game import TerminalNodeDescriptor, DecisionNodeDescriptor, ChanceNodeDescriptor

# def inspect_hist(game_tree, node, bad_hist, pflow=1, verbos=False):
#     node: GameNode = node
#     verbos = verbos or node.node_desc.history == bad_hist
#     if isinstance(node.node_desc, TerminalNodeDescriptor):
#         if verbos:
#             print(pflow, node.node_desc.payoffs[player1], pflow * node.node_desc.payoffs[player1])
#         return
#     if isinstance(node.node_desc, ChanceNodeDescriptor):
#         for action in node.node_desc.actions:
#             inspect_hist(game_tree, node.next[action], bad_hist, pflow * node.node_desc.actions[action], verbos)
#     if isinstance(node.node_desc, DecisionNodeDescriptor):
#         for action in node.node_desc.actions:
#             p = 1
#             if node.node_desc.player != player1:
#                 p = 1/len(node.children) # uniform
#             inspect_hist(game_tree, node.next[action], bad_hist, pflow * p, verbos)

# bad_hist = "/C:KK/P1:r/P2:c/C:Q/P1:r/P2:r/"
# inspect_hist(game_tree, game_tree.root, bad_hist)

# node = [node for node in game_tree.nodes if node.node_desc.history == bad_hist][0]
# tnode = [tnode for tnode in treeplex1.treeplex_nodes if len(tnode.game_nodes) == 1 and tnode.game_nodes[0].node_desc.history == bad_hist][0]
# print(node.node_desc.history)
# print(cfr.rms[tnode.idx].regret_sum)
# idx = game_tree.history_to_info_set_idx[node.node_desc.history]
# print(mccfr.rms[idx].regret_sum)

# strat_cfr = cfr.next_strategy_behavioral()
# strat_mccfr = mccfr.next_strategy_behavioral()
# print('distribution difference', np.abs(strat_cfr - strat_mccfr).max())



# def inspect(tnode, h=0):
#     print(tnode.game_nodes[0].node_desc.history)
#     print("hey ", h, "tnode size=", len(tnode.game_nodes), isinstance(tnode, DecisionTreeplexNode))
#     for child in tnode.children:
#         inspect(child, h+1)


# for tnode in reversed(treeplex1.treeplex_nodes):
#     if isinstance(tnode, DecisionTreeplexNode):
#         reg1 = cfr.rms[tnode.idx]
#         anode: GameNode = tnode.game_nodes[0]
#         idx = game_tree.history_to_info_set_idx[anode.node_desc.history]
#         reg2 = mccfr.rms[idx]
#         diff = np.abs(reg1.regret_sum - reg2.regret_sum).max()
#         if diff > 0.01:
#             print("|", len(tnode.children), len(tnode.game_nodes))
#             inspect(tnode)
#             break

# 
#%%

# cnt = 0
# for tnode in reversed(treeplex1.treeplex_nodes):
#     a = 0
#     b = 0
#     for node in tnode.game_nodes:
#         a = VALUE_DEBUG_CFR[node.node_desc.history]
#         b += VALUE_DEBUG_MCCFR[node.node_desc.history]
#     if abs(a-b) > 0.001:
#         print("failed")
#         break
#     else:
#         cnt += 1
#         # print('pass', cnt, len(tnode.children))

#%%


#%%

# len(tnode.game_nodes)

# #%%

# len(tnode.children)

# #%%

# a, b

# #%%

# node = tnode.game_nodes[0]

# node.node_desc.history

# node.node_desc.payoffs[player1], a, b