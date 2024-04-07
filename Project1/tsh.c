/*
 * tsh - A tiny shell program with job control
 *
 * <Chia-ch'en Cheng:10182100359@stu.ecnu.edu.cn> 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <minix/type.h>
#include <minix/u64.h>
#include <minix/procfs.h>

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define  USED		0x1
#define  IS_TASK	0x2
#define  IS_SYSTEM	0x4
#define  BLOCKED	0x8

/* name of cpu cycle types, in the order they appear in /psinfo. */
const char *cputimenames[] = { "user", "ipc", "kernelcall" };

#define CPUTIMENAMES (sizeof(cputimenames)/sizeof(cputimenames[0]))

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
char command_history[MAXLINE][MAXLINE];
int command_num=1;
struct proc {
	int p_flags;
	endpoint_t p_endpoint;
	pid_t p_pid;
	u64_t p_cpucycles[CPUTIMENAMES];
};
int nr_total,slot;
unsigned int nr_procs,nr_tasks;
struct proc *proc = NULL, *prev_proc = NULL;
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_mytop();

/* error-handling wrapper funtion */
pid_t Fork(void);
pid_t Waitpid(pid_t pid, int *iptr, int options); 

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv);
void sigquit_handler(int sig);

void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine
 */
int main(int argc, char **argv)
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Execute the shell's read/eval loop */
    while (1)
    {
        /* Read command line */
        if (emit_prompt)
        {
            printf("%s", prompt);
            fflush(stdout);
        }
        if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
            app_error("fgets error");
        strcpy(command_history[(command_num++)%MAXLINE],cmdline);
        /* Evaluate the command line */
        eval(cmdline);
        fflush(stdout);
    }

    exit(0); /* control never reaches here */
}

/*
 * eval - Evaluate the command line that the user has just typed in
 */
void eval(char *cmdline)
{
    char *argv[MAXLINE];    /*argument list of execve()*/
    char buf[MAXLINE];      /*hold modified commend line*/
    int process_pos=0;
    pid_t pid;				/* Process id */
    int bg=0;                 /*should the job run in bg or fg?*/
    int i,argc;
    int fdn;
    char* file[2];

    stpcpy(buf,cmdline);
    argc = parseline(buf,argv);

    if(argv[0]==NULL){
        return;     /*ignore empty line*/
    }
	
	file[0]=file[1]=NULL;
    if(!builtin_cmd(argv))
	{                         
        if((pid = Fork())==0)						/* Child runs user job */ 
        {
            for(i=0;i<argc;i++){ 
            	if(*argv[i] == '&'){ 
            		bg=1;
            		argv[i]=NULL;
            	} 
            	else if(*argv[i] == '|'){
            		argv[i]=NULL;
            		int fd[2]; 
            		pipe(&fd[0]);
            		if((pid = Fork())==0){
            			file[1]="pipe";
            			close(fd[0]);
            			close(1);
            			dup2(fd[1],1);
            			close(fd[1]);
            			break;
					}
					else{
						process_pos=i+1;
						file[0]="pipe";
						bg=0;
						close(fd[1]);
						close(0);
						dup2(fd[0],0);
						close(fd[0]);
					}
				}
            	else if(*argv[i] == '<'){
            		file[0]=argv[i+1];
            		argv[i]=NULL;
            		fdn=open(file[0],O_RDONLY);
           			if(fdn<0){
                		printf("tsh: cannot open %s: no such file\n",file[0]);
                 		continue;
                 	}
            		close(0);
            		dup2(fdn,0);
				}
   	    		else if((*argv[i] == '>')&&(*(argv[i]+1) != '>')){
            		file[1]=argv[i+1];
            		argv[i]=NULL;
            		fdn=open(file[1],O_WRONLY|O_CREAT,0666);
            		close(1);
            		dup2(fdn,1);
				}
				else if((*argv[i] == '>')&&(*(argv[i]+1) == '>')){
            		file[1]=argv[i+1];
            		argv[i]=NULL;
            		fdn=open(file[1],O_WRONLY|O_APPEND|O_CREAT,0666);
            		close(1);
            		dup2(fdn,1);
				}
			}
			if(bg){
				if(!file[0]){
					file[0]="/dev/null";
					fdn=open(file[0],O_RDWR);
            		close(0);
            		dup2(fdn,0);
				}
				if(!file[1]){
					file[1]="/dev/null";
					fdn=open(file[1],O_RDWR);
            		close(1);
            		dup2(fdn,1);
				}
				Signal(SIGCHLD,SIG_IGN);
			} 
			if(pid!=0)
				Waitpid(pid,NULL,0);
        	if(execvp(argv[process_pos],(argv+process_pos))){	/* 运行该程序 */
            	printf("%s: Command not found\n",argv[process_pos]);
            	exit(0); 
            } 
        }
		else{ 
			Waitpid(pid,NULL,0);	/* 前台程序需等它结束 */
        } 
    }
    return;
}

