
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

#include <utility>
#include <memory>

#include "Callback.h"
#include "WeakCallbackContainer.h"
#include "Continuable.h"

#include <iostream>
#include <exception>
#include <type_traits>
#include <string>
#include <vector>
#include <typeinfo>

#include <condition_variable>
#include <mutex>
#include <thread>
#include <atomic>
#include <random>

// #include "concurrentqueue.h"

#include <boost/optional.hpp>

enum SpellCastResult
{
    SPELL_FAILED_SUCCESS = 0,
    SPELL_FAILED_AFFECTING_COMBAT = 1,
    SPELL_FAILED_ALREADY_AT_FULL_HEALTH = 2,
    SPELL_FAILED_ALREADY_AT_FULL_MANA = 3,
    SPELL_FAILED_ALREADY_AT_FULL_POWER = 4,
    SPELL_FAILED_ALREADY_BEING_TAMED = 5
};

template<typename T>
using Optional = boost::optional<T>;

Continuable<> Log(std::string const& message)
{
    return make_continuable([=](Callback<>&& callback)
    {
        std::cout << message << std::endl;
        callback();
    });
}

struct ResultSet
{
    ResultSet(std::size_t affected_) :
        affected(affected_) { };

    std::size_t affected;
};

Continuable<ResultSet> AsyncQuery(std::string const& query)
{
    return make_continuable([=](Callback<ResultSet>&& callback)
    {
        std::cout << query << std::endl;
        callback(ResultSet(2));
    });
}

// Original method taking an optional callback.
void CastSpell(int id, Optional<Callback<SpellCastResult>> const& callback = boost::none)
{
    std::cout << "Casting " << id << std::endl;

    // on success call the callback with SPELL_FAILED_SUCCESS
    if (callback)
        (*callback)(SPELL_FAILED_SUCCESS);
}

// Promise wrapped callback decorator.
Continuable<SpellCastResult> CastSpellPromise(int id)
{
    return make_continuable([=](Callback<SpellCastResult>&& callback)
    {
        CastSpell(id, callback);
    });
}

// Void instant returning continuable promise for testing purposes
Continuable<> TrivialPromise(std::string const& msg = "")
{
    return Log(msg);
}

Continuable<bool> Validate()
{
    return make_continuable([=](Callback<bool>&& callback)
    {
        std::cout << "Validate " << std::endl;

        callback(true);
    });
}

Continuable<std::unique_ptr<int>&&> MoveTest()
{
    return make_continuable([=](Callback<std::unique_ptr<int>&&>&& callback)
    {
        // Move the unique ptr out to test moveability
        std::unique_ptr<int> ptr(new int(5));
        callback(std::move(ptr));
    });
}

typedef std::unique_ptr<int> Moveable;

void testMoveAbleNormal(std::function<void(std::unique_ptr<int>&&)> callback)
{
    std::unique_ptr<int> ptr(new int(5));
    callback(std::move(ptr));   
}

template <typename... T>
void test_unwrap(std::string const& msg)
{
    std::cout << msg << " is unwrappable: " << (fu::is_unwrappable<T...>::value ? "true" : "false") << std::endl;
}

#include <iostream>
#include <atomic>
#include <random>

// static std::atomic_size_t move_tracer_index = 0;

/// Class to trace construct, destruct, copy and move operations.
/*
class CopyMoveTracer
{
    std::size_t id;
    std::size_t flags;
    std::size_t copied;
    std::size_t moved;

    bool IsEnabled(std::size_t mask) const
    {
        // msvc warning silencer
        return (flags & mask) != 0;
    }

    void Log(std::string const& msg) const
    {

    }

public:
    enum Flags : std::size_t
    {
        CAPTURE_NONE      = 0x1,
        CAPTURE_CONSTRUCT = 0x1,
        CAPTURE_DESTRUCT  = 0x2,
        CAPTURE_COPY      = 0x4,
        CAPTURE_MOVE      = 0x8,
        CAPTURE_ALL       = CAPTURE_CONSTRUCT | CAPTURE_DESTRUCT | CAPTURE_COPY | CAPTURE_MOVE
    };

    // Empty construct
    CopyMoveTracer() : id(++move_tracer_index), flags(CAPTURE_ALL), copied(0), moved(0)
    {
        if (IsEnabled(CAPTURE_CONSTRUCT))
            Log("Tracer constructed");
    }

    // Construct with flags
    CopyMoveTracer(std::size_t flags_) : id(++move_tracer_index), flags(flags_), copied(0), moved(0)
    {
        if (IsEnabled(CAPTURE_CONSTRUCT))
            Log("Tracer constructed");
    }

    // Copy construct
    CopyMoveTracer(CopyMoveTracer const& right) : id(move_tracer_index++), flags(right.flags), copied(0), moved(0)
    {
        if (IsEnabled(CAPTURE_COPY))
            Log("Tracer copy constructed");
    }

    // Copy construct
    CopyMoveTracer(CopyMoveTracer&& right) : id(right.id), flags(right.flags), copied(0), moved(0)
    {
        if (IsEnabled(CAPTURE_COPY))
            Log("Tracer copy constructed");
    }
};
*/
/*
namespace detail
{
    template<typename, typename>
    struct function_matches_to_args;

    template<typename LeftReturn, typename... LeftArgs,
             typename RightReturn, typename... RightArgs>
    struct function_matches_to_args<
        std::function<LeftReturn(LeftArgs...)>,
        std::function<RightReturn(RightArgs...)>>
    {
            
    };
}
*/

