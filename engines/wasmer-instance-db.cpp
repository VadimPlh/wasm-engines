#include "wasm.h"
#include <wasmer-instance-db.h>

namespace {

void print_wasmer_error() {
    int error_len = wasmer_last_error_length();
    std::string error;
    error.resize(error_len);
    wasmer_last_error_message(error.data(), error_len);
    std::cout << error << std::endl;
}

}

bool wasmer_instance::init_instance(const std::string& path_to_wasm_code) {
    if (!create_module(path_to_wasm_code)) {
        return false;
    }

    if (!create_env()) {
        return false;
    }

    if (!create_imports(module)) {
        return false;
    }

    if (!create_wasm_instance(module)) {
        return false;
    }

    if (!export_data()) {
        return false;
    }

    return true;
}

bool wasmer_instance::run_instanse(char* data) {
    wasm_val_t args_val[1] = { WASM_I64_VAL(host_ptr_to_wasm_ptr(data)) };
    wasm_val_vec_t args = WASM_ARRAY_VEC(args_val);
    wasm_val_vec_t res = WASM_EMPTY_VEC;

    wasm_func_call(run_func, &args, &res);

    /* In secind time return not null pointer...
    if (wasm_func_call(run_func, &args, &res)) {
        std::cout << "Can not run func" << std::endl;
        print_wasmer_error();
        return false;
    }
    */

    return true;
}

char* wasmer_instance::new_in_wasm(int64_t size) {
    if (!wasm_new) {
        std::cout << "Can not allocate memory in wasm(Export function does not exist)" << std::endl;
        return nullptr;
    }

    wasm_val_t args_val[1] = { WASM_I64_VAL(size) };
    wasm_val_t results_val[1] = { WASM_INIT_VAL };
    wasm_val_vec_t args = WASM_ARRAY_VEC(args_val);
    wasm_val_vec_t res = WASM_ARRAY_VEC(results_val);

    wasm_func_call(wasm_new, &args, &res);

    return wasm_ptr_to_host_ptr(results_val[0].of.i64);
}

void wasmer_instance::delete_in_wasm(char* ptr) {
    if (!wasm_delete) {
        std::cout << "Can not delete memory in wasm(Export function does not exist)" << std::endl;
        return;
    }

    auto wasm_ptr = host_ptr_to_wasm_ptr(ptr);
    wasm_val_t args_val[1] = { WASM_I64_VAL(wasm_ptr) };
    wasm_val_vec_t args = WASM_ARRAY_VEC(args_val);
    wasm_val_vec_t res = WASM_EMPTY_VEC;

    wasm_func_call(wasm_delete, &args, &res);
}


bool wasmer_instance::create_module(const std::string& path_to_wasm) {
    std::ifstream file(path_to_wasm.c_str(), std::ios::in);
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    wasm_byte_vec_t binary;
    wasm_byte_vec_new_uninitialized(&binary, size);
    if (!file.read(binary.data, size))
    {
        std::cout << "Can not read code from wasm file" << std::endl;
        return false;
    }

    module = wasm_module_new(store, &binary);
    if (!module) {
        std::cout << "Can not create module" << std::endl;
        return false;
    }

    wasm_byte_vec_delete(&binary);
    return true;
}

bool wasmer_instance::create_env() {
    wasi_config_t* config = wasi_config_new("wasmer_instanse");

    wasi_config_inherit_stdin(config);
    wasi_config_inherit_stdout(config);

    wasi_env = wasi_env_new(config);
    if (!wasi_env) {
        std::cout << "Can not create wasi_enf from config" << std::endl;
        print_wasmer_error();
        return false;
    }

    return true;
}

bool wasmer_instance::create_imports(wasm_module_t* module) {
    wasm_importtype_vec_t import_types;
    wasm_module_imports(module, &import_types);

    wasm_extern_vec_new_uninitialized(&imports, import_types.size);
    wasm_importtype_vec_delete(&import_types);

    bool get_imports_result = wasi_get_imports(store, module, wasi_env, &imports);

    if (!get_imports_result) {
        std::cout << "Can not get wasi imports" << std::endl;
        print_wasmer_error();
        return false;
    }

    return true;
}

bool wasmer_instance::create_wasm_instance(wasm_module_t* module) {
    instance = wasm_instance_new(store, module, &imports, NULL);

    if (!instance) {
        std::cout << "Can not create wasm instance" << std::endl;
        print_wasmer_error();
        return false;
    }

    return true;
}

bool wasmer_instance::export_data() {
    wasm_extern_vec_t exports;
    wasm_instance_exports(instance, &exports);

    if (exports.size > 2) {
        wasm_new = wasm_extern_as_func(exports.data[2]);
        wasm_delete = wasm_extern_as_func(exports.data[3]);
    }

    run_func = wasm_extern_as_func(exports.data[4]);
    if (run_func == NULL) {
        std::cout << "Can not get start function from instance" << std::endl;
        print_wasmer_error();
        return false;
    }

    wasm_memory_t* wasm_memory = wasm_extern_as_memory(exports.data[0]);
    if (wasm_memory == NULL) {
        std::cout << "Failed to get the exported memory!" << std::endl;
        print_wasmer_error();
        return false;
    }

    start_mem = reinterpret_cast<char*>(wasm_memory_data(wasm_memory));
    return true;
}

char* wasmer_instance::wasm_ptr_to_host_ptr(int64_t wasm_ptr) {
    return start_mem + wasm_ptr;
}

int64_t wasmer_instance::host_ptr_to_wasm_ptr(char* ptr) {
    return reinterpret_cast<int64_t>(ptr - start_mem);
}