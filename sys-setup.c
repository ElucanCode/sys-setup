//usr/bin/env gcc -Wall -rdynamic "$0" -o sys-setup -ldl && exec ./sys-setup "$@"

#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "installer.h"

#define BUFFER_SIZE 1024

// :state

enum {
// Package managers
    Pacman   = 1 << 0,  /* implies libalpm */

// Programs
    Git      = 1 << 8,
    Curl     = 1 << 9,  /* implies libcurl */
};

typedef struct {
    Log_Level min_level;
    DA_STRUCT(void*) ptrs;
    char *cc;
    Strings cflags;

    // Only execute one installer after the previous is done
    bool in_sequence;
    bool dry;
    // Allow or forbid direct networking
    bool direct_net;
} State;

static State state;

// :helper

static int _strcmp(const void *a, const void *b)
{
    return strcmp(*(char**)a, *(char**)b);
}

static int _ptrcmp(const void *a, const void *b)
{
    return a - b;
}

// :installer.h :implementation
// :utility

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

    if (ll < state.min_level)
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
    // TODO: check if ptr is somewhere in the state OR if it already is registered
    da_append(&state.ptrs, ptr);
}

Strings _strs(char *first, ...)
{
    Strings res = zero(Strings);

    va_list args;
    va_start(args, first);
    char *cur = first;
    do {
        cur = strdup(cur);
        da_append(&res, cur);
    } while ((cur = va_arg(args, char*)));
    va_end(args);

    da_expand(&state.ptrs, res);
    register_ptr(res.items);
    return res;
}

// :file :path :fs

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

    *result = zero(Strings);
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
    da_expand(&state.ptrs, *result);
    register_ptr(result->items);
    qsort(result->items, result->len, sizeof(*result->items), _strcmp);

    return true;
}

int rm(Strings paths)
{
    int errs = 0;
    for (size_t i = 0; i < paths.len; i += 1) {
        if (-1 == remove(paths.items[i]))  {
            msg(LL_Error, "Failed to remove '%s':", paths.items[i]);
            errs += 1;
        }
    }
    return errs;
}

bool cp(char *from, char *to)
{
    Fd rfd = open(from, O_RDONLY);
    if (rfd == INVALID_FILE_DES) {
        msg(LL_Error, "Failed to open %s:", from);
        return false;
    }
    Fd wfd = open(to, O_CREAT | O_WRONLY | O_TRUNC);
    if (wfd == INVALID_FILE_DES) {
        msg(LL_Error, "Failed to open %s:", to);
        close(rfd);
        return false;
    }

    bool ok = false;
    Buffer bytes = read_all(rfd);
    if (bytes.items)
        ok = write_all(wfd, bytes);

    close(rfd);
    close(wfd);
    if (bytes.items)
        free(bytes.items);
    return ok;
}

bool write_all(Fd fd, Buffer bytes)
{
    size_t written = 0;
    while (written < bytes.len) {
        ssize_t w = write(fd, bytes.items + written, bytes.len - written);
        if (-1 == w) {
            msg(LL_Error, "Failed to write to fd:");
            return false;
        }
        written += w;
    }
    return true;
}

Buffer read_all(Fd fd)
{
    char rbuf[BUFFER_SIZE];
    Buffer buf = zero(Buffer);
    bool error = false;
    bool exit = false;
    do {
        ssize_t r = read(fd, rbuf, sizeof(rbuf));
        switch (r) {
        case -1:
            msg(LL_Error, "Failed to read fd:");
            error = true;
        case 0:
            exit = true;
            break;
        default:
            da_append_many(&buf, rbuf, r);
            break;
        }
    } while (!exit);

    if (error) {
        if (buf.items)
            free(buf.items);
        buf = zero(Buffer);
    } else {
        register_ptr(buf.items);
    }
    
    return buf;
}

// :command :process

