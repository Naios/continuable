
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

#ifndef CONTINUABLE_DETAIL_TRAVERSE_ASYNC_HPP_INCLUDED
#define CONTINUABLE_DETAIL_TRAVERSE_ASYNC_HPP_INCLUDED

#include <atomic>
#include <cassert>
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
/// A tag which is passed to the `operator()` of the visitor
/// if an element is visited synchronously.
struct async_traverse_visit_tag {};

/// A tag which is passed to the `operator()` of the visitor
/// if an element is visited after the traversal was detached.
struct async_traverse_detach_tag {};

/// A tag which is passed to the `operator()` of the visitor
/// if the asynchronous pack traversal was finished.
struct async_traverse_complete_tag {};

/// A tag to identify that a mapper shall be constructed in-place
/// from the first argument passed.
template <typename T>
struct async_traverse_in_place_tag {};

/// Relocates the given pack with the given offset
template <std::size_t Offset, typename Pack>
struct relocate_index_pack;
template <std::size_t Offset, std::size_t... Sequence>
struct relocate_index_pack<Offset,
                           std::integer_sequence<std::size_t, Sequence...>>
    : std::common_type<
          std::integer_sequence<std::size_t, (Sequence + Offset)...>> {};

/// Creates a sequence from begin to end explicitly
template <std::size_t Begin, std::size_t End>
using explicit_range_sequence_of_t =
    typename relocate_index_pack<Begin,
                                 std::make_index_sequence<End - Begin>>::type;

/// Continues the traversal when the object is called
template <typename Frame, typename State>
class resume_traversal_callable {
  Frame frame_;
  State state_;

public:
  explicit resume_traversal_callable(Frame frame, State state)
      : frame_(std::move(frame)), state_(std::move(state)) {
  }

  /// The callable operator for resuming
  /// the asynchronous pack traversal
  void operator()();
};

/// Creates a resume_traversal_callable from the given frame and the
/// given iterator tuple.
template <typename Frame, typename State>
auto make_resume_traversal_callable(Frame&& frame, State&& state)
    -> resume_traversal_callable<std::decay_t<Frame>, std::decay_t<State>> {
  return resume_traversal_callable<std::decay_t<Frame>, std::decay_t<State>>(
      std::forward<Frame>(frame), std::forward<State>(state));
}

template <typename T, typename = void>
struct has_head : std::false_type {};
template <typename T>
struct has_head<T, traits::void_t<decltype(std::declval<T>().head())>>
    : std::true_type {};

template <typename Visitor, typename... Args>
class async_traversal_frame_data : public Visitor {

  std::tuple<Args...> args_;

public:
  explicit async_traversal_frame_data(Visitor visitor, Args... args)
      : Visitor(std::move(visitor)),
        args_(std::make_tuple(std::move(args)...)) {
  }
  template <typename MapperArg>
  explicit async_traversal_frame_data(async_traverse_in_place_tag<Visitor>,
                                      MapperArg&& mapper_arg, Args... args)
      : Visitor(std::forward<MapperArg>(mapper_arg)),
        args_(std::make_tuple(std::move(args)...)) {
  }

  /// Returns the arguments of the frame
  std::tuple<Args...>& head() noexcept {
    return args_;
  }
};
template <typename Visitor>
class async_traversal_frame_no_data : public Visitor {
public:
  explicit async_traversal_frame_no_data(Visitor visitor)
      : Visitor(std::move(visitor)) {
  }
  template <typename MapperArg>
  explicit async_traversal_frame_no_data(async_traverse_in_place_tag<Visitor>,
                                         MapperArg&& mapper_arg)
      : Visitor(std::forward<MapperArg>(mapper_arg)) {
  }
};

template <typename Visitor, typename... Args>
using data_layout_t =
    std::conditional_t<has_head<Visitor>::value,
                       async_traversal_frame_no_data<Visitor>,
                       async_traversal_frame_data<Visitor, Args...>>;

/// Stores the visitor and the arguments to traverse
template <typename Visitor, typename... Args>
class async_traversal_frame : public data_layout_t<Visitor, Args...> {
#ifndef NDEBUG
  std::atomic<bool> finished_;
#endif // NDEBUG

  Visitor& visitor() noexcept {
    return *static_cast<Visitor*>(this);
  }

