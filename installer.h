#ifndef _INSTALLER_H_
#define _INSTALLER_H_

#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>

// NOTE: Everything marked as API is supplied by the sys-setup executable.
#define API

// NOTE: Everything marked as INSTALLER_* is the responsibility of the specific installer
#define INSTALLER_NECESSARY
#define INSTALLER_OPTIONAL

#define ASYNC

#define DA_INIT_CAP (8)
#define DA_STRUCT(item_ty) struct { item_ty *items; size_t len, cap; }

#define zero(ty) ((ty) { 0 })

// adapted from: https://github.com/tsoding/nob.h
#define da_reserve(da, space) do {                                              \
    if ((da)->cap >= (da)->len + (space))                                       \
        break;                                                                  \
    if ((da)->cap == 0)                                                         \
        (da)->cap = DA_INIT_CAP;                                                \
    while ((da)->cap < (da)->len + (space))                                     \
        (da)->cap *= 2;                                                         \
    (da)->items = realloc((da)->items, (da)->cap * sizeof(*(da)->items));       \
    if (!(da)->items)                                                           \
        die("Reallocation failed:");                                            \
} while (0);

#define da_append(da, item) do {                                                \
    da_reserve((da), 1);                                                        \
    (da)->items[(da)->len++] = (item);                                          \
} while (0);

#define da_append_many(da, new_items, new_items_len) do {                       \
    da_reserve((da), (new_items_len));                                          \
    memcpy((da)->items + (da)->len, (new_items),                                \
           (new_items_len) * sizeof(*(da)->items));                             \
    (da)->len += (new_items_len);                                               \
} while (0);

#define da_expand(da, other_da) do {                                            \
    typeof(other_da) _other_ = (other_da);                                      \
    da_append_many(da, _other_.items, _other_.len);                             \
} while (0);

#define da_shift(da, idx, dir) do {                                             \
    if ((idx) < 0 || (idx) >= (da)->len) break;                                 \
    if ((dir) > 0) {                                                            \
        da_reserve((da), 1);                                                    \
        if ((da)->len < (da)->cap) {                                            \
            memmove((char*)(da)->items + ((idx) + 1) * sizeof(*(da)->items),    \
                    (char*)(da)->items + (idx) * sizeof(*(da)->items),          \
                    ((da)->len - (idx)) * sizeof(*(da)->items));                \
        }                                                                       \
        (da)->len++;                                                            \
    } else if ((dir) < 0) {                                                     \
        if ((idx) > 0) {                                                        \
            memmove((char*)(da)->items + ((idx) - 1) * sizeof(*(da)->items),    \
                    (char*)(da)->items + (idx) * sizeof(*(da)->items),          \
                    ((da)->len - (idx)) * sizeof(*(da)->items));                \
            (da)->len--;                                                        \
        }                                                                       \
    }                                                                           \
} while (0);

#define da_remove_swap(da, idx) do {                                            \
    size_t _i_ = (idx);                                                         \
    (da)->items[_i_] = (da)->items[--(da)->count];                              \
} while (0);

#define da_remove_shift(da, idx) do {                                           \
    if ((da)->len <= (idx))                                                     \
        break;                                                                  \
    da_shift((da), (idx) + 1, -1);                                              \
    (da)->len -= 1;                                                             \
} while (0);

#define da_insert_swap(da, idx, item)                                           \
    die("TODO")

#define da_insert_shift(da, idx, item) do {                                     \
    if ((da)->len <= (idx)) {                                                   \
        da_append((da), (item));                                                \
        break;                                                                  \
    }                                                                           \
    da_shift((da), (idx), 1);                                                   \
    (da)->items[idx] = (item);                                                  \
} while (0);

// expects 0 for the correct result, this way cmp functions can be used
#define da_find(da, find_fn, find_data, idx_ptr) do {                           \
    *(idx_ptr) = -1;                                                            \
    for (ssize_t _i_ = 0; _i_ < (da)->len; _i_ += 1) {                          \
        if (0 == (find_fn)((find_data), (da)->items[_i_])) {                    \
            *(idx_ptr) = (typeof(*(idx_ptr)))_i_;                               \
            break;                                                              \
        }                                                                       \
    }                                                                           \
} while (0);

#define da_print(da, item_fmt, delim) do {                                      \
    for (size_t _i_ = 0; _i_ < (da)->len; _i_ += 1) {                           \
         printf(item_fmt delim, (da)->items[_i_]);                              \
    }                                                                           \
} while (0);

