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

#ifndef _CALLBACK_CONTAINER_H_
#define _CALLBACK_CONTAINER_H_

#include <unordered_map>

#include "Callback.h"

class CallbackContainer
{
    std::shared_ptr<CallbackContainer> self_reference;

    size_t handle;

    struct InternalReference
    {
    };

    std::unordered_map<decltype(handle), InternalReference> container;

    template<typename _CTy, typename... Args>
    struct ProxyFactory;

    template<typename _CTy, typename... Args>
    struct ProxyFactory<_CTy, std::tuple<Args...>>
    {
        // Creates a weak callback proxy which prevents invoking to an invalid context.
        static callback_of_t<_CTy> CreateProxy(_CTy&& callback)
        {
            return [callback](Args&&... args)
            {

                // Invoke the original callback
                callback(std::forward<Args...>(args...));
            };
        }
    };

public:
    CallbackContainer()
        : self_reference(this, [](decltype(this) me) { }), handle(0L) { }

    ~CallbackContainer() = default;

    CallbackContainer(CallbackContainer const&) = delete;
    CallbackContainer(CallbackContainer&&) = delete;

    CallbackContainer& operator= (CallbackContainer const&) = delete;
    CallbackContainer& operator= (CallbackContainer&&) = delete;

    CallbackContainer& Clear()
    {
        container.clear();
        handle = 0L;
        return *this;
    }

    template<typename _CTy>
    auto operator()(_CTy&& callback)
        -> callback_of_t<_CTy>
    {
        // Create a weak proxy callback which removes the callback on execute
        callback_of_t<_CTy> proxy =
            ProxyFactory<_CTy, ::fu::argument_type_of_t<_CTy>>::
                CreateProxy(std::forward<_CTy>(callback));

        return std::move(proxy);
    }
};

#endif /// _CALLBACK_CONTAINER_H_
