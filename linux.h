#ifdef __linux__ 
#include <sys/sysinfo.h> // get_nprocs

int logical_cpus() {
  return get_nprocs();
}
#endif
