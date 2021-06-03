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
  long loops;
  int signals;
  double time_sec;
};

struct thread *threads;

void *work_thread(void *arg) {
  struct timespec start_time;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start_time);

  thread_id = *(int *)arg;
  free(arg);

  for (long i = 0; i < threads[thread_id].loops; i++) {
  }

  struct timespec end_time;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end_time);
  threads[thread_id].time_sec = (end_time.tv_sec - start_time.tv_sec) + ((double)end_time.tv_nsec - (double)start_time.tv_nsec) / 10e9;
  return NULL;
}

void work(int thread_count, int work_scale) {
  threads = malloc(thread_count * sizeof(struct thread));
  pthread_t * pthreads = malloc(thread_count * sizeof(pthread_t));
  assert(pthreads != NULL);
  for (int i = 0; i < thread_count; i++) {
    threads[i].signals = 0;
    threads[i].loops = 1e9 * work_scale;
    int * thread_id = malloc(sizeof(thread_id));
    assert(thread_id != NULL);
    *thread_id = i;
    assert(pthread_create(&pthreads[i], NULL, work_thread, (void *) thread_id) == 0);
  }

  printf("thread_id,signals,cpu_seconds,hz\n");
  double sum_hz = 0;
  for (int i = 0; i < thread_count; i++) {
    assert(pthread_join(pthreads[i], NULL) == 0);
    double thread_hz = (double)threads[i].signals / (threads[i].time_sec);
    printf("%d,%d,%.3f,%.0f\n", i, threads[i].signals, threads[i].time_sec, thread_hz);
    sum_hz += thread_hz;
  }

  double avg_hz = sum_hz / thread_count;
  printf("\nprocess hz (total): %.f\n", sum_hz);
  printf("thread hz (avg): %.f\n", avg_hz);

  // A signal handler might still be running, so give it some time to complete.
  // TODO: Is there a way to properly deal with this race condition?
  usleep(100);
  free(pthreads);
  free(threads);
}


void signal_handler() {
  threads[thread_id].signals++;
}

void setup_setitimer(int hz) {
  assert(signal(SIGPROF, (void (*)(int)) signal_handler) != SIG_ERR);
  struct itimerval it;
  it.it_interval.tv_sec = 0;
  it.it_interval.tv_usec = 1000000 / hz;
  it.it_value = it.it_interval;
  assert(setitimer(ITIMER_PROF, &it, NULL) == 0);
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

  int opt;
  while((opt = getopt(argc, argv, "hit:f:w:")) != -1) { 
    switch(opt) { 
      case 'h':
        printf(
          "usage: proftest <options>\n"
          "\n"
          "  -h\tPrint help and exit\n"
          "  -t\tNumber of threads to spawn (default: %d)\n"
          "  -i\tUse setitimer(2) for profiling\n"
          "  -f\tFrequency in hz for profiling (default: %d)\n"
          "  -w\tAmount of work per thread given in billions of operations. (default: %d)\n",
          threads,
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
      case '?': 
        printf("unknown option: %c\n", optopt);
        return 1;
    } 
  } 

  char * os = os_name();
  printf("threads=%d setitimer=%d hz=%d work=%d os=%s\n\n", threads, setitimer, hz, work_scale, os);
  free(os);

  if (setitimer) {
    setup_setitimer(hz);
  }
  work(threads, work_scale);
}
