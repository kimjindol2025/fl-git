/* Process/file boundary for generated JavaScript; no frontend policy or JS
 * runtime source here. The supplied SpiderMonkey/GJS pair is platform input.
 * Public ABI: GJS 1.80 context.h and SpiderMonkey ESR115 Initialization.h.
 * DisableJitBackend must precede GJS's library initialization. The interpreter
 * retains JavaScript execution, without executable JIT or WebAssembly support.
 */
#define _POSIX_C_SOURCE 200809L
#include <dlfcn.h>
#include <stdbool.h>
#include <stdint.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef struct _GjsContext GjsContext;
/* GError's public C ABI: GQuark domain, integer code and owned message. */
typedef struct _GError { uint32_t domain; int code; char *message; } EngineError;

static void *required(void *library, const char *name) {
    void *function = dlsym(library, name);
    if (!function) {
        fprintf(stderr, "FL_JS_ENGINE=FAIL\nCAUSE=Missing public runtime API: %s\n", name);
        exit(2);
    }
    return function;
}

static int limit_memory(void) {
    struct rlimit memory;
    if (getrlimit(RLIMIT_AS, &memory)) return -1;
    const rlim_t cap = 2147483648ULL;
    if (memory.rlim_cur == RLIM_INFINITY || memory.rlim_cur > cap) memory.rlim_cur = cap;
    if (memory.rlim_max == RLIM_INFINITY || memory.rlim_max > cap) memory.rlim_max = cap;
    struct rlimit core = {0, 0};
    return setrlimit(RLIMIT_AS, &memory) || setrlimit(RLIMIT_CORE, &core) ? -1 : 0;
}

int main(int argc, char **argv) {
    if (argc < 4 || (argc > 4 && strcmp(argv[4], "--"))) {
        fputs("Usage: fl-js-engine <absolute-mozjs-library> <absolute-gjs-library> <generated-script> [-- args...]\n", stderr);
        return 2;
    }
    if (limit_memory()) {
        fputs("FL_JS_ENGINE=FAIL\nCAUSE=Cannot preserve Native memory limits\n", stderr);
        return 2;
    }
    /* GJS print converts UTF-8 through the embedding process's character
     * locale. A C embedder starts in ASCII even with a UTF-8 LANG variable. */
    if (!setlocale(LC_CTYPE, "C.UTF-8") && !setlocale(LC_CTYPE, "C.utf8")) {
        fputs("FL_JS_ENGINE=FAIL\nCAUSE=A process-local UTF-8 character locale is required\n", stderr);
        return 2;
    }
    struct stat file;
    if (argv[1][0] != '/' || argv[2][0] != '/' ||
        stat(argv[1], &file) || !S_ISREG(file.st_mode) ||
        stat(argv[2], &file) || !S_ISREG(file.st_mode) ||
        stat(argv[3], &file) || !S_ISREG(file.st_mode)) {
        fputs("FL_JS_ENGINE=FAIL\nCAUSE=Select existing runtime libraries and a regular generated script\n", stderr);
        return 2;
    }
    void *engine = dlopen(argv[1], RTLD_NOW | RTLD_GLOBAL);
    if (!engine) {
        fprintf(stderr, "FL_JS_ENGINE=FAIL\nCAUSE=Engine library: %s\n", dlerror());
        return 2;
    }
    void (*disable_jit)(void) = required(engine, "_ZN2JS17DisableJitBackendEv");
    disable_jit();
    if (setenv("GJS_DISABLE_JIT", "1", 1)) return 2;
    void *binding = dlopen(argv[2], RTLD_NOW | RTLD_LOCAL);
    if (!binding) {
        fprintf(stderr, "FL_JS_ENGINE=FAIL\nCAUSE=Binding library: %s\n", dlerror());
        return 2;
    }
    GjsContext *(*create)(void) = required(binding, "gjs_context_new");
    void (*set_args)(GjsContext *, ssize_t, const char **) = required(binding, "gjs_context_set_argv");
    bool (*evaluate)(GjsContext *, const char *, int *, EngineError **) = required(binding, "gjs_context_eval_file");
    void (*release)(void *) = required(binding, "g_object_unref");
    void (*free_error)(EngineError *) = required(binding, "g_error_free");
    GjsContext *context = create();
    if (!context) {
        fputs("FL_JS_ENGINE=FAIL\nCAUSE=JavaScript context initialization failed\n", stderr);
        return 1;
    }
    set_args(context, argc > 4 ? argc - 5 : 0, (const char **)(argc > 4 ? argv + 5 : argv + argc));
    EngineError *error = NULL;
    int status = 1;
    bool ok = evaluate(context, argv[3], &status, &error);
    if (error) {
        fprintf(stderr, "FL_JS_ENGINE=FAIL\nCAUSE=%s\n", error->message ? error->message : "JavaScript evaluation failed");
        free_error(error);
    }
    release(context);
    /* GJS's process finalizer owns engine shutdown. Its registered GObject
     * types keep these libraries alive until normal process exit. */
    return ok ? status : 1;
}