/*
class DispatcherPool
{
    enum TerminationMode
    {
        NONE,
        TERMINATE,
        AWAIT
    };

    typedef std::function<void()> Callable;

    std::vector<std::thread> _pool;

    std::atomic<TerminationMode> _shutdown;

    std::mutex _mutex;

    std::condition_variable _condition;

    // moodycamel::ConcurrentQueue<Callable> _queue;

public:
    DispatcherPool() : DispatcherPool(std::thread::hardware_concurrency()) { }

    DispatcherPool(unsigned int const threads) : _shutdown(NONE)
    {
        for (unsigned int i = 0; i < threads; ++i)
        {
            _pool.emplace_back([&, i]
            {
                // Reserve the consumer token
                // moodycamel::ConsumerToken token(_queue);

                Callable callable;
                while (_shutdown != TERMINATE)
                {
                    if (_queue.try_dequeue(token, callable))
                    {
                        std::string msg = "Thread " + std::to_string(i) + " is dispatching...\n";
                        // std::cout << msg;
                        callable();
                    }
                    else
                    {
                        if (_shutdown == AWAIT)
                            break;

                        {
                            std::string msg = "Thread " + std::to_string(i) + " out of work...\n";
                            // std::cout << msg;
                        }

                        std::unique_lock<std::mutex> lock(_mutex);

                        // Lock until new tasks are added
                        _condition.wait(lock);

                        {
                            std::string msg = "Thread " + std::to_string(i) + " wakes up...\n";
                            // std::cout << msg;
                        }
                    }
                }
            });
        }
    }

    ~DispatcherPool()
    {
        Shutdown();
    }

    template<typename Functional>
    void Dispatch(Functional&& functional)
    {
        _queue.enqueue(std::forward<Functional>(functional));
        std::unique_lock<std::mutex> lock(_mutex);
        _condition.notify_one();
    }

    void Shutdown()
    {
        _Shutdown(TERMINATE);
    }

    void Await()
    {
        _Shutdown(AWAIT);
    }

    void _Shutdown(TerminationMode const mode)
    {
        _shutdown = mode;
        _condition.notify_all();
        for (auto&& thread : _pool)
            if (thread.joinable())
                thread.join();
    }
};
*/

