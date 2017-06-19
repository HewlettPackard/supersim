# SuperSim - Basic Simulations

The process from running a simulation to receiving useful results contains 4
main steps:

1. Generating the simulation settings
2. Running the simulator
3. Analyzing the data
4. Plotting the results

This document walks through this process step-by-step with a basic example.

## Setup
Let's create a directory to hold this investigation.

``` sh
mkdir ~/ssdev/basic_sims
cd ~/ssdev/basic_sims

```

## Generating the simulation settings
The configuration system of SuperSim uses [libsettings][] which is a JSON-based
settings file system that allows command line modifications. Let's take an
example settings file from the SuperSim project. We'll modify the settings
on the command line when we run the simulator.

``` sh
cp ../supersim/json/foldedclos_iq_blast.json sample.json
```

## Running the simulator
Run the simulator with the settings file and some modifications. These
modifications will set the desired output files as well as the traffic
pattern to use in the simulation.

``` sh
../supersim/bin/supersim sample.json \
  network.channel_log.file=string=channels.csv \
  workload.message_log.file=string=messages.mpf.gz \
  workload.applications[0].rate_log.file=string=rates.csv \
  workload.applications[0].blast_terminal.traffic_pattern.type=string=random_exchange
```

The first thing SuperSim does is prints the settings as it interprets them.
If you have problems, look here to see that your settings were set the way
you thought they should be.

As the simulator runs it prints the progress of the simulator. When the
simulation completes is prints out a statistics summary of the simulation.

You can view the channel utilization with the following command:

``` sh
column -t -s, channels.csv | less
```

You can view the injection and ejection rates with the following command:

``` sh
column -t -s, rates.csv | less
```

The time based information related to flits, packets, messages, and transactions
is contained in the `message_log` which is a custom file format. In this
example, we've also SuperSim to compress the file by giving it a `.gz` file
extension. View its structure with the following command:

``` sh
gzip -c -d messages.mpf.gz | less
```

Scroll down until you see lines that start with `+M`, `+P`, and `F`. Notice the
hierarchy of transactions, messages, packets, and flits. This file can be used
to generate all types latency-based analyses.

## Analyzing the data
Assuming we care about packet latency as our metric, let's run the parsing
program [SSLatency][] to get prepared for plotting the results. We can also use
this program to provide a basic analysis. Run the following command:

``` sh
../sslatency/bin/sslatency -a aggregate.csv -p packets.csv.gz messages.mpf.gz
```
The outputs of this command are 2 files:
1. aggregate.csv - Simple analysis info in a CSV format
2. packets.csv.gz - Packet latency info in a compressed CSV format

Take a look in the aggregate.csv file to discover how our network performed
with the specified traffic pattern:

``` sh
column -t -s, aggregate.csv
```

Note: you can also tell SSLatency to generate message latency data and
transaction latency data using the `-m` and `-t` flags, respectively.

## Plotting the resuls
Let's plot the packet latency results using the [SSPlot][] plotting package.
This package has many executables. The only one we'll use for this example is:

sslqp - Latency quad plot focused on latency distribution

Run it and view the plot with the following commands:
Note: I'm using `eog` image viewer. Any image viewer will do.

``` sh
sslqp packets.csv.gz packets.png
eog packets.png
```

[libsettings]: https://github.com/nicmcd/libsettings
[SSLatency]: https://github.com/nicmcd/sslatency
[SSPlot]: https://github.com/nicmcd/ssplot
