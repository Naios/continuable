
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

#ifndef _CONTINUABLE_H_
#define _CONTINUABLE_H_

#include "Callback.h"

// Debug includes
#include <iostream>
#include <typeinfo>
#include <string>
template <typename T>
void log_type(T t, std::string const& msg = "")
{
    std::cout << msg << ": " << typeid(t).name() << std::endl;
}
void debug(std::string const& m)
{
    std::cout << m << std::endl;
}
/// Debug end

// Continuable forward declaration.
template<typename...>
class Continuable;

namespace detail
{
    // convert_void_to_continuable forward declaration.
    /// Corrects void return types from functional types which should be
    /// Continuable<DefaultContinuableState, Callback<>>
    template<typename _RTy>
    struct convert_void_to_continuable;

    // unary_chainer forward declaration.
    template<typename _NextRTy, typename... _NextATy>
    struct unary_chainer;

    // creates an empty callback.
    template<typename _FTy>
    struct create_empty_callback;

    // trait to identify continuable types
    template<typename _CTy>
    struct is_continuable
        : public std::false_type { };

    template<typename... Args>
    struct is_continuable<Continuable<Args...>>
        : public std::true_type { };

    // MSVC 12 has issues to detect the parameter pack otherwise.
    template<typename _NextRTy, typename... _NextATy>
    struct unary_chainer<_NextRTy, fu::identity<_NextATy...>>
    {
        typedef convert_void_to_continuable<_NextRTy> base;

        typedef typename convert_void_to_continuable<_NextRTy>::type result_t;

        typedef typename result_t::CallbackFunction callback_t;
    };

    template<typename... _CTy>
    struct multiple_all_chainer
    {

    };

    template <typename _CTy>
    using unary_chainer_t = unary_chainer<
        fu::return_type_of_t<typename std::decay<_CTy>::type>,
        fu::argument_type_of_t<typename std::decay<_CTy>::type>>;

    template<typename... Args>
    struct create_empty_callback<std::function<void(std::function<void(Args...)>&&)>>
    {
        static auto create()
            -> Callback<Args...>
        {
            return [](Args...)
            {
            };
        }
    };

    template<typename Args>
    struct void_wrap_trait;

    template <typename _CTy>
    struct functional_corrector;

} // detail

template<typename... _ATy>
class Continuable
{
    // Make all templates of Continuable to a friend.
    template<typename...>
    friend class Continuable;

public:
    typedef Callback<_ATy...> CallbackFunction;
    typedef std::function<void(Callback<_ATy...>&&)> ForwardFunction;

// private:
    /// Functional which expects a callback that is inserted from the Continuable
    /// to chain everything together
    ForwardFunction _callback_insert;

    bool _released;

    template <typename _CTy>
    void invoke(_CTy&& callback)
    {
        if (!_released)
        {
            // Invalidate this
            _released = true;

            // Invoke this
            _callback_insert(std::forward<_CTy>(callback));
        }
    }

    /// Wrap void returning functionals to returns an empty continuable.
    template <typename _CTy>
    static auto remove_void_trait(_CTy&& functional)
        -> typename std::enable_if<
                std::is_void<
                    fu::return_type_of_t<
                        typename std::decay<_CTy>::type
                    >
                >::value,
                /*decltype(
                    detail::void_wrap_trait<
                        fu::return_type_of_t<
                            typename std::decay<_CTy>::type
                        >
                    >::wrap(std::declval<_CTy>())
                )
                */
                decltype(
                    detail::functional_corrector<_CTy>::correct(std::declval<_CTy>())
                )
                // std::function<Continuable<>()>
            >::type
    {
        /*return detail::void_wrap_trait<
            fu::return_type_of_t<
                typename std::decay<_CTy>::type
            >
        >::wrap(std::forward<_CTy>(functional));*/
        return detail::functional_corrector<_CTy>::correct(std::forward<_CTy>(functional));
    }

    /// Route continuable returning functionals through.
    /*template <typename _CTy>
    static auto remove_void_trait(_CTy&& functional)
        -> typename std::enable_if<
                !std::is_void<
                    fu::return_type_of_t<
                        typename std::decay<_CTy>::type
                    >
                >::value,
                _CTy>::type
    {
        return std::forward<_CTy>(functional);
    }*/

