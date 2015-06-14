
#include "Callback.h"
#include "WeakCallbackContainer.h"
#include "Continuable.h"

#include <iostream>
#include <exception>
#include <type_traits>
#include <string>
#include <vector>

enum SpellCastResult
{
    SPELL_FAILED_SUCCESS = 0,
    SPELL_FAILED_AFFECTING_COMBAT = 1,
    SPELL_FAILED_ALREADY_AT_FULL_HEALTH = 2,
    SPELL_FAILED_ALREADY_AT_FULL_MANA = 3,
    SPELL_FAILED_ALREADY_AT_FULL_POWER = 4,
    SPELL_FAILED_ALREADY_BEING_TAMED = 5
};

Continuable<SpellCastResult> CastSpell(int id)
{
    return make_continuable([=](Callback<SpellCastResult>&& callback)
    {
        std::cout << "Cast " << id << std::endl;

        // on success call the callback with SPELL_FAILED_SUCCESS
        callback(SPELL_FAILED_SUCCESS);
    });
}

template <typename... T>
void test_unwrap(std::string const& msg)
{
    std::cout << msg << " is unwrappable: " << (fu::is_unwrappable<T...>::value ? "true" : "false") << std::endl;
}

int main(int /*argc*/, char** /*argv*/)
{
    Continuable<bool> cb = make_continuable([](Callback<bool>&& callback)
    {

        callback(true);
    });

    test_unwrap<void()>("void()");
    test_unwrap<std::function<void()>>("std::function<void()>");
    test_unwrap<std::vector<std::string>>("std::vector<std::string>");

    make_continuable([=](Callback<>&&)
    {

    });

    int i = 0;
    ++i;

    auto lam = [=](Callback<SpellCastResult>&&)
    {
        // on success call the callback with SPELL_FAILED_SUCCESS
        // callback(SPELL_FAILED_SUCCESS);
    };

    fu::function_type_of_t<decltype(lam)> fun1;
    fun1 = lam;
    fun1(Callback<SpellCastResult>());

    fu::function_type_of_t<Callback<int>> fun2;
    
    shared_callback_of_t<std::function<void(int)>> sc1;
    weak_callback_of_t<Callback<int>> sc2;
    
    make_weak_wrapped_callback(sc1);
    make_weak_wrapped_callback(sc2);

    WeakCallbackContainer callback;
    
    auto weakCallback = callback([]
    {
    });

    typedef Continuable<bool> cont123;

    typedef Continuable<bool> myty1;
    typedef Continuable<bool, float> myty2;

    // Continuable<Callback<SpellCastResult>> spell
    CastSpell(63362)
        .then([](SpellCastResult)
        {
            return CastSpell(63362);
        })
        .then([](SpellCastResult)
        {

        });

    // Wraps a callback function into a continuable
    auto cba1 = make_continuable([=](Callback<SpellCastResult>&&)
    {



    });

    cba1.invalidate();

    CastSpell(63362);

    std::vector<int> myvec;

    typedef fu::requires_functional_constructible<std::function<void()>>::type test_assert1;
    // typedef fu::requires_functional_constructible<std::vector<int>>::type test_assert2;

    // Brainstorming: this shows an example callback chain
    // Given by continuable
    std::function<void(Callback<SpellCastResult>&&)> fn1 = [](Callback<SpellCastResult>&& callback)
    {
        callback(SPELL_FAILED_AFFECTING_COMBAT);
    };

    // Implemented by user
    std::function<std::function<void(Callback<bool>&&)>(SpellCastResult)> cn1 = [](SpellCastResult)
    {
        // Given by continuable
        return [](Callback<bool>&& callback)
        {
            callback(true);
        };
    };

    // Auto created wrapper by the continuable
    std::function<void(SpellCastResult)> wr1 = [&cn1](SpellCastResult result)
    {
        // Wrapper functional to process unary or multiple promised callbacks
        // Returned from the user
        std::function<void(Callback<bool>&&)> fn2 = cn1(result);

        fn2([](bool val)
        {
            // Finished
            std::cout << "Callback chain finished! -> " << val << std::endl;
        });
    };

    Callback<> entry = [&]
    {
        fn1(std::move(wr1));
    };

    entry();

    std::cout << "ok" << std::endl;
    return 0;
}
