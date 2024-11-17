import numpy as np
import typing as t
import matplotlib.pyplot as plt

from game_description import GameDescriptor, NodeDescriptor, ChanceNodeDescriptor, DecisionNodeDescriptor, TerminalNodeDescriptor, Action, Player, History
from regret_minimizer import RegretMatchingPlus, RegretMatching, ListActionSpace, MultiplicativeWeightUpdates
from dataclasses import dataclass
from abc import abstractmethod


class GameNode:
    def __init__(self, node_desc: NodeDescriptor):
        self.node_desc: NodeDescriptor = node_desc
        self.idx: int = None
        self.parent: GameNode = None
        self.next: t.Dict[Action, GameNode] = dict()
        self.last_action: Action = None

    def is_terminal(self):
        return isinstance(self.node_desc, TerminalNodeDescriptor)
    
    def is_chance(self):
        return isinstance(self.node_desc, ChanceNodeDescriptor)
    
    def is_decision(self):
        return isinstance(self.node_desc, DecisionNodeDescriptor)
    
    @property
    def children(self):
        return list(self.next.values())


class GameInfoSet:
    def __init__(self, nodes: t.List[GameNode], player: Player, actions: t.List[Action]):
        self.actions = actions
        self.nodes = nodes
        self.player = player


class GameTree:
    def __init__(self, game_descriptor: GameDescriptor):
        self.root = GameNode(node_desc=game_descriptor.nodes.get("/"))
        self.game_descriptor = game_descriptor
        self.nodes: t.List[GameNode] = []
        self.init_rec(cur_node=self.root)
        self.chance_strat_behavioral = self.get_chance_strategy_behavioral()
        self.history_to_node = dict()
        self.history_to_info_set_idx = dict()
        for node in self.nodes:
            self.history_to_node[node.node_desc.history] = node
        for info_idx, info_set in enumerate(self.game_descriptor.info_sets):
            for hist in info_set.nodes:
                self.history_to_info_set_idx[hist] = info_idx
        self.info_sets: t.List[GameInfoSet] = [
            GameInfoSet(
                nodes=[self.history_to_node[hist] for hist in infoset.nodes],
                player=self.history_to_node[next(iter(infoset.nodes))].node_desc.player,
                actions=list(self.history_to_node[next(iter(infoset.nodes))].node_desc.actions)
            )
            for infoset in game_descriptor.info_sets
        ]

    def init_rec(self, cur_node: GameNode, parent_node: t.Optional[GameNode]=None):
        # initialize node
        cur_node.idx = len(self.nodes)
        cur_node.parent = parent_node

        # add to nodes
        self.nodes.append(cur_node)
        if isinstance(cur_node.node_desc, TerminalNodeDescriptor):
            return
        for action in cur_node.node_desc.actions:
            if isinstance(cur_node.node_desc, DecisionNodeDescriptor):
                child_hist = cur_node.node_desc.history + f"P{cur_node.node_desc.player}:{action}/"
            if isinstance(cur_node.node_desc, ChanceNodeDescriptor):
                child_hist = cur_node.node_desc.history + f"C:{action}/"
            
            assert child_hist in self.game_descriptor.nodes
            child_node = GameNode(node_desc=self.game_descriptor.nodes[child_hist])
            cur_node.next[action] = child_node
            child_node.last_action = action
            self.init_rec(child_node, cur_node)

    def get_empty_prop(self):
        return np.zeros(len(self.nodes), dtype=float)

    def set_prop(self, prop: np.ndarray, node: GameNode, val):
        prop[node.idx] = val

    def get_prop(self, prop: np.ndarray, node: GameNode):
        return prop[node.idx]

    def get_utilities(self, player: Player):
        u = self.get_empty_prop()
        u[:] = 0
        for node in self.nodes:
            if isinstance(node.node_desc, TerminalNodeDescriptor):
                u[node.idx] = node.node_desc.payoffs[player]
        return u

    def get_chance_strategy_behavioral(self):
        strat = self.get_empty_prop()
        strat[:] = 1
        for node in self.nodes:
            if node.parent is not None:
                if isinstance(node.parent.node_desc, ChanceNodeDescriptor):
                    strat[node.idx] = node.parent.node_desc.actions[node.last_action]
        return strat

    def get_player_strategy_uniform_behavioral(self, player: Player):
        strat = self.get_empty_prop()
        strat[:] = 1
        for node in self.nodes:
            if node.parent is not None:
                if isinstance(node.parent.node_desc, DecisionNodeDescriptor):
                    if node.parent.node_desc.player == player:
                        strat[node.idx] = 1/len(node.parent.children)
        return strat
    
    def behavioral_to_reach_probability(self, strat):
        reach = np.ones_like(strat)
        for node in self.nodes:
            if node.parent is not None:
                reach[node.idx] = strat[node.idx] * reach[node.parent.idx]
        return reach
    
    def eval_utility(self, all_strats: t.Dict[Player, np.ndarray], player: Player):
        chance_strat = self.get_chance_strategy_behavioral()
        v = self.get_empty_prop()
        for node in reversed(self.nodes):
            if isinstance(node.node_desc, TerminalNodeDescriptor):
                v[node.idx] = node.node_desc.payoffs[player]
            elif isinstance(node.node_desc, DecisionNodeDescriptor):
                v[node.idx] = sum(all_strats[node.node_desc.player][child.idx] * v[child.idx] for child in node.children)
            elif isinstance(node.node_desc, ChanceNodeDescriptor):
                v[node.idx] = sum(node.node_desc.actions[a] * v[child.idx] for a, child in node.next.items())
            else:
                assert (False)
        return v[0]
    
    def eval_utility_by_reach_probability(self, all_reach_probability: t.Dict[Player, np.ndarray], player: Player):
        assert(set(all_reach_probability.keys()) == set(self.game_descriptor.players))

        chance_strat = self.get_chance_strategy_behavioral()
        p_flow = self.behavioral_to_reach_probability(chance_strat)
        for s in all_reach_probability.values():
            p_flow *= s
        u = self.get_utilities(player)
        return (u * p_flow).sum()


