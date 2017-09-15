#!/usr/bin/env python3

import argparse
import os
import sssweep
import taskrun

ap = argparse.ArgumentParser()
ap.add_argument('-g', '--granularity', type=int, default=6,
                help='the granularity of the injection rate sweeps')
args = ap.parse_args()

# get the current amount of resources
cpus = os.cpu_count()
mem = taskrun.MemoryResource.current_available_memory_gib();

# build the task manager
rm = taskrun.ResourceManager(taskrun.CounterResource('cpus', 9999, cpus),
                             taskrun.MemoryResource('mem', 9999, mem))
cob = taskrun.FileCleanupObserver()
vob = taskrun.VerboseObserver(description=False, summary=True)
tm = taskrun.TaskManager(resource_manager=rm,
                 observers=[cob, vob],
                 failure_mode=taskrun.FailureMode.ACTIVE_CONTINUE)

# define resource usage
def get_resources(task_type, config):
  if task_type is 'sim':
    return {'cpus': 1, 'mem': 5}
  else:
    return {'cpus': 1, 'mem': 3}

# paths
supersim_path = '../supersim/bin/supersim'
settings_path = 'settings.json'
sslatency_path = '../sslatency/bin/sslatency'
out_dir = 'output'

# create sweeper
s = sssweep.Sweeper(supersim_path, settings_path,
                    sslatency_path, out_dir,
                    parse_scalar=0.001, parse_filters=[], latency_units='ns',
                    latency_ymin=0, latency_ymax=500,
                    rate_ymin=0, rate_ymax=200,
                    titles='long', plot_style='colon',
                    latency_mode='message', # 'packet-header', 'packet', 'message', 'transaction'
                    sim=True, parse=True,
                    lplot=True, rplot=True, qplot=True, cplot=True,
                    web_viewer=True, get_resources=get_resources)

# routing variable
routing_algorithms = ['oblivious','adaptive']
def set_ra_cmd(_ra, _config):
  cmd = ('network.traffic_classes[0].routing.adaptive=bool={0} '
         .format('true' if _ra == 'adaptive' else 'false'))
  return cmd
s.add_variable('Routing_Algorithm', 'RA', routing_algorithms, set_ra_cmd, compare=True)

# loads
start = 0
stop = 100
step = args.granularity
def set_l_cmd(_l, _config):
  cmd = ('workload.applications[0].blast_terminal.request_injection_rate=float={0} '
         'workload.applications[0].blast_terminal.enable_responses=bool=false '
         .format(0.001 if _l == '0.00' else _l))
  return cmd
s.add_loads('Load', 'l', start, stop, step, set_l_cmd)

# auto-add tasks to the task manager
s.create_tasks(tm)

# run the tasks!
tm.run_tasks()
