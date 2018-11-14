#include <benchmark/benchmark.h>
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <continuable/continuable.hpp>

static auto do_sth_continuable() {
  return cti::make_continuable<void>([](auto&& promise) {
    //
    promise.set_value();
  });
}

static void bm_continuable(benchmark::State& state) {
  for (auto _ : state) {
    cti::continuable<> cont = do_sth_continuable()
                                  .then([] {
                                    // ...
                                    return do_sth_continuable();
                                  })
                                  .then([] {
                                    // ...
                                    return do_sth_continuable();
                                  });
  }
}

BENCHMARK(bm_continuable);

static boost::future<void> do_sth_future() {
  boost::promise<void> p;
  boost::future<void> fu = p.get_future();
  p.set_value();

  return fu;
}

static void bm_future(benchmark::State& state) {
  for (auto _ : state) {
    auto fut = do_sth_future()
                   .then([](auto&&) {
                     // ...
                     return do_sth_future();
                   })
                   .then([](auto&&) {
                     // ...
                     return do_sth_future();
                   });

    // fut.get();
  }
}

BENCHMARK(bm_future);

int main(int argc, char** argv) {
  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;
  ::benchmark::RunSpecifiedBenchmarks();
  return 0;
}
