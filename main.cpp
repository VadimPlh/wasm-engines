#include <iostream>
#include <chrono>
#include <limits>

#include "wasmer-instance-db.h"

constexpr int64_t IT_COUNT = 1000000;

struct test_sum_t {
    explicit test_sum_t(int a_, int b_)
    : a(a_),
      b(b_) {}

    int a;
    int b;

    int ans;
};

double simple_wasmer_bench() {
    wasmer_instance_db inst;
    inst.add_new_script("../examples/new-delete.wasm", "sum");

    auto raw_ptr = inst.alloc_memory_in_wasm_script("sum", sizeof(test_sum_t));
    assert(raw_ptr != nullptr);
    auto obj_ptr = reinterpret_cast<test_sum_t*>(raw_ptr);

    double min_time = std::numeric_limits<double>::max();

    for (int i = 0; i < IT_COUNT; ++i) {
        new (raw_ptr) test_sum_t(i, i + 1);

        auto start = std::chrono::high_resolution_clock::now();
        inst.run_script("sum", raw_ptr);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> time = end - start;
        min_time = std::min(min_time, time.count());

        obj_ptr->~test_sum_t();
    }

    inst.delete_memory_in_wasm_script("sum", raw_ptr);
    return min_time;
}

double simple_bench_without_wasm() {
    double min_time = std::numeric_limits<double>::max();

    for (int i = 0; i < IT_COUNT; ++i) {
        test_sum_t t(i, i+ 1);

        auto start = std::chrono::high_resolution_clock::now();
        t.ans = t.a + t.b;
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> time = end - start;
        min_time = std::min(min_time, time.count());
    }

    return min_time;
}

int main() {
    std::cout << "Simple time(without wasm-engine): " << simple_bench_without_wasm() << std::endl;
    std::cout << "Wasmer time: " << simple_wasmer_bench() << std::endl;
    return 0;
}