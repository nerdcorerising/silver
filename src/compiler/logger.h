
#pragma once

#include <cstdio>
#include <cstdarg>

namespace logging
{
    class Logger
    {
    private:
        static bool sEnabled;

    public:
        static void setEnabled(bool enabled)
        {
            sEnabled = enabled;
        }

        static bool isEnabled()
        {
            return sEnabled;
        }

        static void log(const char* format, ...)
        {
            if (!sEnabled) return;

            va_list args;
            va_start(args, format);
            vfprintf(stderr, format, args);
            va_end(args);
        }
    };

    // Static member initialization
    inline bool Logger::sEnabled = false;
}

// Convenience macro for logging
#define LOG(...) logging::Logger::log(__VA_ARGS__)
