#!/usr/bin/env python3

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
import time

try:
  COLOR = True
  import termcolor
except:
  COLOR = False

def main(args):
  error = False

  # mark the start time
  start_time = time.time()

  # check that this is being run from the supersim directory
  files = [('scripts', 'auto_sims.py'),
           ('scripts', 'easy_sims.py'),
           ('docs', 'install.md'),
           ('docs', 'basic_sims.md'),
           ('docs', 'auto_sims.md'),
           ('docs', 'easy_sims.md')]
  files = [os.path.join(os.getcwd(), x[0], x[1]) for x in files]
  for f in files:
    if not os.path.exists(f):
      print('can\nt find {}'.format(f))
      error = True
      break

  if not error:
    # create a temporary directory for an installation
    tmpd = tempfile.mkdtemp(prefix='supersim_regression_')

    try:
      # run tests within the temporary directory
      test('Install', install_test, args.verbose, tmpd)
      test('Basic sims', basic_sims_test, args.verbose, tmpd)
      test('Auto sims', auto_sims_test, args.verbose, tmpd)
      test('Easy sims', easy_sims_test, args.verbose, tmpd)
    except RuntimeError:
      error = True

    # delete the temporary directory, or tell the user about it
    if not args.skip_clean:
      shutil.rmtree(tmpd)
    else:
      print('  need to manually clean {}'.format(tmpd))

  # mark the end time and report total time
  end_time = time.time()
  print('Total time: {:.2f}s'.format(end_time - start_time))

# utility functions
def test(name, func, verbose, *args):
  start_time = time.time()
  print('{} test running ... '.format(name))
  sys.stdout.flush()
  res = func(verbose, *args)
  end_time = time.time()
  if res == None:
    msg = '  passed {:.2f}s'.format(end_time - start_time)
    if COLOR:
      msg = termcolor.colored(msg, 'green')
    print(msg)
  else:
    msg = '  failed {:.2f}s\n{}'.format(end_time - start_time, res)
    if COLOR:
      msg = termcolor.colored(msg, 'red')
    print(msg)
    raise RuntimeError()

def run(cmd, verbose):
  if not verbose:
    proc = subprocess.run(cmd, shell=True, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT)
  else:
    proc = subprocess.run(cmd, shell=True)
  if proc.returncode != 0:
    return proc.stdout.decode('utf-8')
  return None

def markdown_commands(filename, *skip):
  cmds = []
  with open(filename, 'r') as fd:
    in_cmd = False
    for line in fd:
      line = line.strip()
      if in_cmd:
        if line.startswith('``` sh'):
          assert False, 'state machine error'
        elif line.startswith('```'):
          in_cmd = False
        else:
          match = False
          for s in skip:
            if line.startswith(s):
              match = True
              break
          if not match:
            cmds.append(line)
      else:
        if line.startswith('``` sh'):
          in_cmd = True
  return cmds

# tests below here
def install_test(verbose, tmpd):
  cmds = markdown_commands('docs/install.md', 'sudo', 'pip3',
                           '~/ssdev/supersim/bazel-bin/supersim')
  replace = [('~/ssdev', os.path.join(tmpd, 'ssdev')),
             ('for prj in supersim ssparse', 'for prj in ssparse')]
  for idx in range(len(cmds)):
    for rpl in replace:
      cmds[idx] = cmds[idx].replace(*rpl)
  sourcefile = os.path.join(tmpd, 'install_sourcefile')
  runfile = os.path.join(tmpd, 'install_runfile')
  with open(sourcefile, 'w') as fd:
    ssdir = os.path.join(tmpd, 'ssdev', 'supersim')
    print('mkdir -p {}'.format(ssdir), file=fd)
    print('cp -R src config BUILD WORKSPACE {}'.format(ssdir), file=fd)
    for cmd in cmds:
      print(cmd, file=fd)
    print('cd {}'.format(ssdir), file=fd)
    print('bazel build -c opt :supersim :supersim_test :lint', file=fd)
    print('{} --help'.format(os.path.join(
      tmpd, 'ssdev/supersim/bazel-bin/supersim')), file=fd)
  with open(runfile, 'w') as fd:
    print('#!/bin/bash', file=fd)
    print('set -e errexit', file=fd)
    print('which g++ git python3 wget column', file=fd)
    print('python3 -c "import setuptools; import numpy; import matplotlib;"',
          file=fd)
    print('source {}'.format(sourcefile), file=fd)
  assert run('chmod +x {}'.format(runfile), verbose) == None
  res = run(runfile, verbose)
  return res

