#include <debug.h>
#include <sstream>
#include <cstring>

const char * sysdarft_errors[] = {
    "Success",
    "No such module",
    "Module exists",
};

inline std::string init_error_msg(const fs_error_t::error_types_t types)
{
    std::ostringstream str;
    _log::output_to_stream(str, _log::RED, _log::BOLD,
        "FILE SYSTEM ERROR DETECTED! MORE INFORMATION FOLLOWS:\n", _log::YELLOW,
        "Exception Thrown: ", _log::RED, sysdarft_errors[types], ". ", _log::YELLOW, "System Error (errno) = ",
        _log::RED, errno, " (", strerror(errno), ")\n", _log::CLEAR,
        _log::GREEN,
        "Obtained stack frame (**note that meaningful strace usually starts from 3 and upwards**):\n",
        _log::CLEAR,
        obtain_stack_frame(), "\n", _log::CLEAR);
    return str.str();
}

fs_error_t::fs_error_t(const fs_error_t::error_types_t types, graceful_exit_t & handler)
    : std::runtime_error(init_error_msg(types)), exit_handler(handler), err_code(types)
{
    log(_log::LOG_NORMAL, "Exception generated!\n");
    err_info = std::runtime_error::what();
    log(_log::LOG_NORMAL, "Now executing exit handler...\n");
    exit_handler.graceful_exit_handler();
}

const char * fs_error_t::what() const noexcept
{
    return err_info.c_str();
}