    /// Wrap continuables into the continuable returning functional type.
    template<typename _CTy>
    static auto box_continuable_trait(_CTy&& continuable)
        -> typename std::enable_if<detail::is_continuable<typename std::decay<_CTy>::type>::value,
                std::function<typename std::decay<_CTy>::type(_ATy...)>>::type
    {
        // Trick C++11 lambda capture rules for non copyable but moveable continuables.
        std::shared_ptr<typename std::decay<_CTy>::type> shared_continuable =
            std::make_shared<typename std::decay<_CTy>::type>(std::forward<_CTy>(continuable));

        // Create a fake function which returns the value on invoke.
        return [shared_continuable](_ATy...)
        {
            return std::move(*shared_continuable);
        };
    }

    /// Route functionals through and forward to remove_void_trait
    template<typename _CTy>
    static auto box_continuable_trait(_CTy&& continuable)
        -> typename std::enable_if<!detail::is_continuable<typename std::decay<_CTy>::type>::value,
                typename std::decay<_CTy>::type>::type
    {
        return continuable;
    }

public:
    /// Deleted copy construct
    Continuable(Continuable const&) = delete;

    /// Move construct
    Continuable(Continuable&& right)
        : _released(right._released), _callback_insert(std::move(right._callback_insert))
    {
        right._released = true;
    }

    // Construct through a ForwardFunction
    template<typename _FTy>
    Continuable(_FTy&& callback_insert)
        : _callback_insert(std::forward<_FTy>(callback_insert)), _released(false) { }

    template<typename... _RATy, typename _FTy>
    Continuable(_FTy&& callback_insert, Continuable<_RATy...>&& right)
        : _callback_insert(std::forward<_FTy>(callback_insert)), _released(right._released)
    {
        right._released = true;
    }

    /// Destructor which calls the dispatch chain if needed.
    ~Continuable()
    {
        // Dispatch everything.
        if (!_released)
        {
            // Set released to true to prevent multiple calls
            _released = true;

            // Invoke everything with an empty callback
            _callback_insert(detail::create_empty_callback<ForwardFunction>::create());
        }
    }

    /// Deleted copy assign
    Continuable& operator= (Continuable const&) = delete;

    /// Move construct assign
    Continuable& operator= (Continuable&& right)
    {
        _released = right._released;
        right._released = true;

        _callback_insert = std::move(right._callback_insert);
        return *this;
    }

    /// Waits for this continuable and invokes the given callback.
    template<typename _CTy>
    auto then(_CTy&& functional)
        -> typename std::enable_if<!detail::is_continuable<typename std::decay<_CTy>::type>::value,
                typename detail::unary_chainer_t<_CTy>::result_t>::type
    {
        // Transfer the insert function to the local scope.
        // Also use it as an r-value reference to try to get move semantics with c++11 lambdas.
        ForwardFunction&& callback = std::move(_callback_insert);

        return typename detail::unary_chainer_t<_CTy>::result_t(
            [functional, callback](typename detail::unary_chainer_t<_CTy>::callback_t&& call_next)
        {
            callback([functional, call_next](_ATy&&... args) mutable
            {
                // Invoke the next callback
                detail::unary_chainer_t<_CTy>::base::invoke(functional, std::forward<_ATy>(args)...)
                    .invoke(std::move(call_next));
            });

        }, std::move(*this));
    }

    /// Waits for this continuable and continues with the given one.
    template<typename _CTy>
    auto then(_CTy&& continuable)
        -> typename std::enable_if<detail::is_continuable<typename std::decay<_CTy>::type>::value,
                typename std::decay<_CTy>::type>::type
    {
        static_assert(std::is_rvalue_reference<_CTy&&>::value,
            "Given continuable must be passed as r-value!");

        return then(box_continuable_trait(std::forward<_CTy>(continuable)));
    }

    template<typename... _CTy>
    Continuable& _wrap_all(_CTy&&...)
    {
        typedef detail::multiple_all_chainer<_CTy...> type;

        return *this;
    }

