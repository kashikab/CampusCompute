#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "campus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>
#include <limits.h>
#include <dirent.h>

/* ===================== FR3: bounded producer-consumer logger ===================== */
#define CAPACITY 4
#define MAX_EVENTS 200
typedef struct {int queue[CAPACITY],in,out,size,closed;pthread_mutex_t mu;pthread_cond_t has_space,has_data;FILE *file;int consumed;} LogQueue;
typedef struct {LogQueue *q;int id,count;} Producer;

/* Producer: blocks on has_space while the queue is full (condition variable, not busy-wait). */
static void *produce(void *arg){Producer *p=arg;LogQueue *q=p->q;for(int i=1;i<=p->count;i++){
 pthread_mutex_lock(&q->mu);while(q->size==CAPACITY)pthread_cond_wait(&q->has_space,&q->mu);
 int event=p->id*1000+i;q->queue[q->in]=event;q->in=(q->in+1)%CAPACITY;q->size++;
 printf("producer %d -> event %d (queue=%d/%d)\n",p->id,event,q->size,CAPACITY);
 pthread_cond_signal(&q->has_data);pthread_mutex_unlock(&q->mu);
 }printf("producer %d finished (%d events produced)\n",p->id,p->count);return NULL;}

/* Consumer: blocks on has_data while empty; exits only when queue is empty AND closed. */
static void *consume(void *arg){LogQueue *q=arg;for(;;){pthread_mutex_lock(&q->mu);
 while(q->size==0&&!q->closed)pthread_cond_wait(&q->has_data,&q->mu);
 if(q->size==0&&q->closed){pthread_mutex_unlock(&q->mu);break;}
 int event=q->queue[q->out];q->out=(q->out+1)%CAPACITY;q->size--;q->consumed++;
 /* One readable, timestamped log line per event. */
 time_t now=time(NULL);struct tm tmv;char ts[32];localtime_r(&now,&tmv);strftime(ts,sizeof(ts),"%Y-%m-%d %H:%M:%S",&tmv);
 fprintf(q->file,"[%s] seq=%d producer=%d event=%d status=LOGGED\n",ts,q->consumed,event/1000,event);
 printf("consumer <- event %d (queue=%d/%d)\n",event,q->size,CAPACITY);
 pthread_cond_signal(&q->has_space);pthread_mutex_unlock(&q->mu);
 }printf("consumer exited cleanly\n");return NULL;}

int run_logger(int count,const char *filename){
 if(count<1||count>MAX_EVENTS){fprintf(stderr,"Events per producer must be 1..%d\n",MAX_EVENTS);return 1;}
 if(mkdir("logs",0755)&&errno!=EEXIST){perror("mkdir logs");return 1;}   /* only our own logs/ dir */
 FILE *file=fopen(filename,"w");if(!file){perror("log file");return 1;}
 LogQueue q={.file=file};int rc=0;pthread_t producers[2],consumer;int started=0,have_consumer=0;
 if(pthread_mutex_init(&q.mu,NULL)!=0){fclose(file);return 1;}
 pthread_cond_init(&q.has_space,NULL);pthread_cond_init(&q.has_data,NULL);
 if(pthread_create(&consumer,NULL,consume,&q)==0)have_consumer=1;else{fprintf(stderr,"Could not create consumer\n");rc=1;goto done;}
 Producer args[2]={{&q,1,count},{&q,2,count}};
 for(int i=0;i<2;i++){if(pthread_create(&producers[i],NULL,produce,&args[i])){fprintf(stderr,"Could not create producer %d\n",i+1);rc=1;break;}started++;}
 for(int i=0;i<started;i++){pthread_join(producers[i],NULL);printf("Joined producer %d\n",i+1);}
 pthread_mutex_lock(&q.mu);q.closed=1;pthread_cond_broadcast(&q.has_data);pthread_mutex_unlock(&q.mu);
 pthread_join(consumer,NULL);have_consumer=0;printf("Joined consumer\n");
 printf("Threads joined; produced=%d consumed=%d; bounded capacity=%d\n",started*count,q.consumed,CAPACITY);
 if(q.consumed!=started*count)rc=1;
 done:if(have_consumer){pthread_mutex_lock(&q.mu);q.closed=1;pthread_cond_broadcast(&q.has_data);pthread_mutex_unlock(&q.mu);pthread_join(consumer,NULL);}
 pthread_cond_destroy(&q.has_space);pthread_cond_destroy(&q.has_data);pthread_mutex_destroy(&q.mu);fclose(file);
 printf("Log saved to %s\n",filename);return rc;
}

