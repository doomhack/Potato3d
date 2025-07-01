#include "../include/mainloop.h"
#include "../include/setup.h"

int main(int argc, char *argv[])
{
    Setup setup;
    MainLoop* loop = new MainLoop;

    setup.DoSetup();
    loop->Run();

    return 0;
}
