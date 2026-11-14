#include "campus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

static int parse_int(const char *s,int *v){char *end;long x;if(!s||!*s)return -1;x=strtol(s,&end,10);while(isspace((unsigned char)*end))end++;if(*end||x<INT_MIN||x>INT_MAX)return -1;*v=(int)x;return 0;}
int load_jobs(const char *path,Job *jobs,int *count){
 FILE *f=fopen(path,"r");char line[512];int n=0,lineno=0;
 if(!f){perror(path);return -1;}
 while(fgets(line,sizeof(line),f)){
  lineno++;if(!strchr(line,'\n')&&!feof(f)){fprintf(stderr,"Job file line %d too long\n",lineno);fclose(f);return -1;}
  char *p=line;while(isspace((unsigned char)*p))p++;if(!*p||*p=='#')continue;
  if(n==MAX_JOBS){fprintf(stderr,"Too many jobs (max %d)\n",MAX_JOBS);fclose(f);return -1;}
  char *parts[5]={0};int m=0;char *tok=strtok(p,",\r\n");while(tok&&m<5){parts[m++]=tok;tok=strtok(NULL,",\r\n");}
  int id,a,b,pr;if(m!=4||tok||parse_int(parts[0],&id)||parse_int(parts[1],&a)||parse_int(parts[2],&b)||parse_int(parts[3],&pr)||id<=0||a<0||b<=0||pr<0||a>MAX_TIME||b>MAX_TIME){fprintf(stderr,"Invalid job at line %d (expected id,arrival>=0,burst>0,priority>=0)\n",lineno);fclose(f);return -1;}
  for(int i=0;i<n;i++)if(jobs[i].id==id){fprintf(stderr,"Duplicate job ID %d\n",id);fclose(f);return -1;}
  jobs[n++]=(Job){.id=id,.arrival=a,.burst=b,.priority=pr,.remaining=b,.start=-1,.completion=-1};
 }
 fclose(f);if(n==0){fprintf(stderr,"No jobs loaded\n");return -1;}
 *count=n;return 0;
}
static int choose(Job *j,int n,int time,int priority){int best=-1;for(int i=0;i<n;i++)if(j[i].remaining&&j[i].arrival<=time){if(best<0|| (priority&&j[i].priority<j[best].priority)|| (priority&&j[i].priority==j[best].priority&&j[i].arrival<j[best].arrival)|| (priority&&j[i].priority==j[best].priority&&j[i].arrival==j[best].arrival&&j[i].id<j[best].id)|| (!priority&&(j[i].arrival<j[best].arrival||(j[i].arrival==j[best].arrival&&j[i].id<j[best].id))))best=i;}return best;}
static int next_arrival(Job *j,int n,int t){int next=INT_MAX;for(int i=0;i<n;i++)if(j[i].remaining&&j[i].arrival>t&&j[i].arrival<next)next=j[i].arrival;return next;}
static void segment(int start,int end,int id){if(id)printf("  [%d -> %d] J%d\n",start,end,id);else printf("  [%d -> %d] IDLE\n",start,end);}
int schedule(const Job *input,int n,const char *policy,int quantum,Metrics *result,int verbose){
 if(n<1||n>MAX_JOBS||(strcmp(policy,"fcfs")&&strcmp(policy,"rr")&&strcmp(policy,"priority"))){fprintf(stderr,"Unknown policy\n");return -1;}
 if(!strcmp(policy,"rr")&&(quantum<1||quantum>MAX_TIME)){fprintf(stderr,"Quantum must be 1..%d\n",MAX_TIME);return -1;}
 Job j[MAX_JOBS];memcpy(j,input,sizeof(Job)*n);for(int i=0;i<n;i++){j[i].remaining=j[i].burst;j[i].start=-1;j[i].completion=-1;}
 int time=0,done=0;long long total_burst=0;for(int i=0;i<n;i++)total_burst+=j[i].burst;
 if(total_burst>(long long)MAX_JOBS*MAX_TIME){fprintf(stderr,"Burst total too large\n");return -1;}
 if(verbose){printf("Policy: %s",policy);if(!strcmp(policy,"rr"))printf(" | quantum=%d",quantum);printf("\nTie-break: earlier arrival, then lower job ID; priority: lower number is higher.\n");if(!strcmp(policy,"rr"))printf("RR rule: jobs arriving during/at the end of a slice are queued BEFORE the preempted job.\n");printf("Timeline:\n");}
 if(strcmp(policy,"rr")){
  while(done<n){int k=choose(j,n,time,!strcmp(policy,"priority"));if(k<0){int next=next_arrival(j,n,time);if(next==INT_MAX)return -1;if(verbose)segment(time,next,0);time=next;continue;}
   j[k].start=time;int end=time+j[k].burst;if(verbose)segment(time,end,j[k].id);time=end;j[k].remaining=0;j[k].completion=time;done++;
  }
 }else{
  /* At slice end, enqueue arrivals up to the end before re-queuing the running job. */
  int q[MAX_JOBS+1],head=0,len=0;
  int admitted[MAX_JOBS]={0};
  while(done<n){
   /* Deterministic arrival ordering, including arrivals during previous quantum. */
   while(1){int k=-1;for(int i=0;i<n;i++)if(!admitted[i]&&j[i].arrival<=time&&(k<0||j[i].arrival<j[k].arrival||(j[i].arrival==j[k].arrival&&j[i].id<j[k].id)))k=i;
    if(k<0)break;
    admitted[k]=1;q[(head+len)%(MAX_JOBS+1)]=k;len++;
   }
   if(!len){int next=next_arrival(j,n,time);if(next==INT_MAX)return -1;if(verbose)segment(time,next,0);time=next;continue;}
   int k=q[head];head=(head+1)%(MAX_JOBS+1);len--;if(j[k].start<0)j[k].start=time;
   int slice=j[k].remaining<quantum?j[k].remaining:quantum;int old=time;time+=slice;j[k].remaining-=slice;if(verbose)segment(old,time,j[k].id);
   /* Any new arrivals during the slice join before the preempted job. */
   while(1){int a=-1;for(int i=0;i<n;i++)if(!admitted[i]&&j[i].arrival<=time&&(a<0||j[i].arrival<j[a].arrival||(j[i].arrival==j[a].arrival&&j[i].id<j[a].id)))a=i;
    if(a<0)break;
    admitted[a]=1;q[(head+len)%(MAX_JOBS+1)]=a;len++;
   }
   if(j[k].remaining){q[(head+len)%(MAX_JOBS+1)]=k;len++;}else{j[k].completion=time;done++;}
  }
 }
 double tw=0,tt=0,tr=0;
 if(verbose)printf("\n%-6s %-7s %-6s %-8s %-7s %-11s %-11s %-8s\n","Job","Arrival","Burst","Priority","Finish","Turnaround","Waiting","Response");
 /* Metrics are printed in input order. */
 for(int i=0;i<n;i++){int turn=j[i].completion-j[i].arrival;int wait=turn-j[i].burst;int resp=j[i].start-j[i].arrival;tw+=wait;tt+=turn;tr+=resp;if(verbose)printf("J%-5d %-7d %-6d %-8d %-7d %-11d %-11d %-8d\n",j[i].id,j[i].arrival,j[i].burst,j[i].priority,j[i].completion,turn,wait,resp);}
 if(verbose)printf("Average: waiting=%.2f turnaround=%.2f response=%.2f | makespan=%d\n",tw/n,tt/n,tr/n,time);
 if(result)*result=(Metrics){.wait=tw/n,.turn=tt/n,.resp=tr/n,.finish=time};
 return 0;
}
