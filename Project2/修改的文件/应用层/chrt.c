#include <sys/cdefs.h>
#include "namespace.h"
#include <lib.h>

#include <string.h>
#include <unistd.h>

int chrt(deadline)
const long deadline; 
{
  message m;
  memset(&m, 0, sizeof(m));
  m.m_m2.m2l1 = deadline;
  
  if (deadline <= 0)
  	return 0; 
 
  alarm(deadline); 

  return(_syscall(PM_PROC_NR, PM_CHRT, &m));
}
