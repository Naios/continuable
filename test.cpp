
#include "Callback.h"
#include "WeakCallbackContainer.h"
#include "Continuable.h"

#include <iostream>
#include <exception>
#include <type_traits>

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

int main(int argc, char** argv)
{
    auto lam = [=](Callback<SpellCastResult>&& callback)
    {
        // on success call the callback with SPELL_FAILED_SUCCESS
        // callback(SPELL_FAILED_SUCCESS);
    };

    // static_assert(std::is_void<decltype(lam)>::value, "blub");
    
    fu::function_type_of_t<decltype(lam)> fun1;
    fun1 = lam;
    fun1(Callback<SpellCastResult>());

    fu::function_type_of_t<Callback<int>> fun2;
    
    shared_callback_of_t<std::function<void(int)>> sc1;
    weak_callback_of_t<Callback<int>> sc2;
    
    make_weak_wrapped_callback(sc1);
    make_weak_wrapped_callback(sc2);
    
    typedef Continuable<bool> cont123;

    typedef Continuable<Callback<bool>> myty1;
    typedef Continuable<Callback<bool>, float> myty2;

    // Continuable<Callback<SpellCastResult>> spell
    CastSpell(63362)
      .then([](SpellCastResult result)
        {
            return CastSpell(63362);
        })
        .then([](SpellCastResult result)
        {

        });

    // Wraps a callback function into a continuable
    auto cba1 = make_continuable([=](Callback<SpellCastResult>&& callback)
    {



    });

    std::cout << "ok" << std::endl;
    return 0;
}
