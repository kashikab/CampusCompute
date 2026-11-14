#include "campus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
/* File format: n_processes n_resources; available; allocation matrix; maximum matrix. */
static int number(FILE *f,int *out){char token[80],*end;long v;if(fscanf(f,"%79s",token)!=1)return -1;v=strtol(token,&end,10);if(*end||v<0||v>1000000)return -1;*out=(int)v;return 0;}
int run_banker(const char *path){FILE *f=fopen(path,"r");if(!f){perror(path);return 1;}
 int n,m,available[MAX_RES],allocation[MAX_PROCS][MAX_RES],maximum[MAX_PROCS][MAX_RES],need[MAX_PROCS][MAX_RES];
 if(number(f,&n)||number(f,&m)||n<1||n>MAX_PROCS||m<1||m>MAX_RES){fprintf(stderr,"Invalid Banker's dimensions\n");fclose(f);return 1;}
 for(int r=0;r<m;r++)if(number(f,&available[r]))goto malformed;
 for(int i=0;i<n;i++)for(int r=0;r<m;r++)if(number(f,&allocation[i][r]))goto malformed;
 for(int i=0;i<n;i++)for(int r=0;r<m;r++){if(number(f,&maximum[i][r]))goto malformed;if(maximum[i][r]<allocation[i][r]){fprintf(stderr,"Maximum cannot be less than allocation (P%d R%d)\n",i,r);fclose(f);return 1;}need[i][r]=maximum[i][r]-allocation[i][r];}
 char extra[32];if(fscanf(f,"%31s",extra)==1){fprintf(stderr,"Unexpected extra data in %s\n",path);fclose(f);return 1;}
 fclose(f);printf("Banker input: %s | processes=%d resources=%d\n",path,n,m);
 printf("Available:");for(int r=0;r<m;r++)printf(" %d",available[r]);printf("\nProcess | Allocation | Maximum | Need\n");
 for(int i=0;i<n;i++){printf("P%-7d",i);for(int r=0;r<m;r++)printf(" %d",allocation[i][r]);printf(" | ");for(int r=0;r<m;r++)printf("%d ",maximum[i][r]);printf("| ");for(int r=0;r<m;r++)printf("%d ",need[i][r]);puts("");}
 long long work[MAX_RES];int finished[MAX_PROCS]={0},order[MAX_PROCS],done=0;for(int r=0;r<m;r++)work[r]=available[r];
 while(done<n){int found=-1;for(int i=0;i<n;i++)if(!finished[i]){int can=1;for(int r=0;r<m;r++)if(need[i][r]>work[r])can=0;if(can){found=i;break;}}if(found<0)break;
  printf("Step %d: P%d can finish; Work",done+1,found);for(int r=0;r<m;r++){work[r]+=allocation[found][r];printf(" %lld",work[r]);}puts("");
  finished[found]=1;order[done++]=found;
 }
 if(done==n){printf("Result: SAFE. Safe sequence:");for(int i=0;i<n;i++)printf(" P%d",order[i]);puts("");}
 else{printf("Result: UNSAFE. Only %d/%d processes can finish; blocked:",done,n);for(int i=0;i<n;i++)if(!finished[i])printf(" P%d",i);puts("");}
 return 0;
malformed:fprintf(stderr,"Malformed Banker's file: %s\n",path);fclose(f);return 1;
}
