#include "v8-instance-db.h"
#include "v8.h"
#include <ctime>
#include <iostream>
#include <memory>
#include <chrono>
#include <thread>

namespace  {

std::atomic<std::chrono::time_point<std::chrono::high_resolution_clock> > stop_time;

void thread_foo(std::chrono::time_point<std::chrono::high_resolution_clock> start_time, double time_limit, v8::Isolate* isolate) {
    bool stop = false;
    while (true) {
        if (stop) {
            break;
        }
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time = now - start_time;
        if (time.count() > time_limit && !stop) {
            stop = true;
            stop_time.store(std::chrono::high_resolution_clock::now());
            isolate->TerminateExecution();
        } else {
            std::this_thread::yield();
        }
    }
}

}

bool v8_instance::init_instance(const std::string& path_to_wasm_code) {
    if (!compile_script(path_to_wasm_code)) {
        return false;
    }

    if (!create_function()) {
        return false;
    }

    return true;
}

bool v8_instance::run_instanse(char* data) {
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::TryCatch try_catch(isolate);
    v8::Local<v8::Context> local_ctx = v8::Local<v8::Context>::New(isolate, context);
    v8::Context::Scope context_scope(local_ctx);

    const int argc = 1;
    auto array = v8::ArrayBuffer::New(isolate, store);
    v8::Local<v8::Value> argv[argc] = { array };
    v8::Local<v8::Value> result;

    v8::Local<v8::Function> local_function = v8::Local<v8::Function>::New(isolate, function);

    std::thread t(thread_foo, std::chrono::high_resolution_clock::now(), 1.0, isolate);

    auto res = local_function->Call(local_ctx, local_ctx->Global(), argc, argv).ToLocal(&result);
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = now - stop_time.load();
    std::cout << diff.count() << std::endl;

    if (!res) {
        v8::String::Utf8Value error(isolate, try_catch.Exception());
        std::cout << "Can not run script: " << std::string(*error, error.length()) << std::endl;
        t.join();
        return false;
    }

    t.join();
    return true;
}

char* v8_instance::new_in_wasm(int64_t size) {
    char* data_ptr = new char[size];

    store = v8::ArrayBuffer::NewBackingStore(data_ptr, size, v8::BackingStore::EmptyDeleter, nullptr);
    return data_ptr;
}

void v8_instance::delete_in_wasm(char* ptr) {
    delete[] ptr;
}

v8::MaybeLocal<v8::String> v8_instance::read_file(const std::string& script_path) {
    std::ifstream file(script_path.c_str(), std::ios::in);
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::string binary;
    binary.resize(size);
    if (!file.read(binary.data(), size))
    {
        std::cout << "Can not read code from wasm file" << std::endl;
    }

    v8::MaybeLocal<v8::String> result = v8::String::NewFromUtf8(isolate, binary.c_str(), v8::NewStringType::kNormal, size);
    return result;
}

bool v8_instance::compile_script(const std::string script_path) {
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::TryCatch try_catch(isolate);
    v8::Local<v8::Context> local_ctx = v8::Context::New(isolate);
    v8::Context::Scope context_scope(local_ctx);

    v8::Local<v8::Script> compiled_script;
    v8::Local<v8::String> script_code = read_file(script_path).ToLocalChecked();
    if (!v8::Script::Compile(local_ctx, script_code).ToLocal(&compiled_script)) {
        v8::String::Utf8Value error(isolate, try_catch.Exception());
        std::cout << "Can not compile script: " << std::string(*error, error.length()) << std::endl;
        return false;
    }

    v8::Local<v8::Value> result;
    if (!compiled_script->Run(local_ctx).ToLocal(&result)) {
        printf("run script error\n");
    }

    context.Reset(isolate, local_ctx);

    return true;
}

bool v8_instance::create_function() {
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> local_ctx = v8::Local<v8::Context>::New(isolate, context);
    v8::Context::Scope context_scope(local_ctx);

    v8::Local<v8::String> function_name = v8::String::NewFromUtf8Literal(isolate, "user_script");
    v8::Local<v8::Value> function_val;
    if (!local_ctx->Global()->Get(local_ctx, function_name).ToLocal(&function_val) || !function_val->IsFunction()) {
        std::cout << "Can not create process for function" << std::endl;
        return false;
    }

    function.Reset(isolate, function_val.As<v8::Function>());
    return true;
}
