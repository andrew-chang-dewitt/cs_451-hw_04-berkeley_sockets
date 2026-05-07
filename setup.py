"""Set up FABRIC nodes for assignment."""

from pathlib import Path
from sys import exit

from fabrictestbed_extensions.fablib.fablib import FablibManager
from fabrictestbed_extensions.fablib.node import Interface, Node

print("Phase -1: requisitioning slice")

print("    setting up fablib from ./fabric_rc...", end="")
fablib = FablibManager(fabric_rc="./fabric_rc")
print(" done")

slice_name = "hw_04"
image = "default_ubuntu_22"
site = "INDI"

slice = fablib.new_slice(name=slice_name)
print(f"    slice {slice.get_name()} created")

all_nodes: list[Node] = []
nics: list[Interface] = []


def init_node(name: str) -> None:
    node = slice.add_node(name=name, image=image, cores=2, ram=4, disk=9, site=site)
    try:
        nic = node.add_component(model="NIC_Basic", name="iface1").get_interfaces()[0]  # type: ignore
    except KeyError:
        print("    expected nics to be list!")
        exit(1)

    all_nodes.append(node)
    nics.append(nic)


init_node("main")

for i in range(1, 11):
    init_node(f"worker{i}")

net = slice.add_l2network(name="net", interfaces=nics)

print("    node configuration set up")

print("    submitting request for slice...", end="")
slice.submit()
print(" done")


#
# Constants
#

SIZE = 1581  # sqrt(50^2 * 1000) — 1000x more cells than assignment 3
CYCLES = 100
NUM_WORKERS = 10
BASE_PORT = 9000
REPO_DIR = "~/hw_04"
MAIN_IP = "10.0.0.1"

REPORT_DIR = Path("report")
REPORT_DIR.mkdir(exist_ok=True)

main_node = slice.get_node("main")
worker_nodes = [slice.get_node(f"worker{i}") for i in range(1, 11)]
all_nodes = [main_node] + worker_nodes


def make_init(size: int) -> str:
    rows = [["0"] * size for _ in range(3)]
    # glider, left side (rows 0-2, cols 1-4)
    rows[0][1] = "1"
    rows[1][2] = "1"
    rows[2][2] = "1"
    rows[2][3] = "1"
    rows[2][4] = "1"
    # horizontal blinker, right side (row 1, cols size-6 to size-4)
    b = size - 6
    rows[1][b] = "1"
    rows[1][b + 1] = "1"
    rows[1][b + 2] = "1"
    return "".join("".join(r) for r in rows)


INIT = make_init(SIZE)


def save(prefix: str, stdout: str, stderr: str) -> None:
    (REPORT_DIR / f"{prefix}.out").write_text(stdout)
    (REPORT_DIR / f"{prefix}.err").write_text(stderr)


#
# Phase 0: configure L2 network, install deps, deploy code
#

print("Phase 0: configure L2 network, install deps, deploy code")

for i, node in enumerate(all_nodes):
    try:
        iface = node.get_interfaces()[0].get_os_interface()  # type: ignore
    except KeyError:
        print("    expected interfaces to be list!")
        exit(1)

    node.execute(
        f"sudo ip addr add 10.0.0.{i + 1}/24 dev {iface} && sudo ip link set {iface} up"
    )

print("    installing prereqs (gcc, make, valgrind)on all nodes...", end="")
install_threads = [
    node.execute_thread(
        "sudo apt-get update -y && sudo apt-get install -y gcc make valgrind"
    )
    for node in all_nodes
]
print(" done")

print("    uploading source files...", end="")
upload_threads = [node.upload_directory_thread(".", REPO_DIR) for node in all_nodes]
for t in install_threads + upload_threads:
    t.result()
print(" done")

#
# Phase 1: release build + timed run  →  a.out/a.err, b.out/b.err
#

print("    starting worker builds...", end="")
worker_build_threads = [
    node.execute_thread(f"cd {REPO_DIR} && make clean && make worker")
    for node in worker_nodes
]
print(" done")

