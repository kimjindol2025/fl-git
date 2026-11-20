/* Literal file IO for Native fix policy. No source parsing or rule decisions.
 * Linux renameat2 keeps each publication/restoration atomic. A private durable
 * manifest and file identities allow interrupted work to be recovered without
 * replacing a later writer's content. The group is not a multi-file atomic IO.
 */
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <openssl/evp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <unistd.h>

#ifndef RENAME_NOREPLACE
#define RENAME_NOREPLACE 1
#define RENAME_EXCHANGE 2
#endif

typedef struct { char *path, *hash; } Input;
typedef struct {
    char *src, *dst, *stage, *before, *after, *fresh, *retired;
    uint64_t old_dev, old_ino, new_dev, new_ino;
    mode_t mode; uid_t uid; gid_t gid;
    int parent, old_fd;
} Write;
typedef struct {
    char *project, *web, *run, *nonce;
    uint64_t root_dev, root_ino;
    Input *inputs; size_t input_n;
    char **initial; size_t initial_n;
    Write *writes; size_t write_n;
    int root, run_fd, journal;
    size_t published, restored, conflicts;
    int recovering, owns_marker, marker_created, preparing;
    char error[256];
} Tx;
typedef struct { char **v; size_t n, at; } Fields;
static volatile sig_atomic_t stopped;
static const char marker[] = ".fl-fix.active";
static void on_signal(int number) { stopped = number; }
static int fail(Tx *t, const char *message) {
    if (!t->error[0]) snprintf(t->error, sizeof(t->error), "%s%s%s", message,
                              errno ? ": " : "", errno ? strerror(errno) : "");
    return -1;
}
static void *grow(void *ptr, size_t size) {
    void *next = realloc(ptr, size); if (!next) { perror("file transaction allocation"); exit(1); } return next;
}
static char *copy(const char *s) { char *v = strdup(s); if (!v) { perror("file transaction allocation"); exit(1); } return v; }
static char *join(const char *a, const char *b) {
    char *out = NULL; if (asprintf(&out, "%s/%s", a, b) < 0) exit(1); return out;
}
static int write_all(int fd, const void *data, size_t n) {
    const char *p = data;
    while (n) { ssize_t k = write(fd, p, n); if (k < 0 && errno == EINTR) continue;
        if (k <= 0) return -1;
        p += k; n -= (size_t)k; } return 0;
}
static int fields_read(int fd, Fields *f) {
    char buffer[8192], *part = NULL; size_t length = 0; ssize_t n;
    memset(f, 0, sizeof(*f));
    while ((n = read(fd, buffer, sizeof(buffer))) != 0) {
        if (n < 0) { if (errno == EINTR) continue; free(part); return -1; }
        for (ssize_t i = 0; i < n; i++) {
            if (!buffer[i]) { part = grow(part, length + 1); part[length] = 0;
                f->v = grow(f->v, (f->n + 1) * sizeof(*f->v)); f->v[f->n++] = part; part = NULL; length = 0;
            } else { part = grow(part, length + 1); part[length++] = buffer[i]; }
        }
    }
    free(part); if (length) { errno = EINVAL; return -1; } return 0;
}
static const char *take(Fields *f) { return f->at < f->n ? f->v[f->at++] : NULL; }
static int number(const char *s, uint64_t *out) {
    if (!s || !*s || *s == '-') return -1;
    char *end = NULL; errno = 0; unsigned long long value = strtoull(s, &end, 10);
    if (errno || *end) return -1;
    *out = (uint64_t)value; return 0;
}
static int digest_string(const char *s) {
    if (!s || strlen(s) != 64) return 0;
    for (; *s; s++) if (!strchr("0123456789abcdef", *s)) return 0;
    return 1;
}
static int relative(const char *s) {
    if (!s || !*s || *s == '/') return 0;
    const char *p = s;
    while (*p) { const char *end = strchr(p, '/'); size_t n = end ? (size_t)(end-p) : strlen(p);
        if (!n || (n == 1 && *p == '.') || (n == 2 && p[0] == '.' && p[1] == '.')) return 0;
        if (!end) return 1;
        p = end + 1; } return 0;
}
static const char *leaf(const char *path) { const char *p = strrchr(path, '/'); return p ? p + 1 : path; }
static char *directory(const char *path) { char *s = copy(path), *p = strrchr(s, '/'); if (p) *p = 0; else *s = 0; return s; }
static int absolute_dir(const char *path) {
    if (!path || *path != '/' || path[1] == 0) { errno = EINVAL; return -1; }
    int fd = open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC); if (fd < 0) return -1;
    char *text = copy(path + 1), *save = NULL, *part = strtok_r(text, "/", &save);
    while (part) {
        if (!strcmp(part, ".") || !strcmp(part, "..")) { close(fd); free(text); errno = EINVAL; return -1; }
        int next = openat(fd, part, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        close(fd); if (next < 0) { free(text); return -1; } fd = next; part = strtok_r(NULL, "/", &save);
    }
    free(text); return fd;
}
static int parent_at(int root, const char *path) {
    if (!relative(path)) { errno = EINVAL; return -1; }
    char *dir = directory(path), *save = NULL, *part = strtok_r(dir, "/", &save);
    int fd = dup(root); if (fd < 0) { free(dir); return -1; }
    while (part) { int next = openat(fd, part, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        close(fd); if (next < 0) { free(dir); return -1; } fd = next; part = strtok_r(NULL, "/", &save); }
    free(dir); return fd;
}
static int regular_fd(int fd, struct stat *s, int single) {
    return fstat(fd, s) == 0 && S_ISREG(s->st_mode) && s->st_uid == geteuid() && (!single || s->st_nlink == 1);
}
static int hash_fd(int fd, char out[65]) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new(); unsigned char bytes[65536], digest[EVP_MAX_MD_SIZE];
    unsigned int count = 0; off_t at = 0; int ok = ctx && EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) == 1;
    while (ok) { ssize_t n = pread(fd, bytes, sizeof(bytes), at);
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) { ok = 0; break; } if (!n) break;
        ok = EVP_DigestUpdate(ctx, bytes, (size_t)n) == 1; at += n;
    }
    if (ok) ok = EVP_DigestFinal_ex(ctx, digest, &count) == 1 && count == 32;
    EVP_MD_CTX_free(ctx); if (!ok) return -1;
    for (unsigned i = 0; i < count; i++) snprintf(out + i*2, 3, "%02x", digest[i]);
    return 0;
}
static int hash_equal(int fd, const char *expected) { char actual[65]; return hash_fd(fd, actual) == 0 && !strcmp(actual, expected); }
static int identity(const struct stat *s, uint64_t dev, uint64_t ino) { return (uint64_t)s->st_dev == dev && (uint64_t)s->st_ino == ino; }
static int file_at(int parent, const char *name, uint64_t dev, uint64_t ino, const char *hash) {
    int fd = openat(parent, name, O_RDONLY | O_NOFOLLOW | O_CLOEXEC); if (fd < 0) return errno == ENOENT ? 0 : -1;
    struct stat s; int yes = regular_fd(fd, &s, 0) && identity(&s, dev, ino) && (!hash || hash_equal(fd, hash)); close(fd); return yes ? 1 : -1;
}
static int captured_original(int parent,const char*name,Write*w) {
    int fd=openat(parent,name,O_RDONLY|O_NOFOLLOW|O_CLOEXEC);struct stat s;
    int ok=fd>=0&&regular_fd(fd,&s,0)&&!identity(&s,w->new_dev,w->new_ino);
    if(fd>=0)close(fd);
    return ok;
}
static int absent_at(int parent, const char *name) { struct stat s; return fstatat(parent, name, &s, AT_SYMLINK_NOFOLLOW) < 0 && errno == ENOENT; }
static int field_out(int fd, const char *s) { return write_all(fd, s, strlen(s) + 1); }
static int numeric_out(int fd, uint64_t n) { char b[32]; snprintf(b, sizeof(b), "%" PRIu64, n); return field_out(fd, b); }
static int state(Tx *t, const char *name) {
    int fd = openat(t->journal, "state.next", O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW | O_CLOEXEC, 0600);
    if (fd < 0) return -1;
    int ok = write_all(fd, name, strlen(name)) == 0 && fsync(fd) == 0; close(fd);
    return ok && renameat(t->journal, "state.next", t->journal, "state") == 0 && fsync(t->journal) == 0 ? 0 : -1;
}
static char *read_state(Tx *t) {
    int fd = openat(t->journal, "state", O_RDONLY | O_NOFOLLOW | O_CLOEXEC); char b[64];
    if (fd < 0) return NULL;
    ssize_t n = read(fd, b, sizeof(b)-1); close(fd); if (n <= 0) return NULL; b[n] = 0; return copy(b);
}
static int copy_fd(int from, int to, const char *hash) {
    unsigned char b[65536]; off_t at = 0;
    while (1) { ssize_t n = pread(from, b, sizeof(b), at); if (n < 0 && errno == EINTR) continue;
        if (n < 0) return -1;
        if (!n) break;
        if (write_all(to, b, (size_t)n) < 0) return -1;
        at += n; }
    return fsync(to) == 0 && hash_equal(to, hash) ? 0 : -1;
}
static int copy_attributes(int from, int to) {
    ssize_t n = flistxattr(from, NULL, 0);
    if (n < 0) return errno == ENOTSUP ? 0 : -1;
    if (!n) return 0;
    char *names = grow(NULL, (size_t)n);
    if (flistxattr(from, names, (size_t)n) != n) { free(names); return -1; }
    for (ssize_t i = 0; i < n;) {
        const char *name = names + i;
        size_t size = strnlen(name, (size_t)(n-i));
        if (size == (size_t)(n-i)) { free(names); errno = EINVAL; return -1; }
        ssize_t count = fgetxattr(from, name, NULL, 0);
        if (count < 0) { free(names); return -1; }
        void *value = grow(NULL, count ? (size_t)count : 1);
        int ok = fgetxattr(from, name, value, (size_t)count) == count &&
                 fsetxattr(to, name, value, (size_t)count, 0) == 0;
        free(value);
        if (!ok) { free(names); return -1; }
        i += (ssize_t)size + 1;
    }
    free(names);
    return 0;
}
static int load_request(Tx *t, int fd) {
    Fields f; if (fields_read(fd, &f) < 0) return fail(t, "invalid file request");
    const char *tag = take(&f); if (!tag || strcmp(tag, "fl-file-request/1")) { errno = EINVAL; return fail(t, "invalid file request schema"); }
    while ((tag = take(&f))) {
        if (!strcmp(tag, "E")) { if (f.at != f.n) return fail(t, "trailing file request fields"); return 0; }
        if (!strcmp(tag, "I")) {
            const char *p = take(&f), *h = take(&f); if (!relative(p) || !digest_string(h)) return fail(t, "invalid source guard");
            t->inputs = grow(t->inputs, (t->input_n+1)*sizeof(*t->inputs)); t->inputs[t->input_n++] = (Input){copy(p),copy(h)};
        } else if (!strcmp(tag, "L")) {
            const char *p = take(&f); if (!relative(p)) return fail(t, "invalid source inventory");
            t->initial = grow(t->initial, (t->initial_n+1)*sizeof(*t->initial)); t->initial[t->initial_n++] = copy(p);
        } else if (!strcmp(tag, "W")) {
            const char *src = take(&f), *dst = take(&f), *stage = take(&f), *before = take(&f), *after = take(&f);
            if (!relative(src) || !relative(dst) || !relative(stage) || strchr(stage, '/') || !digest_string(before) || !digest_string(after)) return fail(t, "invalid file operation");
            char *a = directory(src), *b = directory(dst); int same = !strcmp(a,b); free(a); free(b);
            if (!same) return fail(t, "file moves must keep their directory");
            t->writes = grow(t->writes, (t->write_n+1)*sizeof(*t->writes));
            Write *w = &t->writes[t->write_n++]; memset(w,0,sizeof(*w)); w->parent = w->old_fd = -1;
            w->src=copy(src); w->dst=copy(dst); w->stage=copy(stage); w->before=copy(before); w->after=copy(after);
        } else return fail(t, "unknown file request record");
    }
    return fail(t, "incomplete file request");
}
static int source_guards(Tx *t) {
    for (size_t i=0;i<t->input_n;i++) {
        Input *in=&t->inputs[i]; int parent=parent_at(t->root,in->path);
        if (parent<0) return fail(t,"source directory changed");
        int fd=openat(parent,leaf(in->path),O_RDONLY|O_NOFOLLOW|O_CLOEXEC); close(parent); struct stat s;
        int ok=fd>=0 && regular_fd(fd,&s,1) && hash_equal(fd,in->hash); if(fd>=0)close(fd);
        if(!ok)return fail(t,"source changed, is linked or cannot be read");
    }
    return 0;
}
static int request_consistent(Tx *t) {
    for (size_t i=0;i<t->input_n;i++)
        for (size_t j=0;j<i;j++)
            if (!strcmp(t->inputs[i].path,t->inputs[j].path)) return fail(t,"duplicate source guard");
    for (size_t i=0;i<t->initial_n;i++) {
        int guarded=0;
        for(size_t j=0;j<t->input_n;j++)
            if(!strcmp(t->initial[i],t->inputs[j].path)) guarded=1;
        if(!guarded)return fail(t,"inventory has no source guard");
        for(size_t j=0;j<i;j++)
            if(!strcmp(t->initial[i],t->initial[j]))return fail(t,"duplicate source inventory");
    }
    return 0;
}
static int compare_string(const void *a,const void *b){return strcmp(*(char *const*)a,*(char *const*)b);}
static int scan(int fd,const char *prefix,char ***out,size_t *n) {
    int duplicate=dup(fd); if(duplicate<0)return -1; DIR *dir=fdopendir(duplicate); if(!dir){close(duplicate);return -1;}
    struct dirent *e; int result=0;
    while((e=readdir(dir))) {
        if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;
        struct stat s; if(fstatat(fd,e->d_name,&s,AT_SYMLINK_NOFOLLOW)<0){result=-1;break;}
        if(S_ISLNK(s.st_mode)){errno=ELOOP;result=-1;break;}
        char *path=join(prefix,e->d_name);
        if(S_ISDIR(s.st_mode)) {
            if(e->d_name[0]=='.'){free(path);errno=EINVAL;result=-1;break;}
            int child=openat(fd,e->d_name,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
            if(child<0||scan(child,path,out,n)<0){if(child>=0)close(child);free(path);result=-1;break;} close(child);
        } else if(S_ISREG(s.st_mode)) {
            size_t len=strlen(path); if((len>=3&&!strcmp(path+len-3,".fl"))||(len>=4&&!strcmp(path+len-4,".flx"))){*out=grow(*out,(*n+1)*sizeof(**out));(*out)[(*n)++]=path;path=NULL;}
        }
        free(path);
    }
    closedir(dir);return result;
}
static int inventory(Tx *t,int transformed) {
    const char *prefix=t->web+strlen(t->project)+1; char **actual=NULL;size_t n=0;
    const char *names[]={"pages","islands"};
    for(size_t i=0;i<2;i++) {
        char *p=join(prefix,names[i]);int parent=parent_at(t->root,p),fd=parent<0?-1:openat(parent,leaf(p),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
        if(parent>=0)close(parent);
        if(fd<0&&i==1&&errno==ENOENT){free(p);continue;}
        if(fd<0||scan(fd,p,&actual,&n)<0){if(fd>=0)close(fd);free(p);return fail(t,"source inventory is no longer regular");}close(fd);free(p);
    }
    char **expected=grow(NULL,(t->initial_n+1)*sizeof(*expected));
    for(size_t i=0;i<t->initial_n;i++) {
        expected[i]=t->initial[i];
        if(transformed)for(size_t j=0;j<t->write_n;j++)
            if(!strcmp(expected[i],t->writes[j].src)){expected[i]=t->writes[j].dst;break;}
    }
    qsort(actual,n,sizeof(*actual),compare_string);qsort(expected,t->initial_n,sizeof(*expected),compare_string);
    int ok=n==t->initial_n;for(size_t i=0;ok&&i<n;i++)ok=!strcmp(actual[i],expected[i]);
    for(size_t i=0;i<n;i++)free(actual[i]);
    free(actual);free(expected);return ok?0:fail(t,"source inventory changed");
}
static int root_current(Tx *t) {
    int fd=absolute_dir(t->project);struct stat s;int ok=fd>=0&&fstat(fd,&s)==0&&identity(&s,t->root_dev,t->root_ino);if(fd>=0)close(fd);
    return ok?0:fail(t,"selected project directory changed");
}
static int parent_current(Tx *t,Write *w) {
    int fd=parent_at(t->root,w->src);struct stat a,b;int ok=fd>=0&&fstat(fd,&a)==0&&fstat(w->parent,&b)==0&&a.st_dev==b.st_dev&&a.st_ino==b.st_ino;if(fd>=0)close(fd);
    return ok?0:fail(t,"source parent directory changed");
}
static int save_manifest(Tx *t) {
    int fd=openat(t->journal,"manifest.nul",O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);if(fd<0)return fail(t,"create recovery manifest");
    int ok=field_out(fd,"fl-file-transaction/1")==0&&field_out(fd,t->project)==0&&field_out(fd,t->web)==0&&field_out(fd,t->nonce)==0&&numeric_out(fd,t->root_dev)==0&&numeric_out(fd,t->root_ino)==0;
    for(size_t i=0;ok&&i<t->write_n;i++) {
        Write*w=&t->writes[i];ok=field_out(fd,"W")==0&&field_out(fd,w->src)==0&&field_out(fd,w->dst)==0&&field_out(fd,w->stage)==0&&field_out(fd,w->before)==0&&field_out(fd,w->after)==0&&field_out(fd,w->fresh)==0&&field_out(fd,w->retired)==0&&numeric_out(fd,w->old_dev)==0&&numeric_out(fd,w->old_ino)==0&&numeric_out(fd,w->mode)==0&&numeric_out(fd,w->uid)==0&&numeric_out(fd,w->gid)==0;
    }
    if(ok)ok=field_out(fd,"E")==0&&fsync(fd)==0;
    close(fd);return ok&&fsync(t->journal)==0?0:fail(t,"persist recovery manifest");
}
static int save_after_id(Tx*t,size_t i) {
    char name[64];snprintf(name,sizeof(name),"after-%zu.id",i);int fd=openat(t->journal,name,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);if(fd<0)return -1;
    Write*w=&t->writes[i];int ok=numeric_out(fd,w->new_dev)==0&&numeric_out(fd,w->new_ino)==0&&fsync(fd)==0;close(fd);return ok&&fsync(t->journal)==0?0:-1;
}
static int marker_owned(Tx*t) {
    int fd=openat(t->root,marker,O_RDONLY|O_NOFOLLOW|O_CLOEXEC);if(fd<0)return 0;Fields f;struct stat s;
    int ok=regular_fd(fd,&s,1)&&fields_read(fd,&f)==0&&f.n==2&&!strcmp(f.v[0],t->run)&&!strcmp(f.v[1],t->nonce);close(fd);return ok;
}
static int marker_remove(Tx*t) {
    if(!marker_owned(t)){errno=ESTALE;return fail(t,"transaction ownership marker changed");}
    if(unlinkat(t->root,marker,0)<0||fsync(t->root)<0)return fail(t,"release transaction marker");
    t->owns_marker=0;return 0;
}
static int prepare(Tx*t) {
    if(root_current(t)<0||source_guards(t)<0||inventory(t,0)<0)return -1;
    if(!absent_at(t->root,marker))return fail(t,"an unfinished source transaction requires recovery");
    for(size_t i=0;i<t->write_n;i++) {
        Write*w=&t->writes[i];int guarded=0;
        for(size_t j=0;j<t->input_n;j++)if(!strcmp(w->src,t->inputs[j].path)&&!strcmp(w->before,t->inputs[j].hash))guarded=1;
        if(!guarded)return fail(t,"write has no matching source guard");
        for(size_t j=0;j<i;j++)if(!strcmp(w->src,t->writes[j].src)||!strcmp(w->dst,t->writes[j].dst)||(!strcmp(w->dst,t->writes[j].src)&&strcmp(w->src,w->dst))||(!strcmp(w->src,t->writes[j].dst)&&strcmp(t->writes[j].src,t->writes[j].dst)))return fail(t,"overlapping file operations");
        w->parent=parent_at(t->root,w->src);if(w->parent<0)return fail(t,"open source parent");
        w->old_fd=openat(w->parent,leaf(w->src),O_RDONLY|O_NOFOLLOW|O_CLOEXEC);struct stat s;
        if(w->old_fd<0||!regular_fd(w->old_fd,&s,1)||!hash_equal(w->old_fd,w->before))return fail(t,"source changed before backup");
        if(flock(w->old_fd,LOCK_EX|LOCK_NB)<0)return fail(t,"source is locked by another writer");
        w->old_dev=s.st_dev;w->old_ino=s.st_ino;w->mode=s.st_mode&07777;w->uid=s.st_uid;w->gid=s.st_gid;
        char *a=NULL,*b=NULL;if(asprintf(&a,".fl-fix-%s-%zu.new",t->nonce,i)<0||asprintf(&b,".fl-fix-%s-%zu.old",t->nonce,i)<0)exit(1);w->fresh=a;w->retired=b;
        if(strcmp(w->src,w->dst)&&!absent_at(w->parent,leaf(w->dst)))return fail(t,"rename destination is occupied");
        if(!absent_at(w->parent,w->fresh)||!absent_at(w->parent,w->retired))return fail(t,"temporary source path is occupied");
    }
    if(save_manifest(t)<0||state(t,"preparing")<0)return fail(t,"persist transaction intent");
    t->preparing=1;
    int m=openat(t->root,marker,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);if(m<0)return fail(t,"claim source transaction");
    t->marker_created=1;
    int marked=field_out(m,t->run)==0&&field_out(m,t->nonce)==0&&fsync(m)==0;close(m);
    t->owns_marker=marker_owned(t);
    if(!marked||fsync(t->root)<0)return fail(t,"persist source transaction marker");
    for(size_t i=0;i<t->write_n;i++) {
        Write*w=&t->writes[i];char name[64];snprintf(name,sizeof(name),"before-%zu",i);
        int backup=openat(t->journal,name,O_RDWR|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
        if(backup<0||copy_fd(w->old_fd,backup,w->before)<0){if(backup>=0)close(backup);return fail(t,"persist original backup");}close(backup);
        int stage=openat(t->run_fd,w->stage,O_RDONLY|O_NOFOLLOW|O_CLOEXEC);struct stat s;
        if(stage<0||!regular_fd(stage,&s,1)||!hash_equal(stage,w->after)){if(stage>=0)close(stage);return fail(t,"staged source changed");}
        int fresh=openat(w->parent,w->fresh,O_RDWR|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
        if(fresh<0||fstat(fresh,&s)<0){close(stage);if(fresh>=0)close(fresh);return fail(t,"create replacement source");}
        w->new_dev=s.st_dev;w->new_ino=s.st_ino;
        int prepared=save_after_id(t,i)==0&&fsync(w->parent)==0&&copy_fd(stage,fresh,w->after)==0&&
          fchown(fresh,w->uid,w->gid)==0&&copy_attributes(w->old_fd,fresh)==0&&
          fchmod(fresh,w->mode)==0&&fsync(fresh)==0;
        close(stage);close(fresh);
        if(!prepared)return fail(t,"prepare replacement source");
        if(stopped){errno=EINTR;return fail(t,"source transaction interrupted");}
    }
    if(fsync(t->journal)<0||source_guards(t)<0||inventory(t,0)<0||root_current(t)<0)return -1;
    if(state(t,"publishing")<0)return fail(t,"persist publication state");
    t->preparing=0;
    return 0;
}
static int publish(Tx*t) {
    for(size_t i=0;i<t->write_n;i++) {
        Write*w=&t->writes[i];if(stopped){errno=EINTR;return fail(t,"source transaction interrupted");}
        if(root_current(t)<0||parent_current(t,w)<0||file_at(w->parent,leaf(w->src),w->old_dev,w->old_ino,w->before)!=1)return fail(t,"source changed before publication");
        if(!strcmp(w->src,w->dst)) {
            if(renameat2(w->parent,w->fresh,w->parent,leaf(w->src),RENAME_EXCHANGE)<0)return fail(t,"exchange source file");
            t->published++;
            if(fsync(w->parent)<0||file_at(w->parent,w->fresh,w->old_dev,w->old_ino,w->before)!=1)return fail(t,"source changed during exchange");
        } else {
            if(linkat(w->parent,w->fresh,w->parent,leaf(w->dst),0)<0)return fail(t,"publish moved source");
            t->published++;
            if(fsync(w->parent)<0||file_at(w->parent,leaf(w->src),w->old_dev,w->old_ino,w->before)!=1)return fail(t,"source changed during file move");
            if(renameat2(w->parent,leaf(w->src),w->parent,w->retired,RENAME_NOREPLACE)<0||fsync(w->parent)<0)return fail(t,"retire moved source name");
            if(file_at(w->parent,w->retired,w->old_dev,w->old_ino,w->before)!=1)return fail(t,"source changed during retirement");
        }
    }
    if(stopped){errno=EINTR;return fail(t,"source transaction interrupted");}
    /* Recheck unchanged dependencies and every published identity before the
     * commit record. This is cooperative isolation, not exclusion of writers
     * that ignore locks or a filesystem compare-and-swap across many files. */
    for(size_t i=0;i<t->input_n;i++) {
        Input*in=&t->inputs[i];Write*w=NULL;
        for(size_t j=0;j<t->write_n;j++)if(!strcmp(in->path,t->writes[j].src)){w=&t->writes[j];break;}
        if(w) {
            if(parent_current(t,w)<0||file_at(w->parent,leaf(w->dst),w->new_dev,w->new_ino,w->after)!=1||
               (strcmp(w->src,w->dst)&&!absent_at(w->parent,leaf(w->src))))return fail(t,"published source changed before commit");
        } else {
            int parent=parent_at(t->root,in->path);
            int fd=parent<0?-1:openat(parent,leaf(in->path),O_RDONLY|O_NOFOLLOW|O_CLOEXEC);struct stat s;
            int ok=fd>=0&&regular_fd(fd,&s,1)&&hash_equal(fd,in->hash);
            if(fd>=0)close(fd);
            if(parent>=0)close(parent);
            if(!ok)return fail(t,"source dependency changed before commit");
        }
    }
    if(root_current(t)<0||inventory(t,1)<0)return -1;
    return 0;
}
static int remove_owned(Tx*t,Write*w,const char*name,uint64_t dev,uint64_t ino,const char*hash) {
    int present=file_at(w->parent,name,dev,ino,hash);if(!present)return 0;
    if(present<0){t->conflicts++;return -1;}if(unlinkat(w->parent,name,0)<0){t->conflicts++;return -1;}return 0;
}
static int rollback(Tx*t) {
    if(root_current(t)<0)return -1;
    if(state(t,t->preparing?"rollback-preparing":"rolling-back")<0)fail(t,"persist rollback state");
    for(size_t at=t->write_n;at;at--) {
        Write*w=&t->writes[at-1];if(w->parent<0){w->parent=parent_at(t->root,w->src);if(w->parent<0){t->conflicts++;continue;}}
        if(parent_current(t,w)<0){t->conflicts++;continue;}
        int src_old=file_at(w->parent,leaf(w->src),w->old_dev,w->old_ino,NULL);
        if(t->preparing) {
            if(src_old!=1){t->conflicts++;continue;}
            /* A preparing transaction never publishes scratch content. Its
             * durable inode identity also identifies a partially copied file. */
            if(w->new_ino)remove_owned(t,w,w->fresh,w->new_dev,w->new_ino,NULL);
            else if(!absent_at(w->parent,w->fresh))t->conflicts++;
            if(fsync(w->parent)<0)t->conflicts++;
            continue;
        }
        int src_new=w->new_ino?file_at(w->parent,leaf(w->src),w->new_dev,w->new_ino,w->after):0;
        if(!strcmp(w->src,w->dst)) {
            /* Exchange may capture an external writer's replacement between
             * the guard and the syscall. Restore that captured inode too,
             * only while the visible source is still our exact replacement. */
            int old_temp=captured_original(w->parent,w->fresh,w);
            if(src_new==1&&old_temp) {
                if(renameat2(w->parent,w->fresh,w->parent,leaf(w->src),RENAME_EXCHANGE)<0){t->conflicts++;continue;}t->restored++;
            } else if(src_old!=1){t->conflicts++;continue;}
            if(w->new_ino)remove_owned(t,w,w->fresh,w->new_dev,w->new_ino,w->after);
            else if(!absent_at(w->parent,w->fresh))t->conflicts++;
        } else {
            int retired=captured_original(w->parent,w->retired,w);
            if(!src_old&&retired) {
                if(renameat2(w->parent,w->retired,w->parent,leaf(w->src),RENAME_NOREPLACE)<0){t->conflicts++;continue;}t->restored++;
            } else if(src_old!=1){t->conflicts++;continue;}
            if(w->new_ino){remove_owned(t,w,leaf(w->dst),w->new_dev,w->new_ino,w->after);remove_owned(t,w,w->fresh,w->new_dev,w->new_ino,w->after);}
            else if(!absent_at(w->parent,w->fresh))t->conflicts++;
        }
        if(fsync(w->parent)<0)t->conflicts++;
    }
    if(t->conflicts){state(t,t->preparing?"rollback-preparing":"rollback-incomplete");return -1;}
    if(state(t,"rolled-back")<0||marker_remove(t)<0)return -1;
    return 0;
}
static int committed_cleanup(Tx*t) {
    if(root_current(t)<0)return -1;
    for(size_t i=0;i<t->write_n;i++) {
        Write*w=&t->writes[i];if(w->parent<0){w->parent=parent_at(t->root,w->src);if(w->parent<0)return fail(t,"open committed parent");}
        if(parent_current(t,w)<0)return -1;
        const char*old=!strcmp(w->src,w->dst)?w->fresh:w->retired;
        if(remove_owned(t,w,old,w->old_dev,w->old_ino,w->before)<0)return fail(t,"retired source changed; backups retained");
        if(strcmp(w->src,w->dst)&&remove_owned(t,w,w->fresh,w->new_dev,w->new_ino,w->after)<0)return fail(t,"replacement temporary changed");
        if(fsync(w->parent)<0)return fail(t,"persist temporary cleanup");
    }
    return marker_remove(t);
}
static int cleanup_still_pending(Tx*t) {
    /* Marker removal is the last cleanup step. An absent or altered marker
     * cannot prove completion while any recorded temporary path remains. */
    if(!absent_at(t->root,marker))return 1;
    for(size_t i=0;i<t->write_n;i++) {
        Write*w=&t->writes[i];int parent=parent_at(t->root,w->src);
        if(parent<0)return 1;
        int pending=!absent_at(parent,w->fresh)||!absent_at(parent,w->retired);
        close(parent);
        if(pending)return 1;
    }
    return 0;
}
static int load_manifest(Tx*t) {
    int fd=openat(t->journal,"manifest.nul",O_RDONLY|O_NOFOLLOW|O_CLOEXEC);Fields f;if(fd<0)return fail(t,"open recovery manifest");
    int ok=fields_read(fd,&f)==0;close(fd);const char*tag=ok?take(&f):NULL;
    if(!tag||strcmp(tag,"fl-file-transaction/1"))return fail(t,"invalid recovery manifest");
    const char *project=take(&f),*web=take(&f),*nonce=take(&f);
    if(!project||!web||!nonce||strlen(nonce)!=32||strspn(nonce,"0123456789abcdef")!=32||
       number(take(&f),&t->root_dev)<0||number(take(&f),&t->root_ino)<0)return fail(t,"incomplete recovery manifest");
    t->project=copy(project);t->web=copy(web);t->nonce=copy(nonce);
    while((tag=take(&f))) {
        if(!strcmp(tag,"E"))return f.at==f.n?0:fail(t,"trailing recovery fields");
        if(strcmp(tag,"W"))return fail(t,"invalid recovery operation");
        const char*src=take(&f),*dst=take(&f),*stage=take(&f),*before=take(&f),*after=take(&f),*fresh=take(&f),*retired=take(&f);
        if(!relative(src)||!relative(dst)||!relative(stage)||strchr(stage,'/')||!digest_string(before)||!digest_string(after)||!relative(fresh)||strchr(fresh,'/')||!relative(retired)||strchr(retired,'/'))return fail(t,"invalid recovery paths");
        char *a=directory(src),*b=directory(dst),*expected_new=NULL,*expected_old=NULL;
        int valid_parent=!strcmp(a,b);free(a);free(b);
        if(asprintf(&expected_new,".fl-fix-%s-%zu.new",nonce,t->write_n)<0||
           asprintf(&expected_old,".fl-fix-%s-%zu.old",nonce,t->write_n)<0)exit(1);
        int valid_names=!strcmp(expected_new,fresh)&&!strcmp(expected_old,retired);
        free(expected_new);free(expected_old);
        if(!valid_parent||!valid_names)return fail(t,"inconsistent recovery paths");
        t->writes=grow(t->writes,(t->write_n+1)*sizeof(*t->writes));Write*w=&t->writes[t->write_n++];memset(w,0,sizeof(*w));w->parent=w->old_fd=-1;
        w->src=copy(src);w->dst=copy(dst);w->stage=copy(stage);w->before=copy(before);w->after=copy(after);w->fresh=copy(fresh);w->retired=copy(retired);
        uint64_t mode,uid,gid;if(number(take(&f),&w->old_dev)<0||number(take(&f),&w->old_ino)<0||number(take(&f),&mode)<0||number(take(&f),&uid)<0||number(take(&f),&gid)<0)return fail(t,"invalid recovery identity");w->mode=mode;w->uid=uid;w->gid=gid;
        char name[64];snprintf(name,sizeof(name),"after-%zu.id",t->write_n-1);int id=openat(t->journal,name,O_RDONLY|O_NOFOLLOW|O_CLOEXEC);
        if(id>=0){Fields ids;int valid=fields_read(id,&ids)==0&&ids.n==2&&number(ids.v[0],&w->new_dev)==0&&number(ids.v[1],&w->new_ino)==0;close(id);if(!valid)return fail(t,"invalid staged identity");}
        else if(errno!=ENOENT)return fail(t,"read staged identity");
    }
    return fail(t,"incomplete recovery operations");
}
static void json_string(FILE*f,const char*s) {
    fputc('"',f);for(const unsigned char*p=(const unsigned char*)s;*p;p++){if(*p=='"'||*p=='\\'){fputc('\\',f);fputc(*p,f);}else if(*p<32)fprintf(f,"\\u%04x",*p);else fputc(*p,f);}fputc('"',f);
}
static int result(Tx*t,const char*status,int rc) {
    int fd=openat(t->run_fd,"transaction-result.next",O_WRONLY|O_CREAT|O_TRUNC|O_NOFOLLOW|O_CLOEXEC,0600);if(fd<0)return -1;FILE*f=fdopen(fd,"w");if(!f){close(fd);return -1;}
    fprintf(f,"{\"schema\":\"fl-file-transaction/1\",\"intent\":\"%s\",\"status\":",t->recovering?"recover":"apply");json_string(f,status);
    fprintf(f,",\"ok\":%s,\"needsRecovery\":%s,\"files\":%zu,\"published\":%zu,\"restored\":%zu,\"conflicts\":%zu,\"signal\":%d,\"error\":",rc?"false":"true",(t->owns_marker||strstr(status,"incomplete")||(rc&&t->recovering))?"true":"false",t->write_n,t->published,t->restored,t->conflicts,(int)stopped);
    if(t->error[0])json_string(f,t->error);else fputs("null",f);fputs("}\n",f);
    int ok=fflush(f)==0&&fsync(fd)==0;fclose(f);return ok&&renameat(t->run_fd,"transaction-result.next",t->run_fd,"transaction-result.json")==0&&fsync(t->run_fd)==0?0:-1;
}
int main(int argc,char**argv) {
    Tx t={.root=-1,.run_fd=-1,.journal=-1};int reply=0,rc=1;const char*status="rejected";
    if(argc>=5&&!strcmp(argv[1],"apply")){t.project=copy(argv[2]);t.web=copy(argv[3]);t.run=copy(argv[4]);if(argc>5&&strcmp(argv[5],"--"))return 2;reply=argc>6?6:0;}
    else if(argc>=3&&!strcmp(argv[1],"recover")){t.recovering=1;t.run=copy(argv[2]);if(argc>3&&strcmp(argv[3],"--"))return 2;reply=argc>4?4:0;}
    else {fputs("Usage: fl-source-transaction apply PROJECT WEB RUN [-- REPORTER ARGS...] | recover RUN [-- REPORTER ARGS...]\n",stderr);return 2;}
    struct sigaction action={.sa_handler=on_signal};sigemptyset(&action.sa_mask);sigaction(SIGINT,&action,NULL);sigaction(SIGTERM,&action,NULL);
    t.run_fd=absolute_dir(t.run);struct stat s;
    if(t.run_fd<0||fstat(t.run_fd,&s)<0||s.st_uid!=geteuid()||(s.st_mode&077)){perror("private transaction directory required");return 1;}
    if(t.recovering) {t.journal=openat(t.run_fd,"transaction",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);if(t.journal<0||load_manifest(&t)<0)goto finish;}
    t.root=absolute_dir(t.project);if(t.root<0||flock(t.root,LOCK_EX|LOCK_NB)<0){fail(&t,"project is unavailable or another fixer is active");goto finish;}
    if(fstat(t.root,&s)<0)goto finish;
    if(t.recovering) {
        if(!identity(&s,t.root_dev,t.root_ino)||root_current(&t)<0){fail(&t,"recovery project identity changed");goto finish;}
        char*phase=read_state(&t);if(!phase){fail(&t,"missing durable transaction state");goto finish;}
        t.owns_marker=marker_owned(&t);
        if(!strcmp(phase,"applied")){
            if(t.owns_marker)rc=committed_cleanup(&t)==0?0:1;
            else if(cleanup_still_pending(&t))fail(&t,"committed cleanup ownership is unavailable");
            else rc=0;
            status=rc?"applied-cleanup-incomplete":"applied";
        }
        else if(!strcmp(phase,"rolled-back")){
            if(t.owns_marker)rc=marker_remove(&t)==0?0:1;
            else if(cleanup_still_pending(&t))fail(&t,"rollback cleanup ownership is unavailable");
            else rc=0;
            status=rc?"rollback-incomplete":"rolled-back";
        }
        else if(!t.owns_marker){fail(&t,"recovery ownership marker is unavailable");status="rollback-incomplete";}
        else if(!strcmp(phase,"preparing")||!strcmp(phase,"rollback-preparing")||!strcmp(phase,"publishing")||!strcmp(phase,"rolling-back")||!strcmp(phase,"rollback-incomplete")) {
            t.preparing=!strcmp(phase,"preparing")||!strcmp(phase,"rollback-preparing");
            rc=rollback(&t)==0?0:1;status=rc?"rollback-incomplete":"rolled-back";
        } else fail(&t,"unrecognized durable transaction state");
        free(phase);goto finish;
    }
    t.root_dev=s.st_dev;t.root_ino=s.st_ino;
    if(strncmp(t.web,t.project,strlen(t.project))||t.web[strlen(t.project)]!='/'||!relative(t.web+strlen(t.project)+1)){fail(&t,"web root must be inside selected project");goto finish;}
    unsigned char random[16];if(getrandom(random,sizeof(random),0)!=(ssize_t)sizeof(random)){fail(&t,"transaction identity unavailable");goto finish;}
    t.nonce=grow(NULL,33);for(size_t i=0;i<sizeof(random);i++)snprintf(t.nonce+i*2,3,"%02x",random[i]);
    if(load_request(&t,STDIN_FILENO)<0||request_consistent(&t)<0)goto finish;
    if(mkdirat(t.run_fd,"transaction",0700)<0){fail(&t,"transaction already exists");goto finish;}
    t.journal=openat(t.run_fd,"transaction",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
    if(t.journal<0||fsync(t.run_fd)<0){fail(&t,"persist recovery directory");goto finish;}
    if(prepare(&t)<0||publish(&t)<0) {
        if(t.owns_marker){int restored=rollback(&t);status=restored<0?"rollback-incomplete":"rolled-back";}
        else if(t.marker_created)status="rollback-incomplete";
        goto finish;
    }
    if(state(&t,"applied")<0){fail(&t,"persist committed transaction");status=rollback(&t)<0?"rollback-incomplete":"rolled-back";goto finish;}
    rc=committed_cleanup(&t)==0?0:1;status=rc?"applied-cleanup-incomplete":"applied";
finish:
    if(result(&t,status,rc)<0){perror("persist transaction result");return 1;}
    if(t.error[0])fprintf(stderr,"FL_FILE_TRANSACTION=%s\nCAUSE=%s\n",status,t.error);
    if(reply){execv(argv[reply],&argv[reply]);perror("Native transaction reporter");return 1;}
    int out=openat(t.run_fd,"transaction-result.json",O_RDONLY|O_NOFOLLOW|O_CLOEXEC);if(out>=0){char b[8192];ssize_t n;while((n=read(out,b,sizeof(b)))>0)if(write_all(STDOUT_FILENO,b,(size_t)n)<0)break;close(out);}return rc;
}
