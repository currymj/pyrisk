import logging
from typing import Dict, Iterable, List, Tuple

from ai import AI
from territory import Territory

LOG = logging.getLogger("pyrisk.ai.DeterministicAI")


class DeterministicAI(AI):
    """
    A deterministic AI intended for cross-engine testing.

    The behaviour is fully predictable and does not rely on randomness so that
    the Python and C++ engines can be compared event-for-event.
    """

    def _sorted(self, territories: Iterable[Territory]) -> List[Territory]:
        return sorted(territories, key=lambda t: t.name)

    def _border_territories(self) -> List[Territory]:
        return [t for t in self.player.territories if t.border]

    def _reinforce_targets(self) -> List[Territory]:
        borders = self._border_territories()
        candidates = borders if borders else list(self.player.territories)
        if not candidates:
            return []

        def priority(t: Territory) -> Tuple[int, int, str]:
            enemy_force = sum(
                n.forces for n in t.connect if n.owner is not None and n.owner != self.player
            )
            return (-enemy_force, -t.forces, t.name)

        return sorted(candidates, key=priority)

    def start(self):
        LOG.info("DeterministicAI starting with territories: %s", len(self.world.territories))

    def end(self):
        LOG.info("DeterministicAI finished")

    def event(self, msg):
        # Keep minimal logging for traceability without altering decisions
        LOG.debug("Event: %s", msg[0])

    def initial_placement(self, empty, remaining):
        choices = empty if empty is not None else list(self.player.territories)
        if not choices:
            return None
        return self._sorted(choices)[0]

    def reinforce(self, available):
        allocations: Dict[Territory, int] = {}
        targets = self._reinforce_targets()
        if not targets:
            return allocations
        for i in range(available):
            target = targets[i % len(targets)]
            allocations[target] = allocations.get(target, 0) + 1
        return allocations

    def _attack_strategy(self, src: Territory, dst: Territory):
        # Attack while the attacker has strictly more forces than the defender.
        return lambda atk, defense: atk > defense

    def _move_strategy(self, src: Territory, dst: Territory):
        # Move the minimum required forces, capped by the rules.
        return lambda remaining: min(remaining - 1, 3)

    def attack(self):
        plans = []
        targeted = set()
        for territory in self._sorted(self.player.territories):
            for adjacent in self._sorted(territory.connect):
                if adjacent.owner != self.player and territory.forces > adjacent.forces + 1:
                    if adjacent.name in targeted:
                        continue
                    plans.append(
                        (
                            territory,
                            adjacent,
                            self._attack_strategy(territory, adjacent),
                            self._move_strategy(territory, adjacent),
                        )
                    )
                    targeted.add(adjacent.name)
        return plans

    def freemove(self):
        # Keep movement deterministic by skipping it for the test AI.
        return None
