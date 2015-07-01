
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

template<typename...>
class Continuable;

namespace detail
{
    /// Trait to identify continuable types
    template<typename _CTy>
    struct is_continuable
        : public std::false_type { };

    template<typename... Args>
    struct is_continuable<Continuable<Args...>>
        : public std::true_type { };

    /// Creates an empty callback.
    template<typename _FTy>
    struct create_empty_callback;

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

    template <typename _CTy, typename... _ATy>
    struct unary_chainer_t;

    /// Functional traits forward declaration.
    template <typename... _ATy>
    struct functional_traits;

} // detail

/// A continuable provides useful methods to react on the result of callbacks
/// and allows to chain multiple callback calls to a chain.
template<typename... _ATy>
class Continuable
{
    // Make all templates of Continuable to a friend.
    template<typename...>
    friend class Continuable;

public:
    typedef Callback<_ATy...> CallbackFunction;
    typedef std::function<void(Callback<_ATy...>&&)> ForwardFunction;

private:
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
        -> typename detail::unary_chainer_t<_CTy, _ATy...>::continuable_t
    {
        static_assert(std::is_same<fu::identity<_ATy...>,
            typename detail::unary_chainer_t<_CTy, _ATy...>::arguments_t>::value,
                "Given function signature isn't correct, for now it must match strictly!");

        // Transfer the insert function to the local scope.
        // Also use it as an r-value reference to try to get move semantics with c++11 lambdas.
        ForwardFunction&& callback = std::move(_callback_insert);

        auto&& corrected = detail::functional_traits<_ATy...>::
            correct(std::forward<_CTy>(functional));

        return typename detail::unary_chainer_t<_CTy, _ATy...>::continuable_t(
            [corrected, callback](typename detail::unary_chainer_t<_CTy, _ATy...>::callback_t&& call_next)
        {
            callback([corrected, call_next](_ATy&&... args) mutable
            {
                // Invoke the next callback
                corrected(std::forward<_ATy>(args)...)
                    .invoke(std::move(call_next));
            });

        }, std::move(*this));
    }

    /*
    template<typename... _CTy>
    Continuable& _wrap_all(_CTy&&...)
    {
        typedef detail::multiple_all_chainer<_CTy...> type;

        return *this;
    }
    */

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

namespace detail
{
    /// Helper trait for unary chains like `Continuable::then`
    template <typename _CTy, typename... _ATy>
    struct unary_chainer_t
    {
        // Corrected user given functional
        typedef decltype(detail::functional_traits<_ATy...>::
            correct(std::declval<typename std::decay<_CTy>::type>())) corrected_t;

        typedef fu::return_type_of_t<corrected_t> continuable_t;

        typedef fu::argument_type_of_t<corrected_t> arguments_t;

        typedef typename continuable_t::CallbackFunction callback_t;

        typedef fu::argument_type_of_t<callback_t> callback_arguments_t;
    };

    template<typename Left, typename Right>
    struct concat_identities;

    template<typename... Left, typename... Right>
    struct concat_identities<fu::identity<Left...>, fu::identity<Right...>>
    {
        typedef fu::identity<Left..., Right...> type;
    };

    template<typename Left, typename Right>
    struct concat_identities_as_pack;

    template<typename... Left, typename... Right>
    struct concat_identities_as_pack<fu::identity<Left...>, fu::identity<Right...>>
    {
        typedef fu::identity<Left..., std::tuple<Right...>> type;
    };

    template<typename... _CTy>
    struct multiple_chainer_test
    {
        
    };

    template<typename _ATy>
    struct void_wrap_trait;

    /// Trait needed for functional_traits::remove_void_trait
    template<typename... _ATy>
    struct void_wrap_trait<fu::identity<_ATy...>>
    {
        template<typename _CTy>
        static std::function<Continuable<>(_ATy...)> wrap(_CTy&& functional)
        {
            return [functional](_ATy... args)
            {
                // Invoke the original callback
                functional(std::forward<_ATy>(args)...);

                // Return an empty continuable
                return make_continuable();
            };
        }
    };

