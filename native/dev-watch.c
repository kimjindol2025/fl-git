/* Linux notification host. Source selection and rebuild policy stay in .fl. */
#define _GNU_SOURCE
#define _XOPEN_SOURCE 700
#include <sys/inotify.h>
#include <sys/stat.h>
#include <ftw.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

typedef struct { int descriptor, seen; } Watch;
static Watch* watches;
static size_t watch_count, watch_capacity;
static int notify_fd = -1;
static volatile sig_atomic_t stopping;

static void stop_watch(int signal_number) { (void)signal_number; stopping = 1; }
static int emit(const char* text) {
    size_t length = strlen(text);
    while (length && !stopping) {
        ssize_t n = write(STDOUT_FILENO, text, length);
        if (n < 0 && errno == EINTR) continue;
        if (n < 0 && errno == EPIPE) { stopping = 1; return 0; }
        if (n <= 0) return -1;
        text += n; length -= (size_t)n;
    }
    return 0;
}
static int add_directory(const char* path, const struct stat* info, int kind, struct FTW* place) {
    (void)info;
    if (stopping) return FTW_STOP;
    if (kind == FTW_DNR || kind == FTW_NS) {
        if (place->level && (errno == ENOENT || errno == ENOTDIR)) return FTW_CONTINUE;
        if (!errno) errno = EACCES;
        return FTW_STOP;
    }
    if (kind != FTW_D) return FTW_CONTINUE;
    const char* name = path + place->base;
    if (place->level && (!strcmp(name, ".git") || !strcmp(name, ".hg")
        || !strcmp(name, ".svn") || !strcmp(name, "node_modules"))) return FTW_SKIP_SUBTREE;
    uint32_t mask = IN_MODIFY | IN_CLOSE_WRITE | IN_ATTRIB | IN_CREATE | IN_DELETE
        | IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE_SELF | IN_MOVE_SELF | IN_ONLYDIR | IN_DONT_FOLLOW;
    int descriptor = inotify_add_watch(notify_fd, path, mask);
    if (descriptor < 0) {
        if (place->level && (errno == ENOENT || errno == ENOTDIR || errno == ELOOP)) return FTW_CONTINUE;
        return FTW_STOP;
    }
    for (size_t i = 0; i < watch_count; i++) {
        if (watches[i].descriptor == descriptor) { watches[i].seen = 1; return FTW_CONTINUE; }
    }
    if (watch_count == watch_capacity) {
        size_t capacity = watch_capacity ? watch_capacity * 2 : 64;
        if (capacity < watch_capacity || capacity > SIZE_MAX / sizeof(*watches)) {
            errno = ENOMEM; return FTW_STOP;
        }
        Watch* next = realloc(watches, capacity * sizeof(*watches));
        if (!next) return FTW_STOP;
        watches = next; watch_capacity = capacity;
    }
    watches[watch_count++] = (Watch){ descriptor, 1 };
    return FTW_CONTINUE;
}
static int refresh(int count, char** roots) {
    for (size_t i = 0; i < watch_count; i++) watches[i].seen = 0;
    for (int i = 0; i < count; i++) {
        struct stat info;
        if (lstat(roots[i], &info) < 0) return -1;
        if (!S_ISDIR(info.st_mode)) { errno = ENOTDIR; return -1; }
        if (nftw(roots[i], add_directory, 32, FTW_PHYS | FTW_ACTIONRETVAL) != 0) return -1;
    }
    size_t kept = 0;
    for (size_t i = 0; i < watch_count; i++) {
        if (watches[i].seen) watches[kept++] = watches[i];
        else if (inotify_rm_watch(notify_fd, watches[i].descriptor) < 0 && errno != EINVAL) return -1;
    }
    watch_count = kept;
    return 0;
}
int main(int argc, char** argv) {
    if (argc == 2 && !strcmp(argv[1], "--clock-ms")) {
        struct timespec now;
        if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) { perror("Native monotonic clock"); return 1; }
        printf("%lld\n", (long long)now.tv_sec * 1000 + now.tv_nsec / 1000000);
        return 0;
    }
    if (argc < 2) { fprintf(stderr, "Usage: fl-dev-watch-host <directory> [directory...]\n"); return 2; }
    struct sigaction action = {0};
    action.sa_handler = stop_watch;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGTERM, &action, NULL) < 0 || sigaction(SIGINT, &action, NULL) < 0) {
        perror("Native watch signal setup"); return 1;
    }
    action.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &action, NULL) < 0) { perror("Native watch pipe setup"); return 1; }
    notify_fd = inotify_init1(IN_CLOEXEC);
    if (notify_fd < 0) { perror("Native directory watch"); return 1; }
    int code = 0;
    if (refresh(argc - 1, argv + 1) < 0) {
        if (!stopping) { perror("Native recursive directory watch"); code = 1; }
        goto cleanup;
    }
    if (emit("READY\n") < 0) { code = 1; goto cleanup; }
    while (!stopping) {
        union { struct inotify_event alignment; char bytes[65536]; } buffer;
        ssize_t length = read(notify_fd, buffer.bytes, sizeof(buffer.bytes));
        if (length < 0 && errno == EINTR) continue;
        if (length <= 0) { if (!stopping) { perror("Native watch read"); code = 1; } break; }
        int refresh_needed = 0;
        size_t offset = 0;
        while (offset < (size_t)length) {
            if ((size_t)length - offset < sizeof(struct inotify_event)) { errno = EPROTO; code = 1; break; }
            struct inotify_event* event = (struct inotify_event*)(buffer.bytes + offset);
            if (event->len > (size_t)length - offset - sizeof(*event)) { errno = EPROTO; code = 1; break; }
            if (event->mask & (IN_ISDIR | IN_IGNORED | IN_Q_OVERFLOW | IN_DELETE_SELF | IN_MOVE_SELF)) refresh_needed = 1;
            offset += sizeof(*event) + event->len;
        }
        if (code) { perror("Native watch event"); break; }
        if (refresh_needed && refresh(argc - 1, argv + 1) < 0) {
            if (!stopping) { perror("Native watch refresh"); code = 1; } break;
        }
        if (emit("CHANGE\n") < 0) { perror("Native watch output"); code = 1; break; }
    }
cleanup:
    close(notify_fd);
    free(watches);
    return code;
}
