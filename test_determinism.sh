#!/bin/bash
# Test script to verify deterministic behavior across runs
# Usage: ./test_determinism.sh [num_seeds]

NUM_SEEDS=${1:-20}
FAILED=0

echo "Testing determinism with $NUM_SEEDS random seeds..."

for seed in $(seq 1 $NUM_SEEDS); do
    python3 pyrisk.py --nocurses -d 0 -s $seed --trace /tmp/py1_$seed.jsonl StupidAI*2 2>/dev/null
    python3 pyrisk.py --nocurses -d 0 -s $seed --trace /tmp/py2_$seed.jsonl StupidAI*2 2>/dev/null

    if ! diff -q /tmp/py1_$seed.jsonl /tmp/py2_$seed.jsonl >/dev/null; then
        echo "FAIL at seed $seed"
        diff /tmp/py1_$seed.jsonl /tmp/py2_$seed.jsonl | head -20
        FAILED=1
        break
    fi
done

# Cleanup
rm -f /tmp/py1_*.jsonl /tmp/py2_*.jsonl

if [ $FAILED -eq 0 ]; then
    echo "PASS: All $NUM_SEEDS seeds produced identical traces"
    exit 0
else
    echo "FAIL: Determinism test failed"
    exit 1
fi
