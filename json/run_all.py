#!/usr/bin/env python3

import argparse
import glob
import math
import os
import psutil
import subprocess
import sys
import taskrun

try:
  import termcolor
  canColor = True
except ImportError:
  canColor = False

def logFile(jsonFile):
  base = os.path.splitext(os.path.basename(jsonFile))[0]
  return '/tmp/supersimtest_{0}'.format(base)

def good(s):
  if canColor:
    print('  ' + termcolor.colored(s, 'green'))
  else:
    print('  ' + s)

def bad(s):
  if canColor:
    print('  ' + termcolor.colored(s, 'red'))
  else:
    print('  ' + s)

def main(args):
  total_cpus = args.cpus
  if total_cpus == None:
    total_cpus = os.cpu_count()

  total_mem = args.mem
  if total_mem == None:
    psmem = psutil.virtual_memory()
    total_mem = math.floor((psmem.free + psmem.cached) / (1024 * 1024 * 1024))

  print('using up to {0} CPUs'.format(total_cpus))
  print('using up to {0} GiB of memory'.format(total_mem))

  if args.check:
    subprocess.check_call('valgrind -h', shell=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE)

  rm = taskrun.ResourceManager(
    taskrun.CounterResource('cpu', 9999, total_cpus),
    taskrun.MemoryResource('mem', 9999, total_mem))
  vob = taskrun.VerboseObserver()
  cob = taskrun.FileCleanupObserver()
  tm = taskrun.TaskManager(resource_manager=rm,
                           observers=[vob,cob],
                           failure_mode='passive_fail')

  # find all files
  settingsFiles = glob.glob('json/{0}.json'.format(args.glob))
  print('config files to test: {0}'.format(settingsFiles))

  # check if binary exists
  if not os.path.exists('./bazel-bin/bin'):
    print('./bazel-bin/bin does not exist')
    return -1

  # generate all tasks
  for settingsFile in settingsFiles:
    cmd = './bazel-bin/bin {0}'.format(settingsFile)
    if args.check:
      cmd = ('valgrind --log-fd=1 --leak-check=full --show-reachable=yes '
             '--track-origins=yes --track-fds=yes {0}'.format(cmd))
    log = logFile(settingsFile)
    if not args.skip:
      try:
        os.remove(log)
      except OSError:
        pass
    cmd = '{0} 2>&1 | tee {1}'.format(cmd, log)
    task = taskrun.ProcessTask(tm, settingsFile, cmd)
    task.resources = {'cpu': 1, 'mem': 10 if args.check else 3}

  # run tasks
  if args.skip:
    print('skipping simulations')
  else:
    print('running simulations')
    tm.run_tasks()
    print('done')

  # check output for failures
  anyError = False
  for settingsFile in settingsFiles:
    error = False
    print('analyzing {0} output'.format(settingsFile))

    # read in text
    log = logFile(settingsFile)
    with open(log, 'r') as fd:
      lines = fd.readlines();

    # analyze output
    simComplete = False
    for idx, line in enumerate(lines):

      if line.find('Simulation complete') >= 0:
        simComplete = True
      if args.check:
        if (line.find('Open file descriptor') >= 0 and
            lines[idx+1].find('inherited from parent') < 0):
          error = True
          bad('open file descriptor')
        if line.find('blocks are definitely lost') >= 0:
          error = True
          bad('definitely lost memory')
        if line.find('blocks are indirectly lost') >= 0:
          error = True
          bad('indirectly lost memory')
        if (line.find('blocks are still reachable') >= 0 and
            # TODO(nic): REMOVE ME WHEN G++ STOPS SUCKING
            not line.find('72,704 bytes') >= 0):
          error = True
          bad('still reachable memory')
        if line.find('depends on uninitialised value') >= 0:
          error = True
          bad('depends on uninitialised value')

    if not simComplete:
      error = True;
      bad('no "Simulation complete" message')

    # show status
    if error:
      anyError = True
    else:
      good('passed all tests')

  return 0 if not anyError else -1

if __name__ == '__main__':
  ap = argparse.ArgumentParser()
  ap.add_argument('-c', '--cpus', type=int,
                  help='number of CPUs to utilize')
  ap.add_argument('-r', '--mem', type=float,
                  help='amount of memory to utilize (in GiB)')
  ap.add_argument('-m', '--check', action='store_true',
                  help='Use valgrind to check the memory validity')
  ap.add_argument('-g', '--glob', default='*',
                  help='Glob expression match on json filenames')
  ap.add_argument('-s', '--skip', action='store_true',
                  help='Skip simulations (advanced)')
  args = ap.parse_args()
  sys.exit(main(args))
