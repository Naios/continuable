
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

/*
 * The functional unwrap trait provides the possibility to extract
 * various types out of c++ functional types, like std::function, functors and more.
 */

#ifndef _FUNCTIONAL_UNWRAP_HPP_
#define _FUNCTIONAL_UNWRAP_HPP_

#include <functional>
#include <type_traits>
#include <utility>
#include <cstddef>

namespace fu
{
    /// Identity class which is used to carry parameter packs of types.
    template<typename... Args>
    struct identity { };

    /// Sequence class which is used to carry parameter packs of unsigned integers.
    template<std::size_t...>
    struct sequence { };

    /// The Sequence generator generates a sequence of ascending numbers with the given size.
    template<std::size_t...>
    struct sequence_generator;

    template<std::size_t... Stack>
    struct sequence_generator<0, Stack...>
    {
        typedef sequence<Stack...> type;
    };

    template<std::size_t Position, std::size_t... Stack>
    struct sequence_generator<Position, Stack...>
        : sequence_generator<Position - 1, Position - 1, Stack...> { };

    /// Sequence generator alias
    template<std::size_t Size>
    using sequence_of_t = typename sequence_generator<Size>::type;

    namespace detail
    {
        template<typename Function>
        struct unwrap_function_impl;

        template<typename _RTy, typename... _ATy>
        struct unwrap_function_impl<_RTy(_ATy...)>
        {
            /// The return type of the function.
            typedef _RTy return_type;

            /// The argument types of the function as pack in fu::identity.
            typedef identity<_ATy...> argument_type;

            /// The function provided as std::function
            typedef std::function<_RTy(_ATy...)> function_type;

            /// The function provided as function_ptr
            typedef _RTy(*function_ptr)(_ATy...);
        };

        /// Pack in fu::identity
        template<typename _RTy, typename... _ATy>
        struct unwrap_function_impl<identity<_RTy, _ATy...>>
            : unwrap_function_impl<_RTy(_ATy...)> { };

        /// STL: std::function
        template<typename _RTy, typename... _ATy>
        struct unwrap_function_impl<std::function<_RTy(_ATy...)>>
            : unwrap_function_impl<_RTy(_ATy...)> { };

        /// STL: std::tuple
        template<typename _RTy, typename... _ATy>
        struct unwrap_function_impl<std::tuple<_RTy, _ATy...>>
            : unwrap_function_impl<_RTy(_ATy...)> { };

        /// Const function pointers
        template<typename _RTy, typename... _ATy>
        struct unwrap_function_impl<_RTy(*const)(_ATy...)>
            : unwrap_function_impl<_RTy(_ATy...)> { };

        /// Mutable function pointers
        template<typename _RTy, typename... _ATy>
        struct unwrap_function_impl<_RTy(*)(_ATy...)>
            : unwrap_function_impl<_RTy(_ATy...)> { };

        /// Const class method pointers
        template<typename _CTy, typename _RTy, typename... _ATy>
        struct unwrap_function_impl<_RTy(_CTy::*)(_ATy...) const>
            : unwrap_function_impl<_RTy(_ATy...)> { };

        /// Mutable class method pointers
        template<typename _CTy, typename _RTy, typename... _ATy>
        struct unwrap_function_impl<_RTy(_CTy::*)(_ATy...)>
            : unwrap_function_impl<_RTy(_ATy...)> { };

        /// Unwrap through pointer of functor.
        template<typename Function>
        static auto select_best_unwrap_unary_arg(int)
            -> unwrap_function_impl<decltype(&Function::operator())>;

        /// Unwrap through plain type.
        template<typename Function>
        static auto select_best_unwrap_unary_arg(long)
            -> unwrap_function_impl<Function>;

        template<typename... _FTy>
        struct select_best_unwrap;

        /// Enable only if 1 template argument is given.
        template<typename _FTy>
        struct select_best_unwrap<_FTy>
        {
            typedef decltype(select_best_unwrap_unary_arg<_FTy>(0)) type;
        };

