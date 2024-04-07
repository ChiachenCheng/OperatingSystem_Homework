/* This file deals with CHRT system call. 
 * The entry points into this file are:
 *   do_chrt: perform the CHRT system call
 */

#include "pm.h"
#include <sys/wait.h>
#include <assert.h>
#include <minix/callnr.h>
#include <minix/com.h>
#include <minix/sched.h>
#include <minix/vm.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <signal.h>
#include "mproc.h"
#include <sys/time.h>

/*===========================================================================*
 *				do_chrt				     *
 *===========================================================================*/
int do_chrt(void)
{
  return (sys_chrt(mp->mp_endpoint,m_in.m_m2.m2l1)); 
}

