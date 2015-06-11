
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

} // detail

template <typename... Args>
struct ContinuableState
{
};

template <typename _CTy, typename _State = ContinuableState<>>
struct Continuable;

template <typename... _ATy, typename _State>
struct Continuable<std::function<void(_ATy...)>, _State>
{
    typedef std::function<void(Callback<_ATy...>&&)> ForwardFunction;

    // Function which expects a callback that is inserted from the Continuable
    // to chain everything together
    ForwardFunction _callback_insert;

    Continuable() { }

    template<typename _FTy>
    Continuable(_FTy&& callback_insert)
        : _callback_insert(std::forward<_FTy>(callback_insert)) { }

    template <typename _CTy>
    Continuable<Callback<_ATy...>> then(_CTy&&)
    {
        return Continuable<Callback<_ATy...>>();
    }
};

namespace detail
{
    template <typename _FTy, typename _RTy, typename... _ATy>
    struct ContinuableFactory;

    template <typename _FTy, typename _RTy, typename... _ATy>
    struct ContinuableFactory<_FTy, _RTy, ::fu::identity<std::function<void(_ATy...)>&&>>
    {
        static auto CreateFrom(_FTy&& functional)
            -> Continuable<Callback<_ATy...>>
        {
            return Continuable<Callback<_ATy...>>(
                typename Continuable<Callback<_ATy...>>::ForwardFunction(std::forward<_FTy>(functional)));
        }
    };

    template <typename _FTy>
    using continuable_factory_t = ContinuableFactory<
        _FTy, ::fu::return_type_of_t<_FTy>, ::fu::argument_type_of_t<_FTy>>;

} // detail

/// Wraps a functional object which expects a r-value callback as argument into a continuable.
/// The callable is invoked when the continuable shall continue.
/// For example:
/// make_continuable([](Callback<int>&& callback)
/// {
///     /* Continue here */
///     callback(5);
/// });
template <typename _FTy>
inline auto make_continuable(_FTy&& functional)
    -> decltype(detail::continuable_factory_t<_FTy>::CreateFrom(std::declval<_FTy>()))
{
    return detail::continuable_factory_t<_FTy>::CreateFrom(std::forward<_FTy>(functional));
}

#endif // _CONTINUABLE_H_
