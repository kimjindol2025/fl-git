/* Literal child execution for FreeLang hosts. All selection, cache, endpoint
 * and result policy stays in FreeLang. Linux/glibc spawn closes inherited fds;
 * only this invocation's group is cancelled and its direct child is reaped. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "process-argv.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

extern char **environ;
#define FRONT_ARGV_MAX 1024u

typedef struct { char *data; size_t used, capacity; } FrontCapture;

static int64_t front_clock_ms(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now)) return -1;
    return (int64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}
static int front_private_fd(int fd) {
    if (fd < 0 || fd > STDERR_FILENO) return fd;
    int moved = fcntl(fd, F_DUPFD_CLOEXEC, STDERR_FILENO + 1);
    close(fd);
    return moved;
}
static int front_capture_add(FrontCapture *out, const char *data, size_t count,
                             size_t limit, int *overflow) {
    size_t available = limit - out->used;
    if (count > available) { count = available; *overflow = 1; }
    if (!count) return 0;
    size_t needed = out->used + count + 1;
    if (needed > out->capacity) {
        size_t capacity = out->capacity ? out->capacity : 4096;
        while (capacity < needed) capacity *= 2;
        if (capacity > limit + 1) capacity = limit + 1;
        char *next = realloc(out->data, capacity);
        if (!next) return -1;
        out->data = next; out->capacity = capacity;
    }
    memcpy(out->data + out->used, data, count);
    out->used += count; out->data[out->used] = 0;
    return 0;
}
static FLValue front_capture_string(const FrontCapture *capture) {
    size_t size = capture->used;
    FLString *s = fl_arena_alloc(sizeof(*s) + size + 1);
    if (!s) return fl_nil();
    s->base.type = FL_STRING; s->base.rc = 1; s->len = (uint32_t)size;
    if (size) memcpy(s->data, capture->data, size);
    s->data[size] = 0;
    return (FLValue){.tag=FL_STRING,.obj=(FLObject *)s};
}
static int front_integer(FLValue value, int64_t lower, int64_t upper, int64_t *out) {
    if (value.tag == FL_INT && value.i >= lower && value.i <= upper) {
        *out = value.i; return 1;
    }
    if (value.tag == FL_FLOAT && isfinite(value.f) && value.f >= lower &&
        value.f <= upper && trunc(value.f) == value.f) {
        *out = (int64_t)value.f; return 1;
    }
    return 0;
}

FLValue fl_front_run_argv(FLValue arguments, FLValue timeout, FLValue output_limit) {
    int64_t milliseconds, max_output;
    /* Managed atoms share the public vector tag, but are not argv arrays. */
    if (arguments.tag != FL_VECTOR || !arguments.obj || arguments.obj->type != FL_VECTOR)
        fl_throw(fl_make_error("TypeError", "literal process arguments must be an array"));
    FLVector *values = (FLVector *)arguments.obj;
    if (!values->len || values->len > FRONT_ARGV_MAX)
        fl_throw(fl_make_error("RangeError", "literal process requires 1..1024 string arguments"));
    if (!front_integer(timeout, 1, 30000, &milliseconds))
        fl_throw(fl_make_error("RangeError", "literal process timeout must be an integer in 1..30000ms"));
    if (!front_integer(output_limit, 1, FL_FILE_IO_MAX_BYTES, &max_output))
        fl_throw(fl_make_error("RangeError", "literal process output limit exceeds Native file IO bounds"));
    char *argv[FRONT_ARGV_MAX + 1];
    for (uint32_t i = 0; i < values->len; i++) {
        if (values->data[i].tag != FL_STRING || !values->data[i].obj)
            fl_throw(fl_make_error("TypeError", "literal process arguments must all be strings"));
        FLValue text = fl_v11_utf8_string(values->data[i]);
        FLString *s = (FLString *)text.obj;
        if (memchr(s->data, 0, s->len))
            fl_throw(fl_make_error("TypeError", "literal process arguments must not contain null bytes"));
        argv[i] = s->data;
    }
    argv[values->len] = NULL;
    if (argv[0][0] != '/')
        fl_throw(fl_make_error("TypeError", "literal process executable must be an absolute path"));

    int pipes[2][2] = {{-1,-1},{-1,-1}}, input = -1;
    int status = -1, io_error = 0, spawn_error = 0, overflow = 0, timed_out = 0, signal_number = 0;
    int actions_ready = 0, attributes_ready = 0, group_ready = 0;
    int64_t now = front_clock_ms(), deadline = now + milliseconds;
    pid_t child = -1;
    FrontCapture captured[2] = {{0},{0}};
    posix_spawn_file_actions_t actions;
    posix_spawnattr_t attributes;
    if (now < 0) { io_error = EIO; goto finish; }
    for (int i = 0; i < 2; i++) {
        if (pipe2(pipes[i], O_CLOEXEC)) { io_error = errno; goto finish; }
        for (int j = 0; j < 2; j++) {
            pipes[i][j] = front_private_fd(pipes[i][j]);
            if (pipes[i][j] < 0) { io_error = errno; goto finish; }
        }
        int flags = fcntl(pipes[i][0], F_GETFL);
        if (flags < 0 || fcntl(pipes[i][0], F_SETFL, flags | O_NONBLOCK)) {
            io_error = errno; goto finish;
        }
    }
    input = front_private_fd(open("/dev/null", O_RDONLY | O_CLOEXEC));
    if (input < 0) { io_error = errno; goto finish; }
    int error = posix_spawn_file_actions_init(&actions);
    if (error) { io_error = error; goto finish; }
    actions_ready = 1;
    if ((error = posix_spawn_file_actions_adddup2(&actions, input, STDIN_FILENO)) ||
        (error = posix_spawn_file_actions_adddup2(&actions, pipes[0][1], STDOUT_FILENO)) ||
        (error = posix_spawn_file_actions_adddup2(&actions, pipes[1][1], STDERR_FILENO)) ||
        (error = posix_spawn_file_actions_addclosefrom_np(&actions, STDERR_FILENO + 1))) {
        io_error = error; goto finish;
    }
    if ((error = posix_spawnattr_init(&attributes))) { io_error = error; goto finish; }
    attributes_ready = 1;
    if ((error = posix_spawnattr_setpgroup(&attributes, 0)) ||
        (error = posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETPGROUP))) {
        io_error = error; goto finish;
    }
    error = posix_spawn(&child, argv[0], &actions, &attributes, argv, environ);
    if (error) { spawn_error = error; child = -1; goto finish; }
    group_ready = 1;
    close(input); input = -1;
    close(pipes[0][1]); pipes[0][1] = -1;
    close(pipes[1][1]); pipes[1][1] = -1;
    while (pipes[0][0] >= 0 || pipes[1][0] >= 0) {
        now = front_clock_ms();
        if (now < 0) { io_error = EIO; break; }
        if (now >= deadline) { timed_out = 1; break; }
        struct pollfd ready[2] = {{pipes[0][0],POLLIN,0},{pipes[1][0],POLLIN,0}};
        int wait_ms = (int)(deadline - now);
        int found = poll(ready, 2, wait_ms);
        if (found < 0 && errno == EINTR) continue;
        if (found < 0) { io_error = errno; break; }
        if (!found) { timed_out = 1; break; }
        for (int i = 0; i < 2; i++) {
            if (pipes[i][0] < 0 || !ready[i].revents) continue;
            if (ready[i].revents & POLLNVAL) { io_error = EBADF; break; }
            char buffer[8192];
            for (;;) {
                now = front_clock_ms();
                if (now < 0) { io_error = EIO; break; }
                if (now >= deadline) { timed_out = 1; break; }
                ssize_t count = read(pipes[i][0], buffer, sizeof(buffer));
                if (count > 0) {
                    if (front_capture_add(&captured[i], buffer, (size_t)count,
                                          (size_t)max_output, &overflow)) { io_error = ENOMEM; break; }
                    if (overflow) break;
                } else if (!count) { close(pipes[i][0]); pipes[i][0] = -1; break; }
                else if (errno == EINTR) continue;
                else if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                else { io_error = errno; break; }
            }
            if (io_error || overflow || timed_out) break;
        }
        if (io_error || overflow || timed_out) break;
    }
