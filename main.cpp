#include <iostream>

#include "wasmer-instance-db.h"

struct test_sum_t {
    explicit test_sum_t(int a_, int b_)
    : a(a_),
      b(b_) {}

    int a;
    int b;

    int ans;
};

void simple_test() {
    wasmer_instance_db inst;
    inst.add_new_script("/home/vadim/wasm-engines/examples/new-delete.wasm", "sum");

    auto raw_ptr = inst.alloc_memory_in_wasm_script("sum", sizeof(test_sum_t));
    assert(raw_ptr != nullptr);

    for (int i = 0; i < 1000000; ++i) {
        new (raw_ptr) test_sum_t(i, i + 1);
        inst.run_script("sum", raw_ptr);
        auto ptr = reinterpret_cast<test_sum_t*>(raw_ptr);
        assert(ptr->a + ptr->b == ptr->ans);
    }

    inst.delete_memory_in_wasm_script("sum", raw_ptr);
}

int main() {
    simple_test();
    return 0;
}