"""
by default assume start is always behavioral when passed to functions except otherwise mentioned
"""


class TreeplexNode:
    def __init__(self, game_nodes: t.List[GameNode]):
        self.children: t.List["TreeplexNode"] = []
        self.game_nodes: t.List[GameNode] = game_nodes
        self.idx = -1 # to be set

class DecisionTreeplexNode(TreeplexNode):
    def __init__(self, game_nodes: t.List[GameNode], actions: t.List[Action]):
        super().__init__(game_nodes)
        self.actions: t.List[Action] = actions
        self.next: t.Dict[Action, TreeplexNode] = dict()

    def add_child(self, action: Action, child: TreeplexNode):
        self.next[action] = child
        self.children.append(child)

class ObservationTreeplexNode(TreeplexNode):
    def __init__(self, game_nodes: t.List[GameNode]):
        super().__init__(game_nodes)

    def add_child(self, child: TreeplexNode):
        self.children.append(child)

# creates a treeplex
class Treeplex:
    def __init__(self, game_tree: GameTree, player: Player):
        self.game_tree = game_tree
        self.player = player

        node_to_info_set_reach = [set() for _ in range(len(self.game_tree.nodes))]
        for node in reversed(self.game_tree.nodes):
            if self.is_decision_node(node):
                info_idx = self.game_tree.history_to_info_set_idx[node.node_desc.history]
                node_to_info_set_reach[node.idx].add(info_idx)
            else:
                for child in node.children:
                    node_to_info_set_reach[node.idx].update(node_to_info_set_reach[child.idx])
        node_to_info_set_reach = [frozenset(st) for st in node_to_info_set_reach]

        self.troot = self.init_rec([self.game_tree.root], node_to_info_set_reach)
        self.treeplex_nodes: t.List[TreeplexNode] = []
        self.init_treeplex(self.troot)

    def init_treeplex(self, node: TreeplexNode):
        node.idx = len(self.treeplex_nodes)
        self.treeplex_nodes.append(node)
        for child in node.children:
            self.init_treeplex(child)

    def init_rec(self, cur_nodes: t.List[GameNode], node_to_info_set_reach: t.List[t.FrozenSet[int]]):
        assert len(cur_nodes) > 0
        if all(self.is_decision_node(node) for node in cur_nodes):
            # we should be at an info set
            one_node = cur_nodes[0]
            assert len(node_to_info_set_reach[one_node.idx]) == 1
            assert all(node_to_info_set_reach[node.idx] == node_to_info_set_reach[one_node.idx] for node in cur_nodes)
            assert all(set(node.node_desc.actions) == set(one_node.node_desc.actions) for node in cur_nodes)
            actions = list(set(one_node.node_desc.actions))

            treeplex_node = DecisionTreeplexNode(game_nodes=list(cur_nodes), actions=actions)

            for a in actions:
                child_cur_nodes = [node.next[a] for node in cur_nodes]
                treeplex_node.add_child(a, self.init_rec(child_cur_nodes, node_to_info_set_reach))
        else:
            treeplex_node = ObservationTreeplexNode(game_nodes=list(cur_nodes))
            reach_set_to_nodes: t.Dict[t.Set, t.List] = dict()
            for node in cur_nodes:
                for child in node.children:
                    reach_set = node_to_info_set_reach[child.idx]
                    if reach_set not in reach_set_to_nodes:
                        reach_set_to_nodes[reach_set] = []
                    reach_set_to_nodes[reach_set].append(child)
            for reach_set, nodes in reach_set_to_nodes.items():
                treeplex_node.add_child(self.init_rec(nodes, node_to_info_set_reach))
        return treeplex_node

    def is_decision_node(self, node: GameNode):
        return isinstance(node.node_desc, DecisionNodeDescriptor) and node.node_desc.player == self.player
    
    def utility_for_strats(self, other_strats: t.Dict[Player, np.ndarray]):
        return self.utility_for_reach_probability({
            p: self.game_tree.behavioral_to_reach_probability(s)
            for p, s in other_strats.items() if p != self.player
        })

    def utility_for_reach_probability(self, other_reach_probability: t.Dict[Player, np.ndarray]):
        other_reach_probability = {p: s for p, s in other_reach_probability.items() if p != self.player}
        assert set(other_reach_probability.keys()) == set(self.game_tree.game_descriptor.players) - {self.player}

        chance_strat = self.game_tree.get_chance_strategy_behavioral()
        p_flow = self.game_tree.behavioral_to_reach_probability(chance_strat)
        for p, s in other_reach_probability.items():
            p_flow *= s
        u = self.game_tree.get_utilities(self.player)
        u *= p_flow
        treeplex_u = np.zeros(len(self.treeplex_nodes), dtype=float)
        for i, tnode in enumerate(self.treeplex_nodes):
            treeplex_u[i] = sum(u[node.idx] for node in tnode.game_nodes)
        return treeplex_u

    def treeplex_strat_to_behavioral_game_strat(self, treeplex_strat: np.ndarray):
        strat = self.game_tree.get_empty_prop()
        strat[:] = 1.0
        for tnode in self.treeplex_nodes:
            if isinstance(tnode, DecisionTreeplexNode):
                for node in tnode.game_nodes:
                    for a in tnode.actions:
                        strat[node.next[a].idx] = treeplex_strat[tnode.next[a].idx]
        return strat
    
    def empty_treeplex_prop(self):
        return np.zeros(len(self.treeplex_nodes), dtype=float)
    
    def best_response(self, other_strats: t.Dict[Player, np.ndarray], reach_probability=False):
        if reach_probability:
            tu = self.utility_for_reach_probability(other_strats)
        else:
            tu = self.utility_for_strats(other_strats)
        response = self.empty_treeplex_prop()

        for tnode in reversed(self.treeplex_nodes):
            if isinstance(tnode, DecisionTreeplexNode):
                children = tnode.children
                utils = [tu[child.idx] for child in children]
                argmax = np.argmax(utils)
                best_child = children[argmax]
                for child in children:
                    response[child.idx] = 1.0 if child.idx == best_child.idx else 0.0
                tu[tnode.idx]+= utils[argmax]
            else:
                tu[tnode.idx]+= sum(tu[child.idx] for child in tnode.children)
        strat = self.treeplex_strat_to_behavioral_game_strat(response)
        if reach_probability:
            eval_util = self.game_tree.eval_utility_by_reach_probability({**other_strats, self.player: self.game_tree.behavioral_to_reach_probability(strat)}, self.player)
        else:
            eval_util = self.game_tree.eval_utility({**other_strats, self.player: strat}, self.player)
        if abs(eval_util - tu[0]) > 1e-6:
            print("error")
            print("eval_util", eval_util)
            print("tu[0]", tu[0])
        assert abs(eval_util - tu[0]) < 1e-6 # eps check
        return strat        

    def show(self):
        for tnode in self.treeplex_nodes:
            print(tnode.idx, ":")
            if isinstance(tnode, DecisionTreeplexNode):
                print("Decision")
                print("Actions",  tnode.actions)
                for a, child in tnode.next.items():
                    print(f"  {a} -> {child.idx}")
                print("Histories: ", [node.node_desc.history for node in tnode.game_nodes])
            else:
                print("Observation")
                for child in tnode.children:
                    print(f"  {child.idx}")
                print("Histories: ", [node.node_desc.history for node in tnode.game_nodes])
            print()


