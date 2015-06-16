
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

#ifndef _CONTINUABLE_H_
#define _CONTINUABLE_H_

#include "Callback.h"

#include <iostream>
#include <typeinfo>
#include <string>
template <typename T>
void log_type(T t, std::string const& msg = "")
{
    std::cout << msg << ": " << typeid(t).name() << std::endl;
}
void debug(std::string const& m)
{
    std::cout << m << std::endl;
}


namespace detail
{
    // ContinuableState forward declaration.
    /// The internal state of the continuable
    /// which is used to save certain internal types.
    template<typename... Args>
    struct ContinuableState;

    // ContinuableImpl forward declaration.
    template<typename _STy, typename _CTy>
    class _ContinuableImpl;

    // convert_void_to_continuable forward declaration.
    /// Corrects void return types from functional types which should be
    /// Continuable<DefaultContinuableState, Callback<>>
    template<typename _RTy>
    struct convert_void_to_continuable;

    // unary_chainer forward declaration.
    template<typename _NextRTy, typename... _NextATy>
    struct unary_chainer;

    // creates an empty callback.
    template<typename _FTy>
    struct create_empty_callback_factory;

    template<typename... _Cain, typename _Proxy>
    struct ContinuableState<std::tuple<_Cain...>, _Proxy>
    {
    };

    typedef ContinuableState<std::tuple<>, void> DefaultContinuableState;

    // MSVC 12 has issues to detect the parameter pack otherwise.
    template<typename _NextRTy, typename... _NextATy>
    struct unary_chainer<_NextRTy, fu::identity<_NextATy...>>
    {
        typedef convert_void_to_continuable<_NextRTy> base;

        typedef typename convert_void_to_continuable<_NextRTy>::type result_t;

        typedef typename result_t::CallbackFunction callback_t;
    };

    template <typename _CTy>
    using unary_chainer_t = unary_chainer<
        fu::return_type_of_t<typename std::decay<_CTy>::type>,
        fu::argument_type_of_t<typename std::decay<_CTy>::type>>;

    template<typename... Args>
    struct create_empty_callback_factory<std::function<void(std::function<void(Args...)>&&)>>
    {
        static auto create()
            -> Callback<Args...>
        {
            return [](Args...)
            {
            };
        }
    };

    template<>
    struct convert_void_to_continuable<void>
    {
        typedef _ContinuableImpl<DefaultContinuableState, Callback<>> type;

        template<typename Fn, typename... Args>
        static type invoke(Fn functional, Args... args)
        {
            // Invoke the void returning functional
            functional(std::forward<Args>(args)...);

            // Return a fake void continuable
            return type([](Callback<>&&)
            {
            });
        }
    };

    template<typename _State, typename _CTy>
    struct convert_void_to_continuable<_ContinuableImpl<_State, _CTy>>
    {
        typedef _ContinuableImpl<_State, _CTy> type;

        template<typename Fn, typename... Args>
        static type invoke(Fn functional, Args... args)
        {
            return functional(std::forward<Args>(args)...);
        }
    };

    template<typename... _STy, typename... _ATy>
    class _ContinuableImpl<ContinuableState<_STy...>, std::function<void(_ATy...)>>
    {
        // Make all instances of _ContinuableImpl to a friend.
        template<typename, typename>
        friend class _ContinuableImpl;

    public:
        typedef Callback<_ATy...> CallbackFunction;
        typedef std::function<void(Callback<_ATy...>&&)> ForwardFunction;

    private:
        /// Functional which expects a callback that is inserted from the Continuable
        /// to chain everything together
        ForwardFunction _callback_insert;

        bool _released;

    public:
        // Empty for debugging
        /*_ContinuableImpl()
            : _released(true), _callback_insert() { }*/

        /// Deleted copy construct
        _ContinuableImpl(_ContinuableImpl const&) = delete;

        /// Move construct
        _ContinuableImpl(_ContinuableImpl&& right)
            : _released(right._released), _callback_insert(std::move(right._callback_insert))
        {
            right._released = true;
        }

