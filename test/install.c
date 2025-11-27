#include "../installer.h"

Setup_Result setup(Context ctx)
{
    assert(strcmp(ctx.name, "test") == 0);
    return setup_ok();
}

bool run_install() {
    Tree_Node dirs;
    tree("./test/test_dirs", FF_Any, 10, &dirs);
    cp_dir(&dirs, "./test/test_dirs_copy", NULL);
    return true;
}

void cleanup()
{

}