print("    building main...", end="")
main_build_out, main_build_err = main_node.execute(
    f"cd {REPO_DIR} && make clean && make berk"
)
save("a", main_build_out, main_build_err)
print(" done")

print("    waiting for worker builds to complete...", end="")
for t in worker_build_threads:
    # this is actually a Future<Thread>, type signature on fablib.Node.execute_thread() is wrong
    t.result()  # type: ignore
print(" done")

print("    starting worker binaries...", end="")
worker_run_threads = [
    node.execute_thread(
        f"cd {REPO_DIR} && ./target/release/bin/berkeley_worker"
        f" {MAIN_IP} {BASE_PORT + i}"
    )
    for i, node in enumerate(worker_nodes)
]
print(" done")

print("    executing main...", end="")
run_out, run_err = main_node.execute(
    f"cd {REPO_DIR} && time ./target/release/bin/berkeley_life"
    f" -s {SIZE} -c {CYCLES} -g {NUM_WORKERS} -P {BASE_PORT} -i {INIT}"
)
save("b", run_out, run_err)
print(" done")

print("    waiting for worker executions to complete...", end="")
for t in worker_run_threads:
    t.result()  # type: ignore
print(" done")


#
# Phase 2: valgrind  →  c.out/c.err
#

worker_debug_threads = [
    node.execute_thread(f"cd {REPO_DIR} && make clean && DBG=true make worker")
    for node in worker_nodes
]
main_node.execute(f"cd {REPO_DIR} && make clean && DBG=true make berk")
for t in worker_debug_threads:
    t.result()  # type: ignore

worker_valgrind_threads = [
    node.execute_thread(
        f"cd {REPO_DIR} && valgrind --leak-check=full"
        f" ./target/debug/bin/berkeley_worker {MAIN_IP} {BASE_PORT + i}"
    )
    for i, node in enumerate(worker_nodes)
]
valgrind_out, valgrind_err = main_node.execute(
    f"cd {REPO_DIR} && valgrind --leak-check=full ./target/debug/bin/berkeley_life"
    f" -s {SIZE} -c {CYCLES} -g {NUM_WORKERS} -P {BASE_PORT} -i {INIT}"
)
save("c", valgrind_out, valgrind_err)
for t in worker_valgrind_threads:
    t.result()  # type: ignore

#
# Phase 3: gprof  →  d.out/d.err, d_worker{N}.out/d_worker{N}.err
#

worker_prf_threads = [
    node.execute_thread(f"cd {REPO_DIR} && make clean && PRF=true make worker")
    for node in worker_nodes
]
main_node.execute(f"cd {REPO_DIR} && make clean && PRF=true make berk")
for t in worker_prf_threads:
    t.result()  # type: ignore

worker_prf_run_threads = [
    node.execute_thread(
        f"cd {REPO_DIR} && ./target/debug/bin/berkeley_worker {MAIN_IP} {BASE_PORT + i}"
    )
    for i, node in enumerate(worker_nodes)
]
main_node.execute(
    f"cd {REPO_DIR} && ./target/debug/bin/berkeley_life"
    f" -s {SIZE} -c {CYCLES} -g {NUM_WORKERS} -P {BASE_PORT} -i {INIT}"
)
for t in worker_prf_run_threads:
    t.result()  # type: ignore

gprof_out, gprof_err = main_node.execute(
    f"cd {REPO_DIR} && gprof ./target/debug/bin/berkeley_life gmon.out"
)
save("d", gprof_out, gprof_err)

worker_gprof_threads = [
    node.execute_thread(
        f"cd {REPO_DIR} && gprof ./target/debug/bin/berkeley_worker gmon.out"
    )
    for node in worker_nodes
]
for i, t in enumerate(worker_gprof_threads):
    out, err = t.result()  # type: ignore
    save(f"d_worker{i + 1}", out, err)
