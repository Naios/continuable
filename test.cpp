
#include "fluent++.hpp"

#include "Callback.h"

void CastSpell(int id, Callback<bool> const& callback)
{
    std::cout << "Cast " << id << std::endl;

    // on success call true
    callback(true);
}

void MoveTo(int point, Callback<bool> const& callback)
{
    std::cout << "Move to point " << point << std::endl;

    // on success call true
    callback(true);
}

int main(int argc, char** argv)
{
    make_waterfall<Callback<bool>>()
        .then(std::bind(&CastSpell, 71382, std::placeholders::_1))
        .then([](bool success, Callback<bool> const& callback)
        {
            MoveTo(1, callback);
        })
        .then([](bool success)
        {
            // Do something final
            std::cout << "finish everything" << std::endl;
        });

    typedef Callback<bool> cbd1;
    typedef WeakCallback<int> cbd2;
    typedef SharedCallback<std::string> cbd3;

    cbd1 _cbd1;
    cbd2 _cbd2;
    cbd3 _cbd3;

    auto cb = make_shared_callback([](bool)
    {
    
    });

    auto cbt = std::make_shared<std::function<void()>>([]()
    {
    });

    return 0;
}
