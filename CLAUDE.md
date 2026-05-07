# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```bash
make all          # build gran (release)
make gran         # granular multithreaded life
make test         # build & run peer tests
make clean        # remove target/

DBG=true make     # debug build (-O0 -g)
PRF=true make     # profiling build (-pg, gprof)
VERBOSE=true make # enable VERBOSE print debugging (#ifdef VERBOSE blocks)
PRETTY=true make  # enable pretty-print output
```

Binaries land in `target/release/bin/` or `target/debug/bin/` depending on mode.

## Run

```bash
./target/release/bin/granular_multithreaded_life -s <size> -c <cycles> -g <num_parts> -i <init_string>
```

Example:
```bash
# Glider (10x10, 8 cycles, 4 partitions)
./target/release/bin/granular_multithreaded_life -s 10 -c 8 -g 4 -i 0100000000001000000011100000000000000000000000000000000000000000000000000000000000000000000000000000000000
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
| `granular_multithreaded_life.c` | Finer-grained thread partitioning |
| `berkeley_life.c` | Distributed (FABRIC) — currently empty |
| `world.c/h` | World state: `init_world`, `print_world`, accessors |
| `step.c/h` | `step()` (single-thread), `step_part()` (partition-aware) |
| `args.c/h` | CLI parsing → `Config` struct |

**Memory model:** `world_history` is a flat `char*` buffer of size `cycles × size × size`. Generation `i` starts at offset `i * size * size`. Threads write non-overlapping row ranges into the same buffer; a `pthread_barrier_t` (initialized to `num_parts + 1`, so main thread participates) synchronizes each generation before printing.

**Partitioning:** `Config.parts` is an array of row counts, one per thread. `step_part()` takes `part_start`/`part_end` row indices and operates only on that slice of the current generation.
