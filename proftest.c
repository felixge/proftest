#define _GNU_SOURCE // asprintf

#include <stdio.h> // print
#include <unistd.h> // getopt, usleep
#include <pthread.h> // pthreads
#include <stdlib.h> // malloc, free, atoi
#include <assert.h> // assert
#include <stdbool.h> // bool type
#include <sys/time.h> // setitimer
#include <signal.h> // signal stuff
#include <sys/utsname.h> // uname
#include <string.h> // strlen

// OS specific proftest stuff
#include "darwin.h"
#include "linux.h"

// thread local storage for our logical thread id, used by signal handlers
__thread int thread_id;

struct thread {
  long loops; // amount of work this thread should do
  int signals; // number of signals this thread received
  double time_sec; // CPU time this thread consumed

  bool timer_create; // setup timer_create for this thread?
  int hz; // only relevant if timer_create is true
};

// threads holds the state of each worker thread
struct thread *threads;

void *work_thread(void *arg) {
  thread_id = *(int *)arg;
  free(arg);

  struct thread cur_thread = threads[thread_id];
  if (cur_thread.timer_create == true) {
    setup_timer_create(cur_thread.hz);
  }

  struct timespec start_time;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start_time);
  // do some "work"
  for (long i = 0; i < cur_thread.loops; i++) {
    // intentionally left blank
  }

  struct timespec end_time;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end_time);
  threads[thread_id].time_sec = (end_time.tv_sec - start_time.tv_sec) + ((double)end_time.tv_nsec - (double)start_time.tv_nsec) / 10e9;
  return NULL;
}

void work(int thread_count, int work_scale, bool timer_create, int hz) {
  threads = malloc(thread_count * sizeof(struct thread));
  pthread_t * pthreads = malloc(thread_count * sizeof(pthread_t));
  assert(pthreads != NULL);
  for (int i = 0; i < thread_count; i++) {
    threads[i].hz = hz;
    threads[i].signals = 0;
    threads[i].timer_create = timer_create;
    threads[i].loops = 1e9 * work_scale;
    int * thread_id = malloc(sizeof(thread_id));
    assert(thread_id != NULL);
    *thread_id = i;
    assert(pthread_create(&pthreads[i], NULL, work_thread, (void *) thread_id) == 0);
  }

  printf("thread_id,signals,cpu_seconds,hz\n");
  for (int i = 0; i < thread_count; i++) {
    assert(pthread_join(pthreads[i], NULL) == 0);
  }

  // A signal handler might still be running, so give it 1ms to complete.
  // TODO: Is there a way to properly deal with this race condition?
  usleep(1000);

  double sum_hz = 0;
  double min_hz = 0;
  double max_hz = 0;
  for (int i = 0; i < thread_count; i++) {
    double thread_hz = (double)threads[i].signals / (threads[i].time_sec);
    printf("%d,%d,%.3f,%.0f\n", i, threads[i].signals, threads[i].time_sec, thread_hz);
    sum_hz += thread_hz;
    if (thread_hz > max_hz) {
      max_hz = thread_hz;
    }
    if (thread_hz < min_hz || min_hz == 0) {
      min_hz = thread_hz;
    }
  }

  double avg_hz = sum_hz / thread_count;
  printf("\nprocess hz (total): %.f\n", sum_hz);
  printf("thread hz (min): %.f\n", min_hz);
  printf("thread hz (avg): %.f\n", avg_hz);
  printf("thread hz (max): %.f\n", max_hz);

  free(pthreads);
  free(threads);
}


void signal_handler() {
  threads[thread_id].signals++;
}

void setup_setitimer(int hz) {
  struct itimerval it;
  it.it_interval.tv_sec = 0;
  it.it_interval.tv_usec = 1e6 / hz;
  it.it_value = it.it_interval;
  assert(setitimer(ITIMER_PROF, &it, NULL) == 0);
}

void setup_signal_handler() {
  struct sigaction sa;
  sa.sa_sigaction = signal_handler;
  sa.sa_flags = SA_RESTART | SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  assert(sigaction(SIGPROF, &sa, NULL) == 0);
}

char * os_name() {
  // TODO Am I responsible for freeing the fields in the buf struct o_O?
  struct utsname buf;
  assert(uname(&buf) == 0);
  char * dst;
  int r = asprintf(&dst, "%s %s %s %s", buf.sysname, buf.release, buf.version, buf.machine);
  assert(r != -1);
  return dst;
}

int main(int argc, char *argv[]) {
  int threads = logical_cpus();
  int hz = 100;
  int work_scale = 1;
  bool setitimer = false;
  bool timer_create = false;

  int opt;
  while((opt = getopt(argc, argv, "hict:f:w:")) != -1) {
    switch(opt) { 
      case 'h':
        printf(
          "usage: proftest <options>\n"
          "\n"
          "  -h\tPrint help and exit\n"
          "  -t\tNumber of threads to spawn (default: %d)\n"
          "  -i\tUse setitimer(2) for profiling (default: %d)\n"
          "  -c\tUse timer_create(2) for profiling (default: %d)\n"
          "  -f\tFrequency in hz for profiling (default: %d)\n"
          "  -w\tAmount of work per thread given in billions of operations. (default: %d)\n",
          threads,
          setitimer,
          timer_create,
          hz,
          work_scale
        );
        return 0;
      case 't':
        threads = atoi(optarg);
        break;
      case 'f':
        hz = atoi(optarg);
        break;
      case 'w':
        work_scale = atoi(optarg);
        break;
      case 'i':
        setitimer = true;
        break;
      case 'c':
        timer_create = true;
        break;
      case '?': 
        printf("unknown option: %c\n", optopt);
        return 1;
    } 
  } 

  char * os = os_name();
  printf("threads: %d\n", threads);
  printf("work: %d\n", work_scale);
  printf("setitimer: %d\n", setitimer);
  printf("timer_create: %d\n", timer_create);
  printf("hz: %d\n", hz);
  printf("os: %s\n", os);
  printf("\n");
  free(os);

  setup_signal_handler();
  if (setitimer) {
    setup_setitimer(hz);
  }
  if (timer_create && has_timer_create == false) {
    printf("timer_create(2) is not available on this OS\n");
    return 1;
  }

  work(threads, work_scale, timer_create, hz);
}
