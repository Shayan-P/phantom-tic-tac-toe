import numpy as np
import typing as t

from abc import abstractmethod


Strategy = np.ndarray
UtilityVec = np.ndarray


class ActionSpace:
    @property
    @abstractmethod
    def dim(self) -> int:
        pass

    @abstractmethod
    def sample(self, dist: Strategy):
        pass

class IndexActionSpace(ActionSpace):
    def __init__(self, d: int) -> None:
        super().__init__()
        self.d = d

    @property
    def dim(self):
        return self.d
    
    def sample(self, dist: Strategy):
        assert self.d == dist.size
        return np.random.choice(self.d, p=dist)


class ListActionSpace(ActionSpace):
    def __init__(self, actions: t.List[str]) -> None:
        super().__init__()
        self.actions = list(actions)

    @property
    def dim(self):
        return len(self.actions)
    
    def sample(self, dist: Strategy):
        assert len(self.actions) == dist.size
        idx = np.random.choice(self.d, p=dist)
        return self.actions[idx]


def relu(x):
    return x * (x > 0)


def softmax(x: np.ndarray):
    x = x - np.max(x, axis=-1, keepdims=True)
    exps = np.exp(x)
    return exps / np.sum(exps, axis=-1, keepdims=True)



class RegretMinimizer:
    def __init__(self, action_space: ActionSpace):
        self.action_space = action_space

    @abstractmethod
    def observe_utility(self, g: UtilityVec):
        pass

    @abstractmethod
    def next_strategy(self) -> Strategy:
        pass


class RegretMatching(RegretMinimizer):
    def __init__(self, action_space: ActionSpace):
        super().__init__(action_space)
        self.regret_sum = np.zeros(action_space.dim)
        self.prev_strat: Strategy = None
        self.acc_reg = np.zeros(self.action_space.dim)

    def accumulate_regret(self, g: UtilityVec):
        self.acc_reg += g

    def flush_observe_utility(self):
        assert self.prev_strat is not None
        self.regret_sum += self.acc_reg - (self.prev_strat @ self.acc_reg)
        self.acc_reg[:] = 0
        
    def observe_utility(self, g: UtilityVec):
        assert self.prev_strat is not None
        self.regret_sum += g - (self.prev_strat @ g)
    
    def next_strategy(self) -> Strategy:
        probs = relu(self.regret_sum)
        if probs.sum() == 0:
            probs = np.ones(self.action_space.dim)
            # probs = np.random.rand(self.action_space.dim)
        self.prev_strat = probs / probs.sum()
        return self.prev_strat


class RegretMatchingPlus(RegretMinimizer):
    def __init__(self, action_space: ActionSpace):
        super().__init__(action_space)
        self.regret_sum = np.zeros(action_space.dim)
        self.prev_strat: Strategy = None

    def observe_utility(self, g: UtilityVec):
        assert self.prev_strat is not None
        self.regret_sum += relu(g - (self.prev_strat @ g)) # this is the only difference with RegretMatching
    
    def next_strategy(self) -> Strategy:
        probs = relu(self.regret_sum)
        if probs.sum() == 0:
            # probs = np.random.rand(self.action_space.dim)
            probs = np.ones(self.action_space.dim)
        self.prev_strat = probs / probs.sum()
        return self.prev_strat


class MultiplicativeWeightUpdates(RegretMinimizer):
    def __init__(self, action_space: ActionSpace, nu: float):
        super().__init__(action_space)
        self.nu = nu
        self.regret_sum = np.zeros(action_space.dim)
        self.prev_strat: Strategy = None

    def observe_utility(self, g: UtilityVec):
        assert self.prev_strat is not None
        self.regret_sum += g - (self.prev_strat @ g)
    
    def next_strategy(self) -> Strategy:
        self.prev_strat = softmax(self.regret_sum)
        return self.prev_strat
