#include "syslib.h"

int sys_chrt(proc_ep, deadline)
endpoint_t proc_ep;
long deadline;
{
  message m;
  int r;

  m.m_lsys_krn_sys_trace.endpt = proc_ep;
  m.m_lsys_krn_sys_trace.data = deadline;
  
  r = _kernel_call(SYS_CHRT, &m);
  return r;
}