/* ===================== FR4: POSIX shared memory + NAMED semaphores ===================== */
typedef struct {int stage;char message[128];char reply[128];} Shared;

/* sem_timedwait so a crashed peer can never hang the demo forever. */
static int wait_sem(sem_t *s,int seconds){struct timespec ts;clock_gettime(CLOCK_REALTIME,&ts);ts.tv_sec+=seconds;
 while(sem_timedwait(s,&ts)<0){if(errno==EINTR)continue;return -1;}return 0;}
static int exists(const char *path){struct stat st;return stat(path,&st)==0;}

int run_ipc(void){
 char shm_name[64],data_name[64],ack_name[64],shm_path[96],data_path[96],ack_path[96];
 long me=(long)getpid();   /* PID in the name => unique, and only OUR objects are ever unlinked */
 snprintf(shm_name,sizeof(shm_name),"/campuscompute_shm_%ld",me);
 snprintf(data_name,sizeof(data_name),"/campuscompute_data_%ld",me);
 snprintf(ack_name,sizeof(ack_name),"/campuscompute_ack_%ld",me);
 snprintf(shm_path,sizeof(shm_path),"/dev/shm%s",shm_name);
 snprintf(data_path,sizeof(data_path),"/dev/shm/sem.%s",data_name+1);
 snprintf(ack_path,sizeof(ack_path),"/dev/shm/sem.%s",ack_name+1);
 int rc=1,fd=-1,shm_made=0;Shared *s=MAP_FAILED;sem_t *data=SEM_FAILED,*ack=SEM_FAILED;int data_made=0,ack_made=0;pid_t pid=-1;

 fd=shm_open(shm_name,O_CREAT|O_EXCL|O_RDWR,0600);if(fd<0){perror("shm_open");goto cleanup;}shm_made=1;
 if(ftruncate(fd,sizeof(Shared))){perror("ftruncate");goto cleanup;}
 s=mmap(NULL,sizeof(Shared),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);if(s==MAP_FAILED){perror("mmap");goto cleanup;}
 memset(s,0,sizeof(*s));
 data=sem_open(data_name,O_CREAT|O_EXCL,0600,0);if(data==SEM_FAILED){perror("sem_open data");goto cleanup;}data_made=1;
 ack =sem_open(ack_name ,O_CREAT|O_EXCL,0600,0);if(ack ==SEM_FAILED){perror("sem_open ack");goto cleanup;}ack_made=1;
 printf("Created shared memory %s (exists=%s)\n",shm_path,exists(shm_path)?"yes":"no");
 printf("Created named semaphores %s and %s (exist=%s)\n",data_path,ack_path,exists(data_path)&&exists(ack_path)?"yes":"no");

 pid=fork();if(pid<0){perror("fork");goto cleanup;}
 if(pid==0){ /* ---- child: attaches to the objects BY NAME, like an unrelated cooperating process ---- */
  int cfd=shm_open(shm_name,O_RDWR,0);if(cfd<0)_exit(2);
  Shared *cs=mmap(NULL,sizeof(Shared),PROT_READ|PROT_WRITE,MAP_SHARED,cfd,0);if(cs==MAP_FAILED)_exit(3);
  sem_t *cd=sem_open(data_name,0),*ca=sem_open(ack_name,0);if(cd==SEM_FAILED||ca==SEM_FAILED)_exit(4);
  snprintf(cs->message,sizeof(cs->message),"Hello from worker PID %ld",(long)getpid());cs->stage=1;
  printf("Child PID %ld: wrote request into shared memory, posting semaphore\n",(long)getpid());fflush(stdout);
  sem_post(cd);
  if(wait_sem(ca,5))_exit(5);
  printf("Child: read parent's reply '%s' (stage=%d)\n",cs->reply,cs->stage);fflush(stdout);
  int ok=(cs->stage==2);   /* read BEFORE unmapping */
  sem_close(cd);sem_close(ca);munmap(cs,sizeof(Shared));close(cfd);_exit(ok?0:6);
 }
 /* ---- parent ---- */
 if(wait_sem(data,5)){fprintf(stderr,"Parent: timed out waiting for child\n");kill(pid,SIGKILL);}
 else{printf("Parent: semaphore signaled; stage=%d; message='%s'\n",s->stage,s->message);
  snprintf(s->reply,sizeof(s->reply),"Parent PID %ld received your message",(long)getpid());s->stage=2;sem_post(ack);}
 int status=0;if(waitpid(pid,&status,0)<0)perror("waitpid");
 else printf("Parent: child exit code=%d\n",WIFEXITED(status)?WEXITSTATUS(status):-1);
 rc=(WIFEXITED(status)&&WEXITSTATUS(status)==0&&s->stage==2)?0:1;

cleanup: /* runs on success AND on every error path; releases only what THIS run created */
 if(s!=MAP_FAILED)munmap(s,sizeof(Shared));
 if(fd>=0)close(fd);
 if(data!=SEM_FAILED)sem_close(data);
 if(ack!=SEM_FAILED)sem_close(ack);
 if(data_made&&sem_unlink(data_name))perror("sem_unlink data");
 if(ack_made&&sem_unlink(ack_name))perror("sem_unlink ack");
 if(shm_made&&shm_unlink(shm_name))perror("shm_unlink");
 if(shm_made){printf("IPC cleanup: shm_unlink + sem_unlink done. Verify: shm gone=%s, semaphores gone=%s\n",
   exists(shm_path)?"NO":"yes",(exists(data_path)||exists(ack_path))?"NO":"yes");
  if(exists(shm_path)||exists(data_path)||exists(ack_path))rc=1;}
 return rc;
}

