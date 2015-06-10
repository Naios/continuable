
#include "Callback.h"
#include "WeakCallbackContainer.h"
#include "Continuable.h"

#include <iostream>
#include <exception>

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
    // typedef shared_callback_of_t<Callback<int>> sc1;
    // typedef weak_callback_of_t<Callback<bool>> sc2;
  
    typedef Continuable<bool> cont123;

    // typedef Continuable<Callback<bool>>::type myty1;
    // typedef Continuable<Callback<bool>, float>::type myty2;

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
