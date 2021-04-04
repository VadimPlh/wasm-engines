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

struct test {
    int a;
    int b;

    int ans;
};


void main_foo(int64_t raw_ptr) {
    auto obj_ptr = reinterpret_cast<test*>(raw_ptr);
    obj_ptr->ans = obj_ptr->a + obj_ptr->b;
}

int main() {
    return 0;
}