
#include "fluent++.hpp"

template <typename... Args>
using Callback = std::function<void(Args...)>;

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

    return 0;
}
