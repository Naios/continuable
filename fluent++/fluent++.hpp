
#ifndef _FLUENT_HPP_
#define _FLUENT_HPP_

#include <memory>
#include <utility>

#include "functional_unwrap.hpp"

class fluent_step
{
    bool released;

    void release()
    {
        int i = 0;
    }

public:
    fluent_step() : released(false) { }

    ~fluent_step()
    {
        if (!released)
            release();
    }

    fluent_step(fluent_step const&) = delete;
    fluent_step(fluent_step&& right) : released(false)
    {
        right.released = true;
    }

    fluent_step& operator= (fluent_step const&) = delete;
    fluent_step& operator= (fluent_step&& right)
    {
        released = false;
        right.released = true;
    }

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
