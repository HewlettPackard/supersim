# SuperSim - Automated Simulations

The types of analyses that can be drawn from simulation depends on the type of
simulations being run. For example, a trace-driven simulation generally is only
good for a single simulation run per network configuration. In contrast,
synthetic workloads are often used to generate load vs. latency plots where the
injection rate is incrementally increased across many simulation runs. As
described in [Simulation Basics](basics.md), there are many steps needed even
for a sinlge simulation run. If you consider multiple workloads, network
configurations, injection rates, etc., you might end up with thousands of
tasks that must be run. Naturally, tasks depend on the completion of previous
tasks. This document covers the [TaskRun][] tool that is used to run all these
tasks in proper order while utilizing all the available hardware (processor
cores, main memory, etc.).

## Example
In this document we'll generate a TaskRun script that will investigate adaptive
routing in a Folded-Clos network topology. With a single command, we'll be able
to run all simulations and plot the results.

We'll compare two routing algorithms: oblivious least common ancestor and
adaptive least common ancestor. For each configuration we'll sweep the injection
rate from 0% to 100% in steps of 6%. In total, there will be 34 simulation runs.
Each simulation result will be parsed by [SSLatency][] and plotted by `sslqp`
([SSPlot][]). Each injection rate sweep will be plotted by `ssllp`. The
combination of both injection rate sweeps will be plotted by `sslcp`. In total,
there are 113 tasks run.

## Setup
Let's create a directory to hold this investigation.

``` sh
mkdir ~/ssdev/auto_sims
cd ~/ssdev/auto_sims

```

Now fetch the TaskRun script that we'll use for automated simulations.

``` sh
cp ../supersim/scripts/auto_sims.py .
```

Let's look through this script to get a better understanding of how TaskRun
works.

``` sh
emacs auto_sims.py
```

The script starts by determining the quantity of local system resources. This
TaskRun script will schedule tasks to keep all the CPUs full and will use the
entire memory space if needed. TaskRun will not oversubscribe these resources
but it will keep them utilized given enough task parallelism.

The next section of the script creates the TaskManager object which is the
central scheduling engine of all tasks. The TaskManager is given a
ResourceManager and two Observers. The FileCleanupObserver will delete the
files of a task if it fails. This ensures that if the script is re-run, any
tasks that previously failed will be re-executed. The VerboseObserver just
provides a nice command line output while the tasks are running.

The next section creates an array for injection rates to be simulated for all
configurations. The granularity of the load vs. latency sweeps can be set using
the `sweep_step` variable.

The next section creates all the simulation tasks. You can see that there is a
doubly nested for loop. One loop sweeps the configurations and the other sweeps
the injection rates. More complex scripts will have more loops defined to create
more configuration permutations. Each task has a name and a command. In this
script, the tasks are also given stdout and stderr files, resources, a priority,
and a file-based condition. The resources state how many `cpus` and `mem` the
process will take. This is how the ResourceManager knows which tasks can
be run at any given time. The priority is set to 0 and all other tasks will have
a priority of 1, which states that any task other than simulation will run
before the simulation tasks where possible. The condition is a
FileModificationCondition object which tracks the input and output files of the
task. If the output files don't exist or if the input files are newer than the
output files, the task will be executed, otherwise it will be bypassed.

The next two sections create the parsing tasks using [SSLatency][] and the
latency distribution plotting tasks using `sslqp`. The methodology is very
similar to creating the simulation task with one exception. The parsing and
plotting tasks can't begin until the corresponding preceding task has completed.
For this functionality the tasks are given a dependency pointing back to the
corresponding preceding task.

The next section generates load vs. latency plots using `ssllp`. Again, the
process is similar to the previous description but in this case the results from
a whole injection rate sweep produce a single plot. Thus, there are many
dependencies for these tasks not just one.

The last section generates the comparative load vs. latency plots using `sslcp`.
This script generates one of these plots for all latency metrics produced by
SSPlot (Mean, Median, 90th%, 99.99th%, etc.). The dependencies of these tasks
list all parsing tasks as they use all of them to generate the plots.

Finally, the script runs all tasks with the `run_tasks()` function on the
TaskManager object.

## Run
We'll use an example settings file from the SuperSim project.

``` sh
cp ../supersim/json/foldedclos_iq_blast.json settings.json
```

Now we are ready to run the TaskRun script.

``` sh
./auto_sims.py
```

As it runs, the VerboseObserver will show you what it going on. It reports when
tasks get started, when/if they are bypassed, and when they complete. Tasks can
complete successfully or fail. Bypassed tasks are colored yellow, successful
tasks are colored green, and failed tasks are colored red. Upon failure the
description of the task is printed, which for a ProcessTask is the command line
that was executed. Everytime a task completes the VerboseObserver will show the
current progress informing you of how many total tasks there are, how many have
completed so far, and an estimate of the remaining time to finish all tasks.
After all tasks have completed a summary is shown of how many tasks were
successful, bypassed, and failed.

Take a look at the resulting plots:

``` sh
eog lplot*png cplot*png
```

TaskRun is designed to only run tasks that need to be run. Let's re-run our
script:

``` sh
./auto_sims.py
```

TaskRun will check the status of all tasks and determine that all of the tasks
should be bypassed. This is really useful for situations where you want to run a
coarse grain simulation to see a general trend. Then when all looks good you can
increase the granularity and re-run the TaskRun script. Only the missing tasks
will be run while the rest of them will be bypassed. Let's increase the
simulation granularity and re-run the script:

``` sh
./auto_sims.py -g 3
```

After it completes, you'll see in the summary the prior tasks were bypassed and
only the new tasks were executed. You can now see the resulting plots have
higher granularity:

``` sh
eog lplot*png cplot*png
```

[TaskRun]: https://github.com/nicmcd/taskrun
[SSLatency]: https://github.com/nicmcd/sslatency
[SSPlot]: https://github.com/nicmcd/ssplot
