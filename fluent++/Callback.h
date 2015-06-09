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

#ifndef _CALLBACK_H_
#define _CALLBACK_H_

#include <functional>
#include <utility>
#include <memory>


template<typename... Args>
using Callback = std::function<void(Args...)>;

template<typename... Args>
using SharedCallback = std::shared_ptr<Callback<Args...>>;

template<typename... Args>
using WeakCallback = std::weak_ptr<Callback<Args...>>;

template<typename... Args>
auto make_shared_callback(Callback<Args...>&& callback)
    -> SharedCallback<Args...>
{
    return std::make_shared<Callback<Args...>>(std::forward<Callback<Args...>>(callback));
}

#endif /// _TASK_SCHEDULER_H_
