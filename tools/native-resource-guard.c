#define _GNU_SOURCE
/* Resource supervisor for an explicitly launched test job. Linux pidfds and a
 * private subreaper protect identity/ownership; this is not an independent
 * authority boundary or a physical-memory cgroup.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <openssl/evp.h>

#define MAX_OWNED 4096
#define MAX_BYTES UINT64_C(2147483648)
typedef struct {
    pid_t pid, ppid, pgrp, sid;
    uint64_t start;
    char state;
} Identity;
typedef struct {
    Identity identity;
    int pidfd, exited, term_sent;
    uint64_t rss;
} Owned;
static Owned owned[MAX_OWNED];
static size_t nowned;
static pid_t supervisor_pid, driver_pid;
static int events_fd=-1, evidence_fd=-1, io_failed, blocked, root_done, root_status;
static uint64_t sequence, peak_rss, max_gap_ns, last_sample_ns, short_unpinned_count;
static uint64_t parent_exit_rescans;
static int parent_exit_pending, final_drain_complete;
static const char *cause="NONE",*cleanup_reason="NONE";
static int clock_failed;
static uint64_t rss_limit=MAX_BYTES;
static volatile sig_atomic_t cancelled;
static void cancel_guard(int signal_number) {(void)signal_number;cancelled=1;}

static uint64_t mono_ns(void) {
    struct timespec t;
    if(clock_gettime(CLOCK_MONOTONIC,&t)) {clock_failed=1;if(!blocked)cause="CLOCK_FAILED";blocked=1;return 0;}
    return (uint64_t)t.tv_sec*UINT64_C(1000000000)+(uint64_t)t.tv_nsec;
}
static int write_all(int fd,const char *s,size_t length) {
    size_t used=0;
    while(used<length) {
        ssize_t n=write(fd,s+used,length-used);
        if(n<0&&errno==EINTR) continue;
        if(n<=0) return -1;
        used+=(size_t)n;
    }
    return 0;
}
static void event(const char *kind,const char *fields,...) {
    if(events_fd<0||io_failed) return;
    char tail[2048],line[2304];
    va_list ap;va_start(ap,fields);
    int a=vsnprintf(tail,sizeof tail,fields,ap);va_end(ap);
    if(a<0||(size_t)a>=sizeof tail) {io_failed=1;if(!blocked)cause="EVIDENCE_WRITE_FAILED";else cleanup_reason="EVIDENCE_WRITE_FAILED";blocked=1;return;}
    int n=snprintf(line,sizeof line,
        "{\"sequence\":%"PRIu64",\"monotonic_ns\":%"PRIu64",\"event\":\"%s\"%s}\n",
        ++sequence,mono_ns(),kind,tail);
    if(n<0||(size_t)n>=sizeof line||write_all(events_fd,line,(size_t)n)||fsync(events_fd))
        {io_failed=1;if(!blocked)cause="EVIDENCE_WRITE_FAILED";else cleanup_reason="EVIDENCE_WRITE_FAILED";blocked=1;}
}
static void deny(const char *why) {
    if(!blocked) {cause=why;blocked=1;event("BLOCKED",",\"cause\":\"%s\"",why);}
}
static int number(const char *s,uint64_t *value) {
    if(!s||!*s||*s=='-'||*s=='+') return 0;
    char *end=NULL;errno=0;
    unsigned long long n=strtoull(s,&end,10);
    if(errno||!end||*end) return 0;
    *value=(uint64_t)n;return 1;
}
static int parse_stat(char *text,Identity *out) {
    char *open=strchr(text,'('),*close=strrchr(text,')'),*end=NULL;
    if(!open||!close||close<open||close[1]!=' ') return 0;
    errno=0;long pid=strtol(text,&end,10);
    if(errno||pid<1||pid>INT_MAX||end!=open-1) return 0;
    char *items[32],*save=NULL;size_t count=0;
    for(char *p=strtok_r(close+2," \t\r\n",&save);p&&count<32;p=strtok_r(NULL," \t\r\n",&save))
        items[count++]=p;
    if(count<22||strlen(items[0])!=1) return 0;
    uint64_t ppid,pgrp,sid,start;
    if(!number(items[1],&ppid)||!number(items[2],&pgrp)||!number(items[3],&sid)||
       !number(items[19],&start)||ppid>INT_MAX||pgrp>INT_MAX||sid>INT_MAX||!start) return 0;
    *out=(Identity){(pid_t)pid,(pid_t)ppid,(pid_t)pgrp,(pid_t)sid,start,items[0][0]};
    return 1;
}
static int parse_statm(char *text,long page_size,uint64_t *rss) {
    char *save=NULL,*size=strtok_r(text," \t\r\n",&save),*resident=strtok_r(NULL," \t\r\n",&save);
    uint64_t a,b;
    if(page_size<=0||!number(size,&a)||!number(resident,&b)||b>UINT64_MAX/(uint64_t)page_size) return 0;
    *rss=b*(uint64_t)page_size;return 1;
}
static int read_proc(pid_t pid,const char *leaf,char *buf,size_t cap) {
    char path[96];int n=snprintf(path,sizeof path,"/proc/%ld/%s",(long)pid,leaf);
    if(n<0||(size_t)n>=sizeof path) {errno=EINVAL;return -1;}
    int fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
    if(fd<0) return -1;
    size_t used=0;int saved=0;
    while(used+1<cap) {
        ssize_t got=read(fd,buf+used,cap-1-used);
        if(got<0&&errno==EINTR) continue;
        if(got<0) {saved=errno;break;}
        if(!got) break;
        used+=(size_t)got;
    }
    if(!saved&&used+1==cap) saved=EOVERFLOW;
    if(close(fd)&&!saved) saved=errno;
    if(saved) {errno=saved;return -1;}
    buf[used]=0;return 0;
}
static int read_identity(pid_t pid,Identity *out) {
    char text[8192];
    if(read_proc(pid,"stat",text,sizeof text)) return -1;
    if(!parse_stat(text,out)||out->pid!=pid) {errno=EPROTO;return -1;}
    return 0;
}
static int pidfd_ready(int fd) {
    struct pollfd p={fd,POLLIN,0};int rc;
    do {rc=poll(&p,1,0);} while(rc<0&&errno==EINTR);
    if(rc<0||p.revents&(POLLNVAL|POLLERR)) return -1;
    return rc>0&&(p.revents&(POLLIN|POLLHUP))?1:0;
}
/* ENOENT/parse failure is never exit evidence by itself. */
static int classify_measurement(int ready,int readable,uint64_t expected,uint64_t actual) {
    if(ready<0) return -1;
    if(ready) return 0;
    if(!readable||expected!=actual) return -1;
    return 1;
}
static int find_owned(pid_t pid,uint64_t start) {
    for(size_t i=0;i<nowned;i++)
        if(owned[i].identity.pid==pid&&owned[i].identity.start==start&&!owned[i].exited&&pidfd_ready(owned[i].pidfd)==0) return (int)i;
    return -1;
}
static int known_live_parent(pid_t pid) {
    for(size_t i=0;i<nowned;i++)
        if(owned[i].identity.pid==pid&&!owned[i].exited&&pidfd_ready(owned[i].pidfd)==0) return 1;
    return 0;
}
static int acquire(const Identity *before,const char *reason,int pinned) {
    if(find_owned(before->pid,before->start)>=0) {if(pinned>=0)close(pinned);return 0;}
    if(nowned==MAX_OWNED) {if(pinned>=0)close(pinned);deny("OWNERSHIP_CAPACITY");return -1;}
    int fd=pinned>=0?pinned:(int)syscall(SYS_pidfd_open,before->pid,0);
    if(fd<0) {
        if(errno==ESRCH) return 0; /* vanished before ownership acquisition */
        deny("PIDFD_ACQUISITION_FAILED");return -1;
    }
    Identity after={0};int ok=read_identity(before->pid,&after)==0;
    int state=classify_measurement(pidfd_ready(fd),ok,before->start,after.start);
    if(state<=0) {
        close(fd);
        if(state<0) deny("IDENTITY_NOT_STABLE");
        return state<0?-1:0;
    }
    owned[nowned++]=(Owned){.identity=after,.pidfd=fd};
    event("OWNED",",\"pid\":%ld,\"ppid\":%ld,\"pgrp\":%ld,\"sid\":%ld,\"start_ticks\":%"PRIu64",\"reason\":\"%s\",\"pidfd\":true",
        (long)after.pid,(long)after.ppid,(long)after.pgrp,(long)after.sid,after.start,reason);
    return 0;
}
static void reap(void) {
    int status;pid_t pid;
    while((pid=waitpid(-1,&status,WNOHANG))>0) {
        event("REAP",",\"pid\":%ld,\"wait_status\":%d",(long)pid,status);
        if(pid==driver_pid) {
            root_done=1;root_status=status;
            if(!WIFEXITED(status)||WEXITSTATUS(status)!=0)deny("DRIVER_EXIT_NONZERO");
        }
    }
    if(pid<0&&errno!=ECHILD) deny("WAIT_STATUS_FAILED");
    if(pid<0&&errno==ECHILD&&driver_pid>0&&!root_done) deny("DRIVER_STATUS_LOST");
}
/* Retained proc task/children descriptors keep a second observation on the
 * same kernel task; PID/path recycling cannot redirect the reread. */
