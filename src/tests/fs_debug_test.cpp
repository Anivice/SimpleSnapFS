#include <debug.h>
#include <cstring>

class deconstruct_on_error : public graceful_exit_t {
public:
    void graceful_exit_handler() override {
        log(_log::LOG_NORMAL, "Exiting...\n");
    }

    ~deconstruct_on_error() override = default;
};

void f3()
{
    deconstruct_on_error handler;
    log(_log::LOG_NORMAL, "Normal\n");
    log(_log::LOG_ERROR, "Error: errno = ", errno, " (", strerror(errno), ')', '\n');
    log(_log::LOG_NORMAL, _log::GREEN, _log::BOLD, "Normal\n", _log::CLEAR);
    throw fs_error_t(fs_error_t::SUCCESS, handler);
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
