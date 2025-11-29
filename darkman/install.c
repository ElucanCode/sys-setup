#include "../installer.h"
#include <string.h>

bool run_install()
{
    fail_if(!ensure_uptodate(strs("darkman")), "darkman not ready");
    fail_if(!cmd_exec(strs("systemctl", "--user", "enable", "--now", "darkman.service")),
            "Failed to start darkman service");

    Tree_Node cfg;
    fail_if(!tree("darkman/config/", FF_Any, 10, &cfg), "failed to collect config files");
    char *cfg_target = concat(getenv("XDG_CONFIG_HOME"), "darkman");
    fail_if(!cp_dir(&cfg, cfg_target, NULL), "failed to copy config files");

    Tree_Node lcl;
    fail_if(!tree("darkman/local-share/", FF_Any, 1, &lcl), "failed to collect scripts");
    char *lcl_target = concat(getenv("XDG_DATA_HOME"), "darkman");
    fail_if(!cp_dir(&lcl, lcl_target, NULL), "failed to copy scripts");

    return true;
}
