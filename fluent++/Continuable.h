
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



template <typename _MTy>
class ContinuableBase
{

};

template <typename _CTy, typename _WTy = void>
class Continuable;

template <typename... _ATy, typename _WTy>
class Continuable<Callback<_ATy...>, typename _WTy>
{
public:
    
};

namespace detail
{
    template <typename _FTy, typename _RTy, typename... _ATy>
    struct ContinuableFactory;

    template <typename _FTy, typename _RTy, typename... _ATy>
    struct ContinuableFactory<_FTy, _RTy, std::tuple<_ATy...>>
    {
        static auto CreateFrom(_FTy&& functional)
            -> int
        {
            return 1;
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
