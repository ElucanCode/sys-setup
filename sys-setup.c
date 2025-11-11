//usr/bin/env gcc -Wall -rdynamic "$0" -o sys-setup -ldl && exec ./sys-setup "$@"

#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "installer.h"

// :state

static Log_Level min_level;

static struct {
    void **items;
    size_t len;
    size_t cap;
} ptrs;

static struct {
    char *name;
    char **flags;
} compiler;

static bool dry;

// :helper

static int _strcmp(const void* a, const void* b)
{
    return strcmp(*(char**)a, *(char**)b);
}

static size_t _arrlen(void** arr)
{
    size_t len = 0;
    for (void **a = arr; *a != NULL; a += 1, len += 1);
    return len;
}

// :installer.h :implementation

void die_loc(Source_Loc loc, char* fmt, ...)
{
    static const char *fatal[] = { "FATAL", "\033[31mFATAL\033[0m" };
    fprintf(stderr, "[%s] "SRCLOC_FMT": ",
            fatal[isatty(STDERR_FILENO)], SRCLOC_ARG(&loc));
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    if (fmt[0] && ':' == fmt[strlen(fmt) - 1]) {
        fputc(' ', stderr);
        perror(NULL);
    } else {
        fprintf(stderr, "\n");
}
    fflush(stderr);
    exit(1);
}

void msg_loc(Source_Loc loc, Log_Level ll, char* fmt, ...)
{
    static const char *PREFIXES[2][_LL_NUM] = {
        {   [LL_Trace] = "TRACE",
            [LL_Debug] = "DEBUG",
            [LL_Info]  = "INFO",
            [LL_Warn]  = "WARN",
            [LL_Error] = "ERROR" },
        {   [LL_Trace] = "\033[37mTRACE\033[0m",
            [LL_Debug] = "\033[36mDEBUG\033[0m",
            [LL_Info]  = "\033[32mINFO\033[0m",
            [LL_Warn]  = "\033[33mWARN\033[0m",
            [LL_Error] = "\033[31mERROR\033[0m" },
    };

    if (ll < min_level)
        return;

    fprintf(stderr, "[%s] "SRCLOC_FMT": ",
            PREFIXES[isatty(STDERR_FILENO)][ll], SRCLOC_ARG(&loc));
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    if (fmt[0] && ':' == fmt[strlen(fmt) - 1]) {
        fputc(' ', stderr);
        perror(NULL);
    } else {
        fprintf(stderr, "\n");
    }
    fflush(stderr);
}

void register_ptr(void *ptr)
{
    da_append(&ptrs, ptr);
}

int exists(char *path, int ff)
{
    if (*path == '\0') {
        msg(LL_Error, "Empty path");
        return 0;
    }
    if (!ff) {
        msg(LL_Error, "Empty filter");
        return 0;
    }

    struct stat s;
    if (lstat(path, &s) == -1) {
        if (errno != ENOENT) // path does not exist
            msg(LL_Error, "stat failed:");
        return 0;
    }

    if (ff == FF_Any)                            return FF_Any;
    if (ff & FF_File      && S_ISREG(s.st_mode)) return FF_File;
    if (ff & FF_Directory && S_ISDIR(s.st_mode)) return FF_Directory;
    if (ff & FF_Symlink   && S_ISLNK(s.st_mode)) return FF_Symlink;
    return 0;
}

bool ls(char *path, int ff, Strings *result)
{
    if (*path == '\0') {
        msg(LL_Error, "Empty path");
        return false;
    }
    if (!ff) {
        msg(LL_Error, "Empty filter");
        return false;
    }
    if (!exists(path, FF_Directory)) {
        msg(LL_Error, "No such directory");
        return false;
    }

    DIR *dir = opendir(path);
    if (!dir) {
        msg(LL_Error, "Failed to open dir '%s':", path);
        return false;
    }

    *result = (Strings) { 0 };
    for (struct dirent *ent = readdir(dir); ent != NULL; ent = readdir(dir)) {
        if ((ent->d_type == DT_REG && (ff & FF_File))
            || (ent->d_type == DT_DIR && (ff & FF_Directory))
            || (ent->d_type == DT_LNK && (ff & FF_Symlink)))
        {
            char *name = strdup(ent->d_name);
            da_append(result, name);
        }
    }
    closedir(dir);
    da_append_many(&ptrs, result->items, result->len);
    register_ptr(result->items);
    qsort(result->items, result->len, sizeof(*result->items), _strcmp);

    return true;
}

