
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v4.2.0

  Copyright(c) 2015 - 2022 Denis Blank <denis.blank at outlook dot com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files(the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions :

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
**/

#ifndef CONTINUABLE_DETAIL_TRAVERSE_HPP_INCLUDED
#define CONTINUABLE_DETAIL_TRAVERSE_HPP_INCLUDED

#include <cstddef>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <continuable/detail/traversal/container-category.hpp>
#include <continuable/detail/utility/traits.hpp>

namespace cti {
namespace detail {
namespace traversal {
/// Exposes useful facilities for dealing with 1:n mappings
namespace spreading {
/// \cond false
/// A struct to mark a tuple to be unpacked into the parent context
template <typename... T>
class spread_box {
  std::tuple<T...> boxed_;

public:
  explicit constexpr spread_box(std::tuple<T...> boxed)
      : boxed_(std::move(boxed)) {
  }

  std::tuple<T...> unbox() {
    return std::move(boxed_);
  }
};
template <>
class spread_box<> {
public:
  explicit constexpr spread_box() noexcept {
  }
  explicit constexpr spread_box(std::tuple<>) noexcept {
  }

  constexpr std::tuple<> unbox() const noexcept {
    return std::tuple<>{};
  }
};

/// Returns an empty spread box which represents an empty
/// mapped object.
constexpr spread_box<> empty_spread() noexcept {
  return spread_box<>{};
}

/// Deduces to a true_type if the given type is a spread marker
template <typename T>
struct is_spread : std::false_type {};
template <typename... T>
struct is_spread<spread_box<T...>> : std::true_type {};

/// Deduces to a true_type if the given type is an empty
/// spread marker
template <typename T>
struct is_empty_spread : std::false_type {};
template <>
struct is_empty_spread<spread_box<>> : std::true_type {};

/// Converts types to the type and spread_box objects to its
/// underlying tuple.
template <typename T>
constexpr T unpack(T&& type) {
  return std::forward<T>(type);
}
template <typename... T>
constexpr auto unpack(spread_box<T...> type) -> decltype(type.unbox()) {
  return type.unbox();
}

/// Deduces to the type unpack is returning when called with the
/// the given type T.
template <typename T>
using unpacked_of_t = decltype(unpack(std::declval<T>()));

/// Converts types to the type and spread_box objects to its
/// underlying tuple. If the type is mapped to zero elements,
/// the return type will be void.
template <typename T>
constexpr auto unpack_or_void(T&& type)
    -> decltype(unpack(std::forward<T>(type))) {
  return unpack(std::forward<T>(type));
}
inline void unpack_or_void(spread_box<>) noexcept {
}

/// Converts types to the a tuple carrying the single type and
/// spread_box objects to its underlying tuple.
template <typename T>
constexpr std::tuple<T> undecorate(T&& type) {
  return std::tuple<T>{std::forward<T>(type)};
}
template <typename... T>
constexpr auto undecorate(spread_box<T...> type) -> decltype(type.unbox()) {
  return type.unbox();
}

/// A callable object which maps its content back to a
/// tuple like type.
template <typename EmptyType, template <typename...> class Type>
struct tupelizer_base {
  // We overload with one argument here so Clang and GCC don't
  // have any issues with overloading against zero arguments.
  template <typename First, typename... T>
  constexpr Type<First, T...> operator()(First&& first, T&&... args) const {
    return Type<First, T...>{std::forward<First>(first),
                             std::forward<T>(args)...};
  }

  // Specifically return the empty object which can be different
  // from a tuple.
  constexpr EmptyType operator()() const noexcept(noexcept(EmptyType{})) {
    return EmptyType{};
  }
};

/// A callable object which maps its content back to a tuple.
template <template <typename...> class Type = std::tuple>
using tupelizer_of_t = tupelizer_base<std::tuple<>, Type>;

/// A callable object which maps its content back to a tuple like
/// type if it wasn't empty. For empty types arguments an empty
/// spread box is returned instead. This is useful to propagate
/// empty mappings back to the caller.
template <template <typename...> class Type = std::tuple>
using flat_tupelizer_of_t = tupelizer_base<spread_box<>, Type>;

/// A callable object which maps its content back to an
/// array like type.
/// This transform can only be used for (flat) mappings which
/// return an empty mapping back to the caller.
template <template <typename, std::size_t> class Type>
struct flat_arraylizer {
  /// Deduces to the array type when the array is instantiated
  /// with the given arguments.
  template <typename First, typename... Rest>
  using array_type_of_t = Type<std::decay_t<First>, 1 + sizeof...(Rest)>;

  // We overload with one argument here so Clang and GCC don't
  // have any issues with overloading against zero arguments.
  template <typename First, typename... T>
  constexpr auto operator()(First&& first, T&&... args) const
      -> array_type_of_t<First, T...> {
    return array_type_of_t<First, T...>{
        {std::forward<First>(first), std::forward<T>(args)...}};
  }

  constexpr auto operator()() const noexcept -> decltype(empty_spread()) {
    return empty_spread();
  }
};

/// Use the recursive instantiation for a variadic pack which
/// may contain spread types
template <typename C, typename... T>
constexpr auto apply_spread_impl(std::true_type, C&& callable, T&&... args)
    -> decltype(
        traits::unpack(std::forward<C>(callable),
                       std::tuple_cat(undecorate(std::forward<T>(args))...))) {
  return traits::unpack(std::forward<C>(callable),
                        std::tuple_cat(undecorate(std::forward<T>(args))...));
}

/// Use the linear instantiation for variadic packs which don't
/// contain spread types.
template <typename C, typename... T>
constexpr auto apply_spread_impl(std::false_type, C&& callable, T&&... args)
    -> decltype(std::forward<C>(callable)(std::forward<T>(args)...)) {
  return std::forward<C>(callable)(std::forward<T>(args)...);
}

/// Deduces to a true_type if any of the given types marks
/// the underlying type to be spread into the current context.
template <typename... T>
using is_any_spread_t = traits::disjunction<is_spread<T>...>;

template <typename C, typename... T>
constexpr auto map_spread(C&& callable, T&&... args)
    -> decltype(apply_spread_impl(is_any_spread_t<T...>{},
                                  std::forward<C>(callable),
                                  std::forward<T>(args)...)) {
  // Check whether any of the args is a detail::flatted_tuple_t,
  // if not, use the linear called version for better
  // compilation speed.
  return apply_spread_impl(is_any_spread_t<T...>{}, std::forward<C>(callable),
                           std::forward<T>(args)...);
}

/// Converts the given variadic arguments into a tuple in a way
/// that spread return values are inserted into the current pack.
template <typename... T>
constexpr auto tupelize(T&&... args)
    -> decltype(map_spread(tupelizer_of_t<>{}, std::forward<T>(args)...)) {
  return map_spread(tupelizer_of_t<>{}, std::forward<T>(args)...);
}

/// Converts the given variadic arguments into a tuple in a way
/// that spread return values are inserted into the current pack.
/// If the arguments were mapped to zero arguments, the empty
/// mapping is propagated backwards to the caller.
template <template <typename...> class Type, typename... T>
constexpr auto flat_tupelize_to(T&&... args)
    -> decltype(map_spread(flat_tupelizer_of_t<Type>{},
                           std::forward<T>(args)...)) {
  return map_spread(flat_tupelizer_of_t<Type>{}, std::forward<T>(args)...);
}

/// Converts the given variadic arguments into an array in a way
/// that spread return values are inserted into the current pack.
/// Through this the size of the array like type might change.
/// If the arguments were mapped to zero arguments, the empty
/// mapping is propagated backwards to the caller.
template <template <typename, std::size_t> class Type, typename... T>
constexpr auto flat_arraylize_to(T&&... args)
    -> decltype(map_spread(flat_arraylizer<Type>{}, std::forward<T>(args)...)) {
  return map_spread(flat_arraylizer<Type>{}, std::forward<T>(args)...);
}

/// Converts an empty tuple to void
template <typename First, typename... Rest>
constexpr std::tuple<First, Rest...>
voidify_empty_tuple(std::tuple<First, Rest...> val) {
  return std::move(val);
}
inline void voidify_empty_tuple(std::tuple<>) noexcept {
}

/// Converts the given variadic arguments into a tuple in a way
/// that spread return values are inserted into the current pack.
///
/// If the returned tuple is empty, voidis returned instead.
template <typename... T>
constexpr decltype(auto) tupelize_or_void(T&&... args) {
  return voidify_empty_tuple(tupelize(std::forward<T>(args)...));
}
/// \endcond
} // namespace spreading

/// Just traverses the pack with the given callable object,
/// no result is returned or preserved.
struct strategy_traverse_tag {};
/// Remaps the variadic pack with the return values from the mapper.
struct strategy_remap_tag {};

/// Deduces to a true type if the type leads to at least one effective
/// call to the mapper.
template <typename Mapper, typename T>
using is_effective_t = traits::is_invocable<typename Mapper::traversor_type, T>;

// TODO find out whether the linear compile-time instantiation is faster:
// template <typename Mapper, typename... T>
// struct is_effective_any_of_t
//     : traits::disjunction<is_effective_t<Mapper, T>...> {};
// template <typename Mapper>
// struct is_effective_any_of_t<Mapper> : std::false_type {};

/// Deduces to a true type if any type leads to at least one effective
/// call to the mapper.
template <typename Mapper, typename... T>
struct is_effective_any_of_t;
template <typename Mapper, typename First, typename... Rest>
struct is_effective_any_of_t<Mapper, First, Rest...>
    : std::conditional<is_effective_t<Mapper, First>::value, std::true_type,
                       is_effective_any_of_t<Mapper, Rest...>>::type {};
template <typename Mapper>
struct is_effective_any_of_t<Mapper> : std::false_type {};

/// Provides utilities for remapping the whole content of a
/// container like type to the same container holding different types.
namespace container_remapping {
/// Deduces to a true type if the given parameter T
/// has a push_back method that accepts a type of E.
template <typename T, typename E, typename = void>
struct has_push_back : std::false_type {};
template <typename T, typename E>
struct has_push_back<
    T, E,
    traits::void_t<decltype(std::declval<T>().push_back(std::declval<E>()))>>
    : std::true_type {};

/// Specialization for a container with a single type T
template <typename NewType, template <class> class Base, typename OldType>
auto rebind_container(Base<OldType> const & /*container*/) -> Base<NewType> {
  return Base<NewType>();
}

/// Specialization for a container with a single type T and
/// a particular allocator,
/// which is preserved across the remap.
/// -> We remap the allocator through std::allocator_traits.
template <
    typename NewType, template <class, class> class Base, typename OldType,
    typename OldAllocator,
    // Check whether the second argument of the container was
    // the used allocator.
    typename std::enable_if<std::uses_allocator<
        Base<OldType, OldAllocator>, OldAllocator>::value>::type* = nullptr,
    typename NewAllocator = typename std::allocator_traits<
        OldAllocator>::template rebind_alloc<NewType>>
auto rebind_container(Base<OldType, OldAllocator> const& container)
    -> Base<NewType, NewAllocator> {
  // Create a new version of the allocator, that is capable of
  // allocating the mapped type.
  return Base<NewType, NewAllocator>(NewAllocator(container.get_allocator()));
}

/// Returns the default iterators of the container in case
/// the container was passed as an l-value reference.
/// Otherwise move iterators of the container are returned.
template <typename C, typename = void>
class container_accessor {
  static_assert(std::is_lvalue_reference<C>::value,
                "This should be a lvalue reference here!");

  C container_;

public:
  container_accessor(C container) : container_(container) {
  }

  auto begin() -> decltype(container_.begin()) {
    return container_.begin();
  }

  auto end() -> decltype(container_.end()) {
    return container_.end();
  }
};
template <typename C>
class container_accessor<
    C, typename std::enable_if<std::is_rvalue_reference<C&&>::value>::type> {
  C&& container_;

public:
  container_accessor(C&& container) : container_(std::move(container)) {
  }

  auto begin() -> decltype(std::make_move_iterator(container_.begin())) {
    return std::make_move_iterator(container_.begin());
  }

  auto end() -> decltype(std::make_move_iterator(container_.end())) {
    return std::make_move_iterator(container_.end());
  }
};

template <typename T>
container_accessor<T> container_accessor_of(T&& container) {
  // Don't use any decay here
  return container_accessor<T>(std::forward<T>(container));
}

/// Deduces to the type the homogeneous container is containing
///
/// This alias deduces to the same type on which
/// container_accessor<T> is iterating.
///
/// The basic idea is that we deduce to the type the homogeneous
/// container T is carrying as reference while preserving the
/// original reference type of the container:
/// - If the container was passed as l-value its containing
///   values are referenced through l-values.
/// - If the container was passed as r-value its containing
///   values are referenced through r-values.
template <typename Container>
using element_of_t = typename std::conditional<
    std::is_rvalue_reference<Container&&>::value,
    decltype(std::move(*(std::declval<Container>().begin()))),
    decltype(*(std::declval<Container>().begin()))>::type;

/// Removes all qualifier and references from the given type
/// if the type is a l-value or r-value reference.
template <typename T>
using dereferenced_of_t = typename std::conditional<std::is_reference<T>::value,
                                                    std::decay_t<T>, T>::type;

/// Returns the type which is resulting if the mapping is applied to
/// an element in the container.
///
/// Since standard containers don't allow to be instantiated with
/// references we try to construct the container from a copied
/// version.
template <typename Container, typename Mapping>
using mapped_type_from_t = dereferenced_of_t<spreading::unpacked_of_t<decltype(
    std::declval<Mapping>()(std::declval<element_of_t<Container>>()))>>;

/// Deduces to a true_type if the mapping maps to zero elements.
template <typename T, typename M>
using is_empty_mapped = spreading::is_empty_spread<
    std::decay_t<decltype(std::declval<M>()(std::declval<element_of_t<T>>()))>>;

/// We are allowed to reuse the container if we map to the same
/// type we are accepting and when we have
/// the full ownership of the container.
template <typename T, typename M>
using can_reuse = std::integral_constant<
    bool, std::is_same<element_of_t<T>, mapped_type_from_t<T, M>>::value &&
              std::is_rvalue_reference<T&&>::value>;

/// Categorizes a mapping of a homogeneous container
///
/// \tparam IsEmptyMapped Identifies whether the mapping maps to
///         to zero arguments.
/// \tparam CanReuse Identifies whether the container can be
///         re-used through the mapping.
template <bool IsEmptyMapped, bool CanReuse>
struct container_mapping_tag {};

/// Categorizes the given container through a container_mapping_tag
template <typename T, typename M>
using container_mapping_tag_of_t =
    container_mapping_tag<is_empty_mapped<T, M>::value, can_reuse<T, M>::value>;

/// Deduces to a true type if the given parameter T supports a `reserve` method
template <typename From, typename To, typename = void>
struct is_reservable_from : std::false_type {};
template <typename From, typename To>
struct is_reservable_from<From, To,
                          traits::void_t<decltype(std::declval<To>().reserve(
                              std::declval<From>().size()))>> : std::true_type {
};

template <typename Dest, typename Source>
void reserve_if(std::true_type, Dest&& dest, Source&& source) {
  // Reserve the mapped size
  dest.reserve(source.size());
}
template <typename Dest, typename Source>
void reserve_if(std::false_type, Dest&&, Source&&) noexcept {
  // We do nothing here, since the container doesn't support reserving
}

/// We create a new container, which may hold the resulting type
template <typename M, typename T>
auto remap_container(container_mapping_tag<false, false>, M&& mapper,
                     T&& container)
    -> decltype(rebind_container<mapped_type_from_t<T, M>>(container)) {
  static_assert(has_push_back<std::decay_t<T>, element_of_t<T>>::value,
                "Can only remap containers that provide a push_back "
                "method!");

  // Create the new container, which is capable of holding
  // the remappped types.
  auto remapped = rebind_container<mapped_type_from_t<T, M>>(container);

  // We try to reserve the original size from the source
  // container inside the destination container.
  reserve_if(
      is_reservable_from<std::decay_t<T>, std::decay_t<decltype(remapped)>>{},
      remapped, container);

  // Perform the actual value remapping from the source to
  // the destination.
  // We could have used std::transform for this, however,
  // I didn't want to pull a whole header for it in.
  for (auto&& val : container_accessor_of(std::forward<T>(container))) {
    remapped.push_back(spreading::unpack(
        std::forward<M>(mapper)(std::forward<decltype(val)>(val))));
  }

  return remapped; // RVO
}

/// The remapper optimized for the case that we map to the same
/// type we accepted such as int -> int.
template <typename M, typename T>
auto remap_container(container_mapping_tag<false, true>, M&& mapper,
                     T&& container) -> std::decay_t<T> {
  for (auto&& val : container_accessor_of(std::forward<T>(container))) {
    val = spreading::unpack(
        std::forward<M>(mapper)(std::forward<decltype(val)>(val)));
  }
  return std::forward<T>(container);
}

/// Remap the container to zero arguments
template <typename M, typename T>
auto remap_container(container_mapping_tag<true, false>, M&& mapper,
                     T&& container) -> decltype(spreading::empty_spread()) {
  for (auto&& val : container_accessor_of(std::forward<T>(container))) {
    // Don't save the empty mapping for each invocation
    // of the mapper.
    std::forward<M>(mapper)(std::forward<decltype(val)>(val));
  }
  // Return one instance of an empty mapping for the container
  return spreading::empty_spread();
}

/// \cond false
/// Remaps the content of the given container with type T,
/// to a container of the same type which may contain
/// different types.
template <typename T, typename M>
auto remap(
    strategy_remap_tag, T&& container, M&& mapper,
    typename std::enable_if<is_effective_t<M, element_of_t<T>>::value>::type* =
        nullptr) -> decltype(remap_container(container_mapping_tag_of_t<T, M>{},
                                             std::forward<M>(mapper),
                                             std::forward<T>(container))) {
  return remap_container(container_mapping_tag_of_t<T, M>{},
                         std::forward<M>(mapper), std::forward<T>(container));
}
/// \endcond

/// Just call the visitor with the content of the container
template <typename T, typename M>
void remap(
    strategy_traverse_tag, T&& container, M&& mapper,
    typename std::enable_if<is_effective_t<M, element_of_t<T>>::value>::type* =
        nullptr) {
  for (auto&& element : container_accessor_of(std::forward<T>(container))) {
    std::forward<M>(mapper)(std::forward<decltype(element)>(element));
  }
}
} // end namespace container_remapping

/// Provides utilities for remapping the whole content of a
/// tuple like type to the same type holding different types.
namespace tuple_like_remapping {
template <typename Strategy, typename Mapper, typename T,
          typename Enable = void>
struct tuple_like_remapper;

/// Specialization for std::tuple like types which contain
/// an arbitrary amount of heterogenous arguments.
template <typename M, template <typename...> class Base, typename... OldArgs>
struct tuple_like_remapper<strategy_remap_tag, M, Base<OldArgs...>,
                           // Support for skipping completely untouched types
                           typename std::enable_if<is_effective_any_of_t<
                               M, OldArgs...>::value>::type> {
  M mapper_;

  template <typename... Args>
  auto operator()(Args&&... args) -> decltype(spreading::flat_tupelize_to<Base>(
      std::declval<M>()(std::forward<Args>(args))...)) {
    return spreading::flat_tupelize_to<Base>(
        mapper_(std::forward<Args>(args))...);
  }
};
template <typename M, template <typename...> class Base, typename... OldArgs>
struct tuple_like_remapper<strategy_traverse_tag, M, Base<OldArgs...>,
                           // Support for skipping completely untouched types
                           typename std::enable_if<is_effective_any_of_t<
                               M, OldArgs...>::value>::type> {
  M mapper_;

  template <typename... Args>
  auto operator()(Args&&... args) -> traits::void_t<
      decltype(std::declval<M>()(std::declval<OldArgs>()))...> {
    int dummy[] = {0, ((void)mapper_(std::forward<Args>(args)), 0)...};
    (void)dummy;
  }
};

/// Specialization for std::array like types, which contains a
/// compile-time known amount of homogeneous types.
template <typename M, template <typename, std::size_t> class Base,
          typename OldArg, std::size_t Size>
struct tuple_like_remapper<
    strategy_remap_tag, M, Base<OldArg, Size>,
    // Support for skipping completely untouched types
    typename std::enable_if<is_effective_t<M, OldArg>::value>::type> {
  M mapper_;

  template <typename... Args>
  auto operator()(Args&&... args)
      -> decltype(spreading::flat_arraylize_to<Base>(
          mapper_(std::forward<Args>(args))...)) {
    return spreading::flat_arraylize_to<Base>(
        mapper_(std::forward<Args>(args))...);
  }
};
template <typename M, template <typename, std::size_t> class Base,
          typename OldArg, std::size_t Size>
struct tuple_like_remapper<
    strategy_traverse_tag, M, Base<OldArg, Size>,
    // Support for skipping completely untouched types
    typename std::enable_if<is_effective_t<M, OldArg>::value>::type> {
  M mapper_;

  template <typename... Args>
  auto operator()(Args&&... args)
      -> decltype((std::declval<M>()(std::declval<OldArg>()))()) {
    int dummy[] = {0, ((void)mapper_(std::forward<Args>(args)), 0)...};
    (void)dummy;
  }
};

/// Remaps the content of the given tuple like type T,
/// to a container of the same type which may contain
/// different types.
template <typename Strategy, typename T, typename M>
auto remap(Strategy, T&& container, M&& mapper) -> decltype(traits::unpack(
    std::declval<
        tuple_like_remapper<Strategy, std::decay_t<M>, std::decay_t<T>>>(),
    std::forward<T>(container))) {
  return traits::unpack(
      tuple_like_remapper<Strategy, std::decay_t<M>, std::decay_t<T>>{
          std::forward<M>(mapper)},
      std::forward<T>(container));
}
} // end namespace tuple_like_remapping

/// Base class for making strategy dependent behaviour available
/// to the mapping_helper class.
template <typename Strategy>
struct mapping_strategy_base {
  template <typename T>
  auto may_void(T&& element) const -> std::decay_t<T> {
    return std::forward<T>(element);
  }
};
template <>
struct mapping_strategy_base<strategy_traverse_tag> {
  template <typename T>
  void may_void(T&& /*element*/) const noexcept {
  }
};

/// A helper class which applies the mapping or
/// routes the element through
template <typename Strategy, typename M>
class mapping_helper : protected mapping_strategy_base<Strategy> {
  M mapper_;

  class traversal_callable_base {
    mapping_helper* helper_;

  public:
    explicit traversal_callable_base(mapping_helper* helper) : helper_(helper) {
    }

  protected:
    mapping_helper* get_helper() noexcept {
      return helper_;
    }
  };

  /// A callable object which forwards its invocations
  /// to mapping_helper::traverse.
  class traversor : public traversal_callable_base {
  public:
    using traversal_callable_base::traversal_callable_base;

    /// SFINAE helper
    template <typename T>
    auto operator()(T&& element)
        -> decltype(std::declval<traversor>().get_helper()->traverse(
            Strategy{}, std::forward<T>(element)));

    /// An alias to this type
    using traversor_type = traversor;
  };

  /// A callable object which forwards its invocations
  /// to mapping_helper::try_traverse.
  ///
  /// This callable object will accept any input,
  /// since elements passed to it are passed through,
  /// if the provided mapper doesn't accept it.
  class try_traversor : public traversal_callable_base {
  public:
    using traversal_callable_base::traversal_callable_base;

    template <typename T>
    auto operator()(T&& element)
        -> decltype(std::declval<try_traversor>().get_helper()->try_traverse(
            Strategy{}, std::forward<T>(element))) {
      return this->get_helper()->try_traverse(Strategy{},
                                              std::forward<T>(element));
    }

    /// An alias to the traversor type
    using traversor_type = traversor;
  };

  /// Invokes the real mapper with the given element
  template <typename T>
  auto invoke_mapper(T&& element) -> decltype(
      std::declval<mapping_helper>().mapper_(std::forward<T>(element))) {
    return mapper_(std::forward<T>(element));
  }

  /// SFINAE helper for plain elements not satisfying the tuple like
  /// or container requirements.
  ///
  /// We use the proxy function invoke_mapper here,
  /// because some compilers (MSVC) tend to instantiate the invocation
  /// before matching the tag, which leads to build failures.
  template <typename T>
  auto match(container_category_tag<false, false>, T&& element) -> decltype(
      std::declval<mapping_helper>().invoke_mapper(std::forward<T>(element)));

  /// SFINAE helper for elements satisfying the container
  /// requirements, which are not tuple like.
  template <typename T>
  auto match(container_category_tag<true, false>, T&& container)
      -> decltype(container_remapping::remap(Strategy{},
                                             std::forward<T>(container),
                                             std::declval<traversor>()));

  /// SFINAE helper for elements which are tuple like and
  /// that also may satisfy the container requirements
  template <bool IsContainer, typename T>
  auto match(container_category_tag<IsContainer, true>, T&& tuple_like)
      -> decltype(tuple_like_remapping::remap(Strategy{},
                                              std::forward<T>(tuple_like),
                                              std::declval<traversor>()));

  /// This method implements the functionality for routing
  /// elements through, that aren't accepted by the mapper.
  /// Since the real matcher methods below are failing through SFINAE,
  /// the compiler will try to specialize this function last,
  /// since it's the least concrete one.
  /// This works recursively, so we only call the mapper
  /// with the minimal needed set of accepted arguments.
  template <typename MatcherTag, typename T>
  auto try_match(MatcherTag, T&& element) -> decltype(
      std::declval<mapping_helper>().may_void(std::forward<T>(element))) {
    return this->may_void(std::forward<T>(element));
  }

  /// Match plain elements not satisfying the tuple like or
  /// container requirements.
  ///
  /// We use the proxy function invoke_mapper here,
  /// because some compilers (MSVC) tend to instantiate the invocation
  /// before matching the tag, which leads to build failures.
  template <typename T>
  auto try_match(container_category_tag<false, false>, T&& element) -> decltype(
      std::declval<mapping_helper>().invoke_mapper(std::forward<T>(element))) {
    // T could be any non container or non tuple like type here,
    // take int or hpx::future<int> as an example.
    return invoke_mapper(std::forward<T>(element));
  }

  /// Match elements satisfying the container requirements,
  /// which are not tuple like.
  template <typename T>
  auto try_match(container_category_tag<true, false>, T&& container)
      -> decltype(container_remapping::remap(Strategy{},
                                             std::forward<T>(container),
                                             std::declval<try_traversor>())) {
    return container_remapping::remap(Strategy{}, std::forward<T>(container),
                                      try_traversor{this});
  }

  /// Match elements which are tuple like and that also may
  /// satisfy the container requirements
  /// -> We match tuple like types over container like ones
  template <bool IsContainer, typename T>
  auto try_match(container_category_tag<IsContainer, true>, T&& tuple_like)
      -> decltype(tuple_like_remapping::remap(Strategy{},
                                              std::forward<T>(tuple_like),
                                              std::declval<try_traversor>())) {
    return tuple_like_remapping::remap(Strategy{}, std::forward<T>(tuple_like),
                                       try_traversor{this});
  }

  /// Traverses a single element.
  ///
  /// SFINAE helper: Doesn't allow routing through elements,
  /// that aren't accepted by the mapper
  template <typename T>
  auto traverse(Strategy, T&& element)
      -> decltype(std::declval<mapping_helper>().match(
          std::declval<container_category_of_t<std::decay_t<T>>>(),
          std::declval<T>()));

  /// \copybrief traverse
  template <typename T>
  auto try_traverse(Strategy, T&& element)
      -> decltype(std::declval<mapping_helper>().try_match(
          std::declval<container_category_of_t<std::decay_t<T>>>(),
          std::declval<T>())) {
    // We use tag dispatching here, to categorize the type T whether
    // it satisfies the container or tuple like requirements.
    // Then we can choose the underlying implementation accordingly.
    return try_match(container_category_of_t<std::decay_t<T>>{},
                     std::forward<T>(element));
  }

public:
  explicit mapping_helper(M mapper) : mapper_(std::move(mapper)) {
  }

  /// \copybrief try_traverse
  template <typename T>
  decltype(auto) init_traverse(strategy_remap_tag, T&& element) {
    return spreading::unpack_or_void(
        try_traverse(strategy_remap_tag{}, std::forward<T>(element)));
  }
  template <typename T>
  void init_traverse(strategy_traverse_tag, T&& element) {
    try_traverse(strategy_traverse_tag{}, std::forward<T>(element));
  }

  /// Calls the traversal method for every element in the pack,
  /// and returns a tuple containing the remapped content.
  template <typename First, typename Second, typename... T>
  decltype(auto) init_traverse(strategy_remap_tag strategy, First&& first,
                               Second&& second, T&&... rest) {
    return spreading::tupelize_or_void(
        try_traverse(strategy, std::forward<First>(first)),
        try_traverse(strategy, std::forward<Second>(second)),
        try_traverse(strategy, std::forward<T>(rest))...);
  }

  /// Calls the traversal method for every element in the pack,
  /// without preserving the return values of the mapper.
  template <typename First, typename Second, typename... T>
  void init_traverse(strategy_traverse_tag strategy, First&& first,
                     Second&& second, T&&... rest) {
    try_traverse(strategy, std::forward<First>(first));
    try_traverse(strategy, std::forward<Second>(second));
    int dummy[] = {0,
                   ((void)try_traverse(strategy, std::forward<T>(rest)), 0)...};
    (void)dummy;
  }
};

/// Traverses the given pack with the given mapper and strategy
template <typename Strategy, typename Mapper, typename... T>
decltype(auto) transform(Strategy strategy, Mapper&& mapper, T&&... pack) {
  mapping_helper<Strategy, std::decay_t<Mapper>> helper(
      std::forward<Mapper>(mapper));
  return helper.init_traverse(strategy, std::forward<T>(pack)...);
}
} // namespace traversal
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_TRAVERSE_HPP_INCLUDED
