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

#ifndef _CONTINUABLE_H_
#define _CONTINUABLE_H_

#include "Callback.h"



template <typename _MTy>
class ContinuableBase
{

};

template <typename _CTy, typename _WTy = void>
class Continuable;

template <typename... _ATy>
class Continuable<Callback<_ATy...>, void>
{
public:
    typedef bool type;
};

template <typename... _ATy, typename _WTy>
class Continuable<Callback<_ATy...>, _WTy>
{
public:
    typedef int type;
};


/*
template <typename _CTy>
auto make_continuable(_CTy&& callback)
    -> 
{
}
*/

#endif /// _CONTINUABLE_H_
