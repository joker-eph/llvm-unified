#ifndef BENCHMARK_CONTAINER_BENCHMARKS_HPP
#define BENCHMARK_CONTAINER_BENCHMARKS_HPP

#include <cassert>

#include "benchmark/benchmark_api.h"

namespace ContainerBenchmarks {

template <class Container, class GenInputs>
void BM_InsertValue(benchmark::State& st, Container c, GenInputs gen) {
    auto in = gen(st.range_x());
    const auto end = in.end();
    while (st.KeepRunning()) {
        c.clear();
        for (auto it = in.begin(); it != end; ++it) {
            benchmark::DoNotOptimize(&(*c.insert(*it).first));
        }
        benchmark::ClobberMemory();
    }
}

template <class Container, class GenInputs>
void BM_InsertValueRehash(benchmark::State& st, Container c, GenInputs gen) {
    auto in = gen(st.range_x());
    const auto end = in.end();
    while (st.KeepRunning()) {
        c.clear();
        c.rehash(16);
        for (auto it = in.begin(); it != end; ++it) {
            benchmark::DoNotOptimize(&(*c.insert(*it).first));
        }
        benchmark::ClobberMemory();
    }
}

template <class Container, class GenInputs>
static void BM_Find(benchmark::State& st, Container c, GenInputs gen) {
    auto in = gen(st.range_x());
    c.insert(in.begin(), in.end());
    benchmark::DoNotOptimize(&(*c.begin()));
    const auto end = in.data() + in.size();
    while (st.KeepRunning()) {
        for (auto it = in.data(); it != end; ++it) {
            benchmark::DoNotOptimize(&(*c.find(*it)));
        }
        benchmark::ClobberMemory();
    }
}

template <class Container, class GenInputs>
static void BM_FindRehash(benchmark::State& st, Container c, GenInputs gen) {
    c.rehash(8);
    auto in = gen(st.range_x());
    c.insert(in.begin(), in.end());
    benchmark::DoNotOptimize(&(*c.begin()));
    const auto end = in.data() + in.size();
    while (st.KeepRunning()) {
        for (auto it = in.data(); it != end; ++it) {
            benchmark::DoNotOptimize(&(*c.find(*it)));
        }
        benchmark::ClobberMemory();
    }

}

} // end namespace ContainerBenchmarks

#endif // BENCHMARK_CONTAINER_BENCHMARKS_HPP
