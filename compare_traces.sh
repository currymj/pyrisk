#!/bin/bash
# Compare Python and C++ implementation traces
# Usage: ./compare_traces.sh <seed> <python_trace> <cpp_trace>
#
# Example:
#   python3 pyrisk.py --nocurses -d 0 -s 42 --trace py.jsonl StupidAI*2
#   ./pyrisk_cpp -s 42 --trace cpp.jsonl StupidAI StupidAI
#   ./compare_traces.sh 42 py.jsonl cpp.jsonl

SEED=$1
PY_TRACE=$2
CPP_TRACE=$3

if [ -z "$PY_TRACE" ] || [ -z "$CPP_TRACE" ]; then
    echo "Usage: $0 <seed> <python_trace> <cpp_trace>"
    exit 1
fi

echo "Comparing traces for seed $SEED..."
echo "Python trace: $PY_TRACE"
echo "C++ trace: $CPP_TRACE"
echo ""

if diff -q "$PY_TRACE" "$CPP_TRACE" >/dev/null; then
    echo "PASS: Traces are identical"
    exit 0
else
    echo "FAIL: Traces differ"
    echo ""
    echo "First differences:"
    diff -y --suppress-common-lines "$PY_TRACE" "$CPP_TRACE" | head -30
    exit 1
fi
