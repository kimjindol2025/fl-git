#define RESOURCE_GUARD_SELFTEST
#define main resource_guard_existing_selftest
#include "../tools/native-resource-guard.c"
#undef main

/* Real /proc and pidfds: no syscall mocks, resource faults, DB or network. */
static void lifecycle_pause(long milliseconds) {
    struct timespec delay={milliseconds/1000,milliseconds%1000*1000000};
    while(nanosleep(&delay,&delay)&&errno==EINTR) {}
}
static int adopted_fixture(void) {
    pid_t child=fork();if(child<0)return 2;
    if(!child) {
        if(setsid()<0)_exit(3);
        pid_t grandchild=fork();if(grandchild<0)_exit(4);
        if(grandchild>0)_exit(0);
        printf("LIFECYCLE_ADOPTED=%ld\n",(long)getpid());fflush(stdout);
        lifecycle_pause(350);_exit(0);
    }
    int status=0;
    if(waitpid(child,&status,0)!=child||status!=0)return 5;
    return 0; /* The supervisor must discover and drain the living grandchild. */
}
int main(int argc,char **argv) {
    if(argc==2&&!strcmp(argv[1],"adopted"))return adopted_fixture();
    if(argc==2&&!strcmp(argv[1],"nonzero"))return 7;
    if(argc!=1)return 2;
    if(resource_guard_existing_selftest())return 1;
    int checks=0;
#define LCHECK(v) do {if(!(v)){fprintf(stderr,"lifecycle check %d failed\n",checks+1);return 1;}checks++;}while(0)
    Identity self={0};LCHECK(read_identity(getpid(),&self)==0);
    Owned parent={.identity=self,.pidfd=(int)syscall(SYS_pidfd_open,getpid(),0)};
    LCHECK(parent.pidfd>=0);
    const char *reason="NONE",*phase="NONE";int error=0,returned=0;
    LCHECK(parent_reason(&parent,&reason,&error,&returned)==PARENT_LIVE);
    Owned wrong=parent;wrong.identity.start++;
    LCHECK(parent_reason(&wrong,&reason,&error,&returned)==PARENT_REJECTED&&!strcmp(reason,"PARENT_IDENTITY_CHANGED"));
    wrong=parent;wrong.identity.pid=INT_MAX;
    LCHECK(parent_reason(&wrong,&reason,&error,&returned)==PARENT_REJECTED&&!strcmp(reason,"PARENT_STAT_UNREADABLE"));
    wrong=parent;wrong.pidfd=dup(parent.pidfd);LCHECK(wrong.pidfd>=0&&close(wrong.pidfd)==0);
    LCHECK(parent_reason(&wrong,&reason,&error,&returned)==PARENT_REJECTED);
    /* A live parent's unreadable retained input is never an exited parent. */
    LCHECK(confirm_unpinned_absence(&parent,-1,-1,&self,INT_MAX,&reason,&error,&returned,&phase)==PARENT_REJECTED);
    char path[96];snprintf(path,sizeof path,"/proc/%ld/task/%ld",(long)getpid(),(long)getpid());
    int taskfd=open(path,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);LCHECK(taskfd>=0);
    int childrenfd=openat(taskfd,"children",O_RDONLY|O_CLOEXEC|O_NOFOLLOW);LCHECK(childrenfd>=0);
    LCHECK(confirm_unpinned_absence(&parent,taskfd,childrenfd,&self,INT_MAX,&reason,&error,&returned,&phase)==PARENT_LIVE);
    LCHECK(confirm_unpinned_absence(&parent,taskfd,-1,&self,INT_MAX,&reason,&error,&returned,&phase)==PARENT_REJECTED);
    LCHECK(close(childrenfd)==0&&close(taskfd)==0);
    int gate[2];LCHECK(pipe2(gate,O_CLOEXEC)==0);
    pid_t child=fork();LCHECK(child>=0);
    if(!child){close(gate[1]);char token;ssize_t got=read(gate[0],&token,1);_exit(got==1?0:8);}
    close(gate[0]);Identity child_identity={0};LCHECK(read_identity(child,&child_identity)==0);
    Owned departed={.identity=child_identity,.pidfd=(int)syscall(SYS_pidfd_open,child,0)};
    LCHECK(departed.pidfd>=0);
    LCHECK(write(gate[1],"x",1)==1&&close(gate[1])==0);
    struct pollfd wait_for_exit={departed.pidfd,POLLIN,0};
    LCHECK(poll(&wait_for_exit,1,1000)==1);
    LCHECK(parent_reason(&departed,&reason,&error,&returned)==PARENT_EXITED);
    LCHECK(confirm_unpinned_absence(&departed,-1,-1,&child_identity,INT_MAX,&reason,&error,&returned,&phase)==PARENT_EXITED);
    /* Exit observation alone has not granted global success or final drain. */
    LCHECK(!final_drain_complete&&!root_done);
    int status=0;LCHECK(waitpid(child,&status,0)==child&&status==0);
    LCHECK(close(departed.pidfd)==0&&close(parent.pidfd)==0);
    char malformed[]="17 not-a-pid";LCHECK(child_list_contains(malformed,17)==-1);
    printf("GUARD_LIFECYCLE_SELFTEST=%d/%d\n",checks,checks);
    return 0;
}
