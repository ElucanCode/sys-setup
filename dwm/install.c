#include "../installer.h"

static Context ctx;

Setup_Result setup(Context c)
{
    msg(LL_Info, c.reloaded ? "Successfully reloaded" : "Setting dwm up");
    ctx = c;
    Setup_Result res = { 0 };
    res.ok = true;
    if (!c.reloaded)
        res.request_reload = true;
    return res;
}

bool run_install(void)
{
    Cmd m = sudo(make(strs("clean", "install"), ctx.name));
    int res = cmd_exec(m);
    return 0 == res;
}
