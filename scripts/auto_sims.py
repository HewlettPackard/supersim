#!/usr/bin/env python3

import argparse
import os
import ssplot
from taskrun import *

ap = argparse.ArgumentParser()
ap.add_argument('-g', '--granularity', type=int, default=6,
                help='the granularity of the injection rate sweeps')
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
                 failure_mode=FailureMode.ACTIVE_CONTINUE)

# generate an array for the loads to be simulated
sweep_start = 0
sweep_stop = 100
sweep_step = args.granularity
loads = ['{0:.02f}'.format(x/100)
         for x in range(sweep_start, sweep_stop+1, sweep_step)]

ymax = 500

# create all sim tasks
sim_tasks = {}
for a in ['oblivious', 'adaptive']:
  for l in loads:
    id = a + '_' + l
    sim_name = 'sim_' + id
    sim_cmd = ('../supersim/bin/supersim '
               'settings.json '
               'network.traffic_classes[0].routing.adaptive=bool={0} '
               'workload.applications[0].max_injection_rate=float={1} '
               'network.channel_log.file=string={2} '
               'workload.applications[0].rate_log.file=string={3} '
               'workload.message_log.file=string={4} '
               .format(
                 'true' if a == 'adaptive' else 'false',
                 '0.000001' if l == '0.00' else l,
                 'channels_' + id + '.csv',
                 'rates_' + id + '.csv',
                 'messages_' + id + '.mpf.gz'))
    sim_task = ProcessTask(tm, sim_name, sim_cmd)
    sim_task.stdout_file = 'simout_' + id + '.log'
    sim_task.stderr_file = 'simout_' + id + '.log'
    sim_task.resources = {'cpus': 1, 'mem': 3}
    sim_task.priority = 0
    sim_task.add_condition(FileModificationCondition(
      [], ['channels_' + id + '.csv',
           'rates_' + id + '.csv',
           'messages_' + id + '.mpf.gz',
           'simout_' + id + '.log']))
    sim_tasks[id] = sim_task

# create all parse tasks
parse_tasks = {}
for a in ['oblivious', 'adaptive']:
  for l in loads:
    id = a + '_' + l
    parse_name = 'parse_' + id
    parse_cmd = ('../sslatency/bin/sslatency '
                 '-a aggregate_{0}.csv '
                 '-m messages_{0}.csv.gz '
                 '{1}'
                 .format(id, 'messages_' + id + '.mpf.gz'))
    parse_task = ProcessTask(tm, parse_name, parse_cmd)
    parse_task.resources = {'cpus': 1, 'mem': 3}
    parse_task.priority = 1
    parse_task.add_dependency(sim_tasks[id])
    parse_task.add_condition(FileModificationCondition(
      ['messages_' + id + '.mpf.gz'],
      ['aggregate_' + id + '.csv',
       'messages_' + id + '.csv.gz']))
    parse_tasks[id] = parse_task

# create all sslqp tasks
for a in ['oblivious', 'adaptive']:
  for l in loads:
    id = a + '_' + l
    qplot_name = 'qplot_' + id
    qplot_title = '"Algorithm={0} Load={1}"'.format(a, l)
    qplot_cmd = ('sslqp {0} {1} --title {2} '
                 .format('messages_' + id + '.csv.gz',
                         'messages_' + id + '.png',
                         qplot_title))
    qplot_task = ProcessTask(tm, qplot_name, qplot_cmd)
    qplot_task.resources = {'cpus': 1, 'mem': 3}
    qplot_task.priority = 1
    qplot_task.add_dependency(parse_tasks[id])
    qplot_task.add_condition(FileModificationCondition(
      ['messages_' + id + '.csv.gz'],
      ['messages_' + id + '.png']))

# create all ssllp tasks
for a in ['oblivious', 'adaptive']:
  id = a
  lplot_name = 'lplot_' + id
  lplot_title = '"Algorithm={0}"'.format(a)
  lplot_cmd = ('ssllp --row Message --ymin 0 --ymax {0} '
               '{1} {2} {3} {4} --title {5} '
               .format(ymax, 'lplot_' + id + '.png',
                       sweep_start, sweep_stop + 1, sweep_step,
                       lplot_title))
  for l in loads:
    id2 = a + '_' + l
    lplot_cmd += 'aggregate_' + id2 + '.csv '
  lplot_task = ProcessTask(tm, lplot_name, lplot_cmd)
  lplot_task.resources = {'cpus': 1, 'mem': 3}
  lplot_task.priority = 1
  for l in loads:
    id2 = a + '_' + l
    lplot_task.add_dependency(parse_tasks[id2])
  lplot_fmc = FileModificationCondition(
    [], ['lplot_' + id + '.png'])
  for l in loads:
    id2 = a + '_' + l
    lplot_fmc.add_input('aggregate_' + id2 + '.csv')
  lplot_task.add_condition(lplot_fmc)

# create all sslcp tasks
for f in ssplot.LoadLatencyStats.FIELDS:
  cplot_name = 'cplot_' + f
  cplot_title = '"{0} Latency"'.format(f)
  cplot_cmd = ('sslcp --row Message --title {0} '
               '--field {1} {2} {3} {4} {5} --ymin 0 --ymax {6} '
               .format(cplot_title, f, cplot_name + '.png',
                       sweep_start, sweep_stop + 1, sweep_step, ymax))
  for a in ['oblivious', 'adaptive']:
    for l in loads:
      id2 = a + '_' + l
      cplot_cmd += 'aggregate_' + id2 + '.csv '
  for a in ['oblivious', 'adaptive']:
    cplot_cmd += '--label ' + a + ' '
  cplot_task = ProcessTask(tm, cplot_name, cplot_cmd)
  cplot_task.resources = {'cpus': 1, 'mem': 3}
  cplot_task.priority = 1
  for a in ['oblivious', 'adaptive']:
    for l in loads:
      id2 = a + '_' + l
      cplot_task.add_dependency(parse_tasks[id2])
  cplot_fmc = FileModificationCondition(
    [], [cplot_name + '.png'])
  for a in ['oblivious', 'adaptive']:
    for l in loads:
      id2 = a + '_' + l
      cplot_fmc.add_input('aggregate_' + id2 + '.csv')
  cplot_task.add_condition(cplot_fmc)

# run the tasks!
tm.run_tasks()
