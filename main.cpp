#include <iostream>
#include <chrono>
#include <limits>
#include <functional>

#include "wasmer-instance-db.h"
#include "v8-instance-db.h"
#include "jemalloc/jemalloc.h"

struct test_sum_t {
    explicit test_sum_t(int a_, int b_)
    : a(a_),
      b(b_) {}

    int a;
    int b;

    int ans;
};

void print_mem_stat(const std::string& msg) {
    std::cout << msg << std::endl;
    malloc_stats_print(NULL, NULL, "g,l,b,h,m,a");
    std::cout << std::endl;
}

template<typename engine_type>
void mem_bench(const std::string &script_path) {
    print_mem_stat("Start");

    engine_type inst;
    print_mem_stat("Create engine");

    inst.add_new_script(script_path, "1");
    print_mem_stat("Create 1 script");

    inst.add_new_script(script_path, "2");
    print_mem_stat("Create 2 script");

    auto raw_ptr = inst.alloc_memory_in_wasm_script("1", sizeof(test_sum_t));
    assert(raw_ptr != nullptr);
    auto obj_ptr = reinterpret_cast<test_sum_t*>(raw_ptr);
    new (raw_ptr) test_sum_t(1, 2);
    print_mem_stat("Alloc mem");

    inst.run_script("1", raw_ptr);
    print_mem_stat("Run script");

    obj_ptr->~test_sum_t();
    inst.delete_memory_in_wasm_script("1", raw_ptr);
    print_mem_stat("Delete mem");
}

void mem_comparison() {
    std::cout << "!!!!!!!!!!!!!!WASMER!!!!!!!!!!!!!!" << std::endl;
    mem_bench<wasmer_instance_db>("../examples/new-delete.wasm");
    std::cout << std::endl;
    std::cout << "!!!!!!!!!!!!!!V8!!!!!!!!!!!!!!" << std::endl;
    mem_bench<v8_instance_db>("/home/vadim/wasm-engines/examples/simple.js");
}

constexpr int64_t IT_COUNT = 700000;

template<typename engine_type>
double time_bench(const std::string& script_path) {
    engine_type engine;
    engine.add_new_script(script_path, script_path);

    auto raw_ptr = engine.alloc_memory_in_wasm_script(script_path, sizeof(test_sum_t));
    assert(raw_ptr != nullptr);
    auto obj_ptr = reinterpret_cast<test_sum_t*>(raw_ptr);

    double min_time = std::numeric_limits<double>::max();

    for (int i = 0; i < IT_COUNT; ++i) {
        new (raw_ptr) test_sum_t(i, i + 1);

        auto start = std::chrono::high_resolution_clock::now();
        engine.run_script(script_path, raw_ptr);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> time = end - start;
        min_time = std::min(min_time, time.count());

        obj_ptr->~test_sum_t();

    }
    engine.delete_memory_in_wasm_script(script_path, raw_ptr);
    return min_time;
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

void time_comparison() {
    simple_bench_without_wasm();
    std::cout << "Wasmer bench: " << time_bench<wasmer_instance_db>("../examples/new-delete.wasm") << std::endl;
    std::cout << "V8 bench: " << time_bench<v8_instance_db>("/home/vadim/wasm-engines/examples/simple.js") << std::endl;

}

struct throughput_bench_t {
    int32_t length;
    int32_t ans;

    int32_t array[];

    void fill_data() {
        for (int32_t i = 0; i < length; ++i) {
            array[i] = i;
        }
    }
};

template<typename engine_type>
void throughput_bench(const std::string& script_path) {
    engine_type engine;

    engine.add_new_script(script_path, script_path);

    std::vector<int> lengths = {10, 100, 1000, 10000};

    for (auto length : lengths) {
        double max_throughput = std::numeric_limits<double>::min();

        for (int i = 0; i < 100000; ++i) {
            auto bytes_count = sizeof(int32_t) * length;
            auto alloc_size = sizeof(throughput_bench_t) + bytes_count;

            auto start = std::chrono::high_resolution_clock::now();

            auto raw_ptr = engine.alloc_memory_in_wasm_script(script_path, alloc_size);
            assert(raw_ptr != nullptr);
            auto obj_ptr = reinterpret_cast<throughput_bench_t*>(raw_ptr);

            obj_ptr->length = length;
            obj_ptr->ans = 0;
            obj_ptr->fill_data();

            engine.run_script(script_path, raw_ptr);
            obj_ptr->~throughput_bench_t();

            engine.delete_memory_in_wasm_script(script_path, raw_ptr);

            auto end = std::chrono::high_resolution_clock::now();

            std::chrono::duration<double> time = end - start;

            max_throughput = std::max(max_throughput, bytes_count / time.count());
        }

        std::cout << "Length: " << length << " max_throughput: " << max_throughput << std::endl;
    }
}

void throughput_comparison() {
    std::cout << "Wasmer throughputs:" << std::endl;
    throughput_bench<wasmer_instance_db>("../examples/sum_array.wasm");
    std::cout << "V8 throughputs:" << std::endl;
    throughput_bench<v8_instance_db>("/home/vadim/wasm-engines/examples/sum_array.js");
}

void wasm_inside_v8() {
    std::cout << "V8 wasm bench: " << time_bench<v8_instance_db>("/home/vadim/wasm-engines/examples/sum_wasm.js") << std::endl;
}

void stop_terminating() {
    v8_instance_db inst;
    inst.add_new_script("/home/vadim/wasm-engines/examples/loop_with_tmp_objects.js", "1");

    auto raw_ptr = inst.alloc_memory_in_wasm_script("1", sizeof(int));
    assert(raw_ptr != nullptr);
    auto obj_ptr = reinterpret_cast<int*>(raw_ptr);
    *obj_ptr = 100000000;

    inst.run_script("1", raw_ptr);

    inst.delete_memory_in_wasm_script("1", raw_ptr);
}

int main() {
    stop_terminating();
    return 0;
}