  Visitor const& visitor() const noexcept {
    return *static_cast<Visitor const*>(this);
  }

public:
  template <typename... T>
  explicit async_traversal_frame(T&&... args)
      : data_layout_t<Visitor, Args...>(std::forward<T>(args)...)
#ifndef NDEBUG
        ,
        finished_(false)
#endif // NDEBUG
  {
  }

  /// We require a virtual base
  virtual ~async_traversal_frame() override = default;

  /// Calls the visitor with the given element
  template <typename T>
  auto traverse(T&& value) -> decltype(visitor()(async_traverse_visit_tag{},
                                                 std::forward<T>(value))) {
    return visitor()(async_traverse_visit_tag{}, std::forward<T>(value));
  }

  /// Calls the visitor with the given element and a continuation
  /// which is capable of continuing the asynchronous traversal
  /// when it's called later.
  template <typename T, typename Hierarchy>
  void async_continue(T&& value, Hierarchy&& hierarchy) {
    // Cast the frame up
    auto frame = std::static_pointer_cast<async_traversal_frame>(
        this->shared_from_this());

    // Create a callable object which resumes the current
    // traversal when it's called.
    auto resumable = make_resume_traversal_callable(
        std::move(frame), std::forward<Hierarchy>(hierarchy));

    // Invoke the visitor with the current value and the
    // callable object to resume the control flow.
    visitor()(async_traverse_detach_tag{}, std::forward<T>(value),
              std::move(resumable));
  }

  /// Calls the visitor with no arguments to signalize that the
  /// asynchronous traversal was finished.
  void async_complete() {
#ifndef NDEBUG
    {
      bool expected = false;
      assert(finished_.compare_exchange_strong(expected, true));
    }
#endif // NDEBUG

    visitor()(async_traverse_complete_tag{}, std::move(this->head()));
  }
};

template <typename Target, std::size_t Begin, std::size_t End>
struct static_async_range {
  Target* target_;

  constexpr decltype(auto) operator*() const noexcept {
    return std::get<Begin>(*target_);
  }

  template <std::size_t Position>
  constexpr auto relocate(std::integral_constant<std::size_t, Position>) const
      noexcept {
    return static_async_range<Target, Position, End>{target_};
  }

  constexpr auto next() const noexcept {
    return static_async_range<Target, Begin + 1, End>{target_};
  }

  constexpr bool is_finished() const noexcept {
    return false;
  }
};

/// Specialization for the end marker which doesn't provide
/// a particular element dereference
template <typename Target, std::size_t Begin>
struct static_async_range<Target, Begin, Begin> {
  explicit static_async_range(Target*) {
  }

  constexpr bool is_finished() const noexcept {
    return true;
  }
};

/// Returns a static range for the given type
template <typename T>
auto make_static_range(T&& element) {
  using range_t = static_async_range<std::decay_t<T>, 0U,
                                     std::tuple_size<std::decay_t<T>>::value>;

  return range_t{std::addressof(element)};
}

template <typename Begin, typename Sentinel>
struct dynamic_async_range {
  Begin begin_;
  Sentinel sentinel_;

  dynamic_async_range& operator++() noexcept {
    ++begin_;
    return *this;
  }

  auto operator*() const noexcept -> decltype(*std::declval<Begin const&>()) {
    return *begin_;
  }

  dynamic_async_range next() const {
    dynamic_async_range other = *this;
    ++other;
    return other;
  }

  bool is_finished() const {
    return begin_ == sentinel_;
  }
};

template <typename T>
using dynamic_async_range_of_t =
    dynamic_async_range<std::decay_t<decltype(std::begin(std::declval<T>()))>,
                        std::decay_t<decltype(std::end(std::declval<T>()))>>;

/// Returns a dynamic range for the given type
template <typename T>
auto make_dynamic_async_range(T&& element) {
  using range_t = dynamic_async_range_of_t<T>;
  return range_t{std::begin(element), std::end(element)};
}

/// Represents a particular point in a asynchronous traversal hierarchy
template <typename Frame, typename... Hierarchy>
class async_traversal_point {
  Frame frame_;
  std::tuple<Hierarchy...> hierarchy_;
  bool& detached_;

public:
  explicit async_traversal_point(Frame frame,
                                 std::tuple<Hierarchy...> hierarchy,
                                 bool& detached)
      : frame_(std::move(frame)), hierarchy_(std::move(hierarchy)),
        detached_(detached) {
  }

  // Abort the current control flow
  void detach() noexcept {
    assert(!detached_);
    detached_ = true;
  }

