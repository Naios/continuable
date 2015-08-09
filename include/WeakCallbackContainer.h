
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

#ifndef _WEAK_CALLBACK_CONTAINER_H_
#define _WEAK_CALLBACK_CONTAINER_H_

#include <unordered_map>

#include <boost/any.hpp>
#include <boost/optional.hpp>

#include "Callback.h"

class WeakCallbackContainer
{
    std::shared_ptr<WeakCallbackContainer> self_reference;

    typedef std::size_t handle_t;

    std::size_t _handle;

    std::unordered_map<decltype(_handle), boost::any> _container;

    template<typename _CTy, typename... Args>
    struct ProxyFactory;

    template<typename _CTy, typename... Args>
    struct ProxyFactory<_CTy, ::fu::identity<Args...>>
    {
        // Creates a weak proxy callback which prevents invoking to an invalid context.
        // Removes itself from the owner with the given handler.
        static callback_of_t<_CTy> CreateProxy(std::weak_ptr<WeakCallbackContainer> const& weak_owner,
            std::size_t const _handle, weak_callback_of_t<_CTy> const& weak_callback)
        {
            return [=](Args&&... args)
            {
                // Try to get a pointer to the owner
                if (auto const owner = weak_owner.lock())
                    // And to the wrapped functional itself
                    if (auto const callback = weak_callback.lock())
                    {
                        // Invoke the original callback
                        (*callback)(std::forward<Args>(args)...);

                        // Unregister the callback
                        owner->InvalidateCallback(_handle);
                    }
            };
        }
    };

public:
    WeakCallbackContainer()
        : self_reference(this, [](decltype(this)) { }), _handle(0L) { }

    ~WeakCallbackContainer() = default;

    WeakCallbackContainer(WeakCallbackContainer const&) = delete;
    WeakCallbackContainer(WeakCallbackContainer&&) = delete;

    WeakCallbackContainer& operator= (WeakCallbackContainer const&) = delete;
    WeakCallbackContainer& operator= (WeakCallbackContainer&&) = delete;

    WeakCallbackContainer& Clear()
    {
        _container.clear();
        return *this;
    }

    /// Weak wraps the given callable.
    template<typename _CTy>
    auto Wrap(_CTy&& callback)
        -> callback_of_t<_CTy>
    {
        // Create the shared callback
        shared_callback_of_t<_CTy> shared_callback = make_shared_callback(std::forward<_CTy>(callback));

        // Create a weak proxy callback which removes the callback on execute
        auto const this_handle = _handle++;
        callback_of_t<_CTy> proxy =
            ProxyFactory<_CTy, ::fu::argument_type_of_t<_CTy>>::
                CreateProxy(self_reference, this_handle, shared_callback);

        _container.insert(std::make_pair(this_handle, boost::any(std::move(shared_callback))));
        return std::move(proxy);
    }

    /// Calls ::Wrap on the given callable,
    template<typename _CTy>
    inline auto operator()(_CTy&& callback)
        -> decltype(Wrap(std::declval<_CTy>()))
    {
        return Wrap(std::forward<_CTy>(callback));
    }

    boost::optional<handle_t> GetLastCallbackHandle() const
    {
        if (_handle == 0L)
            return boost::none;
        else
            return boost::make_optional(_handle);
    }

    WeakCallbackContainer& InvalidateCallback(handle_t const handle)
    {
        _container.erase(handle);
        return *this;
    }
};

#endif // _WEAK_CALLBACK_CONTAINER_H_