/*
 * parseline - Parse the command line and build the argv array.
 */
int parseline(const char *cmdline, char **argv)
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg=0;                     /* background job? */
    int i;

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
        buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'')
    {
        buf++;
        delim = strchr(buf, '\'');
    }
    else
    {
        delim = strchr(buf, ' ');
    }

    while (delim)
    {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' ')) /* ignore spaces */
            buf++;

        if (*buf == '\'')
        {
            buf++;
            delim = strchr(buf, '\'');
        }
        else
        {
            delim = strchr(buf, ' ');
        }
    }
    argv[argc] = NULL;
    
    return argc;
}

/*
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.
 */
int builtin_cmd(char **argv)
{
    if(!strcmp(argv[0],"cd")){
        if(chdir(argv[1])<0)
        	printf("cd:can't cd to %s\n",argv[1]);
        return 1;
    }
    else if(!strcmp(argv[0],"history")){
        int i,n=strtol(argv[1],NULL,10);
        if(n>command_num-1)
        	n=command_num-1;
        for(i=command_num-n;i<command_num;i++)
        	printf(" %d %s",i,command_history[i%MAXLINE]);
        return 1;
    }
	else if(!strcmp(argv[0],"exit"))		/* 退出tsh */ 
        exit(0);
    else if(!strcmp(argv[0],"mytop")){
    	int pid;
    	if((pid = Fork())==0)
    		do_mytop();
    	else 
			Waitpid(pid,NULL,0);	
    	return 1;
	}    
    return 0;     /* not a builtin command */
}


void parse_file(pid_t pid)
{
	char path[PATH_MAX], type, state;
	int version, endpt;
	unsigned long cycles_hi, cycles_lo;
	FILE *fp;
	struct proc *p;
	int i;

	sprintf(path, "/proc/%d/psinfo", pid);

	if ((fp = fopen(path, "r")) == NULL)
		return;

	if (fscanf(fp, "%d", &version) != 1) {
		fclose(fp);
		return;
	}

	if (version != PSINFO_VERSION) {
		fputs("procfs version mismatch!\n", stderr);
		exit(1);
	}

	if (fscanf(fp, " %c %d", &type, &endpt) != 2) {
		fclose(fp);
		return;
	}

	p = &proc[++slot];

	if (type == TYPE_TASK)
		p->p_flags |= IS_TASK;
	else if (type == TYPE_SYSTEM)
		p->p_flags |= IS_SYSTEM;

	p->p_endpoint = endpt;
	p->p_pid = pid;

	if (fscanf(fp, " %*255s %c %*d %*d %*lu %*u %lu %lu",&state, &cycles_hi, &cycles_lo) != 3) {
		fclose(fp);
		return;
	}

	if (state != STATE_RUN)
		p->p_flags |= BLOCKED;
	p->p_cpucycles[0] = make64(cycles_lo, cycles_hi);

	if (!(p->p_flags & IS_TASK))
		fscanf(fp, " %*lu %*u %*u %*c %*d %*u %*u %*u %*d %*c %*d %*u");

	for(i = 1; i < CPUTIMENAMES; i++) {
		if(fscanf(fp, " %lu %lu", &cycles_hi, &cycles_lo) == 2) {
			p->p_cpucycles[i] = make64(cycles_lo, cycles_hi);
		} else	{
			p->p_cpucycles[i] = 0;
		}
	}

	p->p_flags |= USED;

	fclose(fp);
}
void parse_dir(void)
{
	DIR *p_dir;
	struct dirent *p_ent;
	pid_t pid;
	char *end;

	if ((p_dir = opendir("/proc")) == NULL) {
		perror("opendir on /proc");
		exit(1);
	}

	for (p_ent = readdir(p_dir); p_ent != NULL; p_ent = readdir(p_dir)) {
		pid = strtol(p_ent->d_name, &end, 10);

		if (!end[0] && pid != 0)
			parse_file(pid);
	}

	closedir(p_dir);
}

