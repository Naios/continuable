
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

#define CATCH_CONFIG_RUNNER

#include "catch.hpp"

void test_mockup();
void test_incubator();

int main(int argc, char* const argv[])
{
    test_mockup();

    test_incubator();

    int const result = Catch::Session().run(argc, argv);

    // Attach breakpoint here ,-)
    return result;
}

template<typename T, typename V>
using cross_forward_t =
    typename std::conditional<
        std::is_rvalue_reference<T&&>::value,
        typename std::decay<V>::type&&,
        typename std::decay<V>::type&
    >::type;

template<typename T, typename V>
cross_forward_t<T, V> cross_forward(V&& var)
{
    return static_cast<cross_forward_t<T, V>&&>(var);
}

struct TestContainer
{
    std::shared_ptr<int> ptr;
};

template<typename T>
TestContainer extract(T&& c)
{
    return TestContainer{cross_forward<T>(c.ptr)};
}

TEST_CASE("CrossForward tests", "[CrossForward]")
{
    TestContainer con;

    con.ptr = std::make_shared<int>(0);

    static_assert(
        std::is_same<cross_forward_t<TestContainer&&, std::unique_ptr<int>&>,
        std::unique_ptr<int>&&>::value,
        "cross_forward returns wrong type!");

    static_assert(
        std::is_same<cross_forward_t<TestContainer&, std::unique_ptr<int>&>,
        std::unique_ptr<int>&>::value,
        "cross_forward returns wrong type!");

    SECTION("CrossForward - forward l-value references")
    {
        extract(con);
        REQUIRE(con.ptr.get());
    }

    SECTION("CrossForward - forward r-value references")
    {
        extract(std::move(con));
        REQUIRE_FALSE(con.ptr.get());
    }
}
