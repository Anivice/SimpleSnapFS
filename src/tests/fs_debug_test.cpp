#include <debug.h>
#include <cstring>

void f3()
{
    log(_log::LOG_NORMAL, "Normal\n");
    log(_log::LOG_ERROR, "Error: errno = ", errno, " (", strerror(errno), ')', '\n');
    log(_log::LOG_NORMAL, _log::GREEN, _log::BOLD, "Normal\n", _log::CLEAR);
    throw fs_error_t(fs_error_t::SUCCESS);
}

void f2()
{
    f3();
}

void f1()
{
    f2();
}

int main()
{
    try {
        f1();
    } catch (fs_error_t & e) {
        log(_log::LOG_ERROR, e.what());
    }
}
