
#ifndef _FLUENT_HPP_
#define _FLUENT_HPP_

#include "functional_unwrap.hpp"

struct fluent_step
{
    template <typename Callback>
    fluent_step& then(Callback const& callback)
    {
        return *this;
    }
};

template <typename _ATy = void()>
fluent_step make_waterfall()
{
    return fluent_step();
}





void do_export();

#endif /// _FLUENT_HPP_
