#pragma once

#include "instance-db.h"
#include "wasm.h"
#include "wasmer_wasm.h"

#include <fstream>
#include <ios>
#include <iostream>
#include <ostream>

class wasmer_instance {
public:
    wasmer_instance(wasm_engine_t *engine) {
        store = wasm_store_new(engine);
    }

    ~wasmer_instance() {
        if (instance) {
            wasm_instance_delete(instance);
        }

        if (module) {
            wasm_module_delete(module);
        }

        wasm_extern_vec_delete(&imports);

        if (run_func) {
            wasm_func_delete(run_func);
        }

        if (wasi_env) {
            wasi_env_delete(wasi_env);
        }

        if (store) {
            wasm_store_delete(store);
        }
    }

    bool init_instance(const std::string& path_to_wasm_code);

    bool run_instanse(char* data);

    char* new_in_wasm(int64_t size);
    void delete_in_wasm(char* ptr);

private:
    bool create_module(const std::string& path_to_wasm);

    bool create_env();

    bool create_imports(wasm_module_t* module);

    bool create_wasm_instance(wasm_module_t* module);

    bool export_data();

    char* wasm_ptr_to_host_ptr(int64_t wasm_ptr);
    int64_t host_ptr_to_wasm_ptr(char* ptr);


    wasm_store_t* store{};
    wasi_env_t* wasi_env{};
    wasm_func_t* run_func{};

    wasm_func_t* wasm_new{};
    wasm_func_t* wasm_delete{};

    wasm_extern_vec_t imports{};

    wasm_module_t* module{};
    wasm_instance_t* instance{};

    char* start_mem;
};

class wasmer_instance_db : public instance_db<wasmer_instance> {
public:
    wasmer_instance_db() {
        engine = wasm_engine_new();
    }

    ~wasmer_instance_db() {
        wasm_engine_delete(engine);
    }

private:

    bool create_instance(const std::string& wasm_path, const std::string& name) {
        auto it = all_scripts.emplace(name, engine);
        if (!it.first->second.init_instance(wasm_path)) {
            all_scripts.erase(it.first);
            std::cout << "Can not create instance for " << name << std::endl;
            return false;
        }
        return true;
    }

    wasm_engine_t* engine;
};