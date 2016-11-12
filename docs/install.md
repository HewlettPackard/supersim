# SuperSim - Installation

## Summary
This document outlines how to install SuperSim, its dependencies, and its
accompanying tools. The instructions in the document will build a stand-alone
environment that can be modified and rebuilt easily.

## System requirements
SuperSim is designed to run on Linux, although it doesn't necessarily rely
on Linux features. Clever coders will be able to adapt these instructions
for Windows or Mac, however, this is not officially supported.

These installation instructions require the following software:
- g++ 4.8+ (5 or 6 is recommended)
- git
- python3
- psutil
- zlib
- libtclap
- matplotlib
- wget

These can be installed on a modern Debian based system with the following
commands:

``` sh
sudo apt-get install g++ git python3 python3-pip python3-dev libtclap-dev zlib1g-dev wget
pip3 install setuptools --user
pip3 install numpy matplotlib psutil --user
```

## Create a development directory
Create a directory to hold the development environment:

``` sh
mkdir ~/ssdev
```

## Install Python projects
The required Python packages will be installed with `--user` which means
they won't effect the system installation. These can be installed with
following commands:

``` sh
cd ~/ssdev
wget https://raw.githubusercontent.com/hewlettpackardlabs/supersim/master/scripts/installpy
chmod +x installpy
./installpy clone install
```

If you wanted to, you could uninstall ALL these packages with following command:

``` sh
cd ~/ssdev
./installpy uninstall
```

## Build C++ projects
The C++ projects use a simple Makefile system that installs into a user local
directory. This can be installed with the following commands:

``` sh
git clone https://github.com/nicmcd/make-c-cpp ~/.makeccpp
cd ~/.makeccpp
make
```

If you wanted to, you could uninstall this system with the following command:

``` sh
rm -rf ~/.makeccpp
```

The C++ projects are built as stand-alone libraries and executables.
No system installation takes place. Use the following commands to build the C++ programs:

``` sh
cd ~/ssdev
wget https://raw.githubusercontent.com/hewlettpackardlabs/supersim/master/scripts/installcc
chmod +x installcc
./installcc clone build
```

You can run the unit tests for these projects with the following command:

``` sh
./installcc check
```

## Directory listing
This is what your development environment now looks like:

``` sh
ls ~/ssdev
  hyperxsearch  libbits     libfio   libmut   libsettings  sslatency  taskrun
  installcc     libcolhash  libgrid  libprim  libstrop     ssplot
  installpy     libex       libjson  librnd   percentile   supersim
```

## Reinstalling/rebuilding after modification
If you modify one of the Python packages, simply reinstall them:

``` sh
./installpy install
```

If you modify one of the C++ projects, simply rebuild them:

``` sh
./installcc build
```
