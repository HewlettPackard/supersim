#!/usr/bin/env python3

import argparse
import os
import ssplot
import tempfile
from taskrun import *

ap = argparse.ArgumentParser()
ap.add_argument('-g', '--granularity', type=int, default=5,
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
mem = MemoryResource.current_available_memory_gib();

# build the task manager
rm = ResourceManager(CounterResource('cpus', 9999, cpus),
                     MemoryResource('mem', 9999, mem))
cob = FileCleanupObserver()
vob = VerboseObserver(description=False, summary=True)
tm = TaskManager(resource_manager=rm,
                 observers=[cob, vob],
                 failure_mode=FailureMode.AGGRESSIVE_FAIL)

# output directory
out_dir = args.directory
if not os.path.isdir(out_dir):
  os.mkdir(out_dir)

# generate an array for the loads to be simulated
sweep_start = 0
sweep_stop = 100
sweep_step = args.granularity
loads = ['{0:.02f}'.format(x/100)
         for x in range(sweep_start, sweep_stop+1, sweep_step)]

ymax = 500

# variable to sweep
routing_algorithms = ['deterministic', 'oblivious', 'adaptive']
def set_routing_algorithm(ra):
  if ra == 'deterministic':
    det, red = 'true', 'all_minimal'
  elif ra == 'oblivious':
    det, red = 'false', 'all_minimal'
  elif ra == 'adaptive':
    det, red = 'false', 'least_congested_minimal'
  else:
    assert False
  return (' network.protocol_classes[0].routing.deterministic=bool={0}'
          ' network.protocol_classes[0].routing.reduction.algorithm=string={1}'
          .format(det, red))

# create all sim tasks
sim_tasks = {}
for a in routing_algorithms:
  for l in loads:
    id = a + '_' + l
    sim_name = 'sim_' + id
    sim_cmd = ('{0} {1} '
               'workload.applications[0].blast_terminal.request_injection_rate=float={2} '
               'workload.applications[0].blast_terminal.enable_responses=bool=false '
               'network.channel_log.file=string={3} '
               'workload.applications[0].rate_log.file=string={4} '
               'workload.message_log.file=string={5} '
               .format(
                 args.supersim,
                 args.settings,
                 '0.0001' if l == '0.00' else l,
                 out_dir +'/channels_' + id + '.csv',
                 out_dir +'/rates_' + id + '.csv',
                 out_dir +'/messages_' + id + '.mpf.gz'))
    sim_cmd += set_routing_algorithm(a)
    sim_task = ProcessTask(tm, sim_name, sim_cmd)
    sim_task.stdout_file = out_dir +'/simout_' + id + '.log'
    sim_task.stderr_file = out_dir +'/simout_' + id + '.log'
    sim_task.resources = {'cpus': 1, 'mem': 5}
    sim_task.priority = 0
    sim_task.add_condition(FileModificationCondition(
      [], [out_dir +'/channels_' + id + '.csv',
           out_dir +'/rates_' + id + '.csv',
           out_dir +'/messages_' + id + '.mpf.gz',
           out_dir +'/simout_' + id + '.log']))
    sim_tasks[id] = sim_task

# create all parse tasks
parse_tasks = {}
for a in routing_algorithms:
  for l in loads:
    id = a + '_' + l
    parse_name = 'parse_' + id
    parse_cmd = ('{0} '
                 '-l {1}/latency_{2}.csv '
                 '-m {1}/messages_{2}.csv.gz '
                 '-s 0.001 '
                 '{3}'
                 .format(os.path.join(args.ssparse, 'bin', 'ssparse'),
                         out_dir,
                         id, out_dir + '/messages_' + id + '.mpf.gz'))
    parse_task = ProcessTask(tm, parse_name, parse_cmd)
    parse_task.resources = {'cpus': 1, 'mem': 3}
    parse_task.priority = 1
    parse_task.add_dependency(sim_tasks[id])
    parse_task.add_condition(FileModificationCondition(
      [out_dir +'/messages_' + id + '.mpf.gz'],
      [out_dir +'/latency_' + id + '.csv',
       out_dir +'/messages_' + id + '.csv.gz']))
    parse_tasks[id] = parse_task

# create latency percentile plots
for a in routing_algorithms:
  for l in loads:
    id = a + '_' + l
    qplot_name = 'percentile_' + id
    qplot_title = '"Algorithm={0} Load={1}"'.format(a, l)
    qplot_cmd = ('ssplot lpc {0} {1} --title {2} '
                 .format(out_dir + '/messages_' + id + '.csv.gz',
                         out_dir + '/percentile_' + id + '.png',
                         qplot_title))
    qplot_task = ProcessTask(tm, qplot_name, qplot_cmd)
    qplot_task.resources = {'cpus': 1, 'mem': 3}
    qplot_task.priority = 1
    qplot_task.add_dependency(parse_tasks[id])
    qplot_task.add_condition(FileModificationCondition(
      [out_dir +'/messages_' + id + '.csv.gz'],
      [out_dir +'/percentile_' + id + '.png']))

# create load versus latency tasks
for a in routing_algorithms:
  id = a
  lplot_name = 'loadlatency_' + id
  lplot_title = '"Algorithm={0}"'.format(a)
  lplot_cmd = ('ssplot ll --row Message --ymin 0 --ymax {0} '
               '{1} {2} {3} {4} --title {5} '
               .format(ymax, out_dir + '/loadlatency_' + id + '.png',
                       sweep_start, sweep_stop + 1, sweep_step,
                       lplot_title))
  for l in loads:
    id2 = a + '_' + l
    lplot_cmd += out_dir +'/latency_' + id2 + '.csv '
  lplot_task = ProcessTask(tm, lplot_name, lplot_cmd)
  lplot_task.resources = {'cpus': 1, 'mem': 3}
  lplot_task.priority = 1
  for l in loads:
    id2 = a + '_' + l
    lplot_task.add_dependency(parse_tasks[id2])
  lplot_fmc = FileModificationCondition(
    [], [out_dir +'/loadlatency_' + id + '.png'])
  for l in loads:
    id2 = a + '_' + l
    lplot_fmc.add_input(out_dir +'/latency_' + id2 + '.csv')
  lplot_task.add_condition(lplot_fmc)

# create load versus latency comparison tasks
for f in ssplot.LoadLatencyStats.FIELDS:
  cplot_name = 'loadlatencycompare_' + f
  cplot_title = '"{0} Latency"'.format(f)
  cplot_cmd = ('ssplot llc --row Message --title {0} '
               '--field {1} {2} {3} {4} {5} --ymin 0 --ymax {6} '
               .format(cplot_title, f, out_dir +'/' + cplot_name + '.png',
                       sweep_start, sweep_stop + 1, sweep_step, ymax))
  for a in routing_algorithms:
    for l in loads:
      id2 = a + '_' + l
      cplot_cmd += out_dir +'/latency_' + id2 + '.csv '
  for a in routing_algorithms:
    cplot_cmd += '--data_label ' + a + ' '
  cplot_task = ProcessTask(tm, cplot_name, cplot_cmd)
  cplot_task.resources = {'cpus': 1, 'mem': 3}
  cplot_task.priority = 1
  for a in routing_algorithms:
    for l in loads:
      id2 = a + '_' + l
      cplot_task.add_dependency(parse_tasks[id2])
  cplot_fmc = FileModificationCondition(
    [], [out_dir +'/' + cplot_name + '.png'])
  for a in routing_algorithms:
    for l in loads:
      id2 = a + '_' + l
      cplot_fmc.add_input(out_dir +'/latency_' + id2 + '.csv')
  cplot_task.add_condition(cplot_fmc)

# run the tasks!
tm.run_tasks()
