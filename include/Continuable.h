
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

// debug
#include <typeinfo>

#include <atomic>
#include <mutex>

#include "Callback.h"

template<typename...>
class Continuable;

namespace detail
{
    /// Trait to identify continuable types
    template<typename>
    struct is_continuable
        : std::false_type { };

    template<typename... Args>
    struct is_continuable<Continuable<Args...>>
        : std::true_type { };

    /// Creates an empty callback.
    template<typename>
    struct create_empty_callback;

    template<typename... Args>
    struct create_empty_callback<std::function<void(std::function<void(Args...)>&&)>>
    {
        static auto create()
            -> Callback<Args...>
        {
            return [](Args&&...)
            {
            };
        }
    };

    template <typename, typename...>
    struct unary_chainer_t;

    template <typename...>
    struct multiple_when_all_chainer_t;

    template<typename, typename, typename>
    class multiple_result_storage_t;

    template<std::size_t, typename, typename, typename>
    struct multiple_result_storage_invoker_t;

    /// Functional traits forward declaration.
    template <typename...>
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

    template<std::size_t, typename, typename, typename>
    friend struct detail::multiple_result_storage_invoker_t;

public:
    typedef Callback<_ATy...> CallbackFunction;
    typedef std::function<void(Callback<_ATy...>&&)> ForwardFunction;

private:
    /// Functional which expects a callback that is inserted from the Continuable
    /// to chain everything together
    ForwardFunction _callback_insert;

    /// Was the continuable released (invoked or transfered ownership) already?
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
        : _callback_insert(std::move(right._callback_insert)), _released(right._released)
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