"""
output strategies are all behavioral
they are p[h][a] -> for each history what is the probability of taking this action
this is also the input to the treeplex
you can think of it as transition probability more than the strategy
"""

class CFR:
    def __init__(self, treeplex: Treeplex):
        self.treeplex = treeplex
        self.rms: t.Dict[int, RegretMatchingPlus] = dict()
        for tnode in self.treeplex.treeplex_nodes:
            if isinstance(tnode, DecisionTreeplexNode):
                self.rms[tnode.idx] = RegretMatchingPlus(ListActionSpace(list(tnode.actions)))
        self.last_strat = None

    def next_strategy_behavioral(self):
        strat = self.treeplex.empty_treeplex_prop()
        for tnode in self.treeplex.treeplex_nodes:
            if isinstance(tnode, DecisionTreeplexNode):
                dist = self.rms[tnode.idx].next_strategy()
                actions = self.rms[tnode.idx].action_space.actions
                for a, p in zip(actions, dist):
                    strat[tnode.next[a].idx] = p
        self.last_strat = strat.copy()
        strat = self.treeplex.treeplex_strat_to_behavioral_game_strat(strat)
        return strat

    def improve(self, other_strategies: t.Dict[Player, np.ndarray]):
        u = self.treeplex.utility_for_strats(other_strategies)
        v = np.zeros_like(u)
        for tnode in reversed(self.treeplex.treeplex_nodes):
            if isinstance(tnode, DecisionTreeplexNode):
                v[tnode.idx] = u[tnode.idx] + sum(v[child.idx] * self.last_strat[child.idx] for child in tnode.children)
                rms = self.rms[tnode.idx]
                rms.observe_utility([
                    v[tnode.next[a].idx]
                    for a in rms.action_space.actions
                ])
            else:
                v[tnode.idx] = u[tnode.idx] + sum(v[child.idx] for child in tnode.children)


