#include <iostream>

#include "wasmer-instance-db.h"

int main() {
    wasmer_instance_db inst;
    inst.add_new_script("/home/vadim/wasm-engines/examples/hello.wasm", "hello");
    inst.run_script("hello");
    inst.add_new_script("/home/vadim/wasm-engines/examples/read-input.wasm", "read_input");
    inst.run_script("read_input");
    inst.run_script("read_input");
    return 0;
}