void some_examples()
{
    // CopyMoveTracer tracer;

    /*
    CastSpellPromise(1)
        .then([](SpellCastResult)
        {
            return CastSpellPromise(2);
        })
        .then([](SpellCastResult)
        {
            std::cout << "Pause a callback (void test) " << std::endl;
        })
        .then(Validate())
        .then(AsyncQuery("SELECT * FROM `users`")
                .then([](ResultSet result)
                {
                    // Evaluate result
                    std::size_t const affected = result.affected;
                    return Log(std::to_string(affected) + " rows affected\n");
                })
        )
        .then(TrivialPromise("huhu"))
        .then(CastSpellPromise(3))
        .then(CastSpellPromise(4)
              .then(CastSpellPromise(5))
        )
        .then(CastSpellPromise(6))
        .then([](SpellCastResult)
        {
            return Validate();
        });

    MoveTest()
        .then([](std::unique_ptr<int>&& ptr)
        {
            static_assert(std::is_rvalue_reference<decltype(ptr)>::value, "no rvalue");

            // Error here
            std::unique_ptr<int> other = std::move(ptr);
        });

    // Mockup of aggregate methods
    make_continuable()
        .all(
            [] { return TrivialPromise(); },
            [] { return TrivialPromise(); },
            [] { return TrivialPromise(); }
        )
        .some(2, // Only 2 of 3 must complete
            [] { return TrivialPromise(); },
            [] { return TrivialPromise(); },
            [] { return TrivialPromise(); }
        )
        .any(    // Any of 2.
            [] { return TrivialPromise(); },
            [] { return TrivialPromise(); }
        )
        .then([]
        {
            std::cout << "Finished" << std::endl;
        });
        */
    //Continuable<bool> cb = make_continuable([](Callback<bool>&& callback)
    //{

    //    callback(true);
    //});

    //test_unwrap<void()>("void()");
    //test_unwrap<std::function<void()>>("std::function<void()>");
    //test_unwrap<std::vector<std::string>>("std::vector<std::string>");

    //make_continuable([=](Callback<>&&)
    //{

    //});

    //int i = 0;
    //++i;

    //auto lam = [=](Callback<SpellCastResult>&&)
    //{
    //    // on success call the callback with SPELL_FAILED_SUCCESS
    //    // callback(SPELL_FAILED_SUCCESS);
    //};

    //fu::function_type_of_t<decltype(lam)> fun1;
    //fun1 = lam;
    //fun1(Callback<SpellCastResult>());

    //fu::function_type_of_t<Callback<int>> fun2;
    //
    //shared_callback_of_t<std::function<void(int)>> sc1;
    //weak_callback_of_t<Callback<int>> sc2;
    //
    //make_weak_wrapped_callback(sc1);
    //make_weak_wrapped_callback(sc2);

    //WeakCallbackContainer callback;
    //
    //auto weakCallback = callback([]
    //{
    //});

    //typedef Continuable<bool> cont123;

    //typedef Continuable<bool> myty1;
    //typedef Continuable<bool, float> myty2;

    //// Convertible test
    //
    //// Continuable<Callback<SpellCastResult>> spell
    //{
    //    auto stack = 

    //    int iii = 0;
    //    iii = 1;
    //}

    //std::vector<int> myvec;

    //typedef fu::requires_functional_constructible<std::function<void()>>::type test_assert1;
    //// typedef fu::requires_functional_constructible<std::vector<int>>::type test_assert2;

    //// Brainstorming: this shows an example callback chain
    //// Given by continuable
    //std::function<void(Callback<SpellCastResult>&&)> continuable_1 = [](Callback<SpellCastResult>&& callback)
    //{
    //    callback(SPELL_FAILED_AFFECTING_COMBAT);
    //};

    //// Implemented by user
    //std::function<std::function<void(Callback<bool>&&)>(SpellCastResult)> callback_by_user_1 = [](SpellCastResult)
    //{
    //    // Given by continuable
    //    // Fn2
    //    return [](Callback<bool>&& callback)
    //    {
    //        callback(true);
    //    };
    //};

    //// Implemented by user
    //std::function<std::function<void(Callback<>&&)>(bool)> cn2 = [](bool val)
    //{
    //    // Finished
    //    std::cout << "Callback chain finished! -> " << val << std::endl;

    //    // Given by continuable (auto end)
    //    return [](Callback<>&&)
    //    {
    //        // Empty callback
    //    };
    //};

    //// Entry point
    //std::function<void(Callback<bool>&&>)> entry = [continuable_1 /*= move*/, callback_by_user_1 /*given by the user (::then(...))*/]
    //    (std::function<void(Callback<bool>&&)>)
    //{
    //    // Call with auto created wrapper by the continuable
    //    continuable_1([&](SpellCastResult result /*forward args*/)
    //    {
    //        // Wrapper functional to process unary or multiple promised callbacks
    //        // Returned from the user
    //        std::function<void(Callback<bool>&&)> fn2 = callback_by_user_1(/*forward args*/ result);
    //        return std::move(fn2);
    //    });
    //};
 
    //// Here we go
    //entry();

    detail::unary_chainer_t<
        std::function<Continuable<bool>()>
    >::callback_arguments_t args213987;

    typedef detail::functional_traits<>::result_maker_of_t<

        std::function<Continuable<bool>()>,
        decltype(CastSpellPromise(2)),
        decltype(TrivialPromise()),
        std::function<Continuable<float, double>()>,
        std::function<Continuable<>()>,
        std::function<Continuable<bool>()>

    > maker;
    
    maker::arguments_t test282_args;
    maker::partial_results_t test282_pack;
    auto test282_size = maker::size;
 
    // static_assert(std::is_same<>::value,
    
    detail::concat_identities<fu::identity<int, bool, char>, fu::identity<float, double>>::type myt;

    // fu::identity<detail::functional_traits<>::position<1>> i;

    std::tuple<int, std::vector<int>> tup;

    Moveable moveable(new int(7));

    auto myargs = std::make_tuple(7, std::vector<int>({ 1, 2, 3 }), std::move(moveable));

    std::function<int(int, std::vector<int>, Moveable&&)> lam = [](int given_i, std::vector<int> given_vec, Moveable&& moveable)
    {
        Moveable other = std::move(moveable);

        ++given_i;
        return 1;
    };

    fu::invoke_from_tuple(lam, std::move(myargs));

    fu::sequence_generator<2>::type seqtype;
    fu::sequence_generator<1>::type zero_seqtype;

    detail::multiple_when_all_chainer_t<
        fu::identity<>,
        fu::identity<
            std::function<Continuable<>()>,
            std::function<Continuable<std::string>()>
        >
    >::result_maker::partial_results_t myres123345;

    /*
    auto firstType = detail::multiple_when_all_chainer_t<
        fu::identity<>,
        fu::identity<
            std::function<Continuable<SpellCastResult>()>,    
            std::function<Continuable<>()>,
            std::function<Continuable<SpellCastResult>()>
        >
    >::make_when_all(
    []
    {
        // void
        return CastSpellPromise(10);
    },
    []
    {
        return make_continuable();
    },
    []
    {
        return CastSpellPromise(20);
    })
    .then([](SpellCastResult, SpellCastResult)
    {
        
    })
    .then([]
    {

    });
    */

    make_continuable()
        .all(
            CastSpellPromise(10)
                .then(CastSpellPromise(15)),
            CastSpellPromise(20),
            make_continuable([](Callback<bool, bool, double , std::unique_ptr<std::string>>&& callback)
            {
                callback(true, false, 0.3f, std::unique_ptr<std::string>(new std::string("oh, all work is done!")));
            }),
            TrivialPromise())
        .then([](SpellCastResult r0, SpellCastResult r1, bool r2, bool r3, double r4, std::unique_ptr<std::string> message)
        {
            return TrivialPromise("Lets see... ").then(Log(*message));
        })
        .then([]
        {
            return Log("ok, now its really finished!").then(CastSpellPromise(2));
        });

    // DispatcherPool countPool(1);

    // DispatcherPool pool;

    /*
    auto const seed = std::chrono::steady_clock::now().time_since_epoch().count();

    std::mt19937 rng(static_cast<unsigned int>(seed));
    std::uniform_int_distribution<int> gen(10, 150);

    std::vector<int> container;

    unsigned int counter = 0;

    for (unsigned int run = 0; run < 15; ++run)
    {
        for (unsigned int i = 0; i < 20; ++i)
        {
            unsigned int wait = gen(rng);

            ++counter;

            pool.Dispatch([&countPool, &container, i, run, wait]
            {
                // std::this_thread::sleep_for(std::chrono::milliseconds(wait));
                std::string str = "Pass " + std::to_string(run) + " dispatching " + std::to_string(i) + " (" + std::to_string(wait) + "ms delay)" + "\n";
                // std::cout << str;

                countPool.Dispatch([&container, wait]
                {
                    container.emplace_back(wait);
                });
            });
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // std::cout << "Awaiting termination...\n";

    // std::this_thread::sleep_for(std::chrono::seconds(5));

    // std::this_thread::sleep_for(std::chrono::seconds(5));

    pool.Await();
    countPool.Await();

    std::cout << container.size() << " == " << counter;
    */
}

