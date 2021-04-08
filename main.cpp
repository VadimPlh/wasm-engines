#include <iostream>
#include <chrono>
#include <limits>
#include <functional>

#include "wasmer-instance-db.h"
#include "v8-instance-db.h"

constexpr int64_t IT_COUNT = 7000000;

struct test_sum_t {
    explicit test_sum_t(int a_, int b_)
    : a(a_),
      b(b_) {}

    int a;
    int b;

    int ans;
};

void simple_wasmer_bench() {
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
    std::cout << "Wasmer bench: " << min_time << std::endl;
}

void simple_v8_bench() {
    v8_instance_db v8;
    v8.add_new_script("/home/vadim/wasm-engines/examples/simple.js", "simple");

    auto raw_ptr = v8.alloc_memory_in_wasm_script("simple", sizeof(test_sum_t));
    assert(raw_ptr != nullptr);
    auto obj_ptr = reinterpret_cast<test_sum_t*>(raw_ptr);
    double min_time = std::numeric_limits<double>::max();

    for (int i = 0; i < IT_COUNT; ++i) {
        new (raw_ptr) test_sum_t(i, i + 1);

        auto start = std::chrono::high_resolution_clock::now();
        v8.run_script("simple", raw_ptr);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> time = end - start;
        min_time = std::min(min_time, time.count());

        obj_ptr->~test_sum_t();
    }

    v8.delete_memory_in_wasm_script("simple", raw_ptr);
    std::cout << "V8 bench: " << min_time << std::endl;
}

void simple_bench_without_wasm() {
    double min_time = std::numeric_limits<double>::max();

    std::function<void(test_sum_t*)> sum([](test_sum_t* obj){ obj->ans = obj->a + obj->b; });

    for (int i = 0; i < IT_COUNT; ++i) {
        test_sum_t obj(i, i+ 1);

        auto start = std::chrono::high_resolution_clock::now();
        sum(&obj);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> time = end - start;
        min_time = std::min(min_time, time.count());
    }

    std::cout << "Simple bench: " << min_time << std::endl;
}

int main() {
    //simple_bench_without_wasm();
    //simple_wasmer_bench();
    simple_v8_bench();
    return 0;
}