    /// Continuable processing detail implementation
    template <typename... _ATy>
    struct functional_traits
    {
        /// Wrap void returning functionals to returns an empty continuable.
        template <typename _CTy>
        static auto remove_void_trait(_CTy&& functional)
            -> typename std::enable_if<
                    std::is_void<
                        fu::return_type_of_t<
                            typename std::decay<_CTy>::type
                        >
                    >::value,
                    decltype(
                        detail::void_wrap_trait<
                            fu::argument_type_of_t<
                                typename std::decay<_CTy>::type
                            >
                        >::wrap(std::declval<_CTy>())
                    )
                >::type
        {
            return detail::void_wrap_trait<
                fu::argument_type_of_t<
                    typename std::decay<_CTy>::type
                >
            >::wrap(std::forward<_CTy>(functional));
        }

        /// Route continuable returning functionals through.
        template <typename _CTy>
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
        }

        /// Wrap continuables into the continuable returning functional type.
        template<typename _CTy>
        static auto box_continuable_trait(_CTy&& continuable)
            -> typename std::enable_if<
                    detail::is_continuable<
                        typename std::decay<_CTy>::type
                    >::value,
                    std::function<
                        typename std::decay<_CTy>::type(_ATy...)
                    >
                >::type
        {
            static_assert(std::is_rvalue_reference<_CTy&&>::value,
                "Given continuable must be passed as r-value!");

            // Trick C++11 lambda capture rules for non copyable but moveable continuables.
            std::shared_ptr<typename std::decay<_CTy>::type> shared_continuable =
                std::make_shared<typename std::decay<_CTy>::type>(std::forward<_CTy>(continuable));

            // Create a fake function which returns the value on invoke.
            return [shared_continuable](_ATy&&...)
            {
                return std::move(*shared_continuable);
            };
        }

        /// Route functionals through
        template<typename _CTy>
        inline static auto box_continuable_trait(_CTy&& continuable)
            -> typename std::enable_if<
                    !detail::is_continuable<
                        typename std::decay<_CTy>::type
                    >::value,
                    typename std::decay<_CTy>::type
                >::type
        {
            static_assert(std::is_rvalue_reference<_CTy&&>::value,
                "Given continuable must be passed as r-value!");

            return std::forward<_CTy>(continuable);
        }

        /// Correct user given continuable functionals.
        /// Converts plan continuables to continuable retuning functions.
        /// Converts void return to empty continuable.
        template<typename _CTy>
        static auto correct(_CTy&& functional)
            -> decltype(remove_void_trait(box_continuable_trait(std::declval<_CTy>())))
        {
            return remove_void_trait(box_continuable_trait(std::forward<_CTy>(functional)));
        }

        template<size_t Count, typename Args, typename Pack, typename... Rest>
        struct multiple_result_maker;

        template<size_t Count, typename... Args, typename... Pack>
        struct multiple_result_maker<Count, fu::identity<Args...>, fu::identity<Pack...>>
        {
            typedef fu::identity<Args...> arguments_t;

            typedef fu::identity<Pack...> arguments_storage_t;

            static size_t const size = Count;
        };

        template<size_t Count, typename Args, typename Pack, typename Next>
        struct multiple_result_maker<Count, Args, Pack, Next>
            : public multiple_result_maker<
                Count + 1,
                typename concat_identities<
                    Args,
                    typename unary_chainer_t<Next, _ATy...>::callback_arguments_t
                >::type,
                typename concat_identities_as_pack<
                    Pack,
                    typename unary_chainer_t<Next, _ATy...>::callback_arguments_t
                >::type
              > { };

        template<size_t Count, typename Args, typename Pack, typename Next, typename... Rest>
        struct multiple_result_maker<Count, Args, Pack, Next, Rest...>
            : public multiple_result_maker<
                Count + 1,
                typename concat_identities<
                    Args,
                    typename unary_chainer_t<Next, _ATy...>::callback_arguments_t
                >::type,
                typename concat_identities_as_pack <
                    Pack,
                    typename unary_chainer_t<Next, _ATy...>::callback_arguments_t
                >::type,
                Rest...
            > { };

        template<typename... Args>
        using result_maker_of_t =
            multiple_result_maker<0, fu::identity<>, fu::identity<>, Args...>;
    };
}

#endif // _CONTINUABLE_H_
