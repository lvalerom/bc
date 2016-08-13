/*
Copyright 2016 Luis Valero Martin

This file is part of BC.
BC is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
any later version.
BC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with Through the galaxy.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __LOG_H
#define __LOG_H

#undef log_trace
#undef log_debug
#undef log_info
#undef log_warning
#undef log_error
#undef log_fatal
#undef log_setup

#ifdef NLOG

#define log_trace ((void)0)
#define log_debug ((void)0)
#define log_info ((void)0)
#define log_warning ((void)0)
#define log_error ((void)0)
#define log_fatal ((void)0)
#define log_setup ((void)0)

#else

#define log_trace Log::trace
#define log_debug Log::debug
#define log_info Log::info
#define log_warning Log::warning
#define log_error Log::error
#define log_fatal Log::fatal
#define log_setup Log::set_up

namespace Log
{
    namespace level
    {
        enum ELogLevel
        {
            TRACE = 0,
            DEBUG = 1,
            INFO = 2,
            WARNING = 3,
            _ERROR = 4,
            FATAL = 5
        };
    }

    void set_up(level::ELogLevel lvl = level::FATAL, const char* file = 0, const char* conf = 0);

    void trace(const char* format, ...);
    void debug(const char* format, ...);
    void info(const char* format, ...);
    void warning(const char* format, ...);
    void error(const char* format, ...);
    void fatal(const char* format, ...);
}

#endif // NLOG

#endif
