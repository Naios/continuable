/*
 * Copyright (C) 2008-2015 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _CALLBACK_H_
#define _CALLBACK_H_

#include <functional>
#include <utility>
#include <memory>

#include "functional_unwrap.hpp"

template<class... Args>
using Callback = std::function<void(Args...)>;

template<class... Args>
using SharedCallback = std::shared_ptr<Callback<Args...>>;

template<class... Args>
using WeakCallback = std::weak_ptr<Callback<Args...>>;

namespace detail
{
    template<class... Args>
    struct do_unwrap_callback;

    template<class... Args>
    struct do_unwrap_callback<std::tuple<Args...>>
    {
        typedef Callback<Args...> CallbackType;

        typedef SharedCallback<Args...> SharedCallbackType;

        typedef WeakCallback<Args...> WeakCallbackType;
    };

    template<class _CTy>
    using unwrap_callback = do_unwrap_callback<::fu::argument_type_of_t<_CTy>>;

} // detail

template<class _CTy>
using callback_of_t = typename detail::unwrap_callback<_CTy>::CallbackType;

template<class _CTy>
using shared_callback_of_t = typename detail::unwrap_callback<_CTy>::SharedCallbackType;

template<class _CTy>
using weak_callback_of_t = typename detail::unwrap_callback<_CTy>::WeakCallbackType;

template<class _CTy>
inline typename shared_callback_of_t<_CTy>
    make_shared_callback(_CTy&& callback)
{
    return std::make_shared<typename callback_of_t<_CTy>>
        (std::forward<typename callback_of_t<_CTy>>(callback));
}

#endif /// _CALLBACK_H_