def basic_sims_test(verbose, tmpd):
  cmds = markdown_commands('docs/basic_sims.md', 'eog')
  replace = [('~/ssdev', os.path.join(tmpd, 'ssdev')),
             ('~/sims', os.path.join(tmpd, 'sims')),
             ('| less', '')]
  for idx in range(len(cmds)):
    for rpl in replace:
      cmds[idx] = cmds[idx].replace(*rpl)
  sourcefile = os.path.join(tmpd, 'basic_sims_sourcefile')
  runfile = os.path.join(tmpd, 'basic_sims_runfile')
  with open(sourcefile, 'w') as fd:
    for cmd in cmds:
      print(cmd, file=fd)
  with open(runfile, 'w') as fd:
    print('#!/bin/bash', file=fd)
    print('set -e errexit', file=fd)
    print('source {}'.format(sourcefile), file=fd)
  assert run('chmod +x {}'.format(runfile), verbose) == None
  res = run(runfile, verbose)
  return res

def auto_sims_test(verbose, tmpd):
  cmds = markdown_commands('docs/auto_sims.md', 'emacs', 'eog')
  replace = [('~/ssdev/supersim/scripts/auto_sims.py',
              os.path.join(os.getcwd(), 'scripts', 'auto_sims.py')),
             ('~/ssdev', os.path.join(tmpd, 'ssdev')),
             ('~/sims', os.path.join(tmpd, 'sims'))]
  for idx in range(len(cmds)):
    for rpl in replace:
      cmds[idx] = cmds[idx].replace(*rpl)
  sourcefile = os.path.join(tmpd, 'auto_sims_sourcefile')
  runfile = os.path.join(tmpd, 'auto_sims_runfile')
  with open(sourcefile, 'w') as fd:
    for cmd in cmds:
      print(cmd, file=fd)
  with open(runfile, 'w') as fd:
    print('#!/bin/bash', file=fd)
    print('set -e errexit', file=fd)
    print('source {}'.format(sourcefile), file=fd)
  assert run('chmod +x {}'.format(runfile), verbose) == None
  res = run(runfile, verbose)
  return res

def easy_sims_test(verbose, tmpd):
  cmds = markdown_commands('docs/easy_sims.md', 'emacs', 'python3')
  replace = [('~/ssdev/supersim/scripts/easy_sims.py',
              os.path.join(os.getcwd(), 'scripts', 'easy_sims.py')),
             ('~/ssdev', os.path.join(tmpd, 'ssdev')),
             ('~/sims', os.path.join(tmpd, 'sims'))]
  for idx in range(len(cmds)):
    for rpl in replace:
      cmds[idx] = cmds[idx].replace(*rpl)
  sourcefile = os.path.join(tmpd, 'easy_sims_sourcefile')
  runfile = os.path.join(tmpd, 'easy_sims_runfile')
  with open(sourcefile, 'w') as fd:
    for cmd in cmds:
      print(cmd, file=fd)
  with open(runfile, 'w') as fd:
    print('#!/bin/bash', file=fd)
    print('set -e errexit', file=fd)
    print('source {}'.format(sourcefile), file=fd)
  assert run('chmod +x {}'.format(runfile), verbose) == None
  res = run(runfile, verbose)
  return res

# command line
if __name__ == '__main__':
  ap = argparse.ArgumentParser()
  ap.add_argument('-m', '--memcheck', action='store_true',
                  help='perform memory checking')
  ap.add_argument('-s', '--skip_clean', action='store_true',
                  help='skip cleaning up the directory')
  ap.add_argument('-v', '--verbose', action='store_true',
                  help='print all output as it runs')
  args = ap.parse_args()
  main(args)
