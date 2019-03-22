# Contributing

All contributed code should compile, pass lint checks, pass unit tests, and pass JSON examples.

## Build
``` sh
bazel build :supersim :supersim_test
```

## Lint

``` sh
bazel build :lint
```

## Unit tests
Simple:
``` sh
bazel run :supersim_test
```

With memory check:
``` sh
valgrind --leak-check=full --show-reachable=yes --track-fds=yes ./bazel-bin/supersim_test
```

## JSON Examples:
Simple:
``` sh
./scripts/run_examples.py
```

With memory check:
``` sh
./scripts/run_examples.py -m
```
