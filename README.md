# REAMDE

proftest is a C application for testing the quality of different operating
system APIs for profiling.

Usage:

```
make
# test setitimer
./proftest -i
```

Using docker (TODO: Fix these instructions):

```
docker build -t proftest .
docker run --rm -it proftest bash
```
