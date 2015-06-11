
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