bool prcs_write(Process p, char *in)
{
    if (p.stdin_ == INVALID_FILE_DES) {
        msg(LL_Error, "Can not write to stdin");
        return false;
    }

    const size_t in_len = strlen(in);
    size_t written = 0;
    while (written < in_len) {
        ssize_t w = write(p.stdin_, in + written, in_len - written);
        if (-1 == w) {
            msg(LL_Error, "Failed to write to child stdin (continuing):");
            break;
        }
        written += w;
    }
    return written == in_len;
}

int prcs_await(Process p, Buffer *out, Buffer *err)
{
    int ws;
    if (-1 == waitpid(p.id, &ws, 0)) {
        msg(LL_Error, "Wait failed:");
        return -1;
    }

    if (!out && !err)
        return 0;

    const size_t buf_len = 1024;
    char buf[buf_len];
    Fd fds[2] = { p.stdout_, p.stderr_ };
    Buffer *bs[2] = { out, err };

    for (size_t i = 0; i < 2; i += 1) {
        if (fds[i] == INVALID_FILE_DES || bs[i] == NULL)
            continue;

        bool exit = false;
        do {
            ssize_t r = read(fds[i], buf, buf_len);
            switch (r) {
            case -1:
                msg(LL_Error, "Error while reading std%s:", i == 0 ? "out" : "err");
            case 0:
                exit = true;
                break;
            default:
                da_append_many(bs[i], buf, r);
                break;
            }
        } while (!exit);
        register_ptr(bs[i]->items);

        if (-1 == close(fds[i])) {
            msg(LL_Error, "Failed to close std%s:", i == 0 ? "out" : "err");
        }
    }

    return WIFEXITED(ws) ? WEXITSTATUS(ws) : -1;
}

Process cmd_execa(Cmd cmd, int redirects)
{
    Buffer cmd_str = { 0 };
    for (size_t i = 0; i < cmd.len; i += 1) {
        da_append_many(&cmd_str, cmd.items[i], strlen(cmd.items[i]));
        da_append(&cmd_str, ' ');
    }
    da_append(&cmd_str, '\0');
    msg(LL_Debug, "Running cmd: %s", cmd_str.items);
    free(cmd_str.items);

    // TODO: log the cmd
    // TODO: Exit when something errors?
    int stdin_pipe[2] = { INVALID_FILE_DES, INVALID_FILE_DES };
    if (redirects & IOR_stdin) {
        if (-1 == pipe(stdin_pipe))
            msg(LL_Error, "Failed to create stdin pipe:");
    }

    int stdout_pipe[2] = { INVALID_FILE_DES, INVALID_FILE_DES };
    if (redirects & IOR_stdout) {
        if (-1 == pipe(stdout_pipe))
            msg(LL_Error, "Failed to create stdout pipe:");
    }

    int stderr_pipe[2] = { INVALID_FILE_DES, INVALID_FILE_DES };
    if (redirects & IOR_stderr) {
        if (-1 == pipe(stderr_pipe))
            msg(LL_Error, "Failed to create stderr pipe:");
    }

    Pid id = fork();
    if (id < 0) {
        msg(LL_Error, "Failed to fork child process:");
        return INVALID_PROCESS;
    }

    if (id == 0) {
        if (stdin_pipe[PIPE_READ] != INVALID_FILE_DES) {
            if (-1 == dup2(stdin_pipe[PIPE_READ], STDIN_FILENO))
                msg(LL_Error, "Failed to redirect stdin:");
            close(stdin_pipe[PIPE_WRITE]);
        }
        if (stdout_pipe[PIPE_WRITE] != INVALID_FILE_DES) {
            if (-1 == dup2(stdout_pipe[PIPE_WRITE], STDOUT_FILENO))
                msg(LL_Error, "Failed to redirect stdout:");
            close(stdout_pipe[PIPE_READ]);
        }
        if (stderr_pipe[PIPE_WRITE] != INVALID_FILE_DES) {
            if (-1 == dup2(stderr_pipe[PIPE_WRITE], STDERR_FILENO))
                msg(LL_Error, "Failed to redirect stderr:");
            close(stderr_pipe[PIPE_READ]);
        }

        da_append(&cmd, NULL);
        if (-1 == execvp(cmd.items[0], (char*const*)cmd.items)) {
            msg(LL_Error, "execution failed:");
            exit(1);
        }

        unreachable();
    }

    if (stdin_pipe[PIPE_READ] != INVALID_FILE_DES)
        close(stdin_pipe[PIPE_READ]);
    if (stdout_pipe[PIPE_WRITE] != INVALID_FILE_DES)
        close(stdout_pipe[PIPE_WRITE]);
    if (stderr_pipe[PIPE_WRITE] != INVALID_FILE_DES)
        close(stderr_pipe[PIPE_WRITE]);

    return (Process) {
        .id      = id,
        .stdin_  = stdin_pipe[PIPE_WRITE],
        .stdout_ = stdout_pipe[PIPE_READ],
        .stderr_ = stderr_pipe[PIPE_READ],
    };
}

