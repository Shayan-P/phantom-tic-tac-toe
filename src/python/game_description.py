import numpy as np
import typing as t
from dataclasses import dataclass
from abc import abstractmethod


Action = str
Player = str

History = str

@dataclass
class NodeDescriptor:
    history: History

@dataclass
class DecisionNodeDescriptor(NodeDescriptor):
    history: History
    player: Player
    actions: t.Set[Action]


@dataclass
class ChanceNodeDescriptor(NodeDescriptor):
    history: History
    actions: t.Dict[Action, float]


@dataclass
class TerminalNodeDescriptor(NodeDescriptor):
    history: History
    payoffs: t.Dict[Player, float]


@dataclass
class InfoSet:
    name: str
    nodes: t.Set[History]


@dataclass
class GameDescriptor:
    nodes: t.Dict[History, NodeDescriptor]
    info_sets: t.List[InfoSet]
    players: t.Set[Player]

    @staticmethod
    def parse(string):
        game = GameDescriptor(nodes=dict(), info_sets=[], players=set())
        for line in string.splitlines():
            words = line.split()
            if words[0] == "node":
                if words[2] == "player":
                    assert words[4] == "actions"
                    player = words[3]
                    node = DecisionNodeDescriptor(history=(words[1]), player=player, actions=set(words[5:]))
                    game.players.add(player)
                if words[2] == "chance":
                    assert words[3] == "actions"
                    actions = dict()
                    for ap in words[4:]:
                        a, p = ap.split('=')
                        actions[a] = float(p)
                    node = ChanceNodeDescriptor(history=(words[1]), actions=actions)
                if words[2] == "terminal":
                    assert words[3] == "payoffs"
                    payoffs = dict()
                    for pp in words[4:]:
                        p, payoff = pp.split('=')
                        payoffs[p] = float(payoff)
                    node = TerminalNodeDescriptor(history=(words[1]), payoffs=payoffs)
                game.nodes[node.history] = node

        for line in string.splitlines():
            words = line.split()
            if words[0] == "infoset":
                assert words[2] == "nodes"
                histories = set((h) for h in words[3:])
                info_set = InfoSet(name=words[1], nodes=histories)
                game.info_sets.append(info_set)
        return game
