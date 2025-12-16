"""
Tracing infrastructure for deterministic replay and cross-implementation testing.

Provides:
- TraceLogger: JSON-lines event logger
- TracedRandom: Random wrapper that logs all random decisions
"""

import json
import random as stdlib_random


class TraceLogger:
    """Logs game events to a JSON-lines file for comparison testing."""

    def __init__(self, filename):
        self.f = open(filename, 'w')

    def log(self, event_type, **data):
        """Log an event with arbitrary key-value data."""
        record = {"event": event_type}
        record.update(data)
        self.f.write(json.dumps(record, sort_keys=True) + "\n")
        self.f.flush()

    def close(self):
        self.f.close()


class TracedRandom:
    """
    Random number generator wrapper that logs all random decisions.

    This allows us to verify that a C++ port makes identical random
    decisions in the same sequence.
    """

    def __init__(self, seed=None, trace=None):
        """
        Args:
            seed: Random seed for reproducibility
            trace: TraceLogger instance (or None to disable logging)
        """
        self.rng = stdlib_random.Random(seed)
        self.trace = trace
        self.call_id = 0

    def _log(self, call_type, result, **extra):
        if self.trace:
            self.trace.log("random", call_type=call_type, call_id=self.call_id,
                          result=result, **extra)
        self.call_id += 1

    def choice(self, seq):
        """Choose a random element from a non-empty sequence."""
        # Convert to list to ensure indexability and get stable ordering
        seq_list = list(seq)
        idx = self.rng.randrange(len(seq_list))
        result = seq_list[idx]
        self._log("choice", idx, seq_len=len(seq_list))
        return result

    def randint(self, a, b):
        """Return random integer in range [a, b], including both end points."""
        result = self.rng.randint(a, b)
        self._log("randint", result, low=a, high=b)
        return result

    def shuffle(self, seq):
        """Shuffle list in place."""
        self.rng.shuffle(seq)
        self._log("shuffle", [str(x) for x in seq])

    def randrange(self, start, stop=None, step=1):
        """Choose a random item from range(start, stop[, step])."""
        result = self.rng.randrange(start, stop, step)
        self._log("randrange", result, start=start, stop=stop, step=step)
        return result


class NullTrace:
    """No-op trace logger for when tracing is disabled."""
    def log(self, *args, **kwargs):
        pass
    def close(self):
        pass


def create_trace(filename):
    """Create a TraceLogger, or NullTrace if filename is None."""
    if filename:
        return TraceLogger(filename)
    return NullTrace()