Process cmd_execa(Cmd cmd, int redirects)
{
    Buffer cmd_str = zero(Buffer);
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

bool prcs_await(Process *p, Buffer *out, Buffer *err)
{
    if (-1 == waitpid(p->id, &p->status, 0)) {
        msg(LL_Error, "Wait failed:");
        return false;
    }

    if (!out && !err)
        return true;

    char buf[BUFFER_SIZE];
    Fd fds[2] = { p->stdout_, p->stderr_ };
    Buffer *bs[2] = { out, err };

    for (size_t i = 0; i < 2; i += 1) {
        if (fds[i] == INVALID_FILE_DES || bs[i] == NULL)
            continue;

        bool exit = false;
        do {
            ssize_t r = read(fds[i], buf, sizeof(buf));
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

    return true;
}

int cmd_execw(Cmd cmd, char *in, Buffer *out, Buffer *err)
{
    int redirects = IOR_none;
    if (in)  redirects |= IOR_stdin;
    if (out) redirects |= IOR_stdout;
    if (err) redirects |= IOR_stderr;

    Process p = cmd_execa(cmd, redirects);
    if (p.id == INVALID_PID)
        return -1;
    if (in) {
        // NOTE: Ignoring error because we still need to wait for the process
        prcs_write(p, in);
    }
    if (!prcs_await(&p, out, err))
        return -1;
    return WIFEXITED(p.status) ? WEXITSTATUS(p.status) : -1;
}

Cmd sudo(Cmd cmd)
{
    da_insert_shift(&cmd, 0, "sudo");
    return cmd;
}

Cmd make(Strings rules, char *in_dir)
{
    Cmd cmd = zero(Cmd);
    da_append(&cmd, strdup("make"));
    if (in_dir)
        da_expand(&cmd, strs("-C", in_dir));
    da_expand(&cmd, rules);
    return cmd;
}

Cmd git_clone(char *repo, char *dest_dir, bool init_submodules)
{
    todo();
}

Cmd git_checkout(char *repo_dir, char *target)
{
    todo();
}

Cmd git_clean(char *repo_dir)
{
    todo();
}

// :package :installation

bool is_installed(char *pkg)
{
    todo();
}

bool ensure_uptodate(Strings pkgs)
{
    todo();
}

bool install_pkg(char *name)
{
    todo();
}

// :compilation

bool compile(char *file, char *out, Strings cflags, Strings lflags)
{
    Cmd cmd = zero(Cmd);
    da_append(&cmd, state.cc);
    da_expand(&cmd, state.cflags);
    if (cflags.len)
        da_expand(&cmd, cflags);
    da_expand(&cmd, strs(file, "-o", out));
    if (lflags.len)
        da_expand(&cmd, lflags);

    Buffer err = zero(Buffer);
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

bool compile_so(Strings cfiles, char *so, Strings cflags, Strings lflags)
{
    // TODO: check if the so file already exists and is up to date
    bool ok = true;

    Strings objs = zero(Strings);
    for (size_t i = 0; i < cfiles.len; i += 1) {
        char *obj;
        asprintf(&obj, "%s%s", cfiles.items[i], ".o");
        da_append(&objs, obj);
        
        // TODO: also use passed cflags and lflags
        ok = compile(cfiles.items[i], obj, strs("-c", "-fPIC"), zero(Strings));
        if (!ok)
            goto exit;
    }

    Cmd cmd = zero(Cmd);
    da_append(&cmd, state.cc);
    da_expand(&cmd, state.cflags);
    if (cflags.len)
        da_expand(&cmd, cflags);
    da_expand(&cmd, strs("-shared", "-o", so));
    da_expand(&cmd, objs);
    if (lflags.len)
        da_expand(&cmd, lflags);

    Buffer err = zero(Buffer);
    ok = !cmd_execw(cmd, NULL, NULL, &err);
    if (!ok)
        msg(LL_Error, "Compilation of '%s' failed:\n%.*s", so, (int) err.len, err.items);
    if (err.items)
        free(err.items);

exit:
    if (objs.items) {
        da_expand(&state.ptrs, objs);
        register_ptr(objs.items);
    }
    return ok;
}

// :net :http

Buffer http_get(char *url)
{
    todo();
}

Buffer http_post(char *url, Buffer data)
{
    todo();
}

// :main :handler

typedef struct {
    char *name;
    char *path;
    void *handle;
    typeof(&setup) setup;
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
    msg(LL_Debug, "Initializing state");
    state = (State) {
        .min_level = LL_Trace,
        .ptrs = zero(typeof(state.ptrs)),
        .cc = "gcc", 
        .cflags = strs("-ggdb"),
        .dry =  false,
    };
}

void cleanup_state()
{
    msg(LL_Debug, "Cleaning state");
    if (state.ptrs.items) {
        qsort(state.ptrs.items, state.ptrs.len, sizeof(void*), _ptrcmp);
        for (size_t i = 0; i < state.ptrs.len; i += 1) {
            if (i > 0 && state.ptrs.items[i - 1] == state.ptrs.items[i])
                continue;
            if (state.ptrs.items[i])
                free(state.ptrs.items[i]);
        }
        free(state.ptrs.items);
    }
}

// If i->handle is not NULL this is equivalent to reloading the installer.
__attribute__((nonnull))
bool load_installer(char *path, char *name, Installer *i)
{
    msg(LL_Debug, "Loading: %s at %s", name, path);
    if (i->handle) {
        dlclose(i->handle);
    }
    *i = zero(Installer);
    i->path = strdup(path);
    register_ptr(i->path);
    i->name = strdup(name);
    register_ptr(i->name);

    i->handle = dlopen(i->path, RTLD_NOW);
    if (!i->handle) {
        msg(LL_Error, "Failed to open installer: %s", dlerror());
        return false;
    }
    i->setup = dlsym(i->handle, "setup");
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
    Installers installers = zero(Installers);

    Strings ls_res;
    if (!ls(".", FF_Directory, &ls_res))
        die("ls failed:");

    Buffer source = zero(Buffer);
    Buffer target = zero(Buffer);
    static const char *install_c = "/install.c";
    static const char *libinstaller = "/libinstaller.so";
    for (size_t i = 0; i < ls_res.len; i += 1) {
        if (ls_res.items[i][0] == '.')
            continue;

        source.len = 0;
        target.len = 0;

        da_append_many(&source, ls_res.items[i], strlen(ls_res.items[i]));
        da_append_many(&source, install_c, strlen(install_c) + 1); // +1 copies \0
        if (!exists(source.items, FF_File))
            continue;

        da_append_many(&target, ls_res.items[i], strlen(ls_res.items[i]));
        da_append_many(&target, libinstaller, strlen(libinstaller) + 1); // +1 copies \0

        if (!compile_so(strs(source.items), target.items, zero(Strings), zero(Strings)))
            continue;

        Installer inst = zero(Installer);
        if (!load_installer(target.items, ls_res.items[i], &inst))
            continue;
        
        da_append(&installers, inst);
    }

    free(source.items);
    free(target.items);

    return installers;
}

// sets name and path in ctx if inst->setup is defined
bool run_installer(Installer *inst, Context ctx)
{
    if (inst->setup) {
        ctx.name = inst->name;
        ctx.path = inst->path;
        SetupResult res = inst->setup(ctx);

        // TODO: should res.ok take priority over res.request_reload?
        if (res.request_reload) {
            ctx.reloaded = true;
            ctx.reload_data = res.reload_data;
            if (!load_installer(inst->path, inst->name, inst)) {
                msg(LL_Error, "Installer reload failed");
                return false;
            }
            return run_installer(inst, ctx);
        }

        if (!res.ok) {
            msg(LL_Error, "Setup for installer %s failed", inst->name);
            return false;
        }
    }

    bool ok = inst->run_install();

    if (inst->cleanup)
        inst->cleanup();
    
    return ok;
}

// Steps:
// 0. parse args
// 1. list all sub directories
//    - compile and collect installers
// 2. execute installers
// 3. cleanup
//
// IDEAS:
// - dwnld(url, path) requests url and stores result in the file at path
// - add support for package managers: Instead of having to call pacman the installer
//   can simply request the installation of some package or query some information
//   (version, ...)

int main()
{
    init_state();

    // Simple test example
    // Strings ls_res;
    // if (!ls(".", FF_Any, &ls_res))
    //     die("ls failed");
    // da_print(&ls_res, "%s", "\n");
    // if (!compile_so(strs("darkman/install.c"), strdup("darkman/libinstaller.so"), zero(Strings), zero(Strings)))
    //     die("compilation failed");
    // Installer darkman = zero(Installer);
    // if (!load_installer("darkman/libinstaller.so", "darkman", &darkman))
    //     die("load failed");
    // if (!run_installer(&darkman, zero(Context)))
    //     die("run failed");
    // msg(LL_Info, "ok");

    Installers installers = available_installers();

    printf("Available:\n");
    for (size_t i = 0; i < installers.len; i += 1) {
        printf("  - %s\n", installers.items[i].name);
    }

    printf("Running installers:\n");
    for (size_t i = 0; i < installers.len; i += 1) {
        printf(":: %s \n", installers.items[i].name);
        run_installer(&installers.items[i], (Context) { 0 });
    }

    cleanup_state();
}
