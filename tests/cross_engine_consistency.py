import json
import random
import subprocess
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent.parent
sys.path.append(str(ROOT))

from ai.deterministic import DeterministicAI
from game import Game
from world import AREAS, CONNECT, KEY, MAP
BUILD_DIR = ROOT / "build"
CPP_BINARY = BUILD_DIR / "pyrisk_engine_tester"


def build_cpp_tester():
    BUILD_DIR.mkdir(exist_ok=True)
    sources = [
        ROOT / "cpp" / "engine" / "ai.cpp",
        ROOT / "cpp" / "engine" / "game.cpp",
        ROOT / "cpp" / "engine" / "testing_main.cpp",
    ]
    cmd = ["g++", "-std=c++17", "-O2", "-o", str(CPP_BINARY)] + [str(s) for s in sources]
    subprocess.check_call(cmd)


def run_python_engine(seed: int):
    random.seed(seed)
    events = []
    game = Game(
        curses=False,
        color=False,
        delay=0,
        connect=CONNECT,
        areas=AREAS,
        cmap=MAP,
        ckey=KEY,
        wait=False,
        event_logger=events.append,
        deal=False,
    )
    game.add_player("ALPHA", DeterministicAI)
    game.add_player("BRAVO", DeterministicAI)
    game.play()
    return events


def run_cpp_engine(seed: int):
    if not CPP_BINARY.exists():
        build_cpp_tester()
    result = subprocess.run(
        [str(CPP_BINARY), str(seed)], check=True, capture_output=True, text=True
    )
    return [json.loads(line) for line in result.stdout.splitlines() if line.strip()]


def compare_logs(python_log, cpp_log):
    if len(python_log) != len(cpp_log):
        raise AssertionError(f"Event count mismatch: python={len(python_log)} cpp={len(cpp_log)}")

    for idx, (py_event, cpp_event) in enumerate(zip(python_log, cpp_log)):
        if py_event != cpp_event:
            raise AssertionError(
                f"Mismatch at event {idx}: python={py_event} cpp={cpp_event}"
            )


def main():
    seed = 42
    python_log = run_python_engine(seed)
    cpp_log = run_cpp_engine(seed)
    compare_logs(python_log, cpp_log)
    print("Engine logs match for seed", seed)


if __name__ == "__main__":
    main()