void get_procs(void)
{
	struct proc *p;
	int i;

	p = prev_proc;
	prev_proc = proc;
	proc = p;

	if (proc == NULL) {
		proc = malloc(nr_total * sizeof(proc[0]));

		if (proc == NULL) {
			fprintf(stderr, "Out of memory!\n");
			exit(1);
		}
	}

	for (i = 0; i < nr_total; i++)
		proc[i].p_flags = 0;
		
	parse_dir(); 
}
u64_t cputicks(struct proc *p1, struct proc *p2)
{
	int i;
	u64_t t = 0;
	for(i = 0; i < CPUTIMENAMES; i++) {
		if(!(1 & (1L << (i))))
			continue;
		if(p1->p_endpoint == p2->p_endpoint) {
			t = t + p2->p_cpucycles[i] - p1->p_cpucycles[i];
		} else {
			t = t + p2->p_cpucycles[i];
		}
	}

	return t;
}

void print_procs(struct proc *proc1, struct proc *proc2)
{
	int p;
	u64_t idleticks = 0;
	u64_t kernelticks = 0;
	u64_t systemticks = 0;
	u64_t userticks = 0;
	u64_t total_ticks = 0;

	for(p = 0; p < nr_total; p++) {
		u64_t uticks;
		if(!(proc2[p].p_flags & USED))
			continue;
		uticks = cputicks(&proc1[p], &proc2[p]);
		total_ticks = total_ticks + uticks;
		if(!(proc2[p].p_flags & IS_TASK)) {
			if(proc2[p].p_flags & IS_SYSTEM)
				systemticks = systemticks + uticks;
			else
				userticks = userticks + uticks;
		}
	}

	if (total_ticks == 0)
		return;

	printf("CPU states: %6.2f%% user, ", 100.0 * userticks / total_ticks);
	printf("%6.2f%% system \n", 100.0 * systemticks / total_ticks);
}
int print_memory(void)
{
	FILE *fp;
	unsigned int pagesize;
	unsigned long total, free, largest, cached;

	if ((fp = fopen("/proc/meminfo", "r")) == NULL)
		return 0;

	if (fscanf(fp, "%u %lu %lu %lu %lu", &pagesize, &total, &free,
			&largest, &cached) != 5) {
		fclose(fp);
		return 0;
	}

	fclose(fp);

	printf("main memory: %ldK total, %ldK free, %ldK contig free, "
		"%ldK cached\n",
		(pagesize * total)/1024, (pagesize * free)/1024,
		(pagesize * largest)/1024, (pagesize * cached)/1024);

	return 1;
}
void getkinfo(void)
{
	FILE *fp;

	if ((fp = fopen("/proc/kinfo", "r")) == NULL) {
		fprintf(stderr, "opening /proc/kinfo failed\n");
		exit(1);
	}

	if (fscanf(fp, "%u %u", &nr_procs, &nr_tasks) != 2) {
		fprintf(stderr, "reading from /proc/kinfo failed\n");
		exit(1);
	}

	fclose(fp);

	nr_total = (int) (nr_procs + nr_tasks);
}
/*
 * do_mytop - Execute the builtin mytop commands
 */
void do_mytop(){
	print_memory();
	getkinfo();
	slot=0;
	get_procs();
	if (prev_proc == NULL)
		get_procs();
	print_procs(prev_proc, proc);
	fflush(stdout);
	exit(0);
}

/********************************
 * Error-handling wrapper function
 ********************************/
 
/* $begin forkwrapper */
pid_t Fork(void) 
{
    pid_t pid;

    if ((pid = fork()) < 0)
	unix_error("Fork error");
    return pid;
}

pid_t Waitpid(pid_t pid, int *iptr, int options) 
{
    pid_t retpid;

    if ((retpid  = waitpid(pid, iptr, options)) < 0) 
	unix_error("Waitpid error");
    return(retpid);
}

 /************************************
 * End Error-handling wrapper function
 *************************************/

/***********************
 * Other helper routines
 ***********************/

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}
