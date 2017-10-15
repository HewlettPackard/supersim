# SuperSim

## What is it?
*SuperSim* is an event-driven cycle-accurate flit-level interconnection network simulator written in C++. SuperSim's highest priority is flexibility. As Professor [Christos Kozyrakis][christos] once said:
> If a simulator already does what you want it to do, there's a good chance you aren't asking the right questions.

SuperSim is designed to be a simulation framework, not a ready-to-go simulator for all scenarios. The core of SuperSim is a well-structured abstract class hierarchy that generically represents the interaction between common network components. Specific implementations override the basic abstractions to create the desired functionality without reinventing the wheel for every new model.

SuperSim is meant to produce very detailed and realistic simulations. While anything can in theory be simulated, the convention of SuperSim thus far is to model the real world on a cycle-by-cycle basis. This provides closer to real life simulation results and better verification for logical designs.

## Where did it come from?
SuperSim was developed by [Nic McDonald][nicmcd_hpl] ([github][nicmcd_gh]) at [Hewlett Packard Labs][hpelabs] and was sponsored under the [DARPA POEM][poem] project where it was used to investigate alternative router microarchitectures optimized for very high-radix designs driven by high-bandwidth photonically enabled I/O ports. The architecture designed was called *SuperSwitch*, thus the name of the simulator became *SuperSim*. SuperSim has been and is used for many research projects at Hewlett Packard Labs. It is used academically at Stanford University in the [Interconnection Networks][ee382c] class. Current development of SuperSim is sponsored in part by the [DOE PathForward][pathforward] program where it is being used to explore exascale topologies, routing algorithms, and router microarchitectures.

## Why use it?
SuperSim has some key advantages in its design and development that make it particularly useful.
+ SuperSim strongly separates the application (e.g., workload) modeling from the network modeling. Any application model can use any network model and vice versa. If you want to model some rare workload, it is easy to write an application model and test it on any network design. There are no dependencies between them.
+ SuperSim uses event-driven simulation which has the following major advantages:
  - Event-driven simulation is fast because it only models devices when things change. This is in contrast to cycle-driven simulation that models devices on every cycle regardless if something is going to change or not.
  - Event-driven simulation provides an automatic data structure to the component models. In many cases, the event system can be used to store data that will be needed later and the model doesn't have to store the data manually. This greatly simplifies model development.
+ SuperSim is meticulously developed. It has a very well-defined coding and formatting standard (almost identical to the Google coding standard). There is no mish-mash of different styles and formats.
+ SuperSim uses a proactive linear regression algorithm to determine when a network is saturated or warmed up. All other simulators simply run the simulator for a fixed time then begin logging. This produces untrustworthy results for networks that are close to the saturation point. SuperSim runs the simulator until it can be determined that the network is saturated or it is warmed up. It is never a guessing game in SuperSim whether you ran the simulation long enough to produce correct results.
+ SuperSim is supported by many tools that allow simulations to be run, parsed, analyzed, and plotted very efficiently.
  - [SSLatency][sslatency] is an optimized C++ program that parses the verbose file format output from SuperSim into latency numbers. SSLatency prepares files for analysis and plotting.
  - [SSPlot][ssplot] is a Python plotting tool that generates analyses and plots for individual runs, sweeping runs (e.g., load vs. latency), and comparison against many sweeping runs. SSPlot is a Python package with accompanying command line executable scripts.
  - [TaskRun][taskrun] is an easy-to-use Python package for running tasks with dependencies, conditional execution, resource management, and much more. The process from running simulations, parsing the results, analyzing the data, and plotting the results entails many steps. Each step has dependencies on previous steps. TaskRun scripts make this whole process easy and automated. A simple TaskRun script can elegantly runs thousands of simulations and all required post-simulation tools. TaskRun is also able to interface with batch scheduling systems (e.g., GridEngine, PBS, LFS, etc.).
  - [sssweep][sssweep] is a flexible python package to automatically generate and perform supersim simulations with one or many sweeping variables. sssweep allows the user to easily add and set simulation variables. sssweep automatically generates a web viewer tailored to your simulation parameters to easily analyse and share your results.

## Ok, now what?
Visit the [docs][docs] and have fun simulating!


[christos]: http://csl.stanford.edu/~christos/ "Christos' Home Page"
[nicmcd_hpl]: http://labs.hpe.com/people/nicmcd/ "Nic's Labs Page"
[nicmcd_gh]: https://github.com/nicmcd "Nic's GitHub Page"
[hpelabs]: http://www.labs.hpe.com/ "Hewlett Packard Labs Home"
[poem]: http://www.darpa.mil/program/photonically-optimized-embedded-microprocessors "DARPA POEM Page"
[pathforward]: http://www.exascaleinitiative.org/pathforward "DOE PathForward"
[ee382c]: https://explorecourses.stanford.edu/search?q=ee382c "EE382C Description"
[sslatency]: https://github.com/nicmcd/sslatency "SSLatency at GitHub"
[ssplot]: https://github.com/nicmcd/ssplot "SSPlot at GitHub"
[taskrun]: https://github.com/nicmcd/taskrun "TaskRun at GitHub"
[sssweep]: https://github.com/nicmcd/sssweep "sssweep at GitHub"
[docs]: docs/README.md "SuperSim Documentation"