static int read_retained(int fd,char *buffer,size_t cap) {
    if(lseek(fd,0,SEEK_SET)<0)return -1;
    size_t used=0;int saved=0;
    while(used+1<cap) {
        ssize_t n=read(fd,buffer+used,cap-1-used);
        if(n<0&&errno==EINTR)continue;
        if(n<0){saved=errno;break;}
        if(!n)break;
        used+=(size_t)n;
    }
    if(!saved&&used+1==cap)saved=EOVERFLOW;
    if(saved){errno=saved;return -1;}
    buffer[used]=0;return 0;
}
static int task_identity(int taskfd,pid_t tid,Identity *identity) {
    int fd=openat(taskfd,"stat",O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
    if(fd<0)return -1;
    char bytes[8192];int status=read_retained(fd,bytes,sizeof bytes),saved=errno;
    if(close(fd)&&!status){status=-1;saved=errno;}
    if(!status&&(!parse_stat(bytes,identity)||identity->pid!=tid)){status=-1;saved=EPROTO;}
    errno=saved;return status;
}
static int same_live_parent(Owned *parent) {
    Identity current={0};
    return pidfd_ready(parent->pidfd)==0&&read_identity(parent->identity.pid,&current)==0&&
        current.start==parent->identity.start&&pidfd_ready(parent->pidfd)==0;
}
/* 0=absent,1=present,-1=malformed. No absence is inferred from I/O failure. */
static int child_list_contains(char *bytes,pid_t pid) {
    char *save=NULL;int found=0;
    for(char *token=strtok_r(bytes," \t\r\n",&save);token;token=strtok_r(NULL," \t\r\n",&save)) {
        uint64_t number_value;
        if(!number(token,&number_value)||!number_value||number_value>INT_MAX)return -1;
        if(number_value==(uint64_t)pid)found=1;
    }
    return found;
}
/* Preserve poll errors separately from a pinned parent's observed exit. */
static int diagnostic_pidfd_ready(int fd,int *system_error,int *returned_events) {
    struct pollfd item={fd,POLLIN,0};int result;
    *system_error=0;*returned_events=0;
    do{result=poll(&item,1,0);}while(result<0&&errno==EINTR);
    if(result<0){*system_error=errno;return -1;}
    *returned_events=item.revents;
    if(item.revents&(POLLNVAL|POLLERR))return -1;
    return result>0&&(item.revents&(POLLIN|POLLHUP))?1:0;
}
enum ParentObservation { PARENT_REJECTED=0, PARENT_LIVE=1, PARENT_EXITED=2 };
/* EXITED is a lifecycle transition, not proof that any child exited. */
static int parent_reason(Owned *parent,const char **reason,int *error,int *returned_events) {
    Identity current={0};int ready=diagnostic_pidfd_ready(parent->pidfd,error,returned_events);
    if(ready==1){*reason="PARENT_EXIT_OBSERVED";return PARENT_EXITED;}
    if(ready<0){*reason=*error?"PARENT_PIDFD_POLL_FAILED":"PARENT_PIDFD_REVENT_ERROR";return PARENT_REJECTED;}
    if(read_identity(parent->identity.pid,&current)) {
        int saved=errno;
        ready=diagnostic_pidfd_ready(parent->pidfd,error,returned_events);
        if(ready==1){*reason="PARENT_EXIT_DURING_CHECK";return PARENT_EXITED;}
        if(ready<0){*reason="PARENT_PIDFD_POLL_FAILED";return PARENT_REJECTED;}
        *reason="PARENT_STAT_UNREADABLE";*error=saved;return PARENT_REJECTED;
    }
    ready=diagnostic_pidfd_ready(parent->pidfd,error,returned_events);
    if(ready==1){*reason="PARENT_EXIT_DURING_CHECK";return PARENT_EXITED;}
    if(ready<0){*reason=*error?"PARENT_PIDFD_POLL_FAILED":"PARENT_PIDFD_REVENT_ERROR";return PARENT_REJECTED;}
    if(current.start!=parent->identity.start){*reason="PARENT_IDENTITY_CHANGED";return PARENT_REJECTED;}
    return PARENT_LIVE;
}
/* PARENT_EXITED defers this stale list to owned/adopted-descendant discovery
 * and the final ECHILD drain. It never means absence, exit, or zero RSS. */
static int confirm_unpinned_absence(Owned *parent,int taskfd,int childrenfd,const Identity *thread,pid_t child,const char **reason,int *error,int *returned_events,const char **phase) {
    Identity first={0},last={0};char children[65536];int state;
    *phase="before_child_list";
    state=parent_reason(parent,reason,error,returned_events);
    if(state!=PARENT_LIVE)return state;
    if(task_identity(taskfd,thread->pid,&first)){*reason="THREAD_STAT_UNREADABLE";*error=errno;goto recheck_parent;}
    if(first.start!=thread->start){*reason="THREAD_IDENTITY_CHANGED";return PARENT_REJECTED;}
    *phase="child_list";
    if(read_retained(childrenfd,children,sizeof children)){*reason="RETAINED_CHILD_LIST_READ_FAILED";*error=errno;goto recheck_parent;}
    int present=child_list_contains(children,child);
    if(present<0){*reason="CHILD_LIST_MALFORMED";return PARENT_REJECTED;}
    if(present){*reason="CHILD_STILL_LISTED";goto recheck_parent;}
    *phase="after_child_list";
    if(task_identity(taskfd,thread->pid,&last)){*reason="THREAD_STAT_UNREADABLE";*error=errno;goto recheck_parent;}
    if(last.start!=thread->start){*reason="THREAD_IDENTITY_CHANGED";return PARENT_REJECTED;}
    return parent_reason(parent,reason,error,returned_events);
recheck_parent: {
    const char *parent_failure="NONE";int parent_errno=0,parent_events=0;
    state=parent_reason(parent,&parent_failure,&parent_errno,&parent_events);
    if(state!=PARENT_LIVE){*reason=parent_failure;*error=parent_errno;*returned_events=parent_events;return state;}
    return PARENT_REJECTED;
    }
}
/* Only kernel child lists belonging to pinned, live owned tasks establish
 * ownership. Unrelated /proc entries are neither measured nor controlled. */
static int discover_children(Owned *parent) {
    if(parent->exited||pidfd_ready(parent->pidfd)==1)return 0;
    Identity before={0};
    if(read_identity(parent->identity.pid,&before)||before.start!=parent->identity.start) {
        if(pidfd_ready(parent->pidfd)==1)return 0;
        deny("OWNED_PARENT_UNREADABLE");return -1;
    }
    char path[96];snprintf(path,sizeof path,"/proc/%ld/task",(long)before.pid);
    DIR *tasks=opendir(path);
    if(!tasks) {
        if(pidfd_ready(parent->pidfd)==1)return 0;
        deny("OWNED_TASKS_UNREADABLE");return -1;
    }
    struct dirent *entry;int result=0;
    for(;;) {
        errno=0;entry=readdir(tasks);
        if(!entry){if(errno){deny("OWNED_TASKS_READ_FAILED");result=-1;}break;}
        uint64_t tid;if(!number(entry->d_name,&tid)||!tid||tid>INT_MAX)continue;
        int taskfd=openat(dirfd(tasks),entry->d_name,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
        Identity thread={0};
        if(taskfd<0||task_identity(taskfd,(pid_t)tid,&thread)) {
            if(taskfd>=0)close(taskfd);
            if(pidfd_ready(parent->pidfd)==1)break;
            deny("OWNED_THREAD_IDENTITY_UNREADABLE");result=-1;break;
        }
        int childrenfd=openat(taskfd,"children",O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
        char children[65536];
        if(childrenfd<0||read_retained(childrenfd,children,sizeof children)) {
            if(childrenfd>=0)close(childrenfd);
            close(taskfd);
            if(pidfd_ready(parent->pidfd)==1)break;
            deny("OWNED_CHILD_LIST_UNREADABLE");result=-1;break;
        }
        Identity stable_thread={0};
        if(!same_live_parent(parent)||task_identity(taskfd,(pid_t)tid,&stable_thread)||stable_thread.start!=thread.start) {
            close(childrenfd);close(taskfd);
            if(pidfd_ready(parent->pidfd)==1)break;
            deny("PARENT_OR_THREAD_IDENTITY_CHANGED");result=-1;break;
        }
        char *save=NULL;
        for(char *child=strtok_r(children," \t\r\n",&save);child;child=strtok_r(NULL," \t\r\n",&save)) {
            uint64_t value;
            if(!number(child,&value)||!value||value>INT_MAX){deny("CHILD_LIST_MALFORMED");result=-1;break;}
            pid_t pid=(pid_t)value;
            int handle=(int)syscall(SYS_pidfd_open,pid,0);
            if(handle<0) {
                int saved=errno;
                event("CHILD_PIN_FAILED",",\"pid\":%ld,\"errno\":%d,\"parent_pid\":%ld,\"parent_start_ticks\":%"PRIu64",\"tid\":%ld,\"thread_start_ticks\":%"PRIu64,
                    (long)pid,saved,(long)before.pid,before.start,(long)thread.pid,thread.start);
                const char *check_reason="ERRNO_NOT_ESRCH",*check_phase="not_checked";int check_errno=0,check_revents=0;
                int absence=saved==ESRCH?confirm_unpinned_absence(parent,taskfd,childrenfd,&thread,pid,&check_reason,&check_errno,&check_revents,&check_phase):PARENT_REJECTED;
                if(absence==PARENT_EXITED) {
                    parent_exit_pending=1;parent_exit_rescans++;
                    event("PARENT_EXIT_RESCAN_REQUIRED",",\"pid\":%ld,\"parent_pid\":%ld,\"parent_start_ticks\":%"PRIu64",\"pin_errno\":%d,\"reason\":\"%s\",\"check_revents\":%d,\"rss_bytes\":null,\"final_drain_required\":true",
                        (long)pid,(long)before.pid,before.start,saved,check_reason,check_revents);
                    break; /* Never consume the remainder of this stale list. */
                }
                if(absence==PARENT_LIVE) {
                    short_unpinned_count++;
                    event("NO_LONGER_PRESENT_BEFORE_PIN",",\"pid\":%ld,\"errno\":%d,\"parent_pid\":%ld,\"tid\":%ld,\"thread_start_ticks\":%"PRIu64",\"parent_live_identity_rechecked\":true,\"same_retained_children_fd\":true,\"rss_bytes\":null,\"peak\":\"UNVERIFIED\"",
                        (long)pid,saved,(long)before.pid,(long)thread.pid,thread.start);
                    continue;
                }
                deny("CHILD_IDENTITY_UNAVAILABLE");
                event("UNPINNED_ABSENCE_REJECTED",",\"pid\":%ld,\"pin_errno\":%d,\"phase\":\"%s\",\"reason\":\"%s\",\"check_errno\":%d,\"check_revents\":%d,\"parent_pid\":%ld,\"parent_start_ticks\":%"PRIu64",\"tid\":%ld,\"thread_start_ticks\":%"PRIu64,
                    (long)pid,saved,check_phase,check_reason,check_errno,check_revents,(long)before.pid,before.start,(long)thread.pid,thread.start);
                result=-1;continue;
            }
            Identity info={0};int readable=read_identity(pid,&info)==0;
            int ready=pidfd_ready(handle);
            if(ready==1){event("CHILD_EXITED_BEFORE_SAMPLE",",\"pid\":%ld",(long)pid);close(handle);continue;}
            if(ready<0||!readable){close(handle);deny("LIVE_CHILD_IDENTITY_UNREADABLE");result=-1;continue;}
            if(info.ppid!=before.pid&&info.ppid!=supervisor_pid&&!known_live_parent(info.ppid)) {
                close(handle);deny("CHILD_OWNERSHIP_CHANGED");result=-1;continue;
            }
            acquire(&info,info.ppid==supervisor_pid?"child_or_subreaped":"descendant",handle);
        }
        int children_close=close(childrenfd),task_close=close(taskfd);
        if(children_close||task_close){deny("PROC_INPUT_CLOSE_FAILED");result=-1;}
    }
    if(closedir(tasks)){deny("OWNED_TASKS_CLOSE_FAILED");result=-1;}
    return result;
}
static int sample(void) {
    uint64_t now=mono_ns();
    if(!now) {deny("CLOCK_FAILED");return -1;}
    if(last_sample_ns&&now-last_sample_ns>max_gap_ns) max_gap_ns=now-last_sample_ns;
    last_sample_ns=now;
    /* Newly acquired descendants are traversed in this same sample. */
    for(size_t i=0;i<nowned;i++) discover_children(&owned[i]);
    if(parent_exit_pending) {
        /* The supervisor is owned[0]. Re-read its kernel child lists for
         * newly adopted tasks, then traverse every still-pinned descendant. */
        int complete=1;
        for(size_t i=0;i<nowned;i++)if(discover_children(&owned[i]))complete=0;
        event("OWNED_DESCENDANTS_RECHECKED",",\"complete\":%s,\"owned_count\":%zu,\"final_drain_required\":true",complete?"true":"false",nowned);
    }
    uint64_t sum=0;size_t live=0;long page_size=sysconf(_SC_PAGESIZE);
    for(size_t i=0;i<nowned;i++) {
        Owned *p=&owned[i];
        if(p->exited) continue;
        Identity current={0};int readable=read_identity(p->identity.pid,&current)==0;
        int ready=pidfd_ready(p->pidfd);
        int state=classify_measurement(ready,readable,p->identity.start,current.start);
        if(!state) {p->exited=1;event("EXIT",",\"pid\":%ld,\"start_ticks\":%"PRIu64,(long)p->identity.pid,p->identity.start);continue;}
        if(state<0) {deny("MEASUREMENT_INCOMPLETE");continue;}
        char text[256];uint64_t rss=0;
        if(read_proc(p->identity.pid,"statm",text,sizeof text)||!parse_statm(text,page_size,&rss)) {
            if(pidfd_ready(p->pidfd)==1) {p->exited=1;event("EXIT",",\"pid\":%ld",(long)p->identity.pid);continue;}
            deny("LIVE_RSS_UNREADABLE");continue;
        }
        Identity again={0};readable=read_identity(p->identity.pid,&again)==0;
        state=classify_measurement(pidfd_ready(p->pidfd),readable,p->identity.start,again.start);
        if(!state) {p->exited=1;continue;}
        if(state<0||UINT64_MAX-sum<rss) {deny("MEASUREMENT_NOT_STABLE");continue;}
        p->identity=again;p->rss=rss;sum+=rss;live++;
        event("MEASURE",",\"pid\":%ld,\"ppid\":%ld,\"sid\":%ld,\"start_ticks\":%"PRIu64",\"rss_bytes\":%"PRIu64,
            (long)again.pid,(long)again.ppid,(long)again.sid,again.start,rss);
    }
    if(sum>peak_rss) peak_rss=sum;
    event("SAMPLE",",\"rss_bytes\":%"PRIu64",\"live_owned\":%zu",sum,live);
    if(peak_rss>rss_limit) deny("RSS_LIMIT_EXCEEDED");
    if(io_failed) deny("EVIDENCE_WRITE_FAILED");
    return live>0?0:-1;
}
static size_t live_children(void) {
    size_t count=0;
    for(size_t i=0;i<nowned;i++)
        if(owned[i].identity.pid!=supervisor_pid&&!owned[i].exited&&pidfd_ready(owned[i].pidfd)!=1) count++;
    /* Only ECHILD proves that direct/adopted children have all drained. */
    siginfo_t info={0};
    int result=waitid(P_ALL,0,&info,WEXITED|WNOHANG|WNOWAIT);
    if(!count&&!(result<0&&errno==ECHILD)) count=1;
    return count;
}
static void terminate_owned(void) {
    for(size_t i=0;i<nowned;i++) {
        Owned *p=&owned[i];
        if(p->identity.pid==supervisor_pid||p->exited||p->term_sent) continue;
        if(pidfd_ready(p->pidfd)!=0) continue;
        int rc=(int)syscall(SYS_pidfd_send_signal,p->pidfd,SIGTERM,NULL,0);
        if(!rc) p->term_sent=1;
        event("SIGTERM",",\"pid\":%ld,\"start_ticks\":%"PRIu64",\"result\":%d,\"errno\":%d",
            (long)p->identity.pid,p->identity.start,rc,rc?errno:0);
    }
}
static int evidence_digest(char hex[65]) {
    if(fsync(events_fd)||lseek(events_fd,0,SEEK_SET)<0) return -1;
    EVP_MD_CTX *ctx=EVP_MD_CTX_new();unsigned char hash[32],buf[16384];unsigned n=0;
    int ok=ctx&&EVP_DigestInit_ex(ctx,EVP_sha256(),NULL)==1;
    while(ok) {
        ssize_t got=read(events_fd,buf,sizeof buf);
        if(got<0&&errno==EINTR) continue;
        if(got<0) {ok=0;break;}
        if(!got) break;
        ok=EVP_DigestUpdate(ctx,buf,(size_t)got)==1;
    }
    ok=ok&&EVP_DigestFinal_ex(ctx,hash,&n)==1&&n==32;EVP_MD_CTX_free(ctx);
    if(!ok) return -1;
    for(size_t i=0;i<32;i++) snprintf(hex+i*2,3,"%02x",hash[i]);
    return 0;
}
#ifndef RESOURCE_GUARD_SELFTEST
int main(int argc,char **argv) {
    const char *directory=NULL;uint64_t limit=MAX_BYTES,grace_ms=10000,interval_ms=100,max_ms=600000;
    int driver_mode=0,index=1;
    for(;index<argc;index++) {
        if(!strcmp(argv[index],"--")) {index++;break;}
        if(!strcmp(argv[index],"--driver")) {driver_mode=1;continue;}
        if(index+1>=argc) return 2;
        const char *option=argv[index++],*value=argv[index];
        if(!strcmp(option,"--evidence")) directory=value;
        else if(!strcmp(option,"--rss-limit")) {if(!number(value,&limit)) return 2;}
        else if(!strcmp(option,"--grace-ms")) {if(!number(value,&grace_ms)) return 2;}
        else if(!strcmp(option,"--max-ms")) {if(!number(value,&max_ms)) return 2;}
        else if(!strcmp(option,"--interval-ms")) {if(!number(value,&interval_ms)) return 2;}
        else return 2;
    }
    if(!directory||index>=argc||!limit||limit>MAX_BYTES||grace_ms<100||grace_ms>30000||interval_ms<10||interval_ms>1000||max_ms<100||max_ms>7200000) return 2;
    rss_limit=limit;
    int executable=open(argv[index],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
    if(executable<0) return 2;
    unsigned char magic[4];struct stat binary;
    if(fstat(executable,&binary)||!S_ISREG(binary.st_mode)||pread(executable,magic,4,0)!=4||memcmp(magic,"\177ELF",4)) {close(executable);return 2;}
    if(mkdir(directory,0700)) {close(executable);return 2;}
    evidence_fd=open(directory,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
    if(evidence_fd<0) {close(executable);return 2;}
    events_fd=openat(evidence_fd,"events.jsonl",O_RDWR|O_APPEND|O_CREAT|O_EXCL|O_CLOEXEC,0600);
    if(events_fd<0) {close(executable);close(evidence_fd);return 2;}
    supervisor_pid=getpid();
    struct sigaction action={0};action.sa_handler=cancel_guard;sigemptyset(&action.sa_mask);
    struct sigaction ignored={0},normal_child={0};
    ignored.sa_handler=SIG_IGN;sigemptyset(&ignored.sa_mask);
    normal_child.sa_handler=SIG_DFL;sigemptyset(&normal_child.sa_mask);
    if(sigaction(SIGPIPE,&ignored,NULL)||sigaction(SIGCHLD,&normal_child,NULL)||
       sigaction(SIGTERM,&action,NULL)||sigaction(SIGINT,&action,NULL)) {deny("SIGNAL_SETUP_FAILED");goto finish;}
    if(prctl(PR_SET_CHILD_SUBREAPER,1,0,0,0)) {deny("SUBREAPER_UNAVAILABLE");goto finish;}
    Identity self;
    if(read_identity(supervisor_pid,&self)||acquire(&self,"supervisor",-1)) {deny("SUPERVISOR_IDENTITY_FAILED");goto finish;}
    event("START",",\"rss_limit_bytes\":%"PRIu64",\"native_as_bytes\":%"PRIu64",\"driver_mode\":%s,\"uid\":%lu,\"sample_interval_ms\":%"PRIu64,
        limit,driver_mode?UINT64_C(0):MAX_BYTES,driver_mode?"true":"false",(unsigned long)getuid(),interval_ms);
    sample();
    if(io_failed||clock_failed||cancelled||blocked) {deny("PRELAUNCH_INCOMPLETE");goto finish;}
    uint64_t launched_at=mono_ns();
    if(!launched_at) {deny("CLOCK_FAILED");goto finish;}
    int gate[2];
    if(pipe2(gate,O_CLOEXEC)) {deny("LAUNCH_GATE_FAILED");goto finish;}
    driver_pid=fork();
    if(driver_pid<0) {close(gate[0]);close(gate[1]);deny("FORK_FAILED");goto finish;}
    if(!driver_pid) {
        struct sigaction normal={0};normal.sa_handler=SIG_DFL;sigemptyset(&normal.sa_mask);
        sigaction(SIGTERM,&normal,NULL);sigaction(SIGINT,&normal,NULL);sigaction(SIGPIPE,&normal,NULL);
        if(prctl(PR_SET_PDEATHSIG,SIGTERM)||getppid()!=supervisor_pid) _exit(126);
        close(gate[1]);char allow=0;
        ssize_t got;do{got=read(gate[0],&allow,1);}while(got<0&&errno==EINTR);
        close(gate[0]);
        if(got!=1||allow!=1) _exit(126);
        if(setsid()<0) _exit(126);
        if(!driver_mode) {struct rlimit as={MAX_BYTES,MAX_BYTES};if(setrlimit(RLIMIT_AS,&as)) _exit(126);}
        close(events_fd);close(evidence_fd);
        extern char **environ;
        fexecve(executable,&argv[index],environ);_exit(127);
    }
    close(gate[0]);Identity child;
    if(read_identity(driver_pid,&child)||acquire(&child,"launched_driver",-1)) deny("DRIVER_IDENTITY_FAILED");
    if(cancelled) deny("SUPERVISOR_CANCELLED");
    if(!blocked&&!io_failed&&!clock_failed) {char allow=1;if(write_all(gate[1],&allow,1)) deny("LAUNCH_GATE_WRITE_FAILED");}
    close(gate[1]);close(executable);executable=-1;
    uint64_t stopping_since=0,driver_exit_at=0,stop_steps=0;
    for(;;) {
        sample();reap();
        uint64_t current_time=mono_ns();
        if(clock_failed||!current_time) deny("CLOCK_FAILED");
        if(current_time&&current_time-launched_at>max_ms*UINT64_C(1000000)) deny("RUN_DEADLINE_EXCEEDED");
        if(cancelled) deny("SUPERVISOR_CANCELLED");
        if(peak_rss>limit) deny("RSS_LIMIT_EXCEEDED");
        if(root_done&&!driver_exit_at) driver_exit_at=mono_ns();
        if(root_done&&!live_children()) break;
        if(root_done&&driver_exit_at&&mono_ns()-driver_exit_at>grace_ms*UINT64_C(1000000)) deny("DESCENDANT_DRAIN_TIMEOUT");
        if(blocked||io_failed) {
            if(!stopping_since) stopping_since=mono_ns();
            terminate_owned();
            stop_steps++;
            if((stopping_since&&mono_ns()-stopping_since>grace_ms*UINT64_C(1000000))||
               stop_steps>grace_ms/interval_ms+2) {cleanup_reason="TERMINATION_INCOMPLETE";break;}
        }
        struct timespec delay={(time_t)(interval_ms/1000),(long)(interval_ms%1000)*1000000L};
        while(nanosleep(&delay,&delay)&&errno==EINTR) {if(cancelled||clock_failed) break;}
    }
    reap();sample();reap();
finish:
    if(cancelled) deny("SUPERVISOR_CANCELLED");
    if(executable>=0) close(executable);
    if(clock_failed) deny("CLOCK_FAILED");
    if(peak_rss>rss_limit) deny("RSS_LIMIT_EXCEEDED");
    if(io_failed) deny("EVIDENCE_WRITE_FAILED");
    final_drain_complete=root_done&&live_children()==0;
    if(!final_drain_complete) {blocked=1;if(!strcmp(cause,"NONE")) cause="DRAIN_INCOMPLETE";}
    event("FINAL_DRAIN",",\"complete\":%s,\"driver_reaped\":%s,\"remaining_children\":%zu,\"parent_exit_rescans\":%"PRIu64,
        final_drain_complete?"true":"false",root_done?"true":"false",live_children(),parent_exit_rescans);
    if(final_drain_complete)parent_exit_pending=0;
    int exit_code=root_done&&WIFEXITED(root_status)?WEXITSTATUS(root_status):root_done&&WIFSIGNALED(root_status)?128+WTERMSIG(root_status):126;
    int ok=!blocked&&!io_failed&&root_done&&exit_code==0&&final_drain_complete&&!parent_exit_pending&&live_children()==0;
    event("FINAL_CANDIDATE",",\"status\":\"%s\",\"cause\":\"%s\",\"cleanup_reason\":\"%s\",\"driver_exit\":%d,\"peak_rss_bytes\":%"PRIu64",\"max_sample_gap_ns\":%"PRIu64",\"remaining_children\":%zu",
        ok?"AWAITING_EVIDENCE_SYNC":"BLOCKED",cause,cleanup_reason,exit_code,peak_rss,max_gap_ns,live_children());
    if(cancelled) deny("SUPERVISOR_CANCELLED");
    if(blocked||clock_failed||io_failed) ok=0;
    char hash[65]={0};int persisted=!io_failed&&evidence_digest(hash)==0;
    if(!persisted) {ok=0;if(!blocked)cause="EVIDENCE_FINALIZATION_FAILED";else cleanup_reason="EVIDENCE_FINALIZATION_FAILED";blocked=1;}
    if(cancelled||clock_failed||io_failed||blocked)ok=0;
    char result[1024];int length=snprintf(result,sizeof result,
        "{\"status\":\"%s\",\"cause\":\"%s\",\"cleanup_reason\":\"%s\",\"driver_exit\":%d,\"peak_rss_bytes\":%"PRIu64",\"remaining_children\":%zu,\"events_sha256\":\"%s\",\"short_unpinned_count\":%"PRIu64",\"parent_exit_rescans\":%"PRIu64",\"final_drain_complete\":%s,\"unmeasured_short_child_peak\":\"UNVERIFIED\",\"physical_hard_limit\":false,\"independent_authority\":false}\n",
        ok&&persisted?"PASS":"BLOCKED",cause,cleanup_reason,exit_code,peak_rss,live_children(),hash,short_unpinned_count,parent_exit_rescans,final_drain_complete?"true":"false");
    int out=openat(evidence_fd,"result.json",O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0600);
    if(out<0||length<0||(size_t)length>=sizeof result||write_all(out,result,(size_t)length)||fsync(out)) persisted=0;
    if(out>=0&&close(out)) persisted=0;
    if(fsync(evidence_fd)) persisted=0;
    for(size_t i=0;i<nowned;i++) close(owned[i].pidfd);
    if(events_fd>=0) close(events_fd);
    if(evidence_fd>=0) close(evidence_fd);
    if(cancelled||clock_failed||io_failed||blocked)ok=0;
    printf("NATIVE_RESOURCE_GUARD=%s\n",ok&&persisted?"PASS":"BLOCKED");
    return ok&&persisted?0:2;
}
#else
int main(void) {
    int count=0;Identity info;uint64_t rss=0;
#define CHECK(v) do {if(!(v)) {fprintf(stderr,"guard selftest %d failed\n",count+1);return 1;}count++;} while(0)
    char normal[]="42 (name with ) spaces) S 1 42 42 0 -1 0 0 0 0 0 0 0 0 0 20 0 1 0 12345 4096 1";
    CHECK(parse_stat(normal,&info)&&info.pid==42&&info.ppid==1&&info.start==12345);
    char invalid[]="42 (broken) S 1";CHECK(!parse_stat(invalid,&info));
    char pages[]="100 7 0 0 0";CHECK(parse_statm(pages,4096,&rss)&&rss==28672);
    char bad[]="100 bad 0";CHECK(!parse_statm(bad,4096,&rss));
    char overflow[]="1 18446744073709551615";CHECK(!parse_statm(overflow,4096,&rss));
    CHECK(classify_measurement(0,0,10,0)==-1);
    CHECK(classify_measurement(1,0,10,0)==0);
    CHECK(classify_measurement(0,1,10,11)==-1);
    CHECK(classify_measurement(1,1,10,11)==0);
    CHECK(classify_measurement(0,1,10,10)==1);
    CHECK(classify_measurement(-1,1,10,10)==-1);
    CHECK(!known_live_parent(123456));
    char file[]="/tmp/resource-guard-selftest.XXXXXX";
    int created=mkstemp(file);CHECK(created>=0);close(created);
    events_fd=open(file,O_RDONLY|O_CLOEXEC);unlink(file);CHECK(events_fd>=0);
    event("IO_FAILURE_FIXTURE","");CHECK(io_failed&&blocked&&!strcmp(cause,"EVIDENCE_WRITE_FAILED"));
    close(events_fd);events_fd=-1;io_failed=blocked=0;cause="NONE";
    printf("RESOURCE_GUARD_SELFTEST=%d/%d\n",count,count);return 0;
}
#endif
