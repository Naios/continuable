
#include "Callback.h"
#include "WeakCallbackContainer.h"
#include "Continuable.h"

#include <iostream>
#include <exception>
#include <type_traits>
#include <string>
#include <vector>
#include <typeinfo>

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
    return make_continuable([=](Callback<>&& callback)
    {
        if (!msg.empty())
            std::cout << msg << std::endl;

        callback();
    });
}

Continuable<bool> Validate()
{
    return make_continuable([=](Callback<bool>&& callback)
    {
        std::cout << "Validate " << std::endl;

        callback(true);
    });
}

template <typename... T>
void test_unwrap(std::string const& msg)
{
    std::cout << msg << " is unwrappable: " << (fu::is_unwrappable<T...>::value ? "true" : "false") << std::endl;
}

int main(int /*argc*/, char** /*argv*/)
{
    CastSpellPromise(1)
        .then([](SpellCastResult)
        {
            return CastSpellPromise(2);
        })
        .then([](SpellCastResult)
        {
            std::cout << "Pause a callback (void test) " << std::endl;
        })
        .then(CastSpellPromise(3))
        .then(CastSpellPromise(4))
        .then(CastSpellPromise(5))
        .then([](SpellCastResult)
        {
            return Validate();
        });

    // Mockup of aggregate methods
    empty_continuable()
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
        });

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

    std::cout << "ok" << std::endl;
    return 0;
}