# This does not work. I was trying not to use treeplex and implement CFR and make sure the two versions match...
class MCCFR:
    def __init__(self, treeplex: Treeplex):
        self.game_tree: GameTree = treeplex.game_tree
        self.player = treeplex.player
        self.rms = dict() # map info set idx to rms
        for idx, infoset in enumerate(self.game_tree.info_sets):
            if infoset.player == self.player:
                self.rms[idx] = RegretMatching(ListActionSpace(infoset.actions))
        self.last_strat = None # behavioral probablities

    def next_strategy_behavioral(self):
        strat = self.game_tree.get_empty_prop()
        strat[:] = 1
        for idx, rms in self.rms.items():
            rms: RegretMatching = rms
            dist = rms.next_strategy()
            for node in self.game_tree.info_sets[idx].nodes:
                node: GameNode = node
                for aid, action in enumerate(rms.action_space.actions):
                    strat[node.next[action].idx] = dist[aid]
        self.last_strat = strat
        return strat

    def improve(self, other_strategies: t.Dict[Player, np.ndarray]):
        self.improve_dfs(self.game_tree.root, other_strategies)
        for rms in self.rms.values():
            rms: RegretMatching = rms
            rms.flush_observe_utility()

    def improve_dfs(self, node: GameNode, other_strategies, pflow_others=1):
        if isinstance(node.node_desc, TerminalNodeDescriptor):
            return node.node_desc.payoffs[self.player]
        if isinstance(node.node_desc, ChanceNodeDescriptor):
            res = 0
            for action, prob in node.node_desc.actions.items():
                res += prob * self.improve_dfs(node.next[action], other_strategies, pflow_others*prob)
            return res
        if isinstance(node.node_desc, DecisionNodeDescriptor):
            if node.node_desc.player == self.player:
                feedback = dict()
                vals = []
                for action in node.node_desc.actions:
                    v = self.improve_dfs(node.next[action], other_strategies, pflow_others)
                    vals.append(v * self.last_strat[node.next[action].idx])
                    feedback[action] = pflow_others * v
                rms: RegretMatching = self.rms[self.game_tree.history_to_info_set_idx[node.node_desc.history]]
                rms.accumulate_regret([feedback[action] for action in rms.action_space.actions])
                return sum(vals)
            else:
                res = 0
                for action in node.node_desc.actions:
                    prob = other_strategies[node.node_desc.player][node.next[action].idx]
                    res += prob * self.improve_dfs(node.next[action], other_strategies, pflow_others*prob)
                return res

        

