#ifndef _INSTALLER_H_
#define _INSTALLER_H_

#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>

// NOTE: Everything marked as API is supplied by the sys-setup executable
#define API

// NOTE: Functions marked as INSTALLER_* are the responsibility of the specific installer
#define INSTALLER_NECESSARY
#define INSTALLER_OPTIONAL

#define ASYNC

INSTALLER_OPTIONAL  bool setup(char *your_dir);
// If this function returns `true` the shared object file will be reloaded. This
// is useful when the installer needs dependencies not linked through the direct
// compilation by `sys-setup`. The installer can simply compile itself again and
// request to be reloaded. `setup` will be called after the reload.
// Think of this as a multi-stage compilation process.
INSTALLER_OPTIONAL  bool reload_requested();
INSTALLER_NECESSARY bool run_install();
INSTALLER_OPTIONAL  void cleanup();

#define DA_INIT_CAP 8

// adapted from: https://github.com/tsoding/nob.h
#define da_append(da, item)                                                         \
    if (1) {                                                                        \
        if ((da)->len >= (da)->cap) {                                               \
            (da)->cap = (da)->cap == 0 ? DA_INIT_CAP : (da)->cap * 2;               \
            (da)->items = realloc((da)->items, (da)->cap * sizeof(*(da)->items));   \
            if (!(da)->items)                                                       \
                die("Reallocation failed:");                                        \
        }                                                                           \
        (da)->items[(da)->len++] = (item);                                          \
    }

#define da_append_many(da, new_items, new_items_len)                                \
    if (1) {                                                                        \
        if ((da)->len + (new_items_len) > (da)->cap) {                              \
            if ((da)->cap == 0) {                                                   \
                (da)->cap = DA_INIT_CAP;                                            \
            }                                                                       \
            while ((da)->len + (new_items_len) > (da)->cap) {                       \
                (da)->cap *= 2;                                                     \
            }                                                                       \
            (da)->items = realloc((da)->items, (da)->cap*sizeof(*(da)->items));     \
            if (!(da)->items)                                                       \
                die("Reallocation failed:");                                        \
        }                                                                           \
        memcpy((da)->items + (da)->len, (new_items),                                \
               (new_items_len)*sizeof(*(da)->items));                               \
        (da)->len += (new_items_len);                                               \
    }

// expects 0 for the correct result, this way cmp functions can be used
#define da_find(da, find_fn, find_data, idx_ptr)                                    \
    if (1) {                                                                        \
        *(idx_ptr) = -1;                                                            \
        for (ssize_t _i_ = 0; _i_ < (da)->len; _i_ += 1) {                          \
            if (0 == (find_fn)((find_data), (da)->items[_i_])) {                    \
                *(idx_ptr) = (typeof(*(idx_ptr)))_i_;                               \
                break;                                                              \
            }                                                                       \
        }                                                                           \
    }

#define da_print(da, item_fmt, delim)                                               \
    if (1) {                                                                        \
        for (size_t _i_ = 0; _i_ < (da)->len; _i_ += 1) {                           \
             printf(item_fmt delim, (da)->items[_i_]);                              \
        }                                                                           \
    }

#define src_loc() ((Source_Loc) { __FILE__, __FUNCTION__, __LINE__ })
#define SRCLOC_FMT "%s@%s:%zu"
#define SRCLOC_ARG(sl) (sl)->file, (sl)->function, (sl)->line

#define strs(...) { __VA_ARGS__, NULL }
#define strsc(...) (char*[])strs(__VA_ARGS__)
#define unreachable() die("unreachable")

#define INVALID_PID (-1)
#define INVALID_FILE_DES (-1)
#define INVALID_PROCESS \
    ((Process) { INVALID_PID, INVALID_FILE_DES, INVALID_FILE_DES, INVALID_FILE_DES })
#define PIPE_READ (0)
#define PIPE_WRITE (1)

typedef struct {
    const char *file;
    const char *function;
    const size_t line;
} Source_Loc;

typedef enum {
    LL_Trace,
    LL_Debug,
    LL_Info,
    LL_Warn,
    LL_Error,
    _LL_NUM,
} Log_Level;

typedef enum {
    FF_File      = 1 << 0,
    FF_Directory = 1 << 1,
    FF_Symlink   = 1 << 2,
    FF_Any       = ~0,
} File_Filter;

typedef struct {
    char **items;
    size_t len;
    size_t cap;
} Strings;

typedef Strings Cmd;

typedef struct {
    char *items;
    size_t len;
    size_t cap;
} Buffer;

typedef enum {
    IOR_none   = 0,
    IOR_stdout = 1 << 0,
    IOR_stderr = 1 << 1,
    IOR_stdin  = 1 << 2,
} IO_Redirect;

typedef pid_t Pid;
typedef int Fd;

// If io fd's are equal to INVALID_FD they are not redirected.
typedef struct {
    Pid id;
    Fd stdin_;
    Fd stdout_;
    Fd stderr_;
} Process;

// If fmt ends with ':' errno will also be printed using perror
__attribute__((format(printf, 2, 3))) __attribute__((noreturn))
API void die_loc(Source_Loc loc, char *fmt, ...);

#define die(fmt, ...)\
    die_loc(src_loc(), (fmt), ##__VA_ARGS__)

// If fmt ends with ':' errno will also be printed using perror
__attribute__((format(printf, 3, 4)))
API void msg_loc(Source_Loc loc, Log_Level ll, char *fmt, ...);

#define msg(ll, fmt, ...) \
    msg_loc(src_loc(), (ll), (fmt), ##__VA_ARGS__)

// Registered pointers are automatically freed at execution stop. Do not free
// them, otherwise a double free will happen.
// You may assume, that every pointer you receive through any function marked as
// API is already registered.
__attribute__((nonnull))
API void register_ptr(void *ptr);

// @see File_Filter
__attribute__((nonnull))
API int exists(char *path, int ff);
// @see File_Filter
__attribute__((nonnull))
API bool ls(char *dir, int ff, Strings *result);

__attribute__((nonnull))
API bool prcs_write(Process p, char *in);

API int prcs_await(Process p, Buffer *out, Buffer *err);

// @see IO_Redirect
API ASYNC Process cmd_execa(Cmd cmd, int redirects);

API int cmd_execw(Cmd cmd, char *in, Buffer *out, Buffer *err);

#define cmd_exec(cmd) \
    cmd_execw(cmd, NULL, NULL, NULL)

// The last entry in both cflags and lflags must be NULL. Use strs for convenience
__attribute__((nonnull(1, 2)))
API bool compile(char *file, char *out, char **cflags, char **lflags);

// The last entry in cfiles, cflags and lflags must be NULL. Use strs for convenience
__attribute__((nonnull(1, 2)))
API bool compile_so(char **cfiles, char *so, char **cflags, char **lflags);

__attribute__((nonnull))
API bool git_clone(char *repo, char *dest_dir, bool init_submodules);

__attribute__((nonnull))
API bool git_checkout(char *repo_dir, char *target);

// TODO: Args?
__attribute__((nonnull))
API bool git_clean(char *repo_dir);

#endif // _INSTALLER_H_
