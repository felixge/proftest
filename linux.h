#ifdef __linux__ 
#include <sys/sysinfo.h> // get_nprocs

// see https://github.com/gperftools/gperftools/blob/master/src/profile-handler.cc#L76
#define sigev_notify_thread_id _sigev_un._tid

int logical_cpus() {
  return get_nprocs();
}

bool has_timer_create = true;

void setup_timer_create(int hz) {
  timer_t timerid;
  struct sigevent sev;

  memset(&sev, 0, sizeof(sev));
  sev.sigev_notify_thread_id = gettid();
  sev.sigev_notify = SIGEV_THREAD_ID;
  sev.sigev_signo = SIGPROF;
  sev.sigev_value.sival_ptr = &timerid;
  assert(timer_create(CLOCK_THREAD_CPUTIME_ID, &sev, &timerid) != -1);

  struct itimerspec it;
  it.it_interval.tv_sec = 0;
  it.it_interval.tv_nsec = 1e9 / hz;
  it.it_value = it.it_interval;
  assert(timer_settime(timerid, 0, &it, 0) == 0);
}

#endif
