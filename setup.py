"""Set up FABRIC nodes for assignment."""

from fabrictestbed_extensions.fablib.fablib import FablibManager
from fabrictestbed_extensions.fablib.node import Interface, Node

fablib = FablibManager(fabric_rc="./fabric_rc")

slice_name = "hw_04"
image = "default_ubuntu_22"
site = "INDI"

hw_04 = fablib.new_slice(name=slice_name)

nodes: list[Node] = []
nics: list[Interface] = []

def init_node(name:str) -> None:
    node = hw_04.add_node(name=name,image=image,cores=2,ram=4,disk=9,site=site)
    nic = node.add_component(model="NIC_Basic",name="iface1").get_interfaces()[0]

    nodes.append(node)
    nics.append(nic)


init_node("main")

for i in range(1,11):
    init_node(f"child{i}")

net = hw_04.add_l2network(name="net",interfaces=nics)

# FIXME: this creates slice ok, but then hangs forever in wait_ssh?
hw_04.submit()