/* ===================== FR6 / FR7: workers and resource limit (unchanged) ===================== */
int run_worker(int fail){pid_t pid=fork();if(pid<0){perror("fork");return 1;}
 if(pid==0){printf("Worker child PID %ld: %s\n",(long)getpid(),fail?"controlled SIMULATED failure":"normal task");fflush(stdout);_exit(fail?42:0);}
 printf("Parent PID %ld created worker PID %ld\n",(long)getpid(),(long)pid);
 int status;pid_t w;do{w=waitpid(pid,&status,0);}while(w<0&&errno==EINTR);
 if(w<0){perror("waitpid");return 1;}
 if(WIFEXITED(status)){int code=WEXITSTATUS(status);printf("waitpid: worker exited with status %d (%s)\n",code,code==0?"normal completion":"controlled SIMULATED failure");return code==(fail?42:0)?0:1;}
 if(WIFSIGNALED(status))printf("waitpid: worker killed by signal %d\n",WTERMSIG(status));
 return 1;
}
typedef struct {unsigned long long soft,hard;ssize_t first,second;int first_errno,second_errno;int unlinked;} LimitResult;
int run_limit(void){
 /* A pipe carries evidence back because stdout might be redirected to a regular
    file, which would itself be subject to the child's RLIMIT_FSIZE. */
 int pipefd[2];if(pipe(pipefd)){perror("pipe");return 1;}
 pid_t pid=fork();if(pid<0){perror("fork");close(pipefd[0]);close(pipefd[1]);return 1;}
 if(pid==0){close(pipefd[0]);LimitResult res={0};
  struct rlimit lim={.rlim_cur=64,.rlim_max=64};if(setrlimit(RLIMIT_FSIZE,&lim))_exit(2);
  struct rlimit applied;if(getrlimit(RLIMIT_FSIZE,&applied))_exit(3);
  res.soft=applied.rlim_cur;res.hard=applied.rlim_max;
  signal(SIGXFSZ,SIG_IGN);
  char path[128];snprintf(path,sizeof(path),"/tmp/campuscompute_limit_%ld.tmp",(long)getpid());
  int fd=open(path,O_CREAT|O_EXCL|O_WRONLY,0600);if(fd<0)_exit(4);
  char buf[128];memset(buf,'X',sizeof(buf));
  errno=0;res.first=write(fd,buf,sizeof(buf));res.first_errno=errno;
  errno=0;res.second=write(fd,buf,sizeof(buf));res.second_errno=errno;
  close(fd);res.unlinked=unlink(path)==0;
  ssize_t written=write(pipefd[1],&res,sizeof(res));close(pipefd[1]);
  _exit(written==(ssize_t)sizeof(res)&&res.first==64&&res.second==-1&&res.second_errno==EFBIG&&res.unlinked?0:5);
 }
 close(pipefd[1]);LimitResult res;ssize_t got=read(pipefd[0],&res,sizeof(res));close(pipefd[0]);
 int st;if(waitpid(pid,&st,0)<0){perror("waitpid");return 1;}
 if(got==(ssize_t)sizeof(res)){
  printf("Child RLIMIT_FSIZE soft=%llu hard=%llu bytes\n",res.soft,res.hard);
  printf("Write attempt 128 bytes: returned %zd\n",res.first);
  printf("Second write: returned %zd errno=%d (%s)\n",res.second,res.second_errno,strerror(res.second_errno));
  printf("Child temporary file unlinked: %s\n",res.unlinked?"yes":"no");
 }else fprintf(stderr,"Child failed before limit results were sent\n");
 printf("Parent unaffected; limit test child exit=%d\n",WIFEXITED(st)?WEXITSTATUS(st):-1);
 return got==(ssize_t)sizeof(res)&&WIFEXITED(st)&&WEXITSTATUS(st)==0?0:1;
}

