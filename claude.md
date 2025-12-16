# pyrisk

A Python implementation of a Risk board game variant, designed for AI players.

## Quick Start

```bash
# Run a game with two AI players
python3 pyrisk.py StupidAI*2

# Run without curses display (console mode)
python3 pyrisk.py --nocurses StupidAI BetterAI

# Run multiple games with a fixed seed
python3 pyrisk.py --nocurses -d 0 -s 42 -g 10 StupidAI BetterAI
```

## Dependencies

- Python 3.x (tested with 3.11)
- Standard library only: `curses`, `logging`, `random`, `argparse`, `collections`, `importlib`, `re`, `copy`, `time`

No external packages required.

## Project Structure

```
pyrisk/
├── pyrisk.py           # Main entry point and CLI argument parsing
├── game.py             # Core game logic and turn management
├── world.py            # Map data (territories, connections, ASCII art)
├── territory.py        # Territory, Area, and World classes
├── player.py           # Player class and properties
├── display.py          # Console and curses display handling
├── trace.py            # Tracing infrastructure for cross-implementation testing
├── test_determinism.sh # Script to verify deterministic behavior
├── compare_traces.sh   # Script to compare Python vs C++ traces
└── ai/
    ├── __init__.py     # AI base class with utility methods
    ├── stupid.py       # StupidAI - random play
    ├── better.py       # BetterAI - priority-based continent strategy
    ├── al.py           # AlAI - fixed priority order strategy
    └── chron.py        # ChronAI - advanced strategic AI (has Python 3 bugs)
```

## Available AIs

| AI | Description |
|----|-------------|
| `StupidAI` | Random play - chooses territories and attacks randomly |
| `BetterAI` | Priority-based strategy, focuses on holding continents |
| `AlAI` | Fixed continent priority (Australia > S.America > N.America > Africa > Europe > Asia) |
| `ChronAI` | Advanced strategic AI with pathfinding (note: has Python 3 compatibility issues) |

## Command Line Options

```
python3 pyrisk.py [options] player [player ...]

Options:
  --nocurses      Disable ncurses map display (use console mode)
  --nocolor       Display map without colors
  -l, --log       Write game events to pyrisk.log
  -d, --delay     Delay in seconds after each action (default: 0.1)
  -s, --seed      Random seed for reproducible games
  -g, --games     Number of games to play (default: 1)
  -w, --wait      Pause for keypress after each action
  --deal          Deal territories randomly instead of player choice
  --trace FILE    Write trace log for cross-implementation testing

Players:
  Use AI class names (e.g., StupidAI, BetterAI)
  Use *N suffix for multiple instances (e.g., StupidAI*3)
```

## Writing a New AI

Create a new file `ai/myai.py` with a class `MyAI` extending `AI`:

```python
from ai import AI
import collections

class MyAI(AI):
    def initial_placement(self, empty, remaining):
        """Return a territory to claim/reinforce during setup."""
        if empty:
            return empty[0]  # Claim first empty territory
        return list(self.player.territories)[0]  # Reinforce owned territory

    def reinforce(self, available):
        """Return dict of territory -> reinforcement count."""
        result = collections.defaultdict(int)
        border = [t for t in self.player.territories if t.border]
        for t in border:
            result[t] = available // len(border)
        return result

    def attack(self):
        """Yield (src, dest, attack_strategy, move_strategy) tuples."""
        for t in self.player.territories:
            for adj in t.connect:
                if adj.owner != self.player and t.forces > adj.forces:
                    yield (t, adj, None, None)

    def freemove(self):
        """Return (src, dest, count) or None for end-of-turn movement."""
        return None
```

Then run with: `python3 pyrisk.py MyAI StupidAI`

## Key Classes for AI Development

- `self.player`: Current player object
  - `.territories`: Generator of owned territories
  - `.reinforcements`: Number of reinforcements available
  - `.areas`: Generator of fully owned continents
- `self.world`: World map object
  - `.territories`: Dict of all territories by name
  - `.areas`: Dict of all continents by name
- `self.game`: Game state object
- `AI.simulate(n_atk, n_def)`: Monte Carlo battle simulation returning (win_prob, avg_atk_survive, avg_def_survive)

## Testing

Run a quick test game:
```bash
python3 pyrisk.py --nocurses -d 0 -s 42 StupidAI*2
```

Run multiple games to compare AIs:
```bash
python3 pyrisk.py --nocurses -d 0 -g 100 StupidAI BetterAI
```

## Cross-Implementation Testing

The codebase includes tracing infrastructure for verifying that alternative implementations (e.g., C++) produce identical behavior.

### Generating Traces

```bash
# Generate a trace file with a fixed seed
python3 pyrisk.py --nocurses -d 0 -s 42 --trace trace.jsonl StupidAI*2
```

The trace file is JSON-lines format containing:
- All random number generator calls (with call_id for sequencing)
- All game events (claims, reinforcements, combat, victory)

### Verifying Determinism

```bash
# Test that the same seed produces identical traces
./test_determinism.sh 20  # Test with 20 different seeds
```

### Comparing Implementations

```bash
# Generate Python trace
python3 pyrisk.py --nocurses -d 0 -s 42 --trace py.jsonl StupidAI*2

# Generate C++ trace (when implemented)
./pyrisk_cpp -s 42 --trace cpp.jsonl StupidAI StupidAI

# Compare
./compare_traces.sh 42 py.jsonl cpp.jsonl
```

### Trace File Format

```jsonl
{"call_id": 0, "call_type": "shuffle", "event": "random", "result": ["BRAVO", "ALPHA"]}
{"event": "start", "turn_order": ["BRAVO", "ALPHA"]}
{"call_id": 1, "call_type": "choice", "event": "random", "result": 1, "seq_len": 42}
{"event": "claim", "player": "BRAVO", "territory": "Northwest Territories"}
...
{"event": "victory", "player": "BRAVO"}
```

## Known Issues

- `ChronAI` has Python 3 compatibility issues (uses `dict.keys()` with `random.choice()`)
- The README notes compatibility with Python 2.7, but some AIs may not work with Python 2
