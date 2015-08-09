
/**
 * Copyright 2015 Denis Blank <denis.blank@outlook.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _CALLBACK_H_
#define _CALLBACK_H_

#include <functional>
#include <utility>
#include <memory>
#include <type_traits>
#include <utility>

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
}

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
