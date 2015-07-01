
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

Continuable<> Log(std::string const& message)
{
    return make_continuable([=](Callback<>&& callback)
    {
        std::cout << message << std::endl;
        callback();
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
    return Log(msg).then(make_continuable([=](Callback<>&& callback)
    {
        callback();
    }));
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
/*
template<std::size_t N>
struct Apply {
    template<typename F, typename T, typename... A>
    static inline auto apply(F && f, T && t, A &&... a)
        -> decltype(Apply<N-1>::apply(
            ::std::forward<F>(f), ::std::forward<T>(t),
            ::std::get<N-1>(::std::forward<T>(t)), ::std::forward<A>(a)...
        ))
    {
        return Apply<N-1>::apply(::std::forward<F>(f), ::std::forward<T>(t),
            ::std::get<N-1>(::std::forward<T>(t)), ::std::forward<A>(a)...
        );
    }
};

template<>
struct Apply<0> {
    template<typename F, typename T, typename... A>
    static inline auto apply(F && f, T &&, A &&... a)
        -> decltype(::std::forward<F>(f)(::std::forward<A>(a)...))
    {
        return ::std::forward<F>(f)(::std::forward<A>(a)...);
    }
};

template<typename F, typename T>
inline auto apply(F && f, T && t)
    -> decltype(Apply< ::std::tuple_size<
        typename ::std::decay<T>::type
    >::value>::apply(::std::forward<F>(f), ::std::forward<T>(t)))
{
    return Apply< ::std::tuple_size<
        typename ::std::decay<T>::type
    >::value>::apply(::std::forward<F>(f), ::std::forward<T>(t));
}
*/

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
        .then(Validate())
        .then(TrivialPromise("huhu"))
        .then(CastSpellPromise(3))
        .then(CastSpellPromise(4))
        .then(CastSpellPromise(5))
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
        std::function<Continuable<>()>

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

    std::cout << "ok" << std::endl;
    return 0;
}