int cmd_execw(Cmd cmd, char *in, Buffer *out, Buffer *err)
{
    int redirects = IOR_none;
    if (in)  redirects |= IOR_stdin;
    if (out) redirects |= IOR_stdout;
    if (err) redirects |= IOR_stderr;

    Process p = cmd_execa(cmd, redirects);
    if (p.id == INVALID_PID) {
        return -1;
    }
    if (in) {
        // NOTE: Ignoring error because we still need to wait for the process
        prcs_write(p, in);
    }
    return prcs_await(p, out, err);
}

bool compile(char *file, char *out, char **cflags, char **lflags)
{
    Cmd cmd = { 0 };
    da_append(&cmd, compiler.name);
    da_append_many(&cmd, compiler.flags, _arrlen((void**) compiler.flags));
    if (cflags)
        da_append_many(&cmd, cflags, _arrlen((void**) cflags));
    da_append(&cmd, file);
    da_append(&cmd, "-o");
    da_append(&cmd, out);
    if (lflags)
        da_append_many(&cmd, lflags, _arrlen((void**) lflags));

    Buffer err = { 0 };
    bool ok = !cmd_execw(cmd, NULL, NULL, &err);
    if (!ok)
        msg(LL_Error, "Compilation of '%s' failed:\n%.*s",
            file, (int) err.len, err.items);

    if (cmd.items)
        free(cmd.items);
    if (err.items)
        free(err.items);

    return true;
}

bool compile_so(char **cfile, char *so, char **cflags, char **lflags)
{
    bool ok = true;

    Strings objs = { 0 };
    for (size_t i = 0; i < _arrlen((void**)cfile); i += 1) {
        char *obj;
        asprintf(&obj, "%s%s", cfile[i], ".o");
        da_append(&objs, obj);
        
        // TODO: also use passed cflags
        ok = compile(cfile[i], obj, strsc("-c", "-fPIC"), NULL);
        if (!ok)
            goto exit;
    }

    Cmd cmd = { 0 };
    da_append(&cmd, compiler.name);
    da_append_many(&cmd, compiler.flags, _arrlen((void**) compiler.flags));
    if (cflags)
        da_append_many(&cmd, cflags, _arrlen((void**) cflags));
    da_append(&cmd, "-shared");
    da_append(&cmd, "-o");
    da_append(&cmd, so);
    da_append_many(&cmd, objs.items, objs.len);
    if (lflags)
        da_append_many(&cmd, lflags, _arrlen((void**) lflags));

    Buffer err = { 0 };
    ok = !cmd_execw(cmd, NULL, NULL, &err);
    if (!ok)
        msg(LL_Error, "Compilation of '%s' failed:\n%.*s", so, (int) err.len, err.items);

exit:
    if (objs.items) {
        da_append_many(&ptrs, objs.items, objs.len);
        register_ptr(objs.items);
    }
    return ok;
}

bool git_clone(char *repo, char *dest, bool init_submodules)
{
    bool ok = true;

    Cmd cmd = { 0 };
    char *clone[] = strs("git", "clone", repo, dest);
    da_append_many(&cmd, clone, _arrlen((void**)clone));

    if (cmd_exec(cmd)) {
        msg(LL_Error, "Git clone failed");
        ok = false;
        goto exit;
    }

    if (!init_submodules)
        goto exit;

    cmd.len = 0;
    char *submodule[] = strs("git", "-C", dest, "submodule", "update", "--init",
                          "--recursive");
    da_append_many(&cmd, submodule, _arrlen((void**)submodule));
    if (cmd_exec(cmd)) {
        msg(LL_Error, "Git submodule init failed");
        ok = false;
        goto exit;
    }

exit:
    if (cmd.items)
        free(cmd.items);
    return ok;
}

bool git_checkout(char *repo, char *target)
{
    Cmd cmd = { 0 };
    char *checkout[] = strs("git", "-C", repo, "checkout", target);
    da_append_many(&cmd, checkout, _arrlen((void**)checkout));

    bool ok = !cmd_exec(cmd);
    if (!ok) {
        msg(LL_Error, "Git checkout failed");
    }
    if (cmd.items)
        free(cmd.items);
    return ok;
}