finish:
    if (actions_ready) posix_spawn_file_actions_destroy(&actions);
    if (attributes_ready) posix_spawnattr_destroy(&attributes);
    if (input >= 0) close(input);
    for (int i = 0; i < 2; i++) for (int j = 0; j < 2; j++)
        if (pipes[i][j] >= 0) close(pipes[i][j]);
    if (child > 0) {
        int wait_status = 0;
        pid_t waited = -1;
        if (io_error || overflow || timed_out) (void)kill(group_ready ? -child : child, SIGKILL);
        if (!io_error && !overflow && !timed_out) {
            for (;;) {
                siginfo_t observed = {0};
                int ready = waitid(P_PID, (id_t)child, &observed, WEXITED | WNOHANG | WNOWAIT);
                if (ready < 0 && errno != EINTR) { io_error = errno; break; }
                if (!ready && observed.si_pid == child) {
                    /* Keep the leader unreaped while ending any descendants
                     * that closed their streams. Its PID/group cannot be
                     * recycled before this invocation's cleanup completes. */
                    (void)kill(-child, SIGKILL);
                    break;
                }
                now = front_clock_ms();
                if (now < 0 || now >= deadline) {
                    if (now < 0) io_error = EIO; else timed_out = 1;
                    (void)kill(-child, SIGKILL);
                    break;
                }
                int delay = deadline - now < 10 ? (int)(deadline - now) : 10;
                (void)poll(NULL, 0, delay);
            }
        }
        if (waited != child) {
            do { waited = waitpid(child, &wait_status, 0); } while (waited < 0 && errno == EINTR);
        }
        if (waited < 0) { if (!io_error) io_error = errno; }
        else if (!io_error && !overflow && !timed_out && WIFEXITED(wait_status)) status = WEXITSTATUS(wait_status);
        if (waited == child && WIFSIGNALED(wait_status)) signal_number = WTERMSIG(wait_status);
    }
    FLValue output = front_capture_string(&captured[0]);
    FLValue errors = front_capture_string(&captured[1]);
    free(captured[0].data); free(captured[1].data);
    if (output.tag != FL_STRING || errors.tag != FL_STRING)
        fl_throw(fl_make_error("Error", "process output allocation failed"));
    FLValue fields[] = {
        fl_str_val("ok"), fl_bool(status == 0),
        fl_str_val("exit"), fl_int(status),
        fl_str_val("stdout"), fl_v11_utf8_string(output),
        fl_str_val("stderr"), fl_v11_utf8_string(errors),
        fl_str_val("timedOut"), fl_bool(timed_out),
        fl_str_val("overflow"), fl_bool(overflow),
        fl_str_val("spawnError"), fl_int(spawn_error),
        fl_str_val("ioError"), fl_int(io_error),
        fl_str_val("signal"), fl_int(signal_number)
    };
    return fl_map_from_pairs(fields, 9);
}
