#!/usr/bin/env python3

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
import time

def main(args):
  test('Build', build_test)
  test('Unit', unit_test, args.mem_check)
  test('JSON', json_test, args.mem_check)
  test('Install', install_test)
  test('Auto sims', auto_sims_test)
  test('Easy sims', easy_sims_test)

# utility functions
def test(name, func, *args):
  start_time = time.time()
  print('{} test running ... '.format(name), end='')
  sys.stdout.flush()
  res = func(*args)
  end_time = time.time()
  if res == None:
    print('passed {:.2f}s'.format(end_time - start_time))
  else:
    print('failed {:.2f}s'.format(end_time - start_time))
    print(res)
    exit(-1)

def run(cmd):
  proc = subprocess.run(cmd, shell=True, stdout=subprocess.PIPE,
                        stderr=subprocess.STDOUT)
  if proc.returncode != 0:
    return proc.stdout.decode('utf-8')
  return None

def create_tmpdir():
  return tempfile.mkdtemp(prefix='supersim_regression_')

def cleanup_tmpdir(path):
  shutil.rmtree(path)

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
def build_test():
  res = run('make clean')
  if res is not None:
    return res
  return run('make')

def unit_test(mem_check):
  cmd = 'make check'
  if mem_check:
    cmd += 'm'
  return run(cmd)

def json_test(mem_check):
  cmd = 'json/run_all.py'
  if mem_check:
    cmd += ' -m'
  return run(cmd)

def install_test():
  tmpd = create_tmpdir()
  cmds = markdown_commands('docs/install.md', 'sudo', 'pip3', 'emacs', 'eog')
  replace = [('~/ssdev', os.path.join(tmpd, 'ssdev')),
             ('rm -rf ~/.makeccpp', ''),
             ('git clone https://github.com/nicmcd/make-c-cpp ~/.makeccpp',
              'rm -rf ~/.makeccpp; '
              'git clone https://github.com/nicmcd/make-c-cpp ~/.makeccpp')]
  for idx in range(len(cmds)):
    for rpl in replace:
      cmds[idx] = cmds[idx].replace(*rpl)
  sourcefile = os.path.join(tmpd, 'sourcefile')
  runfile = os.path.join(tmpd, 'runfile')
  with open(sourcefile, 'w') as fd:
    for cmd in cmds:
      print(cmd, file=fd)
  with open(runfile, 'w') as fd:
    print('#!/bin/bash', file=fd)
    print('set -e errexit', file=fd)
    print('source {}'.format(sourcefile), file=fd)
  assert run('chmod +x {}'.format(runfile)) == None
  res = run(runfile)
  cleanup_tmpdir(tmpd)
  return res

def auto_sims_test():
  tmpd = create_tmpdir()
  res = run('./scripts/auto_sims.py --directory {}'.format(tmpd))
  cleanup_tmpdir(tmpd)
  return res

def easy_sims_test():
  tmpd = create_tmpdir()
  res = run('./scripts/easy_sims.py --directory {}'.format(tmpd))
  cleanup_tmpdir(tmpd)
  return res

# command line
if __name__ == '__main__':
  ap = argparse.ArgumentParser()
  ap.add_argument('--mem_check', action='store_true',
                  help='perform memory checking')
  args = ap.parse_args()
  main(args)
