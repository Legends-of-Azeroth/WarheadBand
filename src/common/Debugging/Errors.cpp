/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "Errors.h"
#include <cstdlib>
#include <fmt/core.h>

/**
    @file Errors.cpp

    @brief This file contains definitions of functions used for reporting critical application errors

    It is very important that (std::)abort is NEVER called in place of *((volatile int*)NULL) = 0;
    Calling abort() on Windows does not invoke unhandled exception filters - a mechanism used by WheatyExceptionReport
    to log crashes. exit(1) calls here are for static analysis tools to indicate that calling functions defined in this file
    terminates the application.
 */

#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
#include <Windows.h>
#define Crash(message) \
    ULONG_PTR execeptionArgs[] = { reinterpret_cast<ULONG_PTR>(strdup(message)), reinterpret_cast<ULONG_PTR>(_ReturnAddress()) }; \
    RaiseException(EXCEPTION_ASSERTION_FAILURE, 0, 2, execeptionArgs);
#else

// should be easily accessible in gdb
extern "C"
{
    WH_COMMON_API char const* WarheadAssertionFailedMessage = nullptr;
}

#define Crash(message) \
    WarheadAssertionFailedMessage = strdup(message); \
    *((volatile int*)nullptr) = 0; \
    exit(1);
#endif

namespace
{
    /**
    * @name MakeMessage
    * @brief Make message for display erros
    * @param messageType Message type (ASSERTION FAILED, FATAL ERROR, ERROR) end etc
    * @param file Path to file
    * @param line Line number in file
    * @param function Functionn name
    * @param message Condition to string format
    * @param fmtMessage [optional] Display format message after condition
    * @param debugInfo [optional] Display debug info
    */
    std::string MakeMessage(std::string_view messageType, std::string_view file, int line, std::string_view function,
        std::string_view message, std::string_view fmtMessage = {}, std::string_view debugInfo = {})
    {
        std::string msg = Warhead::StringFormat("\n>> {}\n\n# Location: {}:{}\n# Function: {}\n# Condition: {}\n", messageType, file, line, function, message);

        if (!fmtMessage.empty())
            msg.append(Warhead::StringFormat("# Message: {}\n", fmtMessage));

        if (!debugInfo.empty())
            msg.append(Warhead::StringFormat("\n# Debug info: {}\n", debugInfo));

        return Warhead::StringFormat(
            "#{0:-^{2}}#\n"
            " {1: ^{2}} \n"
            "#{0:-^{2}}#\n", "", msg, 70);
    }

    /**
    * @name MakeAbortMessage
    * @brief Make message for display errors
    * @param file Path to file
    * @param line Line number in file
    * @param function Function name
    * @param fmtMessage [optional] Display format message after condition
    */
    std::string MakeAbortMessage(std::string_view file, int line, std::string_view function, std::string_view fmtMessage = {})
    {
        std::string msg = Warhead::StringFormat("\n>> ABORTED\n\n# Location: {}:{}\n# Function: {}\n", file, line, function);

        if (!fmtMessage.empty())
            msg.append(Warhead::StringFormat("# Message: {}\n", fmtMessage));

        return Warhead::StringFormat(
            "#{0:-^{2}}#\n"
            " {1: ^{2}} \n"
            "#{0:-^{2}}#\n", "", msg, 70);
    }
}

void Warhead::Assert(std::string_view file, int line, std::string_view function, std::string const& debugInfo, std::string_view message, std::string_view fmtMessage /*= {}*/)
{
    std::string formattedMessage = MakeMessage("ASSERTION FAILED", file, line, function, message, fmtMessage, debugInfo);
    fmt::print(stderr, "{}", formattedMessage);
    Crash(formattedMessage.c_str())
}

void Warhead::Abort(std::string_view file, int line, std::string_view function, std::string_view fmtMessage /*= {}*/)
{
    std::string formattedMessage = MakeAbortMessage(file, line, function, fmtMessage);
    fmt::print(stderr, "{}", formattedMessage);
    Crash(formattedMessage.c_str())
}

void Warhead::AbortHandler(int sigval)
{
    // nothing useful to log here, no way to pass args
    std::string formattedMessage = StringFormat("Caught signal {}\n", sigval);
    fmt::print(stderr, "{}", formattedMessage);
    fflush(stderr);
    Crash(formattedMessage.c_str())
}

std::string GetDebugInfo()
{
    return "";
}
