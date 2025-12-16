#!/usr/bin/env python3

import logging
import importlib
import re
import collections
import curses
from game import Game
from trace import create_trace

from world import CONNECT, MAP, KEY, AREAS

LOG = logging.getLogger("pyrisk")
import argparse

parser = argparse.ArgumentParser()

parser.add_argument("--nocurses", dest="curses", action="store_false", default=True, help="Disable the ncurses map display")
parser.add_argument("--nocolor", dest="color", action="store_false", default=True, help="Display the map without colors")
parser.add_argument("-l", "--log", action="store_true", default=False, help="Write game events to a logfile")
parser.add_argument("-d", "--delay", type=float, default=0.1, help="Delay in seconds after each action is displayed")
parser.add_argument("-s", "--seed", type=int, default=None, help="Random number generator seed")
parser.add_argument("-g", "--games", type=int, default=1, help="Number of rounds to play")
parser.add_argument("-w", "--wait", action="store_true", default=False, help="Pause and wait for a keypress after each action")
parser.add_argument("players", nargs="+", help="Names of the AI classes to use. May use 'ExampleAI*3' syntax.")
parser.add_argument("--deal", action="store_true", default=False, help="Deal territories rather than letting players choose")
parser.add_argument("--trace", type=str, default=None, help="Write trace log to file for cross-implementation testing")

args = parser.parse_args()

NAMES = ["ALPHA", "BRAVO", "CHARLIE", "DELTA", "ECHO", "FOXTROT"]

LOG.setLevel(logging.DEBUG)
if args.log:
    logging.basicConfig(filename="pyrisk.log", filemode="w")
elif not args.curses:
    logging.basicConfig()

# Create trace logger if requested
trace = create_trace(args.trace)

player_classes = []
for p in args.players:
    match = re.match(r"(\w+)?(\*\d+)?", p)
    if match:
        #import mechanism
        #we expect a particular filename->classname mapping such that
        #ExampleAI resides in ai/example.py, FooAI in ai/foo.py etc.
        name = match.group(1)
        package = name[:-2].lower()
        if match.group(2):
            count = int(match.group(2)[1:])
        else:
            count = 1
        try:
            klass = getattr(importlib.import_module("ai."+package), name)
            for i in range(count):
                player_classes.append(klass)
        except:
            print("Unable to import AI %s from ai/%s.py" % (name, package))
            raise

kwargs = dict(curses=args.curses, color=args.color, delay=args.delay,
              connect=CONNECT, cmap=MAP, ckey=KEY, areas=AREAS, wait=args.wait, deal=args.deal,
              seed=args.seed, trace=trace)
def wrapper(stdscr, **kwargs):
    g = Game(screen=stdscr, **kwargs)
    for i, klass in enumerate(player_classes):
        g.add_player(NAMES[i], klass)
    return g.play()
        
if args.games == 1:
    if args.curses:
        curses.wrapper(wrapper, **kwargs)
    else:
        wrapper(None, **kwargs)
else:
    wins = collections.defaultdict(int)
    for j in range(args.games):
        kwargs['round'] = (j+1, args.games)
        kwargs['history'] = wins
        if args.curses:
            victor = curses.wrapper(wrapper, **kwargs)
        else:
            victor = wrapper(None, **kwargs)
        wins[victor] += 1
    print("Outcome of %s games" % args.games)
    for k in sorted(wins, key=lambda x: wins[x]):
        print("%s [%s]:\t%s" % (k, player_classes[NAMES.index(k)].__name__, wins[k]))

# Close trace file
trace.close()

