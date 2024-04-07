/* The kernel call implemented in this file:
 *   m_type:	SYS_CHRT
 *
 * The parameters for this kernel call are:
 *   m_lsys_krn_sys_trace.endpt		(endpoint of the process that use chrt)
 *   m_lsys_krn_sys_trace.data		(the deadline of this process)
 */
 
#include "kernel/system.h"
#include "kernel/vm.h"
#include <signal.h>
#include <string.h>
#include <assert.h>

#include <minix/endpoint.h>
#include <minix/u64.h>


/*===========================================================================*
 *				do_chrt					     *
 *===========================================================================*/
int do_chrt(struct proc * caller, message * m_ptr)
{
  struct proc *rpp;			/* process pointer */
  int p_proc;

  if(!isokendpt(m_ptr->m_lsys_krn_sys_trace.endpt, &p_proc))
	return EINVAL;

  rpp = proc_addr(p_proc);
  rpp->p_deadline = m_ptr->m_lsys_krn_sys_trace.data;
    
  return OK;
}
