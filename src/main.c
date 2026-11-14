#include "campus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
static void usage(const char *p){printf("CampusCompute: OS mini-project\nUsage: %s COMMAND [args]\n",p);puts(
"  help                           Show this screen\n"
"  schedule fcfs JOBS.csv         First Come First Served\n"
"  schedule rr JOBS.csv QUANTUM   Round Robin (quantum 1..1000000)\n"
"  schedule priority JOBS.csv     Non-preemptive priority\n"
"  compare JOBS.csv               Compare all policies (RR quantum=2)\n"
"  logger [COUNT]                 2 producers, 1 consumer, bounded queue (default 5)\n"
"  ipc                            POSIX shm_open/mmap + named semaphores (parent<->child)\n"
"  banker FILE.txt                Banker's safety algorithm\n"
"  worker normal|fail             fork/waitpid demonstrations\n"
"  limit                          RLIMIT_FSIZE child demonstration\n"
"  cleanup                        Verify no leftover children/threads/IPC objects\n"
"  all                            Integrated demo using data/ fixtures\n"
"Files: JOBS.csv rows = id,arrival,burst,priority; # comments accepted.\n"
"Banker files: N M, M available, N*M allocations, N*M maximums.\n"
"Tie-break: earlier arrival then lower ID; priority smaller number wins.\n"
"RR: arrivals at slice end enqueue before the preempted job.\n");}
static int positive(const char *s,int max,int *v){char *end;errno=0;long val=strtol(s,&end,10);if(errno||s==end||*end||val<1||val>max)return -1;*v=(int)val;return 0;}
static int compare(const char *path){Job jobs[MAX_JOBS];int n;if(load_jobs(path,jobs,&n))return 1;Metrics m[3];const char *names[]={"fcfs","rr","priority"};for(int i=0;i<3;i++)if(schedule(jobs,n,names[i],2,&m[i],0))return 1;
 printf("Same %d jobs; RR quantum=2\n%-12s %-12s %-12s %-12s %-8s\n",n,"Policy","Avg wait","Avg turn","Avg resp","Makespan");for(int i=0;i<3;i++)printf("%-12s %-12.2f %-12.2f %-12.2f %-8d\n",names[i],m[i].wait,m[i].turn,m[i].resp,m[i].finish);return 0;}
int run_all(const char *jobs,const char *safe,const char *unsafe){Job j[MAX_JOBS];int n,err=0;Metrics m;puts("=== CampusCompute integrated demonstration ===");
 if(load_jobs(jobs,j,&n))return 1;
 const char *p[]={"fcfs","rr","priority"};for(int i=0;i<3;i++){printf("\n=== Scheduler: %s ===\n",p[i]);err|=schedule(j,n,p[i],2,&m,1)!=0;}
 puts("\n=== Scheduling comparison ===");err|=compare(jobs);
 puts("\n=== Logger ===");err|=run_logger(5,"logs/events.log");
 puts("\n=== Shared-memory IPC ===");err|=run_ipc();
 puts("\n=== Banker SAFE ===");err|=run_banker(safe);
 puts("\n=== Banker UNSAFE ===");err|=run_banker(unsafe);
 puts("\n=== Normal worker ===");err|=run_worker(0);
 puts("\n=== Simulated failure ===");err|=run_worker(1);
 puts("\n=== Resource limit ===");err|=run_limit();
 puts("\n=== Cleanup ===");err|=run_clean();
 printf("\n=== Integrated result: %s ===\n",err?"CHECK ERRORS":"ALL DEMONSTRATIONS COMPLETED");return err;
}
int main(int argc,char **argv){setvbuf(stdout,NULL,_IONBF,0);if(argc<2){usage(argv[0]);return 2;}
 if(!strcmp(argv[1],"help")&&argc==2){usage(argv[0]);return 0;}
 if(!strcmp(argv[1],"schedule")&&(argc==4||argc==5)){
  const char *policy=argv[2];int rr=!strcmp(policy,"rr"),q=2;
  if((rr&&argc!=5)||(!rr&&argc!=4)){fprintf(stderr,"Wrong argument count for %s\n",policy);usage(argv[0]);return 2;}
  if(rr&&positive(argv[4],MAX_TIME,&q)){fprintf(stderr,"Invalid RR quantum: 1..%d required\n",MAX_TIME);return 2;}
  Job jobs[MAX_JOBS];int n;if(load_jobs(argv[3],jobs,&n))return 1;return schedule(jobs,n,policy,q,NULL,1)?1:0;
 }
 if(!strcmp(argv[1],"compare")&&argc==3)return compare(argv[2]);
 if(!strcmp(argv[1],"logger")&&(argc==2||argc==3)){int c=5;if(argc==3&&positive(argv[2],200,&c)){fprintf(stderr,"Logger COUNT must be 1..200\n");return 2;}return run_logger(c,"logs/events.log");}
 if(!strcmp(argv[1],"ipc")&&argc==2)return run_ipc();
 if(!strcmp(argv[1],"banker")&&argc==3)return run_banker(argv[2]);
 if(!strcmp(argv[1],"worker")&&argc==3){if(!strcmp(argv[2],"normal"))return run_worker(0);if(!strcmp(argv[2],"fail"))return run_worker(1);}
 if(!strcmp(argv[1],"limit")&&argc==2)return run_limit();
 if(!strcmp(argv[1],"cleanup")&&argc==2)return run_clean();
 if(!strcmp(argv[1],"all")&&argc==2)return run_all("data/jobs.csv","data/banker_safe.txt","data/banker_unsafe.txt");
 fprintf(stderr,"Invalid command or arguments.\n");usage(argv[0]);return 2;
}
