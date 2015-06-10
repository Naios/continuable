
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

// Continuable<Callback<SpellCastResult>>
int CastSpell(int id)
{
    return 1;
    make_continuable([=](Callback<SpellCastResult>&& callback)
    {
        std::cout << "Cast " << id << std::endl;

        // on success call the callback with SPELL_FAILED_SUCCESS
        callback(SPELL_FAILED_SUCCESS);
    });
}

int main(int argc, char** argv)
{
    typedef Continuable<Callback<bool>> cont123;

    // typedef Continuable<Callback<bool>>::type myty1;
    // typedef Continuable<Callback<bool>, float>::type myty2;

    // Continuable<Callback<SpellCastResult>>
    CastSpell(63362);

    Continuable<Callback<SpellCastResult>> spell;
        spell.then([](SpellCastResult result)
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

    return 0;
}