  /// Returns true when we should abort the current control flow
  bool is_detached() const noexcept {
    return detached_;
  }

  /// Creates a new traversal point which
  template <typename Parent>
  auto push(Parent&& parent)
      -> async_traversal_point<Frame, std::decay_t<Parent>, Hierarchy...> {
    // Create a new hierarchy which contains the
    // the parent (the last traversed element).
    auto hierarchy = std::tuple_cat(
        std::make_tuple(std::forward<Parent>(parent)), hierarchy_);

    return async_traversal_point<Frame, std::decay_t<Parent>, Hierarchy...>(
        frame_, std::move(hierarchy), detached_);
  }

  /// Forks the current traversal point and continues the child
  /// of the given parent.
  template <typename Child, typename Parent>
  void fork(Child&& child, Parent&& parent) {
    // Push the parent on top of the hierarchy
    auto point = push(std::forward<Parent>(parent));

    // Continue the traversal with the current element
    point.async_traverse(std::forward<Child>(child));
  }

  /// Async traverse a single element, and do nothing.
  /// This function is matched last.
  template <typename Matcher, typename Current>
  void async_traverse_one_impl(Matcher, Current&& /*current*/) {
    // Do nothing if the visitor doesn't accept the type
  }

  /// Async traverse a single element which isn't a container or
  /// tuple like type. This function is SFINAEd out if the element
  /// isn't accepted by the visitor.
  template <typename Current>
  auto async_traverse_one_impl(container_category_tag<false, false>,
                               Current&& current)
      /// SFINAE this out if the visitor doesn't accept
      /// the given element
      -> traits::void_t<decltype(std::declval<Frame>()->traverse(*current))> {
    if (!frame_->traverse(*current)) {
      // Store the current call hierarchy into a tuple for
      // later re-entrance.
      auto hierarchy =
          std::tuple_cat(std::make_tuple(current.next()), hierarchy_);

      // First detach the current execution context
      detach();

      // If the traversal method returns false, we detach the
      // current execution context and call the visitor with the
      // element and a continue callable object again.
      frame_->async_continue(*current, std::move(hierarchy));
    }
  }

  /// Async traverse a single element which is a container or
  /// tuple like type.
  template <bool IsTupleLike, typename Current>
  void async_traverse_one_impl(container_category_tag<true, IsTupleLike>,
                               Current&& current) {
    auto range = make_dynamic_async_range(*current);
    fork(std::move(range), std::forward<Current>(current));
  }

  /// Async traverse a single element which is a tuple like type only.
  template <typename Current>
  void async_traverse_one_impl(container_category_tag<false, true>,
                               Current&& current) {
    auto range = make_static_range(*current);
    fork(std::move(range), std::forward<Current>(current));
  }

  /// Async traverse the current iterator
  template <typename Current>
  void async_traverse_one(Current&& current) {
    using ElementType = std::decay_t<decltype(*current)>;
    return async_traverse_one_impl(container_category_of_t<ElementType>{},
                                   std::forward<Current>(current));
  }

  /// Async traverse the current iterator but don't traverse
  /// if the control flow was detached.
  template <typename Current>
  void async_traverse_one_checked(Current&& current) {
    if (!is_detached()) {
      async_traverse_one(std::forward<Current>(current));
    }
  }

  template <std::size_t... Sequence, typename Current>
  void async_traverse_static_async_range(
      std::integer_sequence<std::size_t, Sequence...>, Current&& current) {
    int dummy[] = {0, (async_traverse_one_checked(current.relocate(
                           std::integral_constant<std::size_t, Sequence>{})),
                       0)...};
    (void)dummy;
    (void)current;
  }

  /// Traverse a static range
  template <typename Target, std::size_t Begin, std::size_t End>
  void async_traverse(static_async_range<Target, Begin, End> current) {
    async_traverse_static_async_range(
        explicit_range_sequence_of_t<Begin, End>{}, current);
  }

  /// Traverse a dynamic range
  template <typename Begin, typename Sentinel>
  void async_traverse(dynamic_async_range<Begin, Sentinel> range) {
    if (!is_detached()) {
      for (/**/; !range.is_finished(); ++range) {
        async_traverse_one(range);
        if (is_detached()) // test before increment
          break;
      }
    }
  }
};

/// Deduces to the traversal point class of the
/// given frame and hierarchy
template <typename Frame, typename... Hierarchy>
using traversal_point_of_t =
    async_traversal_point<std::decay_t<Frame>, std::decay_t<Hierarchy>...>;

