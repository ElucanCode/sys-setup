#include "../installer.h"

bool run_install()
{
    fail_if(!ensure_uptodate(strs("neovim")), "neovim not ready");
    Tree_Node cfg;
    fail_if(!tree("neovim/config/", FF_Any, 10, &cfg), "failed to collect config files");
    char *target = concat(getenv("XDG_CONFIG_HOME"), "/nvim");
    fail_if(!cp_dir(&cfg, target, NULL), "failed to copy config files");
    // TODO: start neovim so plugins etc can be setup.
    return true;
}