    /// Placeholder
    template<typename... _CTy>
    auto all(_CTy&&... functionals)
        -> Continuable&
    {
        return *this;
    }

    /// Placeholder
    template<typename... _CTy>
    Continuable& some(size_t const count, _CTy&&...)
    {
        return *this;
    }

    /// Placeholder
    template<typename... _CTy>
    auto any(_CTy&&... functionals)
        -> Continuable& // FIXME gcc build &-> decltype(some(1, std::declval<_CTy>()...))
    {
        // Equivalent to invoke `some` with count 1.
        return some(1, std::forward<_CTy>(functionals)...);
    }

    /*
    /// Validates the Continuable
    inline Continuable& Validate()
    {
        _released = false;
        return *this;
    }

    /// Invalidates the Continuable
    inline Continuable& Invalidate()
    {
        _released = true;
        return *this;
    }
    */
};

namespace detail
{
    template <typename _CTy>
    struct functional_corrector
    {
        static int correct(_CTy&&)
        {
            return 1;
        }
    };

    template<typename... Args>
    struct void_wrap_trait<fu::identity<Args...>>
    {
        template<typename _CTy>
        static std::function<Continuable<>(Args...)> wrap(_CTy&& functional)
        {
            return [functional](Args... args)
            {
                // Invoke the original callback
                functional(std::forward<Args>(args)...);

                // FIXME return make_empty_continuable()
                // Return a fake continuable
                return Continuable<>([](Callback<>&& callback)
                {
                    callback();
                });
            };
        }
    };

    
    template<>
    struct convert_void_to_continuable<void>
    {
        typedef Continuable<> type;

        template<typename Fn, typename... Args>
        static type invoke(Fn functional, Args&&... args)
        {
            // Invoke the void returning functional
            functional(std::forward<Args>(args)...);

            // Return a fake void continuable
            return type([](Callback<>&& callback)
            {
                callback();
            });
        }
    };

    template<typename... _CTy>
    struct convert_void_to_continuable<Continuable<_CTy...>>
    {
        typedef Continuable<_CTy...> type;

        template<typename Fn, typename... Args>
        static type invoke(Fn functional, Args&&... args)
        {
            // Invoke the functional as usual.
            return functional(std::forward<Args>(args)...);
        }
    };
    /// A continuable provides useful methods to react on the result of callbacks
    /// and allows to chain multiple callback calls to a chain.

    template<typename _RTy, typename... _ATy>
    struct ContinuableFactory;

    template<typename _RTy, typename... _ATy>
    struct ContinuableFactory<_RTy, ::fu::identity<std::function<void(_ATy...)>&&>>
    {
        template<typename _FTy>
        static auto CreateFrom(_FTy&& functional)
            -> Continuable<_ATy...>
        {
            return Continuable<_ATy...>(
                typename Continuable<_ATy...>::ForwardFunction(std::forward<_FTy>(functional)));
        }
    };

    template<typename _FTy>
    using continuable_factory_t = ContinuableFactory<
        ::fu::return_type_of_t<_FTy>, ::fu::argument_type_of_t<_FTy>>;
}

/// Wraps a functional object which expects a r-value callback as argument into a continuable.
/// The callable is invoked when the continuable shall continue.
/// For example:
/// make_continuable([](Callback<int>&& callback)
/// {
///     /* Continue here */
///     callback(5);
/// });
template<typename _FTy>
inline auto make_continuable(_FTy&& functional)
    -> decltype(detail::continuable_factory_t<_FTy>::CreateFrom(std::declval<_FTy>()))
{
    return detail::continuable_factory_t<_FTy>::CreateFrom(std::forward<_FTy>(functional));
}

/// Creates an empty continuable.
/// Can be used to start a chain with aggregate methods.
/// empty_continuable()
///     .all(...)
///     .some(...)
///     .any(...)
inline auto make_continuable()
    -> Continuable<>
{
    return make_continuable([](Callback<>&& callback)
    {
        callback();
    });
}

#endif // _CONTINUABLE_H_
