#ifndef DEBUG_H
#define DEBUG_H

#include <string>
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <regex>

#define _RED_     "\033[31m"
#define _GREEN_   "\033[32m"
#define _BLUE_    "\033[34m"
#define _PURPLE_  "\033[35m"
#define _YELLOW_  "\033[33m"
#define _CYAN_    "\033[36m"
#define _CLEAR_   "\033[0m"
#define _BOLD_    "\033[1m"

std::string obtain_stack_frame();
std::string demangled_name(const std::string&);
std::string get_addr_from_symbol(const std::string&);

class fs_error_t : public std::runtime_error
{
private:
    std::string err_info;
    int err_code;

public:
    enum error_types_t {
        SUCCESS,
        SHA512SUM_CHECKSUM_ERROR,
        FILE_OPERATION_ERROR
    };

    explicit fs_error_t(error_types_t);
    [[nodiscard]] const char * what() const noexcept override;
    [[nodiscard]] int get_err_code() const noexcept { return err_code; }
};

class Sha512sumChecksumError final : public fs_error_t {
public:
    explicit Sha512sumChecksumError() : fs_error_t(SHA512SUM_CHECKSUM_ERROR) { }
};

class CannotOpenFile final : public fs_error_t {
public:
    explicit CannotOpenFile() : fs_error_t(FILE_OPERATION_ERROR) { }
};

class CannotDetermineFileSize final : public fs_error_t {
public:
    explicit CannotDetermineFileSize() : fs_error_t(FILE_OPERATION_ERROR) { }
};

class SeekingFailed final : public fs_error_t {
public:
    explicit SeekingFailed() : fs_error_t(FILE_OPERATION_ERROR) { }
};

class ReadFailed final : public fs_error_t {
public:
    explicit ReadFailed() : fs_error_t(FILE_OPERATION_ERROR) { }
};

class WriteFailed final : public fs_error_t {
public:
    explicit WriteFailed() : fs_error_t(FILE_OPERATION_ERROR) { }
};

namespace _log
{
    enum console_color_t { RED, GREEN, BLUE, PURPLE, YELLOW, CYAN, CLEAR, BOLD };

    bool is_console_supporting_colored_output();

    template <typename StreamType>
    void output_console_color(StreamType& ostream, console_color_t color)
    {
        if (!is_console_supporting_colored_output()) return;

        switch (color)
        {
            case RED:    ostream << _RED_;    break;
            case GREEN:  ostream << _GREEN_;  break;
            case BLUE:   ostream << _BLUE_;   break;
            case PURPLE: ostream << _PURPLE_; break;
            case YELLOW: ostream << _YELLOW_; break;
            case CYAN:   ostream << _CYAN_;   break;
            case CLEAR:  ostream << _CLEAR_;  break;
            case BOLD:   ostream << _BOLD_;   break;
        }
    }

    template < typename StreamType, typename ParamType >
    void output_to_stream(StreamType& ostream, const ParamType& param)
    {
        if constexpr (std::is_same_v<ParamType, console_color_t>)
        {
            output_console_color(ostream, param);
        }
        else
        {
            ostream << param << std::flush;
        }
    }

    template < typename StreamType, typename ParamType, typename... Args >
    void output_to_stream(StreamType& ostream, const ParamType& param, const Args&... args)
    {
        output_to_stream(ostream, param);
        (output_to_stream(ostream, args), ...);
    }

    enum log_level_t { LOG_NORMAL, LOG_ERROR };
    std::string get_current_date_time();

    // Log function for variadic parameters
    template<typename... Args>
    void log(const log_level_t level, const Args&... args)
    {
        auto& ostream = (level == LOG_ERROR) ? std::cerr : std::cout;
        output_to_stream(ostream, BLUE, BOLD, "[", get_current_date_time(), "]: ", CLEAR);
        output_to_stream(ostream, args...);
    }
}

#endif //DEBUG_H