#define unreachable() die("unreachable")
#define todo() die("todo")
#define fail_if(cond, fmt, ...)                                                 \
    if (cond) {                                                                 \
        msg(LL_Error, fmt, ##__VA_ARGS__);                                      \
        return false;                                                           \
    }

#define src_loc() ((Source_Loc) { __FILE__, __FUNCTION__, __LINE__ })
#define SRCLOC_FMT "%s@%s:%zu"
#define SRCLOC_ARG(sl) (sl)->file, (sl)->function, (sl)->line

#define INVALID_PID (-1)
#define INVALID_FILE_DES (-1)
#define INVALID_PROCESS \
    ((Process) { INVALID_PID, INVALID_FILE_DES, INVALID_FILE_DES, INVALID_FILE_DES })
#define PIPE_READ (0)
#define PIPE_WRITE (1)

// :installer

typedef struct {
    char *key;
    char *val;
} Arg;

typedef DA_STRUCT(Arg) Args;

// TODO: All these values could also be passed using defines when compiling the installer.
typedef struct {
    char *name;
    char *path;
    Args args;
    bool reloaded;
    void *reload_data;
} Context;

typedef struct {
    bool ok;
    // If `true` the shared object file will be reloaded and setup is being
    // called again.
    bool request_reload;
    void *reload_data;
    // TODO: support dependencies?
} SetupResult;

#define setup_ok() ((SetupResult) { .ok = true })
#define setup_reload(data) ((SetupResult) { .reload = true, .reload_data = (data) })

INSTALLER_OPTIONAL  SetupResult setup(Context ctx);

INSTALLER_NECESSARY bool run_install(void);

INSTALLER_OPTIONAL  void cleanup(void);

// :api

typedef DA_STRUCT(char*) Strings;

typedef DA_STRUCT(char) Buffer;

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

// Generally used as a include filter
typedef enum {
    FF_None      = 0,
    FF_File      = 1 << 0,
    FF_Directory = 1 << 1,
    FF_Symlink   = 1 << 2,
    FF_Hidden    = 1 << 3,    // Includes
    FF_Current   = 1 << 8,    // Includes the '.' directory
    FF_Parent    = 1 << 9,    // Includes the '..' directory
    FF_Any       = ~0,
} File_Filter;

typedef DA_STRUCT(struct ls_file { char *name; File_Filter kind; }) Ls_Files;

typedef struct tree_node Tree_Node;
typedef DA_STRUCT(Tree_Node) Tree_Nodes;
struct tree_node {
    char *name;
    Tree_Node *parent;
    enum tree_node_kind { TN_Node, TN_Leaf } kind;
    union {
        struct tree_node_inner {
            Tree_Nodes children;
        } inner;
        struct tree_node_leaf {
            File_Filter kind;
        } leaf;
    } as;
};

typedef bool (*Tree_Node_Filter_fn)(const Tree_Node *node);

typedef Strings Cmd;

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
    // Use macros as documented in `wait(2)`
    int status;
} Process;

typedef DA_STRUCT(Process) Processes;

// :utility

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
// them, otherwise a double free will happen. Multiple registrations of the same
// pointer won't be problematic.
// You may assume, that every pointer you receive through any function marked as
// API is already registered.
__attribute__((nonnull))
API void register_ptr(void *ptr);
__attribute__((nonnull))
API void register_ptrs(void **ptrs, size_t n);

// TODO: a nice way to register a dynamic array especially for for e.g. Strings

// Assumes, that the variadic always ends with `NULL` and only contains `char*`
__attribute__((nonnull(1)))
API Strings _strs(char *first, ...);

#define strs(first, ...) \
    _strs((first), ##__VA_ARGS__, NULL)

// :file :path :fs
// Functionality offered by libc:
// - chmod(path, mode)
// - mkdir(path, mode)
// - rmdir(path)
// - remove(path)
// - access(path, mode)

// @see File_Filter
__attribute__((nonnull))
API int exists(char *path, int ff);

// @see File_Filter
__attribute__((nonnull))
API bool ls(char *dir, int ff, Ls_Files *result);

// @see File_Filter
// As long as max_depth > 0: FF_Directory is implied.
API bool tree(char *dir, int ff, size_t max_depth, Tree_Node *result);

// Returns the number of errors. If 0 nothing went wrong.
API int rm(Strings paths);

__attribute__((nonnull))
API bool cp(char *from, char *to);

// API bool cp_tree(Tree_Node *from, char *to);

API bool cp_tree_filter(Tree_Node *from, char *to, Tree_Node_Filter_fn filter);

#define cp_tree(from, to) \
    cp_tree_filter((from), (to), NULL)

API bool write_all(Fd fd, Buffer bytes);

// On failure are `Buffer.items == NULL` and `Buffer.cap == 0`
API Buffer read_all(Fd fd);

// :command :process

// @see IO_Redirect
API ASYNC Process cmd_execa(Cmd cmd, int redirects);

__attribute__((nonnull))
API bool prcs_write(Process p, char *in);

// Return value of true just means, that the process is not running anymore.
// For information about how it stopped use p->status.
__attribute__((nonnull(1)))
API bool prcs_await(Process *p, Buffer *out, Buffer *err);

// Returns the exit code or -1 if anything went wrong and/or the process exited
// abnormally.
API int cmd_execw(Cmd cmd, char *in, Buffer *out, Buffer *err);

#define cmd_exec(cmd) \
    cmd_execw(cmd, NULL, NULL, NULL)

API Cmd sudo(Cmd cmd);

API Cmd make(Strings rules, char *in_dir);

__attribute__((nonnull))
API Cmd git_clone(char *repo, char *dest_dir, bool init_submodules);

__attribute__((nonnull))
API Cmd git_checkout(char *repo_dir, char *target);

__attribute__((nonnull))
API Cmd git_clean(char *repo_dir);

// :package :installation

API bool exe_exists(char *name);

// TODO: Not really sure how to integrate the pgk managers
API bool is_installed(char *pkg);

// Should be called in `setup`
API bool ensure_uptodate(Strings pkgs);

__attribute__((nonnull))
API bool install_pkg(char *name);

// :compilation

// The last entry in both cflags and lflags must be NULL. Use strs for convenience
__attribute__((nonnull(1, 2)))
API bool compile(char *file, char *out, Strings cflags, Strings lflags);

// The last entry in cfiles, cflags and lflags must be NULL. Use strs for convenience
__attribute__((nonnull(2)))
API bool compile_so(Strings cfiles, char *so, Strings cflags, Strings lflags);

// :net :http

__attribute__((nonnull))
API Buffer http_get(char *url);

__attribute__((nonnull))
API Buffer http_post(char *url, Buffer data);

#endif // _INSTALLER_H_
