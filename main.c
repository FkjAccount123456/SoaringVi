#include "editor.h"
#include "utils.h"
#ifdef _WIN32
#define NOMINMAX
#define NOINTERFACE
#include <conio.h>
#else
#include <unistd.h>
#include <sys/ioctl.h>
#endif

#include <assert.h>

int main(int argc, char *argv[]) {
    u_init();
    init_window_vtable();

    editor e;
    editor_init(&e);

    editor_mainloop(&e);

    editor_free(&e);

    u_fina();
    return 0;
}
