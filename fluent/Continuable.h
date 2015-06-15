
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

namespace detail
{
    /// The internal state of the continuable
    /// which is used to save certain internal types.
    template<typename... Args>
    struct ContinuableState;

    template<typename... _Cain, typename _Proxy>
    struct ContinuableState<std::tuple<_Cain...>, _Proxy>
    {
    };

    typedef ContinuableState<std::tuple<>, void> DefaultContinuableState;

    // ContinuableImpl Forward definition
    template<typename _STy, typename _CTy>
    class _ContinuableImpl;

    /// Corrects void return types from functional types which should be Continuable<DefaultContinuableState, Callback<>>
    template<typename _RTy>
    struct convert_void_to_continuable;

    template<>
    struct convert_void_to_continuable<void>
    {
        typedef _ContinuableImpl<DefaultContinuableState, Callback<>> type;


        //template<typename Fn, typename... Args>
        //static type InvokeAndReturn(std::function<Fn> const& functional, std::forward<Args>... args)
        //{
        //    functional(std::forward<Args>(args)...);
        //    return type(); /*[](Callback<>&& callback)
        //    {
        //        callback();
        //    });*/
        //}
    };

    template<typename _State, typename _CTy>
    struct convert_void_to_continuable<_ContinuableImpl<_State, _CTy>>
    {
        typedef _ContinuableImpl<_State, _CTy> type;

        //template<typename Fn, typename... Args>
        //static type InvokeAndReturn(std::function<Fn> const& functional, std::forward<Args>... args)
        //{
        //    return functional(std::forward<Args>(args)...);
        //}
    };

    template<typename _NextRTy, typename... _NextATy>
    struct unary_chainer;

    // MSVC 12 has issues to detect the parameter pack otherwise.
    template<typename _NextRTy, typename... _NextATy>
    struct unary_chainer<_NextRTy, fu::identity<_NextATy...>>
    {
        typedef typename convert_void_to_continuable<_NextRTy>::type result_t;

        typedef typename result_t::CallbackFunction callback_t;
    };

    template <typename _CTy>
    using unary_chainer_t = unary_chainer<
        fu::return_type_of_t<typename std::decay<_CTy>::type>,
        fu::argument_type_of_t<typename std::decay<_CTy>::type>>;

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

        boost::optional<std::function<void()>> _entry_point;

        bool _released;

        std::function<void()> MakeEmptyEntryPoint()
        {
            return []
            {
            };
        }

        void Dispatch()
        {
            if (_entry_point)
                (*_entry_point)();
        }

    public:
        // Empty for debugging
        _ContinuableImpl()
            : _released(false), _callback_insert(), _entry_point() { }

        /// Deleted copy construct
        _ContinuableImpl(_ContinuableImpl const&) = delete;

        /// Move construct
        _ContinuableImpl(_ContinuableImpl&& right)
            : _released(right._released), _callback_insert(std::move(right._callback_insert)),
             _entry_point(std::move(right._entry_point))
        {
            right._released = true;
        }

        // Construct through a ForwardFunction
        template<typename _FTy>
        _ContinuableImpl(_FTy&& callback_insert)
            : _callback_insert(std::forward<_FTy>(callback_insert)), _released(false), _entry_point() { }

        template<typename _RSTy, typename _RCTy, typename _FTy>
        _ContinuableImpl(_FTy&& callback_insert, _ContinuableImpl<_RSTy, _RCTy>&& right)
            : _callback_insert(std::forward<_FTy>(callback_insert)), _released(right._released), _entry_point()
        {
            right._released = true;
        }

        /// Destructor which calls the dispatch chain if needed.
        ~_ContinuableImpl()
        {
            if (!_released)
            {
                _released = true;
                Dispatch();
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
            ForwardFunction&& callback = std::move(_callback_insert);

            return typename unary_chainer_t<_CTy>::result_t(
                [functional, callback](typename unary_chainer_t<_CTy>::callback_t&& next)
            {
                callback([functional, next](_ATy... args)
                {
                    typename unary_chainer_t<_CTy>::result_t continuable;
                    //=     next(std::forward<_ATy>(args)...);

                    // continuable();
                });

            }, std::move(*this));

            /*
            return typename unary_chainer_t<_CTy>::result_t
                (std::move(*this), [=](typename unary_chainer_t<_CTy>::callback_t&& next_insert_callback)
            {
                
            });
            */
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
