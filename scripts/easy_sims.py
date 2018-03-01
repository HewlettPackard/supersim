#!/usr/bin/env python3

import argparse
import os
import sssweep
import taskrun
import tempfile

ap = argparse.ArgumentParser()
ap.add_argument('-g', '--granularity', type=int, default=6,
                help='the granularity of the injection rate sweeps')
ap.add_argument('-d', '--directory', type=str, default='output',
                help='the output directory')
ap.add_argument('--supersim', type=str, default='bin/supersim',
                help='supersim binary to run')
ap.add_argument('--ssparse', type=str, default='../ssparse/',
                help='ssparse directory')
ap.add_argument('--settings', type=str, default='json/fattree_iq_blast.json',
                help='settings file to use')
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
                 failure_mode=taskrun.FailureMode.AGGRESSIVE_FAIL)

# output directory
out_dir = args.directory
if not os.path.isdir(out_dir):
  os.mkdir(out_dir)

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
supersim_path = args.supersim
settings_path = args.settings
ssparse_path = os.path.join(args.ssparse, 'bin', 'ssparse')
transient_path = os.path.join(args.ssparse, 'scripts', 'transient.py')

# create sweeper
s = sssweep.Sweeper(supersim_path, settings_path,
                    ssparse_path, transient_path,
                    set_task_function, out_dir, sim=True,
                    parse_scalar=0.001, latency_units='ns')
# ssparse
s.add_plot('load-latency-compare', 'ssparse')
s.add_plot('load-latency', 'ssparse')
s.add_plot('load-percent-minimal', 'ssparse', yauto_frame=0.2)
s.add_plot('load-average-hops', 'ssparse', yauto_frame=0.2)
s.add_plot('load-rate-percent', 'ssparse')

s.add_plot('latency-pdf', 'ssparse2', ['+app=0'])
s.add_plot('latency-percentile', 'ssparse2', ['+app=0'])
s.add_plot('latency-cdf', 'ssparse2', ['+app=0'])
s.add_plot('time-latency-scatter', 'ssparse2', ['+app=0'])

# straight through
s.add_plot('load-rate', 'none')

# transient
s.add_plot('time-percent-minimal', 'transient', yauto_frame=0.2)
s.add_plot('time-average-hops', 'transient', non_minimal='n')
s.add_plot('time-latency', 'transient')

# routing variable
routing_algorithms = ['deterministic', 'oblivious', 'adaptive']
def set_ra_cmd(ra, config):
  if ra == 'deterministic':
    det, red = 'true', 'all_minimal'
  elif ra == 'oblivious':
    det, red = 'false', 'all_minimal'
  elif ra == 'adaptive':
    det, red = 'false', 'least_congested_minimal'
  else:
    assert False
  return ('network.protocol_classes[0].routing.deterministic=bool={0} '
          'network.protocol_classes[0].routing.reduction.algorithm=string={1}'
          .format(det, red))
s.add_variable('Routing Algorithm', 'RA', routing_algorithms, set_ra_cmd,
               compare=True)

# loads
start = 0
stop = 100
step = args.granularity
def set_load_cmd(ld, config):
  cmd = ('workload.applications[0].blast_terminal.request_injection_rate=float={0} '
         'workload.applications[0].blast_terminal.enable_responses=bool=false '
         .format(0.001 if ld == '0.00' else ld))
  return cmd
s.add_loads('Load', 'LD', start, stop, step, set_load_cmd)

# auto-add tasks to the task manager
s.create_tasks(tm)

# run the tasks!
tm.run_tasks()
