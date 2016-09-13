
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

#define CATCH_CONFIG_RUNNER

#include "catch.hpp"

#include "Continuable.h"

// void testNextGen();
void test_mockup();
void test_incubator();

int tttt(int argc, char* const argv[])
{
    // testNextGen();
    return 0;

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

    static_assert(std::is_same<
            cross_forward_t<
                TestContainer&&,
                std::unique_ptr<int>&
            >,
            std::unique_ptr<int>&&>::value,
            "cross_forward returns wrong type!");

    static_assert(std::is_same<
            cross_forward_t<
                TestContainer&,
                std::unique_ptr<int>&
            >,
        std::unique_ptr<int>&>::value,
        "cross_forward returns wrong type!");

    SECTION("forward l-value references")
    {
        extract(con);
        REQUIRE(con.ptr.get());
    }

    SECTION("forward r-value references")
    {
        extract(std::move(con));
        REQUIRE_FALSE(con.ptr.get());
    }
}

Continuable<std::string> http_request(std::string const& /*url*/)
{
    return make_continuable([=](std::function<void(std::string)>&& callback)
    {
        // Do request...
        std::string result = "some HTTP content";

        callback(std::move(result));
    });
}

struct ResultSet
{
    std::size_t rows;
};

Continuable<ResultSet> mysql_query(std::string const& /*query*/)
{
    return make_continuable([=](std::function<void(ResultSet)>&& callback)
    {
        callback(ResultSet{5});
    });
}

Continuable<> defect_continuable()
{
    return make_continuable([=](std::function<void()>&& /*callback*/)
    {
        // Callback is never called
    });
}

TEST_CASE("Continuable invocation on destruct", "[Continuable]")
{
    bool invoked = false;

    std::function<void(Callback<>&&)> invokeable = [&](Callback<>&& callback)
    {
        invoked = true;
        callback();
    };

    SECTION("Continuables are not invoked before destruct")
    {
        Continuable<> continuable = make_continuable(std::move(invokeable));

        REQUIRE_FALSE(invoked);
    }

    SECTION("Continuables are invoked on destruct")
    {
        make_continuable(std::move(invokeable));

        REQUIRE(invoked);
    }

    SECTION("Continuables are not invoked after transferred")
    {
        Continuable<> continuable = make_continuable(std::move(invokeable));

        {
            Continuable<> cache = std::move(continuable);

            REQUIRE_FALSE(invoked);
        }

        REQUIRE(invoked);
    }
}

TEST_CASE("Continuable continuation chaining using Continuable::then", "[Continuable]")
{
    std::size_t invoked = 0;

    SECTION("Continuables result is evaluateable")
    {
        make_continuable([](Callback<int>&& callback)
        {
            callback(12345);
        })
        .then([&](int i)
        {
            if (i == 12345)
                ++invoked;
        });

        REQUIRE(invoked == 1);
    }

    SECTION("Continuables are invalidated on chaining (no duplicated call) and order is ok")
    {
        http_request("http://github.com")
            .then([&](/*std::string url (use signature erasure)*/)
            {
                REQUIRE(invoked == 0);
                invoked = 1;
            })
            .then([&]
            {
                REQUIRE(invoked == 1);
                invoked = 2;
            })
            .then([&]
            {
                REQUIRE(invoked == 2);
                invoked = 3;

                return make_continuable([&](Callback<std::size_t>&& callback)
                {
                    REQUIRE(invoked == 3);
                    invoked = 4;

                    callback(5);
                });
            })
            .then([&](std::size_t t)
            {
                REQUIRE(invoked == 4);
                invoked = t;
            });

        REQUIRE(invoked == 5);
    }

    SECTION("Continuation chains need a callback to continue")
    {
        defect_continuable()
            .then([&]
            {
                invoked++;
            });

        REQUIRE(invoked == 0);
    }
}
