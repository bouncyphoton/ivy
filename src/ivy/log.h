#ifndef IVY_LOG_H
#define IVY_LOG_H

#include "ivy/utils/utils.h"
#include <iostream>

#ifdef _MSC_VER
    #define __PRETTY_FUNCTION__ __func__
#endif

/**
 * \brief Debug log what function we're in
 */
#define LOG_CHECKPOINT() ivy::Log::debug(__PRETTY_FUNCTION__)

namespace ivy {

/**
 * \brief Message logging
 */
class Log final {
public:
    /**
     * \brief Log an informational message
     * \param msg The message to log
     */
    static void info(const std::string &message) {
        info("%", message);
    }

    /**
     * \brief Log an informational message with % syntax
     * Example: ("%, %", 4, "hello") -> "4, hello"
     * \tparam Args Arguments
     * \param format Message format
     * \param args Argument list for message
     */
    template<typename ... Args>
    static void info(const std::string &format, const Args &... args) {
        std::ostream &stream = std::cout;

        stream << "[" << get_date_time_as_string() << "][info] ";
        printNext(stream, format, args...);
        stream << std::endl;
    }

    /**
     * \brief Log a debug message
     * \param message The message to log
     */
    static void debug(const std::string &message) {
        debug("%", message);
    }

    /**
     * \brief Log a debug message with % syntax
     * Example: ("%, %", 4, "hello") -> "4, hello"
     * \tparam Args Arguments
     * \param format Message format
     * \param args Argument list for message
     */
    template<typename ... Args>
    static void debug(const std::string &format, const Args &... args) {
        std::ostream &stream = std::cout;

        stream << "[" << get_date_time_as_string() << "][debug] ";
        printNext(stream, format, args...);
        stream << std::endl;
    }

    /**
     * \brief Log a warning message
     * \param message The message to log
     */
    static void warn(const std::string &message) {
        warn("%", message);
    }

    /**
     * \brief Log a warning with % syntax
     * Example: ("%, %", 4, "hello") -> "4, hello"
     * \tparam Args Arguments
     * \param format Message format
     * \param args Argument list for message
     */
    template<typename ... Args>
    static void warn(const std::string &format, const Args &... args) {
        std::ostream &stream = std::cerr;

        stream << "[" << get_date_time_as_string() << "][warn] ";
        printNext(stream, format, args...);
        std::cerr << std::endl;
    }

    /**
     * \brief Log a fatal error and exit
     * \param message The message to log
     */
    [[noreturn]] static void fatal(const std::string &message) {
        fatal("%", message);
    }

    /**
     * \brief Log a fatal error with % syntax and exit
     * Example: ("%, %", 4, "hello") -> "4, hello"
     * \tparam Args Arguments
     * \param format Message format
     * \param args Argument list for message
     */
    template<typename ... Args>
    [[noreturn]] static void fatal(const std::string &format, const Args &... args) {
        std::ostream &stream = std::cerr;

        stream << "[" << get_date_time_as_string() << "][fatal] ";
        printNext(stream, format, args...);
        stream << std::endl;
        exit(1);
    }

private:

    // Base case
    static void printNext(std::ostream &stream, const std::string &original_format) {
        std::string format = original_format;

        // Remove all backslashes that precede percent signs
        size_t pos;
        while ((pos = format.find("\\%")) != std::string::npos) {
            format.erase(pos, 1);
        }

        stream << format;
    }

    // Recursive print
    template<typename T, typename... Args>
    static void printNext(std::ostream &stream, const std::string &original_format, T value, const Args &... args) {
        size_t percentIdx = std::string::npos;
        std::string format = original_format;

        // Special case
        if (format.size() == 1 && format[0] == '%') {
            percentIdx = 0;
        }

        // Find first unescaped %
        for (size_t i = 1; i < format.size(); ++i) {
            if (format[i] == '%') {
                if (format[i - 1] == '\\') {
                    // Remove backslash and continue
                    format.erase(i, 1);
                    --i;
                } else {
                    // We found the next percent
                    percentIdx = i;
                    break;
                }
            }
        }

        if (percentIdx == std::string::npos) {
            // % not found, call base case
            printNext(stream, format);
        } else {
            // Otherwise, print up until %
            stream << format.substr(0, percentIdx);

            // Print our value
            stream << value;

            // Continue recursion until we run out of arguments
            printNext(stream, format.substr(percentIdx + 1), args...);
        }
    }
};

}

#endif // IVY_LOG_H
