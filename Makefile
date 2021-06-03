CC=gcc
CCFLAGS=-lpthread
ifeq ($(shell uname -s),Linux)
	# needed for timer_create
	CCFLAGS += -lrt
endif

.PHONY: proftest
proftest:
	$(CC) -Werror proftest.c $(CCFLAGS) -o proftest