        // Construct through a ForwardFunction
        template<typename _FTy>
        _ContinuableImpl(_FTy&& callback_insert)
            : _callback_insert(std::forward<_FTy>(callback_insert)), _released(false) { }

        template<typename _RSTy, typename _RCTy, typename _FTy>
        _ContinuableImpl(_FTy&& callback_insert, _ContinuableImpl<_RSTy, _RCTy>&& right)
            : _callback_insert(std::forward<_FTy>(callback_insert)), _released(right._released)
        {
            right._released = true;
        }

        /// Destructor which calls the dispatch chain if needed.
        ~_ContinuableImpl()
        {
            // Dispatch everything.
            if (!_released)
            {
                log_type(_callback_insert, "invoke chain");

                // Set released to true to prevent multiple calls
                _released = true;

                // Invoke everything with an empty callback
                _callback_insert(create_empty_callback_factory<ForwardFunction>::create());
            }
        }

        /// Deleted copy assign
        template<typename _RState, typename _RCTy>
        _ContinuableImpl& operator= (_ContinuableImpl const&) = delete;

        /// Move construct assign
        _ContinuableImpl& operator= (_ContinuableImpl&& right)
        {
            _released = right._released;
            right._released = true;
            _callback_insert = std::move(right._callback_insert);
            return *this;
        }

        template<typename _CTy>
        auto then(_CTy&& functional)
            -> typename unary_chainer_t<_CTy>::result_t
        {
            // Transfer the insert function to the local scope.
            // Also use it as an r-value reference to try to get move semantics with c++11.
            ForwardFunction callback = std::move(_callback_insert);

            return typename unary_chainer_t<_CTy>::result_t(
                [functional, callback](typename unary_chainer_t<_CTy>::callback_t&&)
            {
                log_type(callback, "calling");

                callback([functional](_ATy... args)
                {
                    // log_type(functional, "invoking");

                    auto continuable =
                        unary_chainer_t<_CTy>::base::invoke(functional, std::forward<_ATy>(args)...);

                    // log_type(continuable, "result");

                    // auto cc = continuable;

                    int i = 0;


                    // TODO
                    // continuable._callback_insert(std::move(call_next));
                });

            }, std::move(*this));
        }

        /*
        // TODO Accept only correct callbacks
        template<typename... _CTy>
        Continuable<Callback<_ATy...>> all(_CTy&&...)
        {
            // TODO Transmute the returned callback here.
            return Continuable<Callback<_ATy...>>(std::move(*this));
        }
        */

        /// Invalidates the Continuable
        _ContinuableImpl& invalidate()
        {
            _released = true;
            return *this;
        }
    };
}

/// A continuable provides useful methods to react on the result of callbacks
/// and allows to chain multiple callback calls to a chain.
template<typename... Args>
using Continuable = detail::_ContinuableImpl<
    detail::DefaultContinuableState,
    Callback<Args...>>;

namespace detail
{
    template<typename _RTy, typename... _ATy>
    struct ContinuableFactory;

    template<typename _RTy, typename... _ATy>
    struct ContinuableFactory<_RTy, ::fu::identity<std::function<void(_ATy...)>&&>>
    {
        template<typename _FTy>
        static auto CreateFrom(_FTy&& functional)
            -> Continuable<_ATy...>
        {
            return Continuable<_ATy...>(
                typename Continuable<_ATy...>::ForwardFunction(std::forward<_FTy>(functional)));
        }
    };

    template<typename _FTy>
    using continuable_factory_t = ContinuableFactory<
        ::fu::return_type_of_t<_FTy>, ::fu::argument_type_of_t<_FTy>>;

} // detail

/// Wraps a functional object which expects a r-value callback as argument into a continuable.
/// The callable is invoked when the continuable shall continue.
/// For example:
/// make_continuable([](Callback<int>&& callback)
/// {
///     /* Continue here */
///     callback(5);
/// });
template<typename _FTy>
inline auto make_continuable(_FTy&& functional)
    -> decltype(detail::continuable_factory_t<_FTy>::CreateFrom(std::declval<_FTy>()))
{
    return detail::continuable_factory_t<_FTy>::CreateFrom(std::forward<_FTy>(functional));
}

#endif // _CONTINUABLE_H_
