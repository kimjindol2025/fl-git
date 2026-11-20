#define _GNU_SOURCE
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
static void pause_ms(long ms){struct timespec t={ms/1000,ms%1000*1000000};while(nanosleep(&t,&t)&&errno==EINTR){}}
int main(void){
  for(int batch=0;batch<16;batch++){
    pid_t children[16];
    for(int i=0;i<16;i++){
      children[i]=fork();if(children[i]<0)return 2;
      if(!children[i]){pause_ms(6+i%4);_exit(0);}
    }
    for(int i=0;i<16;i++){int status=0;pid_t got;do{got=waitpid(children[i],&status,0);}while(got<0&&errno==EINTR);if(got!=children[i]||status)return 3;}
    pause_ms(2);
  }
  puts("FAST_FORK_CHILDREN_NORMAL=256/256");return 0;
}