# class MCCFR:
#     def __init__(self, treeplex: Treeplex):
#         self.game_tree: GameTree = treeplex.game_tree
#         self.player = treeplex.player
#         self.rms = dict() # map info set idx to rms
#         for idx, infoset in enumerate(self.game_tree.info_sets):
#             if infoset.player == self.player:
#                 self.rms[idx] = RegretMatching(ListActionSpace(infoset.actions))
#         self.last_strat = None # behavioral probablities

#     def next_strategy_behavioral(self):
#         strat = self.game_tree.get_empty_prop()
#         strat[:] = 1
#         for idx, rms in self.rms.items():
#             rms: RegretMatching = rms
#             dist = rms.next_strategy()
#             # print("dist=", dist)
#             # print(rms.regret_sum)
#             for node in self.game_tree.info_sets[idx].nodes:
#                 node: GameNode = node
#                 for aid, action in enumerate(rms.action_space.actions):
#                     strat[node.next[action].idx] = dist[aid]
#         self.last_strat = strat
#         return strat

#     def improve(self, other_strategies: t.Dict[Player, np.ndarray]):
#         me = self.improve_sample(self.game_tree.root, other_strategies)
#         # for i in range(1000):
#         #     self.improve_sample(self.game_tree.root, other_strategies)
#         for rms in self.rms.values():
#             rms.flush_observe_utility()

#     def improve_sample(self, node: GameNode, other_strategies: t.Dict[Player, np.ndarray], reach_probability_others=1, reach_probability_me=1):
#         if isinstance(node.node_desc, TerminalNodeDescriptor):
#             res = node.node_desc.payoffs[self.player] * reach_probability_others # / (reach_probability_me * reach_probability_others)
#             VALUE_DEBUG_MCCFR[node.node_desc.history] = res
#             return res
#         if isinstance(node.node_desc, ChanceNodeDescriptor):
#             actions = list(node.node_desc.actions.keys())
#             sampled_prob = list(node.node_desc.actions.values())
#             sampled_prob = np.array(sampled_prob)
#             sampled_prob /= sampled_prob.sum()
#             res = 0
#             for aidx in range(len(actions)):
#                 action = actions[aidx]
#                 p = sampled_prob[aidx]
#                 res += self.improve_sample(node.next[action], other_strategies, reach_probability_others * p, reach_probability_me)
#             VALUE_DEBUG_MCCFR[node.node_desc.history] = res
#             return res

