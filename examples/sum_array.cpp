#include <iostream>

int64_t wasm_new(int64_t size) __attribute__((used)); //exports.data[2]
void wasm_delete(int64_t ptr) __attribute__((used)); //exports.data[3]
void main_foo(int64_t raw_ptr) __attribute__((used)); //exports.data[4]

int64_t wasm_new(int64_t size) {
    auto ptr = new char[size];
    return reinterpret_cast<int64_t>(ptr);
}

void wasm_delete(int64_t ptr) {
    auto raw_ptr = reinterpret_cast<char*>(ptr);
    delete[] raw_ptr;
}

struct throughput_t {
    int32_t length;
    int32_t ans;

    int32_t array[];
};


void main_foo(int64_t raw_ptr) {
    auto obj_ptr = reinterpret_cast<throughput_t*>(raw_ptr);
    for (int i = 0; i < obj_ptr->length; ++i) {
        obj_ptr->ans += obj_ptr->array[i];
    }
}

int main() {
    return 0;
}