        // Enable if more then 1 template argument is given.
        // (Handles lazy typing)
        template<typename _RTy, typename... _ATy>
        struct select_best_unwrap<_RTy, _ATy...>
        {
            typedef unwrap_function_impl<_RTy(_ATy...)> type;
        };

        template<typename>
        struct to_true
            : std::true_type { };

        /// std::true_type if unwrappable
        template<typename... Function>
        static auto test_unwrappable(int)
            -> to_true<typename select_best_unwrap<Function...>::type::function_type>;

        /// std::false_type if not unwrappable
        template<typename... Function>
        static auto test_unwrappable(long)
            -> std::false_type;

     } // detail

    /// Trait to unwrap function parameters of various types:
    /// Function style definition: Result(Parameters...)
    /// STL `std::function` : std::function<Result(Parameters...)>
    /// STL `std::tuple` : std::tuple<Result, Parameters...>
    /// C++ Function pointers: `Result(*)(Parameters...)`
    /// C++ Class method pointers: `Result(Class::*)(Parameters...)`
    /// Lazy typed signatures: `Result, Parameters...`
    /// Also provides optimized unwrap of functors and functional objects.
    template<typename... Function>
    using unwrap_function =
        typename detail::select_best_unwrap<Function...>::type;

    /// Trait which defines the return type of the function.
    template<typename... Function>
    using return_type_of_t =
        typename detail::select_best_unwrap<Function...>::type::return_type;

    /// Trait which defines the argument types of the function packed in std::tuple.
    template<typename... Function>
    using argument_type_of_t =
        typename detail::select_best_unwrap<Function...>::type::argument_type;

    /// Trait which defines the std::function type of the function.
    template<typename... Function>
    using function_type_of_t =
        typename detail::select_best_unwrap<Function...>::type::function_type;

    /// Trait which defines the function pointer type of the function.
    template<typename... Function>
    using function_ptr_of_t =
        typename detail::select_best_unwrap<Function...>::type::function_ptr;

    /// Trait which defines if the given function is unwrappable or not.
    template<typename... Function>
    struct is_unwrappable
        : decltype(detail::test_unwrappable<Function...>(0)) { };

    /// Converts any functional type in std::function.
    template<typename Functional>
    auto functionfy(Functional&& functional)
        -> function_type_of_t<typename std::decay<Functional>::type>
    {
        return function_type_of_t<typename std::decay<Functional>::type>(std::forward<Functional>(functional));
    }

    template<typename T>
    struct requires_functional_constructible
    {
        static_assert(is_unwrappable<typename std::decay<T>::type>::value,
            "Given type is not functional unwrappable, did you try to use a std::bind expression or a non functional type?");

        typedef T type;
    };

    namespace detail
    {
        /// Implementation of invoke_from_tuple
        template<typename Sequence>
        struct invoker;

        template<std::size_t... Sequence>
        struct invoker<sequence<Sequence...>>
        {
            template<typename _FTy, typename _TTy>
            static auto invoke(_FTy&& functional, _TTy&& tuple)
                -> ::fu::return_type_of_t<typename std::decay<_FTy>::type>
            {
                return std::forward<_FTy>(functional)(std::get<Sequence>(std::forward<_TTy>(tuple))...);
            }
        };
    }

    /// Invokes a function type with the given tuple arguments.
    template<typename _FTy, typename _TTy>
    inline auto invoke_from_tuple(_FTy&& functional, _TTy&& tuple)
        -> return_type_of_t<typename std::decay<_FTy>::type>
    {
        return detail::invoker<
            sequence_of_t<
                std::tuple_size<
                    typename std::decay<_TTy>::type
                >::value
            >
        >::invoke(std::forward<_FTy>(functional), std::forward<_TTy>(tuple));
    }

} // fu

#endif // _FUNCTIONAL_UNWRAP_HPP_
