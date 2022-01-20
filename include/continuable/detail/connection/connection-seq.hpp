
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

#ifndef CONTINUABLE_DETAIL_CONNECTION_SEQ_HPP_INCLUDED
#define CONTINUABLE_DETAIL_CONNECTION_SEQ_HPP_INCLUDED

#include <cassert>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <continuable/continuable-primitives.hpp>
#include <continuable/continuable-traverse-async.hpp>
#include <continuable/detail/connection/connection-aggregated.hpp>
#include <continuable/detail/core/base.hpp>
#include <continuable/detail/utility/traits.hpp>
#include <continuable/detail/utility/util.hpp>

namespace cti {
namespace detail {
namespace connection {
namespace seq {
/// Connects the left and the right continuable to a sequence
///
/// \note This is implemented in an eager way because we would not gain
///       any profit from chaining sequences lazily.
template <typename Left, typename Right>
auto sequential_connect(Left&& left, Right&& right) {
  left.freeze(right.is_frozen());
  right.freeze();

  return std::forward<Left>(left).then(
      [right = std::forward<Right>(right)](auto&&... args) mutable {
        return std::move(right).then(
            [previous = std::make_tuple(std::forward<decltype(args)>(args)...)](
                auto&&... args) mutable {
              return std::tuple_cat(
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
  bool operator()(async_traverse_visit_tag, Box&& box) {
    if (base::attorney::is_ready(box.peek())) {
      // The result can be resolved directly
      traits::unpack(
          [&](auto&&... args) mutable {
            box.assign(std::forward<decltype(args)>(args)...);
          },
          base::attorney::query(box.fetch()));
      return true;
    } else {
      return false;
    }
  }

  template <typename Box, typename N>
  void operator()(async_traverse_detach_tag, Box&& box, N&& next) {
    box.fetch()
        .then([box = std::addressof(box),
               next = std::forward<N>(next)](auto&&... args) mutable {
          // Assign the result to the target
          box->assign(std::forward<decltype(args)>(args)...);

          // Continue the asynchronous sequential traversal
          next();
        })
        .fail([me = this->shared_from_this()](exception_t exception) {
          // Abort the traversal when an error occurred
          std::move(me->data_.callback)(exception_arg_t{},
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

struct connection_strategy_seq_tag {};
template <>
struct is_connection_strategy<connection_strategy_seq_tag> // ...
    : std::true_type {};

/// Finalizes the seq logic of a given connection
template <>
struct connection_finalizer<connection_strategy_seq_tag> {
  /// Finalizes the all logic of a given connection
  template <typename Connection>
  static auto finalize(Connection&& connection, util::ownership ownership) {

    auto res =
        aggregated::box_continuables(std::forward<Connection>(connection));

    auto signature = aggregated::hint_of_data<decltype(res)>();

    return base::attorney::create_from(
        [res = std::move(res)](auto&& callback) mutable {
          // The data from which the visitor is constructed in-place
          using data_t =
              seq::sequential_dispatch_data<std::decay_t<decltype(callback)>,
                                            std::decay_t<decltype(res)>>;

          // The visitor type
          using visitor_t = seq::sequential_dispatch_visitor<data_t>;

          traverse_pack_async(async_traverse_in_place_tag<visitor_t>{},
                              data_t{std::forward<decltype(callback)>(callback),
                                     std::move(res)});
        },
        signature, std::move(ownership));
  }
};
} // namespace connection

/// Specialization for a connection annotation
template <>
struct annotation_trait<connection::connection_strategy_seq_tag>
    : connection::connection_annotation_trait<
          connection::connection_strategy_seq_tag> {};

} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_CONNECTION_SEQ_HPP_INCLUDED
