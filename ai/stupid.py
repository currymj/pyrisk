from ai import AI
import collections

class StupidAI(AI):
    """
    StupidAI: Plays a completely random game, randomly choosing and reinforcing
    territories, and attacking wherever it can without any considerations of wisdom.
    """
    def initial_placement(self, empty, remaining):
        if empty:
            # Sort for deterministic ordering
            return self.random.choice(sorted(empty, key=lambda t: t.name))
        else:
            # Sort for deterministic ordering
            t = self.random.choice(sorted(self.player.territories, key=lambda t: t.name))
            return t

    def attack(self):
        # Sort territories and connections for deterministic ordering
        for t in sorted(self.player.territories, key=lambda x: x.name):
            for a in sorted(t.connect, key=lambda x: x.name):
                if a.owner != self.player:
                    if t.forces > a.forces:
                        yield (t, a, None, None)

    def reinforce(self, available):
        # Sort for deterministic ordering
        border = sorted([t for t in self.player.territories if t.border], key=lambda t: t.name)
        result = collections.defaultdict(int)
        for i in range(available):
            t = self.random.choice(border)
            result[t] += 1
        return result
