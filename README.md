## Environment
- Ubuntu 20.04 (5.8.0-50-generic)
- Python 3.8.5
- g++ 9.3.0
- boost 1.71
- Mininet 2.3.0d6
- OpenFlow 1.3
- Open vSwitch 2.15.0 (see [Enable voyager field](#enable-voyager-field))
- Ryu 4.34

## Install
```
git clone https://github.com/voy49er/voyager.git
cd voyager/src && make
```

## Usage
### Configure
The default config `/config/default.cfg` uses an example topology and enables the custom match field.
```
toponame=compact.example.topo
err=0|0.25|0.50
timeout=0.5
custom=1
```

#### Topology
The key `toponame` specifies a topology file under `data/topo/`. The filename must end with `.topo`. The default file describes the example topology (see [the paper](https://github.com/voy49er/voyager.git)) in a simple format.
- Topology information
  - number of switches $k$
  - $k$ lines
    - switch ID, neighbor IDs
- Rule information ($k$ blocks)
  - switch ID, number of rules $r$
  - $r$ lines
    - rule ID, IPv4 address, in-port, out-port, priority

We include a generator in `data-raw/` that produces formatted topology files for [the dataset](http://www.topology-zoo.org/dataset.html). You can write your own generators for other datasets, and Voyager can work provided it has such a topology file as input.

#### Error rate
The key `err` specifies an error rate. Voyager simulates faults by not installing some rules, the number of which is determined by the error rate. For convenience, a list of error rates separated by `|` is allowed. Voyager will run with each automatically.

#### Timeout
The key `timeout` specifies a timeout. A test packet is considered dropped when its associated timer triggers. For a certain test packet, the ideal timeout is slightly more than the RTT. However, it is too sophisticated to estimate the RTT for each test packet. We use a fixed timeout for all test packets instead.

#### Custom flag
The key `custom` specifies whether the custom match field is used to settle test headers. If not, the `ipv4_src` field will be used. Before setting this flag, be sure [the custom OVS](#enable-voyager-field) is serving on your machine.

### Run
**Step 1: Assign report headers**
```
./run.sh setup
```
By default, the script will setup for the configured topology with an infinite path length threshold. It will do the assignment and persist related headers under `data/store/`. 
- Path headers in `.path.store` file
  - rule path, packet header, test header (for per-path tests on this rule path)
- Switch headers in `.switch.store` file
  - number of bits required $\phi_l$
  - $k$ lines
    - switch ID, $b$, $v$, test header (for per-rule tests on this switch)

You can also run the script with another file or a subdirectory under `/data/topo/`.
```
./run.sh setup [<file>|<directory>]
```
If you want to specify the path length threshold $p$, do not use the script and execute the executable in `/src` directly.
```
cd src && ./setup -f compact.example.topo -m compact -p 2
```
This will slice every path with a step size of 2 before doing the final assignment.

**Step 2: Start the network**
```
./run.sh mininet    # run this in one terminal
```

**Step 3: Start Voyager**
```
./run.sh voyager    # and run this in another
```
You will see a test report for each error rate.

## Enable voyager field
We develop a new match field based on Open vSwitch 2.15.0. All files modified to support this new field can be found in `customovs/`. The script inside will help to install the custom OVS from scratch.
```
sudo ./customovs/install.sh
```
Verify the installation. (You will see the output if everything is ok.)
```
sudo ovs-vsctl add-br br0
sudo ovs-ofctl dump-table-features br0 -O OpenFlow13 | grep -o voyager
```

## Quick assignment
We provide tools to facilitate the analysis of path length thresholds. You can derive a `.tss` file from a `.topo` file.
```
cd src && ./tss -f compact.example.topo -m store
```
This will have a `.tss` file in `/data/tss/`, which stores the TargetS Set of the topology. You can do very quick assignment with such a `.tss` file.
```
cd src && ./tss -f compact.example.tss -m compact
```
Also, you can compare the results of greedy coloring and our compact coloring.
```
cd src && ./tss -f compact.example.tss -m compare
```
Another benefit of `.tss` files is faster path splitting.
```
./split.sh compact.example.tss 1 3
```
This will generate three `.tss` files with thresholds 1 to 3 respectively.