TODO:

- [x] implement distributed life w/ berkely sockets
- [x] test that it works w/ multiple procs on one machine using loopback
- [ ] script fabric setup
- [ ] gen report on fabric

---

# CS451: Introduction to Parallel and Distributed Computing
**Spring 2026 — Assignment 4: Berkeley sockets**

## Setup

1. Create a FABRIC slice containing eleven nodes provisioned with Ubuntu 22.04.5.
2. On each node, install GCC, Valgrind, and gprof.

## Development

3. Take the program from step 3 in Assignment 3 and increase the world's size by a factor of 1000. Adapt the program to split `step()` execution across 10 nodes over the network. Remember to serialize state when moving it over the network. The computation of boundary cells must be done by the main program.
4. Run the updated Life program for 100 cycles and time its execution.

## Bonus (+30%)

5. Using Valgrind, check the updated Life binary for memory leaks at all nodes in the network where the binary is being run.
6. Using gprof, profile the updated Life binary at all nodes in the network where the binary is being run.

## Submission

Submit a `.tgz` file containing:

- `berkeley_life.c` — code of the updated Life program from step 3, plus any accompanying `.c` and `.h` files.
- A set of files named `a.out`, `a.err`, `b.out`, etc., each containing:
  - **a.** Output from compilation (step 3).
  - **b.** Output from running the program (step 4).
  - **c.** *(Bonus only)* Output from step 5.
  - **d.** *(Bonus only)* Output from step 6.

Files ending in `.out` contain stdout; files ending in `.err` contain stderr.
