
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

template <typename _CTy>
struct Continuable;

template <typename... _ATy>
struct Continuable<Callback<_ATy...>>
{
    // Function which expects a callback that is inserted from the Continuable
    // to chain everything together
    std::function<void(Callback<_ATy...>)> _callback_insert;

public:
    Continuable<Callback<_ATy...>>() { }
    Continuable<Callback<_ATy...>>(std::function<void(Callback<_ATy...>)>&& callback_insert)
        : _callback_insert(std::forward<std::function<void(Callback<_ATy...>)>>(callback_insert)) { }

    template <typename _CTy>
    Continuable<Callback<_ATy...>>& then(_CTy&& callback)
    {
        return *this;
    }
};

namespace detail
{
    template <typename _FTy, typename _RTy, typename... _ATy>
    struct ContinuableFactory;

    template <typename _FTy, typename _RTy, typename... _ATy>
    struct ContinuableFactory<_FTy, _RTy, ::fu::identity<Callback<_ATy...>>>
    {
        static auto CreateFrom(_FTy&& functional)
            -> Continuable<Callback<_ATy...>>
        {
            return Continuable<Callback<_ATy...>>(std::forward<_FTy>(functional));
        }
    };

    template <typename _FTy>
    using continuable_factory_t = ContinuableFactory<
        _FTy, ::fu::return_type_of_t<_FTy>, ::fu::argument_type_of_t<_FTy>>;

} // detail

template <typename _FTy>
inline auto make_continuable(_FTy&& functional)
    -> decltype(typename detail::continuable_factory_t<_FTy>::CreateFrom(std::declval<_FTy>()))
{
    return detail::continuable_factory_t<_FTy>::CreateFrom(std::forward<_FTy>(functional));
}

#endif /// _CONTINUABLE_H_
