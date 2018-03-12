
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v3.0.0

  Copyright(c) 2015 - 2018 Denis Blank <denis.blank at outlook dot com>

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
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
**/

#ifndef CONTINUABLE_DETAIL_COMPOSITION_SEQ_HPP_INCLUDED
#define CONTINUABLE_DETAIL_COMPOSITION_SEQ_HPP_INCLUDED

#include <cassert>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include <continuable/continuable-traverse-async.hpp>
#include <continuable/detail/base.hpp>
#include <continuable/detail/composition-aggregated.hpp>
#include <continuable/detail/traits.hpp>
#include <continuable/detail/util.hpp>

namespace cti {
namespace detail {
namespace composition {
namespace seq {
/// Connects the left and the right continuable to a sequence
///
/// \note This is implemented in an eager way because we would not gain
///       any profit from chaining sequences lazily.
template <typename Left, typename Right>
auto sequential_connect(Left&& left, Right&& right) {
  left.freeze(right.is_frozen());
  right.freeze();

  return std::forward<Left>(left).then([right = std::forward<Right>(right)](
      auto&&... args) mutable {
    return std::move(right).then([previous = std::make_tuple(
                                      std::forward<decltype(args)>(args)...)](
        auto&&... args) mutable {
      return traits::merge(
          std::move(previous),
          std::make_tuple(std::forward<decltype(args)>(args)...));
    });
  });
}

template <typename Callback, typename Box>
struct sequential_dispatch_data {
  Callback callback;
  Box box;
};

template <typename Data>
class sequential_dispatch_visitor
    : public std::enable_shared_from_this<sequential_dispatch_visitor<Data>>,
      public util::non_movable {

  Data data_;

public:
  explicit sequential_dispatch_visitor(Data&& data) : data_(std::move(data)) {
  }

  virtual ~sequential_dispatch_visitor() = default;

  /// Returns the pack that should be traversed
  auto& head() {
    return data_.box;
  }

  template <typename Box, std::enable_if_t<aggregated::is_continuable_box<
                              std::decay_t<Box>>::value>* = nullptr>
  bool operator()(async_traverse_visit_tag, Box&& /*box*/) {
    return false;
  }

  template <typename Box, typename N>
  void operator()(async_traverse_detach_tag, Box&& box, N&& next) {
    box.fetch()
        .then([ box = std::addressof(box),
                next = std::forward<N>(next) ](auto&&... args) mutable {

          // Assign the result to the target
          box->assign(std::forward<decltype(args)>(args)...);

          // Continue the asynchronous sequential traversal
          next();
        })
        .fail([me = this->shared_from_this()](types::error_type exception) {
          // Abort the traversal when an error occurred
          std::move(me->data_.callback)(types::dispatch_error_tag{},
                                        std::move(exception));
        })
        .done();
  }

  template <typename T>
  void operator()(async_traverse_complete_tag, T&& /*pack*/) {
    return aggregated::finalize_data(std::move(data_.callback),
                                     std::move(data_.box));
  }
};
} // namespace seq

struct composition_strategy_seq_tag {};
template <>
struct is_composition_strategy<composition_strategy_seq_tag> // ...
    : std::true_type {};

/// Finalizes the seq logic of a given composition
template <>
struct composition_finalizer<composition_strategy_seq_tag> {
  template <typename Composition>
  static constexpr auto hint() {
    return decltype(aggregated::deduce_hint(std::declval<Composition>())){};
  }

  /// Finalizes the all logic of a given composition
  template <typename Composition>
  static auto finalize(Composition&& composition) {
    return [composition = std::forward<Composition>(composition)] // ...
        (auto&& callback) mutable {

      auto boxed = aggregated::box_continuables(std::move(composition));

      // The data from which the visitor is constructed in-place
      using data_t =
          seq::sequential_dispatch_data<std::decay_t<decltype(callback)>,
                                        std::decay_t<decltype(boxed)>>;

      // The visitor type
      using visitor_t = seq::sequential_dispatch_visitor<data_t>;

      traverse_pack_async(
          async_traverse_in_place_tag<visitor_t>{},
          data_t{std::forward<decltype(callback)>(callback), std::move(boxed)});
    };
  }
};
} // namespace composition
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_COMPOSITION_SEQ_HPP_INCLUDED