/// A callable object which is capable of resuming an asynchronous
/// pack traversal.
struct resume_state_callable {
  /// Reenter an asynchronous iterator pack and continue
  /// its traversal.
  template <typename Frame, typename Current, typename... Hierarchy>
  void operator()(Frame&& frame, Current&& current,
                  Hierarchy&&... hierarchy) const {
    bool detached = false;
    next(detached, std::forward<Frame>(frame), std::forward<Current>(current),
         std::forward<Hierarchy>(hierarchy)...);
  }

  template <typename Frame, typename Current>
  void next(bool& detached, Frame&& frame, Current&& current) const {
    // Only process the next element if the current iterator
    // hasn't reached its end.
    if (!current.is_finished()) {
      traversal_point_of_t<Frame> point(frame, std::make_tuple(), detached);

      point.async_traverse(std::forward<Current>(current));

      // Don't continue the frame when the execution was detached
      if (detached) {
        return;
      }
    }

    frame->async_complete();
  }

  /// Reenter an asynchronous iterator pack and continue
  /// its traversal.
  template <typename Frame, typename Current, typename Parent,
            typename... Hierarchy>
  void next(bool& detached, Frame&& frame, Current&& current, Parent&& parent,
            Hierarchy&&... hierarchy) const {
    // Only process the element if the current iterator
    // hasn't reached its end.
    if (!current.is_finished()) {
      // Don't forward the arguments here, since we still need
      // the objects in a valid state later.
      traversal_point_of_t<Frame, Parent, Hierarchy...> point(
          frame, std::make_tuple(parent, hierarchy...), detached);

      point.async_traverse(std::forward<Current>(current));

      // Don't continue the frame when the execution was detached
      if (detached) {
        return;
      }
    }

    // Pop the top element from the hierarchy, and shift the
    // parent element one to the right
    next(detached, std::forward<Frame>(frame),
         std::forward<Parent>(parent).next(),
         std::forward<Hierarchy>(hierarchy)...);
  }
};

template <typename Frame, typename State>
void resume_traversal_callable<Frame, State>::operator()() {
  auto hierarchy = std::tuple_cat(std::make_tuple(frame_), state_);
  traits::unpack(resume_state_callable{}, std::move(hierarchy));
}

/// Gives access to types related to the traversal frame
template <typename Visitor, typename... Args>
struct async_traversal_types {
  /// Deduces to the async traversal frame type of the given
  /// traversal arguments and mapper
  using frame_t =
      async_traversal_frame<std::decay_t<Visitor>, std::decay_t<Args>...>;

  /// The type of the demoted visitor type
  using visitor_t = Visitor;
};

template <typename Visitor, typename VisitorArg, typename... Args>
struct async_traversal_types<async_traverse_in_place_tag<Visitor>, VisitorArg,
                             Args...>
    : async_traversal_types<Visitor, Args...> {};

/// Traverses the given pack with the given mapper
template <typename Visitor, typename... Args>
auto apply_pack_transform_async(Visitor&& visitor, Args&&... args) {

  // Provide the frame and visitor type
  using types = async_traversal_types<Visitor, Args...>;
  using frame_t = typename types::frame_t;
  using visitor_t = typename types::visitor_t;

  // Check whether the visitor inherits enable_shared_from_this
  static_assert(std::is_base_of<std::enable_shared_from_this<visitor_t>,
                                visitor_t>::value,
                "The visitor must inherit std::enable_shared_from_this!");

  // Check whether the visitor is virtual destructible
  static_assert(std::has_virtual_destructor<visitor_t>::value,
                "The visitor must have a virtual destructor!");

  // Create the frame on the heap which stores the arguments
  // to traverse asynchronous. It persists until the
  // traversal frame isn't referenced anymore.
  auto frame = std::make_shared<frame_t>(std::forward<Visitor>(visitor),
                                         std::forward<Args>(args)...);

  // Create a static range for the top level tuple
  auto range = std::make_tuple(make_static_range(frame->head()));

  // Create a resumer to start the asynchronous traversal
  auto resumer = make_resume_traversal_callable(frame, std::move(range));

  // Start the asynchronous traversal
  resumer();

  // Cast the shared_ptr down to the given visitor type
  // for implementation invisibility
  return std::static_pointer_cast<visitor_t>(std::move(frame));
}
} // namespace traversal
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_TRAVERSE_ASYNC_HPP_INCLUDED
