#ifndef IVY_LOG_H
#define IVY_LOG_H

#include "ivy/utils/utils.h"
#include <cstdio>
#include <cstdlib>

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
    static void info(const char *message) {
        info("%s", message);
    }

    /**
     * \brief Log an informational message with printf syntax
     * \tparam Args Arguments
     * \param format Message format
     * \param args Argument list for message
     */
    template<typename ... Args>
    static void info(const char *format, const Args &... args) {
        fprintf(stdout, "[%s][info] ", get_date_time_as_string().c_str());
        fprintf(stdout, format, args ...);
        fprintf(stdout, "\n");
        fflush(stdout);
    }

    /**
     * \brief Log a debug message
     * \param message The message to log
     */
    static void debug(const char *message) {
        debug("%s", message);
    }

    /**
     * \brief Log a debug message with printf syntax
     * \tparam Args Arguments
     * \param format Message format
     * \param args Argument list for message
     */
    template<typename ... Args>
    static void debug(const char *format, const Args &... args) {
        fprintf(stdout, "[%s][debug] ", get_date_time_as_string().c_str());
        fprintf(stdout, format, args ...);
        fprintf(stdout, "\n");
        fflush(stdout);
    }

    /**
     * \brief Log a warning message
     * \param message The message to log
     */
    static void warn(const char *message) {
        warn("%s", message);
    }

    /**
     * \brief Log a warning with printf-like syntax
     * \tparam Args Arguments
     * \param format Message format
     * \param args Argument list for message
     */
    template<typename ... Args>
    static void warn(const char *format, const Args &... args) {
        fprintf(stdout, "[%s][warn] ", get_date_time_as_string().c_str());
        fprintf(stdout, format, args ...);
        fprintf(stdout, "\n");
        fflush(stdout);
    }

    /**
     * \brief Log a fatal error and exit
     * \param message The message to log
     */
    [[noreturn]] static void fatal(const char *message) {
        fatal("%s", message);
    }

    /**
     * \brief Log a fatal error with printf-like syntax and exit
     * \tparam Args Arguments
     * \param format Message format
     * \param args Argument list for message
     */
    template<typename ... Args>
    [[noreturn]] static void fatal(const char *format, const Args &... args) {
        fprintf(stderr, "[%s][fatal] ", get_date_time_as_string().c_str());
        fprintf(stderr, format, args ...);
        fprintf(stderr, "\n");
        fflush(stderr);
        exit(1);
    }
};

}

#endif // IVY_LOG_H
