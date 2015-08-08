
#include <string>
#include <functional>

template <typename...>
struct Continuable;

template <typename... Args>
Continuable<> make_continuable(Args&&...)
{
    return Continuable<>();
}

template <typename...>
struct Continuable
{
    template <typename... Args>
    Continuable then(Args&&...)
    {
        return Continuable();
    }

    template <typename... Args>
    Continuable all(Args&&...)
    {
        return Continuable();
    }
};

template <typename... LeftArgs, typename... RightArgs>
Continuable<> operator&& (Continuable<LeftArgs...>&&, Continuable<RightArgs...>&&)
{
    return Continuable<>();
}

template <typename... LeftArgs, typename... RightArgs>
Continuable<> operator|| (Continuable<LeftArgs...>&&, Continuable<RightArgs...>&&)
{
    return Continuable<>();
}

Continuable<> http_request(std::string const& /*URL*/)
{
    return make_continuable([=](std::function<void(std::string)>&& callback)
    {
        // Do request...

        callback("some HTTP content");
    });
}

void test_mockup()
{
    Continuable<> c1 = make_continuable([]
    {
    });

    Continuable<> c2 = make_continuable([]
    {
    });

    Continuable<> c3 = make_continuable([]
    {
    });

    Continuable<> c11 = (std::move(c1) && std::move(c2)) || std::move(c3);
}
