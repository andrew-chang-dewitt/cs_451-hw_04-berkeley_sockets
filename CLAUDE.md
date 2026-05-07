# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```bash
make all          # build berkeley_life + berkeley_worker (release)
make berk         # build berkeley_life binary
make worker       # build berkeley_worker binary
make test         # build & run peer tests
make clean        # remove target/

DBG=true make     # debug build (-O0 -g)
PRF=true make     # profiling build (-pg, gprof)
VERBOSE=true make # enable VERBOSE print debugging (#ifdef VERBOSE blocks)
PRETTY=true make  # enable pretty-print output
```

Binaries land in `target/release/bin/` or `target/debug/bin/` depending on mode.

## Run

`berkeley_life` is the distributed main; `berkeley_worker` is the worker. Each worker connects to one dedicated port.

```bash
# Start main first, then one worker per partition (consecutive ports starting at -P)
./target/release/bin/berkeley_life -s <size> -c <cycles> -g <num_parts> -P <base_port> -i <init_string> &
for i in $(seq 0 $(( num_parts - 1 ))); do
    ./target/release/bin/berkeley_worker 127.0.0.1 $(( base_port + i )) &
done
```

**Startup order matters on WSL2:** WSL2 loopback does not send RST on SYN to unlistened ports — the SYN retransmits and the resulting connection has a broken data path. Always start main before workers so each port is already in LISTEN state when the worker connects.

`test_berkeley.sh` runs all test cases locally (blinker, glider, loaf) across granularities 1–4.

Example (Glider, 10×10, 8 cycles, 4 workers):
```bash
./target/release/bin/berkeley_life -s 10 -c 8 -g 4 -P 9000 -i 0100000000001000000011100000000000000000000000000000000000000000000000000000000000000000000000000000000000 &
for i in 0 1 2 3; do
    sleep 0.15
    ./target/release/bin/berkeley_worker 127.0.0.1 $(( 9000 + i )) &
done
```

## Python / FABRIC tooling

Python env managed with `uv`. `direnv` auto-activates via `.envrc`.

```bash
uv sync           # install deps
uv run setup.py   # provision FABRIC testbed slice (1 main + 10 child nodes)
```

`fabric_rc` holds FABRIC credentials (gitignored). `setup.py` spins up nodes on the FABRIC testbed for distributed experiments — `berkeley_life.c` is the distributed target.

## Architecture

C implementations share common modules:

| File | Role |
|------|------|
| `berkeley_life.c` | Distributed GoL main process (FABRIC target) |
| `berkeley_worker.c` | Distributed GoL worker process |
| `world.c/h` | World state: `init_world`, `print_world`, accessors |
| `step.c/h` | `step()` (single-thread), `step_part()` (partition-aware) |
| `args.c/h` | CLI parsing → `Config` struct (includes `port` field, `-P` flag) |
| `peer.c/h` | TCP peer abstraction: `peer_accept`, `peer_connect`, `peer_send`, `peer_recv`, `peer_close` |

**Distributed protocol:** Main accepts one worker per partition on consecutive ports (`base_port`, `base_port+1`, ...). Sends each worker a 16-byte config (world_size, cycles, part_start, part_end as big-endian uint32_t). Each cycle: main sends full grid to each worker, receives `part_rows × size` bytes back (the computed partition). Sequential per-worker pattern (send→receive worker 0, then worker 1, ...) avoids deadlock.

**Memory model:** `world_history` is a flat `char*` buffer of size `cycles × size × size`. Generation `i` starts at offset `i * size * size`. Main collects worker results directly into the next generation's slot.

**Partitioning:** `Config.parts` is an array of row counts, one per worker. `step_part()` takes `part_start`/`part_end` row indices and operates only on that slice of the current generation. Workers receive the full grid but only compute and return their assigned rows.