bool git_clean(char *repo)
{
    Cmd cmd = { 0 };
    // TODO: which flags specifically??
    char *clean[] = strs("git", "-C", repo, "clean");
    da_append_many(&cmd, clean, _arrlen((void**)clean));

    bool ok = !cmd_exec(cmd);
    if (!ok) {
        msg(LL_Error, "Git checkout failed");
        return false;
    }
    if (cmd.items)
        free(cmd.items);
    return ok;
}

// :main :handler

typedef struct {
    char *name;
    char *path;
    void *handle;
    typeof(&setup) setup;
    typeof(&reload_requested) reload_requested;
    typeof(&run_install) run_install;
    typeof(&cleanup) cleanup;
} Installer;

typedef struct {
    Installer *items;
    size_t len;
    size_t cap;
} Installers;

void init_state()
{
    static char *cf[] = { "-ggdb", NULL, };

    min_level = LL_Info;
    ptrs = (typeof(ptrs)) { 0 };
    compiler = (typeof(compiler)) { .name = "gcc", .flags = (char**) &cf },
    dry = false;
}

void cleanup_state()
{
    if (ptrs.items) {
        for (size_t i = 0; i < ptrs.len; i += 1) {
            if (ptrs.items[i])
                free(ptrs.items[i]);
        }
        free(ptrs.items);
    }
}

// If i->handle is not NULL this is equivalent to reloading the installer.
__attribute__((nonnull))
bool load_installer(char *path, char *name, Installer *i)
{
    if (i->handle) {
        dlclose(i->handle);
    }
    *i = (Installer) { 0 };
    i->path = strdup(path);
    register_ptr(i->path);
    i->name = strdup(name);
    register_ptr(i->name);

    i->handle = dlopen(path, RTLD_NOW);
    if (!i->handle) {
        msg(LL_Error, "Failed to open installer: %s", dlerror());
        return false;
    }
    i->setup = dlsym(i->handle, "setup");
    i->reload_requested = dlsym(i->handle, "reload_requested");
    i->run_install = dlsym(i->handle, "run_install");
    if (!i->run_install) {
        msg(LL_Error, "Failed to find 'run_install' function: %s", dlerror());
        return false;
    }
    i->cleanup = dlsym(i->handle, "cleanup");

    return true;
}

Installers available_installers()
{
    Installers installers = { 0 };

    Strings ls_res;
    if (!ls(".", FF_Directory, &ls_res))
        die("ls failed:");

    Buffer install_name = { 0 };
    Buffer lib_name = { 0 };
    static const char *install_c = "/install.c";
    static const char *libinstaller = "/libinstaller.so";
    for (size_t i = 0; i < ls_res.len; i += 1) {
        if (ls_res.items[i][0] == '.')
            continue;

        install_name.len = 0;
        lib_name.len = 0;

        da_append_many(&install_name, ls_res.items[i], strlen(ls_res.items[i]));
        da_append_many(&install_name, install_c, strlen(install_c) + 1); // +1 copies \0
        if (!exists(install_name.items, FF_File))
            continue;

        da_append_many(&lib_name, ls_res.items[i], strlen(ls_res.items[i]));
        da_append_many(&lib_name, libinstaller, strlen(libinstaller) + 1); // +1 copies \0
        if (!compile_so(strsc(install_name.items), lib_name.items, NULL, NULL))
            continue;

        Installer inst = { 0 };
        if (!load_installer(lib_name.items, ls_res.items[i], &inst))
            continue;
        
        da_append(&installers, inst);
    }

    free(install_name.items);
    free(lib_name.items);

    return installers;
}

// Steps:
// 0. parse args
// 1. list all sub directories
//    - compile and collect installers
// 2. execute installers
// 3. cleanup
//
// IDEAS:
// - add support for package managers: Instead of having to call pacman the installer
//   can simply request the installation of some package or query some information
//   (version, ...)

int main()
{
    init_state();

    Installers installers = available_installers();

    printf("Available:\n");
    for (size_t i = 0; i < installers.len; i += 1) {
        printf("  - %s\n", installers.items[i].name);
    }

    // da_print(&ls_res, "%s", "\n");
    // int idx;
    // da_find(&ls_res, strcmp, "neovim", &idx);
    // printf("%d: %s\n", idx, ls_res.items[idx]);
    //
    // if (!compile_so(strsc("darkman/install.c"), "darkman/libinstall.so", NULL, NULL))
    //     die("compilation failed");
    // Installer darkman = { 0 };
    // if (!load_installer("darkman/libinstall.so", &darkman))
    //     die("Could not open the installer");
    // darkman.run_install();

    cleanup_state();
}
