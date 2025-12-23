#include "../installer.h"

bool run_install(void)
{
    if (!cp("dwm/rofi-switchkeyboard", concat(getenv("HOME"), "/.local/bin/rofi-switchkeyboard")))
        msg(LL_Error, "Failed to install 'rofi-switchkeyboard' keyboard switching wont be usable");

    fail_if(cmd_exec(sudo(make(strs("clean", "install"), "dwm/dwm"))), "Failed to install dwm");
    fail_if(cmd_exec(sudo(make(strs("clean", "install"), "dwm/dwmblocks"))), "Failed to install dwmblocks");

    return true;
}
