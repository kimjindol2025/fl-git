#define _GNU_SOURCE
/* Synthetic descendants only; no files, DB, network, or external services. */
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
static void pause_ms(long ms) {
    struct timespec t={ms/1000,(ms%1000)*1000000};
    while(nanosleep(&t,&t)&&errno==EINTR) {}
}
int main(int argc,char **argv) {
    if(argc!=2) return 2;
    if(!strcmp(argv[1],"sentinel")) {
        char quit;return read(0,&quit,1)==1&&quit=='q'?0:2;
    }
    if(!strcmp(argv[1],"normal")) {
        int gate[2];if(pipe(gate)) return 2;
        pid_t child=fork();if(child<0) return 2;
        if(!child) {
            close(gate[0]);if(setsid()<0) _exit(3);
            pid_t grandchild=fork();if(grandchild<0) _exit(4);
            if(grandchild>0) _exit(0);
            pid_t self=getpid();
            if(write(gate[1],&self,sizeof self)!=sizeof self) _exit(5);
            close(gate[1]);pause_ms(700);_exit(0);
        }
        close(gate[1]);pid_t grandchild=0;
        if(read(gate[0],&grandchild,sizeof grandchild)!=sizeof grandchild) return 6;
        close(gate[0]);int status;
        if(waitpid(child,&status,0)!=child||status!=0) return 7;
        printf("DETACHED_GRANDCHILD=%ld\n",(long)grandchild);fflush(stdout);
        pause_ms(150);return 0;
    }
    if(!strcmp(argv[1],"budget")) {
        size_t n=64u*1024u*1024u;
        volatile unsigned char *memory=malloc(n);if(!memory) return 2;
        for(size_t i=0;i<n;i+=4096) memory[i]=1;
        puts("OWNED_BUDGET_FIXTURE_READY");fflush(stdout);
        pause_ms(2000);free((void*)memory);return 0;
    }
    if(!strcmp(argv[1],"success")) return 0;
    return 2;
}
