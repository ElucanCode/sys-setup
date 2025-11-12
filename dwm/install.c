#include "../installer.h"

static Context ctx;

SetupResult setup(Context c)
{
    msg(LL_Info, "Setting dwm up");
    ctx = c;
    SetupResult res = { 0 };
    res.ok = true;
    if (!c.reloaded)
        res.request_reload = true;
    return res;
}

bool run_install(void)
{
    Cmd m = sudo(make(strs("clean", "install"), ctx.name));
    // Cmd make = { 0 };
    // da_append(&make, "make");
    // da_append(&make, "-C");
    // da_append(&make, ctx.name);
    // da_append(&make, "clean");
    // da_append(&make, "dwm");
    int res = cmd_exec(m);
    return 0 == res;
}
