#include "application.h"

int main(void)
{
    Application application;
    if (!application.Initialize())
    {
        return 1;
    }

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(
        [](void* context) {
            Application* pApplication = reinterpret_cast<Application*>(context);
            pApplication->Run();
        },
        &application,
        0,
        true
    );
#else
    while (application.IsRunning())
    {
        application.Run();
    }
#endif

    application.Cleanup();

    return 0;
}