template<typename T, typename V>
using cross_forward_t =
    typename std::conditional<
        std::is_rvalue_reference<T&&>::value,
        typename std::decay<V>::type&&,
        typename std::conditional<
            std::is_lvalue_reference<T&&>::value,
            typename std::decay<V>::type&,
            typename std::decay<V>::type
        >::type
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

void test_cross_forward()
{
    TestContainer con;

    con.ptr = std::make_shared<int>(5);

    static_assert(
        std::is_same<cross_forward_t<TestContainer&&, std::unique_ptr<int>&>,
        std::unique_ptr<int>&&>::value,
        "oO");

    static_assert(
        std::is_same<cross_forward_t<TestContainer&, std::unique_ptr<int>&>,
        std::unique_ptr<int>&>::value,
        "oO");

    extract(con);

    extract(std::move(con));

    int i = 0;
    ++i;
}

/// Corrects functionals with non expected signatures
/// to match the expected ones.
template<typename... _ATy>
struct partial_corrector
{
    /// Corrector
    template<typename _CTy>
    static auto correct(_CTy&& functional)
        -> typename std::enable_if<
            !std::is_same<
                fu::argument_type_of_t<
                    typename std::decay<_CTy>::type
                >,
                fu::identity<_ATy...>
            >::value,
            std::function<
              fu::return_type_of_t<
                typename std::decay<_CTy>::type
              >(_ATy...)
            >
           >::type
    {
        // Make use of std::bind's signature erasure
        return std::bind(std::forward<_CTy>(functional));
    }

    // Route through
    template<typename _CTy>
    static auto correct(_CTy&& functional)
        -> typename std::enable_if<
            std::is_same<
                fu::argument_type_of_t<
                    typename std::decay<_CTy>::type
                >,
                fu::identity<_ATy...>
            >::value,
            _CTy
           >::type
    {
        return std::forward<_CTy>(functional);
    }
};

void test_incubator()
{
    test_cross_forward();

    std::function<void(int, float)> fn1 = partial_corrector<int, float>::correct([](int, float)
    {
    });

    std::function<void(int, float)> fn2 = partial_corrector<int, float>::correct([]
    {
    });
}
