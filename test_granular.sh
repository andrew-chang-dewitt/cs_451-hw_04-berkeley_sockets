#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIFE="$SCRIPT_DIR/target/release/bin/life"
GRAN="$SCRIPT_DIR/target/release/bin/granular_multithreaded_life"

PASS=0
FAIL=0

if [[ ! -x "$LIFE" || ! -x "$GRAN" ]]; then
    echo "Building..."
    make -C "$SCRIPT_DIR" all
fi

run_test() {
    local name="$1" size="$2" cycles="$3" init="$4"
    shift 4
    local granularities=("$@")

    local reference
    reference=$("$LIFE" -s "$size" -c "$cycles" -i "$init")

    for g in "${granularities[@]}"; do
        local result
        result=$("$GRAN" -s "$size" -c "$cycles" -i "$init" -g "$g")
        if [[ "$result" == "$reference" ]]; then
            echo "PASS  $name  g=$g"
            (( ++PASS ))
        else
            echo "FAIL  $name  g=$g"
            diff <(echo "$reference") <(echo "$result") || true
            (( ++FAIL ))
        fi
    done
}

# Blinker: horizontal bar at center, period-2 oscillator
BLINKER=$(printf '%s' \
    "00000" \
    "00000" \
    "01110" \
    "00000" \
    "00000")
run_test "blinker" 5 6 "$BLINKER" 1 2 3 4

# Glider: diagonal mover, period 4
GLIDER=$(printf '%s' \
    "0100000000" \
    "0010000000" \
    "1110000000" \
    "0000000000" \
    "0000000000" \
    "0000000000" \
    "0000000000" \
    "0000000000" \
    "0000000000" \
    "0000000000")
run_test "glider" 10 8 "$GLIDER" 1 2 3 4

# Loaf: still life — output identical every cycle
LOAF=$(printf '%s' \
    "000000" \
    "001100" \
    "010010" \
    "001010" \
    "000100" \
    "000000")
run_test "loaf" 6 4 "$LOAF" 1 2 3 4

echo ""
echo "Results: $PASS passed, $FAIL failed"
[[ "$FAIL" -eq 0 ]]
