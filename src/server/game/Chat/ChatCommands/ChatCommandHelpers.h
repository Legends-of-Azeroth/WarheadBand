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

#ifndef WARHEAD_CHATCOMMANDHELPERS_H
#define WARHEAD_CHATCOMMANDHELPERS_H

#include "Language.h"
#include "StringFormat.h"
#include <optional>
#include <type_traits>
#include <variant>

class ChatHandler;

namespace Warhead::Impl::ChatCommands
{
    /***************** HELPERS *************************\
    |* These really aren't for outside use...          *|
    \***************************************************/

    static constexpr char COMMAND_DELIMITER = ' ';

    template <typename T, typename = void>
    struct tag_base
    {
        using type = T;
    };

    template <typename T>
    using tag_base_t = typename tag_base<T>::type;

    struct TokenizeResult {
        explicit operator bool() { return !token.empty(); }
        std::string_view token;
        std::string_view tail;
    };

    inline TokenizeResult tokenize(std::string_view args)
    {
        TokenizeResult result;
        if (size_t delimPos = args.find(COMMAND_DELIMITER); delimPos != std::string_view::npos)
        {
            result.token = args.substr(0, delimPos);
            if (size_t tailPos = args.find_first_not_of(COMMAND_DELIMITER, delimPos); tailPos != std::string_view::npos)
                result.tail = args.substr(tailPos);
        }
        else
            result.token = args;

        return result;
    }

    template <typename T, typename... Ts>
    struct are_all_assignable
    {
        static constexpr bool value = (std::is_assignable_v<T&, Ts> && ...);
    };

    template <typename... Ts>
    struct are_all_assignable<void, Ts...>
    {
        static constexpr bool value = false;
    };

    template <std::size_t index, typename T1, typename... Ts>
    struct get_nth : get_nth<index-1, Ts...> { };

    template <typename T1, typename... Ts>
    struct get_nth<0, T1, Ts...>
    {
        using type = T1;
    };

    template <std::size_t index, typename... Ts>
    using get_nth_t = typename get_nth<index, Ts...>::type;

    // this essentially models std::optional<std::string_view>, except it can also hold an error message
    // it has std::string_view's bool conversion and dereference operators
    //
    // monostate <-> unspecified error, typically end-of-string reached or parsing failed
    // std::string <-> specified error, typically character-not-found or invalid item link
    // std::string_view <-> success, string_view is remaining argument string
    struct ChatCommandResult
    {
        ChatCommandResult(std::nullopt_t) : _storage() {}
        ChatCommandResult(std::string const&) = delete;
        ChatCommandResult(std::string&& s) : _storage(std::in_place_type<std::string>, std::forward<std::string>(s)) {}
        ChatCommandResult(char const* c) : _storage(std::in_place_type<std::string>, c) {}
        ChatCommandResult(std::string_view s) : _storage(std::in_place_type<std::string_view>, s) {}

        ChatCommandResult(ChatCommandResult const&) = delete;
        ChatCommandResult(ChatCommandResult&&) = default;
        ChatCommandResult& operator=(ChatCommandResult const&) = delete;
        ChatCommandResult& operator=(ChatCommandResult&&) = default;

        std::string_view operator*() const { return std::get<std::string_view>(_storage); }
        bool IsSuccessful() const { return std::holds_alternative<std::string_view>(_storage); }
        explicit operator bool() const { return IsSuccessful(); }
        bool HasErrorMessage() const { return std::holds_alternative<std::string>(_storage); }
        std::string const& GetErrorMessage() const { return std::get<std::string>(_storage); }

        private:
            std::variant<std::monostate, std::string_view, std::string> _storage;
    };

    WH_GAME_API void SendErrorMessageToHandler(ChatHandler* handler, std::string_view str);
    WH_GAME_API std::string GetWarheadString(ChatHandler const* handler, WarheadStrings which);

    template <typename... Ts>
    std::string FormatWarheadString(ChatHandler const* handler, WarheadStrings which, Ts&&... args)
    {
        return Warhead::StringFormat(fmt::runtime(GetWarheadString(handler, which)), std::forward<Ts>(args)...);
    }
}

#endif