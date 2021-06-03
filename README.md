# REAMDE

proftest is a C application for testing the quality of different operating
system APIs for profiling.

Usage:

```
make

# test setitimer
./proftest -i

# test timer_create
./proftest -c
```

Using docker:

```
docker build -t proftest .
docker run --rm -it proftest ./proftest -i
```

## Results

See the bottom of this [twitter thread](https://twitter.com/felixge/status/1397522130904965120) for the latest results. tl;dr: setitimer(2) seems pretty bad for profiling on Linux.