        return typename detail::unary_chainer_t<_CTy, _ATy...>::continuable_t(
            [
                corrected = detail::functional_traits<_ATy...>::correct(std::forward<_CTy>(functional)),
                callback = std::move(_callback_insert)
            ]
            (typename detail::unary_chainer_t<_CTy, _ATy...>::callback_t&& call_next) mutable
        {
            callback([corrected = std::move(corrected), call_next](_ATy&&... args) mutable
            {
                // Invoke the next callback
                corrected(std::forward<_ATy>(args)...).invoke(std::move(call_next));
            });

        }, std::move(*this));
    }

    template<typename... _CTy>
    auto all(_CTy&&... functionals)
        -> typename detail::multiple_when_all_chainer_t<
                fu::identity<_ATy...>,
                fu::identity<_CTy...>
            >::make_result::continuable_t
    {
        return then(
            detail::multiple_when_all_chainer_t<
                fu::identity<_ATy...>,
                fu::identity<_CTy...>
            >::make_when_all(std::forward<_CTy>(functionals)...));
    }

    /// Placeholder
    template<typename... _CTy>
    Continuable some(std::size_t const count, _CTy&&...)
    {
        return std::move(*this);
    }

    /// Placeholder
    template<typename... _CTy>
    auto any(_CTy&&... functionals)
        -> Continuable // FIXME gcc build &-> decltype(some(1, std::declval<_CTy>()...))
    {
        // Equivalent to invoke `some` with count 1.
        return some(1, std::forward<_CTy>(functionals)...);
    }

    /*
    /// Validates the Continuable
    inline Continuable Validate()
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
    template<typename, typename...>
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

    template<typename, typename>
    struct concat_identities;

    template<typename... Left, typename... Right>
    struct concat_identities<fu::identity<Left...>, fu::identity<Right...>>
    {
        typedef fu::identity<Left..., Right...> type;
    };

    template<typename>
    struct identity_to_tuple;

    template<typename... Args>
    struct identity_to_tuple<fu::identity<Args...>>
    {
        typedef std::tuple<Args...> type;
    };

    template<typename>
    struct void_wrap_trait;

    /// Trait needed for functional_traits::remove_void_trait
    template<typename... _ATy>
    struct void_wrap_trait<fu::identity<_ATy...>>
    {
        template<typename _CTy>
        static std::function<Continuable<>(_ATy...)> wrap(_CTy&& functional_)
        {
            return [functional = std::forward<_CTy>(functional_)](_ATy&&... args)
            {
                // Invoke the original callback
                functional(std::forward<_ATy>(args)...);

                // Return an empty continuable
                return make_continuable();
            };
        }
    };

    /// Position wrapper class to pass ints as type
    template<std::size_t Position, typename Tuple>
    struct partial_result
    {
        static std::size_t const position = Position;

        typedef Tuple tuple;
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
            // TODO Use the stack instead of heap variables.
            // std::shared_ptr<typename std::decay<_CTy>::type> shared_continuable =
                // std::make_shared<typename std::decay<_CTy>::type>(std::forward<_CTy>(continuable));

            // Create a fake function which returns the value on invoke.
            return [continuable_ = std::forward<_CTy>(continuable)](_ATy&&...) mutable
            {
                return std::move(continuable_);
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

        template<std::size_t Position, typename Args, typename Pack, typename... Rest>
        struct multiple_result_maker;

        template<std::size_t Position, typename... Args, typename... Pack>
        struct multiple_result_maker<Position, fu::identity<Args...>, fu::identity<Pack...>>
        {
            typedef fu::identity<Args...> arguments_t;

            typedef fu::identity<Pack...> partial_results_t;

            static std::size_t const size = Position;
        };

        template<std::size_t Position, typename Args, typename Pack, typename Next, typename... Rest>
        struct multiple_result_maker<Position, Args, Pack, Next, Rest...>
            : multiple_result_maker<
                Position +
                std::tuple_size<
                    typename identity_to_tuple<
                        typename unary_chainer_t<Next, _ATy...>::callback_arguments_t
                    >::type
                >::value,
                typename concat_identities<
                    Args,
                    typename unary_chainer_t<Next, _ATy...>::callback_arguments_t
                >::type,
                typename concat_identities<
                    Pack,
                    fu::identity<
                        partial_result<
                            Position,
                            typename identity_to_tuple<
                                typename unary_chainer_t<Next, _ATy...>::callback_arguments_t
                            >::type
                        >
                    >
                >::type,
                Rest...
            > { };

        template<typename... Args>
        using result_maker_of_t =
            multiple_result_maker<0, fu::identity<>, fu::identity<>, Args...>;
    };

    template<typename... _ATy, typename... _RTy, typename... _PTy>
    class multiple_result_storage_t<fu::identity<_ATy...>, fu::identity<_RTy...>, fu::identity<_PTy...>>
    {
        template<std::size_t, typename, typename, typename>
        friend struct multiple_result_storage_invoker_t;

        std::size_t partitions_left;

        std::tuple<_RTy...> result;

        Callback<_RTy...> callback;

        std::mutex lock;

    public:
        multiple_result_storage_t(std::size_t partitions, Callback<_RTy...> callback_)
            : partitions_left(partitions), callback(callback_) { }

        void try_invoke()
        {
            // TODO Improve the lock here
            std::lock_guard<std::mutex> guard(lock);
            {
                // Never call callbacks twice!
                // assert(partitions_left);

                // If all partitions have completed invoke the final callback.
                if (--partitions_left == 0)
                {
                    fu::invoke_from_tuple(std::move(callback), std::move(result));
                }
            }
        }
    };

    template<std::size_t Offset, typename... _ATy, typename... _RTy, typename... _PTy>
    struct multiple_result_storage_invoker_t<Offset, fu::identity<_ATy...>, fu::identity<_RTy...>, fu::identity<_PTy...>>
    {
        template <std::size_t NextOffset>
        using move_position_to = multiple_result_storage_invoker_t<NextOffset, fu::identity<_ATy...>, fu::identity<_RTy...>, fu::identity<_PTy...>>;

        template<typename Tuple, typename Current>
        inline static void partial_set(Tuple& result, Current&& current)
        {
            // Store a single callback result in the tuple
            std::get<Offset>(result) = std::forward<Current>(current);
        }

        template<typename Tuple, typename Current, typename... Rest>
        inline static void partial_set(Tuple& result, Current&& current, Rest&&... rest)
        {
            // Set the result...
            partial_set(result, std::forward<Current>(current));

            // ...and continue with the next parameter.
            move_position_to<Offset + 1>::partial_set(result, std::forward<Rest>(rest)...);
        }

        // Do nothing when trying to store empty packs...
        inline static void store(std::tuple<_RTy...>& /*result*/)
        {
        }

        // Store the args in the result tuple
        template<typename... Args>
        inline static void store(std::tuple<_RTy...>& result, Args&&... args)
        {
            partial_set(result, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static void invoke(std::shared_ptr<multiple_result_storage_t<
                fu::identity<_ATy...>, fu::identity<_RTy...>, fu::identity<_PTy...>>> storage,
                    Continuable<Args...>&& continuable)
        {
            // Invoke the continuable
            continuable.invoke([storage](Args&&... args)
            {
                // Route its result to the cache.
                store(storage->result, std::forward<Args>(args)...);

                // Try to invoke the final callback.
                storage->try_invoke();
            });
        }
    };

    template<typename _ATy, typename _RTy, typename _PTy>
    struct multiple_when_all_chainer_t_make_result;

    template<typename... _ATy, typename... _RTy, typename... _PTy>
    struct multiple_when_all_chainer_t_make_result<fu::identity<_ATy...>, fu::identity<_RTy...>, fu::identity<_PTy...>>
    {
        typedef Continuable<_RTy...> continuable_t;

        typedef std::function<continuable_t()> return_t;

        typedef functional_traits<_ATy...> traits_t;

        typedef multiple_result_storage_t<fu::identity<_ATy...>, fu::identity<_RTy...>, fu::identity<_PTy...>> ResultStorage;

        template <std::size_t Offset>
        using invoker_at = multiple_result_storage_invoker_t<Offset, fu::identity<_ATy...>, fu::identity<_RTy...>, fu::identity<_PTy...>>;

        typedef std::shared_ptr<ResultStorage> shared_result_t;

        typedef std::tuple<_ATy...> shared_args_t;

        template <typename... Stack>
        struct distributor;

        template <std::size_t Position, typename Tuple, typename... Stack>
        struct distributor<partial_result<Position, Tuple>, Stack...>
        {
            /// Real function invocation
            template <typename _CTy, typename Arguments>
            inline static void invoke(shared_result_t storage, Arguments&& args, _CTy&& current)
            {
                // Invoke the continuable from the result storage
                invoker_at<Position>::invoke(
                    storage,
                    fu::invoke_from_tuple(
                        traits_t::correct(std::forward<_CTy>(current)),
                        std::forward<Arguments>(args)));
            }

            /// Invoke and pass recursive to itself
            template <typename _CTy, typename Arguments, typename... Rest>
            inline static void invoke(shared_result_t storage, Arguments&& args, _CTy&& current, Rest&&... rest)
            {
                // Invoke the current continuable...
                invoke(storage, std::forward<Arguments>(args), std::forward<_CTy>(current));

                // And continue with the next
                distributor<Stack...>::invoke(storage, std::forward<Arguments>(args), std::forward<Rest>(rest)...);
            }
        };

        template <typename Sequence>
        struct sequenced_invoke;

        template <std::size_t... Sequence>
        struct sequenced_invoke<fu::sequence<Sequence...>>
        {
            template <typename Arguments, typename TupleFunctional>
            inline static void invoke(shared_result_t result, Arguments&& arguments, TupleFunctional&& functional)
            {
                // Invoke the distributor which invokes all given continuables.
                distributor<_PTy...>::invoke(
                    result,
                    std::forward<Arguments>(arguments),
                    std::get<Sequence>(std::forward<TupleFunctional>(functional))...);
            }
        };

        /// Creates a faked function which invokes all sub continuables
        template <typename... _CTy>
        static return_t create(_CTy&&... functionals)
        {
            // C++11 workaround for move semantics of non copyable types
            // TODO Use the stack instead of heap variables.
            auto shared_functionals = std::make_shared<std::tuple<_CTy...>>(
                std::make_tuple(std::forward<_CTy>(functionals)...)
            );

            return [=](_ATy&&... args) mutable
            {
                // TODO Use the stack instead of heap variables.
                auto shared_args =
                    std::make_shared<std::tuple<_ATy...>>(
                        std::forward_as_tuple(std::forward<_ATy>(args)...));

                // Fake continuable which wraps all continuables together
                return make_continuable([=](Callback<_RTy...>&& callback) mutable
                {
                    sequenced_invoke<
                        fu::sequence_of_t<
                            sizeof...(_CTy)
                        >
                    >::invoke(
                        std::make_shared<ResultStorage>(sizeof...(_CTy), callback),
                        std::move(*shared_args),
                        std::move(*shared_functionals)
                    );
                });
            };
        }
    };

    /// Helper trait for multiple chains like `Continuable::all`
    template <typename... _ATy, typename... _CTy>
    struct multiple_when_all_chainer_t<fu::identity<_ATy...>, fu::identity<_CTy...>>
    {
        typedef typename functional_traits<_ATy...>::template result_maker_of_t<_CTy...> result_maker;

        typedef typename result_maker::arguments_t arguments_t;

        typedef typename result_maker::partial_results_t partial_results_t;

        static std::size_t const size = result_maker::size;

        typedef multiple_when_all_chainer_t_make_result<fu::identity<_ATy...>, arguments_t, partial_results_t> make_result;

        // Creates one continuable from multiple ones
        static auto make_when_all(_CTy&&... args)
            -> typename make_result::return_t
        {
            return make_result::create(std::forward<_CTy>(args)...);
        }
    };
}

#endif // _CONTINUABLE_H_