/* ===================== FR7: real cleanup verification ===================== */
/* Scan DIR for entries starting with PREFIX (our program's own naming scheme, ending in _<pid>).
   Live owner => report only. Dead owner (stale leftover of a crashed run) => remove. Nothing else is touched. */
static int scan_leftovers(const char *dir,const char *prefix,int *removed){
 DIR *d=opendir(dir);if(!d)return 0;struct dirent *e;int found=0;size_t pl=strlen(prefix);
 while((e=readdir(d))){
  if(strncmp(e->d_name,prefix,pl))continue;
  const char *us=strrchr(e->d_name,'_');long pid=us?strtol(us+1,NULL,10):0;
  char path[512];snprintf(path,sizeof(path),"%s/%s",dir,e->d_name);
  if(pid>0&&kill((pid_t)pid,0)==0){printf("  IN USE by running PID %ld (left alone): %s\n",pid,path);found++;}
  else if(unlink(path)==0){printf("  stale leftover removed: %s\n",path);(*removed)++;}
  else{printf("  could not remove %s: %s\n",path,strerror(errno));found++;}
 }
 closedir(d);return found;
}
int run_clean(void){
 int bad=0,removed=0;
 printf("Cleanup verification (only objects named campuscompute_* are examined):\n");
 /* 1. child processes: any un-reaped child of this process? */
 errno=0;pid_t w=waitpid(-1,NULL,WNOHANG);
 if(w<0&&errno==ECHILD)printf("  [ok] no unreaped child processes of PID %ld\n",(long)getpid());
 else{printf("  [!!] a child process is still present (pid=%ld)\n",(long)w);bad=1;}
 /* 2. threads: read this process's thread count from /proc */
 FILE *f=fopen("/proc/self/status","r");char line[256];int threads=-1;
 if(f){while(fgets(line,sizeof(line),f))if(!strncmp(line,"Threads:",8)){threads=atoi(line+8);break;}fclose(f);}
 if(threads==1)printf("  [ok] this process has exactly 1 thread (all logger threads were joined)\n");
 else{printf("  [!!] thread count = %d\n",threads);bad=1;}
 /* 3. POSIX shared memory + named semaphores live in /dev/shm; limit temp file lives in /tmp */
 int left=scan_leftovers("/dev/shm","campuscompute_",&removed)+scan_leftovers("/dev/shm","sem.campuscompute_",&removed)+scan_leftovers("/tmp","campuscompute_limit_",&removed);
 if(left==0)printf("  [ok] no campuscompute shared memory, semaphores or temp files remain\n");else bad=1;
 printf("Log file logs/events.log is intentionally kept as evidence (removed by 'make clean').\n");
 printf("Result: %s\n",bad?"CLEANUP PROBLEMS FOUND":"CLEAN");return bad;
}
