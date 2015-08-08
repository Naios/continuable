
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

Continuable<> http_request(std::string const& url)
{
    return make_continuable([=](std::function<void(std::string)>&& callback)
    {
        // Do request...
        callback("some HTTP content");
    });
}

struct ResultSet { };

Continuable<> mysql_query(std::string const& query)
{
    return make_continuable([=](std::function<void(std::string)>&& callback)
    {
        // Do request...

        callback("a mysql query result");
    });
}

void test_mockup()
{
    {
        Continuable<> continuable = make_continuable([]
        {
            return "hey";
        });
    }

    Continuable<> c1 = make_continuable([]
    {
    });

    Continuable<> c2 = make_continuable([]
    {
    });

    Continuable<> c3 = make_continuable([]
    {
    });

    Continuable<> c4 = make_continuable([]
    {
    });

    (std::move(c1) && std::move(c2))
        .then(http_request("https://github.com/") &&
              http_request("https://www.google.de/") &&
              mysql_query("SELECT name, session FROM users WHERE id = 3726284"))
        .then([](std::string github, std::string google, ResultSet user)
        {
             // result bla bla
        });

    // Continuable<> c11 =  || std::move(c3);
}