#         if isinstance(node.node_desc, DecisionNodeDescriptor):
#             if node.node_desc.player == self.player:
#                 info_idx = self.game_tree.history_to_info_set_idx[node.node_desc.history]
#                 assert(self.game_tree.info_sets[info_idx].player == self.player)
#                 rms: RegretMatching = self.rms[info_idx]
#                 actions = rms.action_space.actions

#                 sampled_prob = [self.last_strat[node.next[a].idx] for a in actions]
#                 sampled_prob = np.array(sampled_prob)
#                 sampled_prob /= sampled_prob.sum()

#                 res = 0
#                 vals = []
#                 for sample_action_idx in range(len(actions)):
#                     sampled_action = actions[sample_action_idx]
#                     p = sampled_prob[sample_action_idx]
#                     val = p * self.improve_sample(node.next[sampled_action], other_strategies, reach_probability_others, reach_probability_me * p)
#                     vals.append(val)
#                 res += sum(vals)
#                 rms.accumulate_regret([
#                     val
#                     for val in vals
#                 ])
#                 VALUE_DEBUG_MCCFR[node.node_desc.history] = res
#                 return res
#             else:
#                 actions = list(node.node_desc.actions)
#                 sampled_prob = [other_strategies[node.node_desc.player][node.next[a].idx] for a in actions]
#                 sampled_prob = np.array(sampled_prob)
#                 sampled_prob /= sampled_prob.sum()
#                 res = 0
#                 for aidx in range(len(actions)):
#                     action = actions[aidx]
#                     p = sampled_prob[aidx]
#                     res += self.improve_sample(node.next[action], other_strategies, reach_probability_others * p, reach_probability_me)
#                 VALUE_DEBUG_MCCFR[node.node_desc.history] = res
#                 return res
            
#         # if isinstance(node.node_desc, TerminalNodeDescriptor):
#         #     return node.node_desc.payoffs[self.player] / (reach_probability_me * reach_probability_others)
#         # if isinstance(node.node_desc, ChanceNodeDescriptor):
#         #     actions = list(node.node_desc.actions.keys())
#         #     sampled_prob = list(node.node_desc.actions.values())
#         #     sampled_prob = np.array(sampled_prob)
#         #     sampled_prob /= sampled_prob.sum()
#         #     aidx = np.random.choice(range(len(actions)), p=sampled_prob)
#         #     action = actions[aidx]
#         #     p = sampled_prob[aidx]
#         #     return p * self.improve_sample(node.next[action], other_strategies, reach_probability_others * p, reach_probability_me)

#         # if isinstance(node.node_desc, DecisionNodeDescriptor):
#         #     if node.node_desc.player == self.player:
#         #         info_idx = self.game_tree.history_to_info_set_idx[node.node_desc.history]
#         #         assert(self.game_tree.info_sets[info_idx].player == self.player)
#         #         rms: RegretMatching = self.rms[info_idx]
#         #         actions = rms.action_space.actions

#         #         sampled_prob = [self.last_strat[node.next[a].idx] for a in actions]
#         #         sampled_prob = np.array(sampled_prob)
#         #         sampled_prob /= sampled_prob.sum()

#         #         sample_action_idx = np.random.choice(range(len(actions)), p=sampled_prob)
#         #         sampled_action = actions[sample_action_idx]
#         #         p = sampled_prob[sample_action_idx]

#         #         val = self.improve_sample(node.next[sampled_action], other_strategies, reach_probability_others, reach_probability_me * p)
#         #         rms.observe_utility([
#         #             val * reach_probability_others if a == sampled_action else 0
#         #             for a in rms.action_space.actions 
#         #         ])
#         #         return p * val
#         #     else:
#         #         actions = list(node.node_desc.actions)
#         #         sampled_prob = [other_strategies[node.node_desc.player][node.next[a].idx] for a in actions]
#         #         sampled_prob = np.array(sampled_prob)
#         #         sampled_prob /= sampled_prob.sum()
#         #         aidx = np.random.choice(range(len(actions)), p=sampled_prob)
#         #         action = actions[aidx]
#         #         p = sampled_prob[aidx]
#         #         return p * self.improve_sample(node.next[action], other_strategies, reach_probability_others * p, reach_probability_me)
