#ifdef __APPLE__
#include <sys/types.h>
#include <sys/sysctl.h>

int logical_cpus() {
  int count;
  size_t count_len = sizeof(count);
  assert(sysctlbyname("hw.logicalcpu", &count, &count_len, NULL, 0) == 0);
  return count;
}

bool has_timer_create = false;

void setup_timer_create(int hz) {
  assert(0); // not available on darwin
}
#endif
