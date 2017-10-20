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

# create task and resources function
def set_task_function(tm, name, cmd, console_out, task_type, config):
  task = taskrun.ProcessTask(tm, name, cmd)
  if console_out:
    task.stdout_file = console_out
    task.stderr_file = console_out
  if task_type is 'sim':
    task.resources = {'cpus': 1, 'mem': 5}
  else:
    task.resources = {'cpus': 1, 'mem': 3}
  return task

# paths
supersim_path = '../supersim/bin/supersim'
settings_path = 'settings.json'
ssparse_path = '../ssparse/bin/ssparse'
out_dir = 'output'

# create sweeper
s = sssweep.Sweeper(supersim_path, settings_path,
                    ssparse_path, set_task_function,
                    out_dir,
                    parse_scalar=0.001, parse_filters=[], latency_units='ns',
                    latency_ymin=0, latency_ymax=500,
                    rate_ymin=0, rate_ymax=200,
                    titles='long', title_style='colon',
                    latency_mode='message', # 'packet-header', 'packet', 'message', 'transaction'
                    sim=True, parse=True,
                    lplot=True, rplot=True, qplot=True, cplot=True,
                    web_viewer=True)

# routing variable
routing_algorithms = ['oblivious','adaptive']
def set_ra_cmd(_ra, _config):
  cmd = ('network.protocol_classes[0].routing.adaptive=bool={0} '
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
