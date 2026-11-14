#ifndef CAMPUS_H
#define CAMPUS_H
#include <stddef.h>
#define MAX_JOBS 64
#define MAX_RES 10
#define MAX_PROCS 20
#define MAX_TIME 1000000

typedef struct {int id,arrival,burst,priority,remaining,start,completion;} Job;
typedef struct {double wait,turn,resp; int finish;} Metrics;
int load_jobs(const char *path, Job *jobs, int *count);
int schedule(const Job *input,int n,const char *policy,int quantum,Metrics *result,int verbose);
int run_logger(int count,const char *filename);
int run_ipc(void);
int run_banker(const char *path);
int run_worker(int fail);
int run_limit(void);
int run_clean(void);
int run_all(const char *jobs,const char *safe,const char *unsafe);
#endif
