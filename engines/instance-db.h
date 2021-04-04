#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <iostream>

template<typename instance_type>
class instance_db {
public:
    bool add_new_script(const std::string& wasm_path, const std::string& name) {
        auto engine_it = all_scripts.find(name);
        if (engine_it != all_scripts.end()) {
            std::cout << "Script " << name << "already exists" << std::endl;
            return false;
        }

        create_instance(wasm_path, name);
        return true;
    }

    bool delete_script(const std::string& name) {
        auto engine_it = all_scripts.find(name);
        if (engine_it == all_scripts.end()) {
            std::cout << "Can not find script " << name << std::endl;
            return false;
        }

        all_scripts.erase(engine_it);
        return true;
    }

    bool run_script(const std::string& name, char* data) {
        auto engine_it = all_scripts.find(name);
        if (engine_it == all_scripts.end()) {
            std::cout << "Can not find script " << name << std::endl;
            return false;
        }

        engine_it->second.run_instanse(data);
        return true;
    }

    char* alloc_memory_in_wasm_script(const std::string& name, int64_t size) {
        auto engine_it = all_scripts.find(name);
        if (engine_it == all_scripts.end()) {
            std::cout << "Can not find script " << name << std::endl;
            return nullptr;
        }

        return engine_it->second.new_in_wasm(size);
    }

    void delete_memory_in_wasm_script(const std::string& name, char* ptr) {
        auto engine_it = all_scripts.find(name);
        if (engine_it == all_scripts.end()) {
            std::cout << "Can not find script " << name << std::endl;
            return;
        }

         engine_it->second.delete_in_wasm(ptr);
    }

protected:
    virtual bool create_instance(const std::string& wasm_path, const std::string& name) = 0;

    std::unordered_map<std::string, instance_type> all_scripts;

};