#define _GNU_SOURCE
/* Synthetic race window: exit only after this Guard has logged a child-pin
 * attempt. This reads the run's own evidence, never production data. */
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
static void pause_ms(long ms) {
    struct timespec delay={ms/1000,ms%1000*1000000};
    while(nanosleep(&delay,&delay)&&errno==EINTR) {}
}
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    pid_t children[64];
    for(int i=0;i<64;i++) {
        children[i]=fork();if(children[i]<0)return 3;
        if(!children[i]){pause_ms(30);_exit(0);}
    }
    for(int i=0;i<64;i++) {
        int status=0;pid_t got;
        do{got=waitpid(children[i],&status,0);}while(got<0&&errno==EINTR);
        if(got!=children[i]||status)return 4;
    }
    for(int attempt=0;attempt<500;attempt++) {
        char bytes[65536];int fd=open(argv[1],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
        if(fd<0)return 5;
        ssize_t length;do{length=read(fd,bytes,sizeof bytes-1);}while(length<0&&errno==EINTR);
        int closed=close(fd);if(length<0||closed)return 6;
        bytes[length]=0;
        if(strstr(bytes,"\"event\":\"CHILD_PIN_FAILED\"")) {
            puts("EXIT_WINDOW_CHILDREN_NORMAL=64/64");return 0;
        }
        pause_ms(1);
    }
    fputs("EXIT_WINDOW_NOT_OBSERVED\n",stderr);return 7;
}
