
/*
 *  Copyright (C) 2015 Naios <naios-dev@live.de>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _CALLBACK_H_
#define _CALLBACK_H_

#include <functional>
#include <utility>
#include <memory>
#include <type_traits>

#include "functional_unwrap.hpp"

/// A general purpose void returing callback type (`std::function<void(Args...)>`).
template<typename... Args>
using Callback = std::function<void(Args...)>;

/// A callback wrapped in a std::shared_ptr
template<typename... Args>
using SharedCallback = std::shared_ptr<Callback<Args...>>;

/// A callback wrapped in a std::weak_ptr
template<typename... Args>
using WeakCallback = std::weak_ptr<Callback<Args...>>;

namespace detail
{
    template<typename Function>
    struct do_unwrap_callback;

    template<typename... _ATy>
    struct do_unwrap_callback<std::function<void(_ATy...)>>
    {
        typedef Callback<_ATy...> CallbackType;

        typedef SharedCallback<_ATy...> SharedCallbackType;

        typedef WeakCallback<_ATy...> WeakCallbackType;
    };

    template<typename _CTy>
    using unwrap_callback_t = do_unwrap_callback<::fu::function_type_of_t<typename std::decay<_CTy>::type>>;

    template<typename _CTy, typename... Args>
    struct WeakProxyFactory;

    template<typename _CTy, typename... Args>
    struct WeakProxyFactory<_CTy, std::weak_ptr<std::function<void(Args...)>>>
    {
        static Callback<Args...> CreateProxy(_CTy&& weak)
        {
            return [=](Args&&... args)
            {
                if (auto const callback = weak.lock())
                    (*callback)(std::forward<Args>(args)...);
            };
        }
    };

    template<typename _CTy, typename... Args>
    struct WeakProxyFactory<_CTy, std::shared_ptr<std::function<void(Args...)>>>
    {
        static Callback<Args...> CreateProxy(_CTy&& shared)
        {
            return WeakProxyFactory<std::weak_ptr<std::function<void(Args...)>>&&,
                std::weak_ptr<std::function<void(Args...)>>>::CreateProxy(std::forward<_CTy>(shared));
        }
    };

} // detail

/// Unwraps the callback type of the given functional object.
template<typename _CTy>
using callback_of_t = typename detail::unwrap_callback_t<_CTy>::CallbackType;

/// Unwraps the shared callback type of the given functional object.
template<typename _CTy>
using shared_callback_of_t = typename detail::unwrap_callback_t<_CTy>::SharedCallbackType;

/// Unwraps the weak callback type of the given functional object.
template<typename _CTy>
using weak_callback_of_t = typename detail::unwrap_callback_t<_CTy>::WeakCallbackType;

/// Creates a callback wrapped in a std::shared_ptr.
template<typename _CTy>
inline auto make_shared_callback(_CTy&& callback)
    -> shared_callback_of_t<_CTy>
{
    return std::make_shared<callback_of_t<_CTy>>
        (std::forward<callback_of_t<_CTy>>(callback));
};

/// Creates a weak callback which wraps the given shared or weak callback.
/// If the given managed callback expires the callback is not invoked anymore.
template<typename _CTy>
inline auto make_weak_wrapped_callback(_CTy&& callback)
    -> decltype(detail::WeakProxyFactory<_CTy, typename std::decay<_CTy>::type>::
        CreateProxy(std::declval<_CTy>()))
{
    return detail::WeakProxyFactory<_CTy, typename std::decay<_CTy>::type>::
        CreateProxy(std::forward<_CTy>(callback));
}

#endif // _CALLBACK_H_
