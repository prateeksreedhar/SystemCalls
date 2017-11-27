#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/signal.h>
#include <linux/signalfd.h>
#include <linux/sched.h>

SYSCALL_DEFINE2(smunch, int, pid, unsigned long, bit_pattern)
{
unsigned long       flags;
struct task_struct *process;

// Get the process with PID = pid
rcu_read_lock();
process = pid_task(find_vpid(pid), PIDTYPE_PID);
rcu_read_unlock();

// Handle error case when the process is not found
if( !process )
	{
	printk( KERN_ALERT "Process %d was not found\n", pid );
	return -1;
	}
	
// Handle error case when process is being traced
if( unlikely(process->ptrace) )
	{
	printk( KERN_ALERT "Process %d is being traced\n", pid );
	return -1;
	}
	
// Handle case for multi-threaded processes 
if( unlikely( !thread_group_empty( process ) ) )
	{
	printk( KERN_ALERT "Process %d is multithreaded\n", pid );
	return -1;
	}
	
// Handle case for Zombie process
if( unlikely(process->exit_state == EXIT_ZOMBIE) )
		{
		printk(KERN_ALERT "Process %d is a zombie\n", pid);
		if( bit_pattern & ( 1 << (SIGKILL -1)) )
			{
			printk(KERN_ALERT "Killing zombie process %d\n", pid);
			release_task( process );
			}
		return 0;
		}
		
		
printk(KERN_ALERT "Sending process %d a bit pattern of 0x%lx\n", pid, bit_pattern);
if( !lock_task_sighand(process, &flags) )
	{
	printk( KERN_ALERT "Failed to grab the lock for process %d\n", pid );
	return -1;
	}

// Send signal to the process
process->signal->shared_pending.signal.sig[0] |= bit_pattern;	
unlock_task_sighand(process, &flags);
wake_up_process(process);
return 0;	
}
