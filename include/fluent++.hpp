
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

#ifndef _FLUENT_HPP_
#define _FLUENT_HPP_

#include <memory>
#include <utility>
#include <iostream>

#include "functional_unwrap.hpp"

/*
struct ProtoContinueable
{
    template<typename Callback>
    ProtoContinueable then(Callback&& callback)
    {
        return ProtoContinueable();
    }

    template<typename Container>
    ProtoContinueable weak(Container& container)
    {
        return ProtoContinueable();
    }

    ProtoContinueable strong()
    {
        return ProtoContinueable();
    }
};
*/

void do_export();

#endif /// _FLUENT_HPP_
