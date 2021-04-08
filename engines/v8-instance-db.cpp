#include "v8-instance-db.h"
#include "v8.h"

bool v8_instance::init_instance(const std::string& path_to_wasm_code) {
    v8::HandleScope handle_scope(isolate);

    auto local_ctx = v8::Context::New(isolate);

    ctx.Reset(isolate, local_ctx);
    v8::Context::Scope ctx_scope(local_ctx);

    if (!compile_script(path_to_wasm_code)) {
        return false;
    }

    if (!create_function()) {
        return false;
    }

    return true;
}

bool v8_instance::run_instanse(char* data) {
    v8::HandleScope handle_scope(isolate);
    v8::TryCatch try_catch(isolate);
    v8::Local<v8::Context> local_ctx = v8::Local<v8::Context>::New(isolate, ctx);
    v8::Context::Scope ctx_scope(local_ctx);

    auto process = v8::Local<v8::Function>::New(isolate, function);

    auto array_for_run = v8::ArrayBuffer::New(isolate, store);

    const int argc = 1;
    v8::Local<v8::Value> argv[argc] = { array_for_run };
    v8::Local<v8::Value> result;

    if (!process->Call(local_ctx, local_ctx->Global(), argc, argv).ToLocal(&result)) {
        v8::String::Utf8Value error(isolate, try_catch.Exception());
        std::cout << "Can not run script: " << std::string(*error, error.length()) << std::endl;
        return false;
    }

    return true;
}

char* v8_instance::new_in_wasm(int64_t size) {
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> local_ctx = v8::Local<v8::Context>::New(isolate, ctx);
    v8::Context::Scope ctx_scope(local_ctx);

    auto local_data_array = v8::ArrayBuffer::New(isolate, size);
    store = local_data_array->GetBackingStore();

    return reinterpret_cast<char*>(store->Data());
}

void v8_instance::delete_in_wasm(char* ptr) {
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> local_ctx = v8::Local<v8::Context>::New(isolate, ctx);
    v8::Context::Scope ctx_scope(local_ctx);

    store.reset();
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
    v8::HandleScope handle_scope(isolate);
    v8::TryCatch try_catch(isolate);
    v8::Local<v8::Context> local_ctx = v8::Local<v8::Context>::New(isolate, ctx);

    v8::Local<v8::String> script_code = read_file(script_path).ToLocalChecked();
    v8::Local<v8::Script> compiled_script;
    if (!v8::Script::Compile(local_ctx, script_code).ToLocal(&compiled_script)) {
        v8::String::Utf8Value error(isolate, try_catch.Exception());
        std::cout << "Can not compile script: " << std::string(*error, error.length()) << std::endl;
        return false;
    }

    v8::Local<v8::Value> result;
    if (!compiled_script->Run(local_ctx).ToLocal(&result)) {
        v8::String::Utf8Value error(isolate, try_catch.Exception());
        std::cout << "Can not run script: " << std::string(*error, error.length()) << std::endl;
        return false;
    }

    return true;
}

bool v8_instance::create_function() {
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> local_ctx = v8::Local<v8::Context>::New(isolate, ctx);

    v8::Local<v8::String> function_name = v8::String::NewFromUtf8Literal(isolate, "user_script");
    v8::Local<v8::Value> function_val;
    if (!local_ctx->Global()->Get(local_ctx, function_name).ToLocal(&function_val) || !function_val->IsFunction()) {
        std::cout << "Can not create process for function" << std::endl;
        return false;
    }

    v8::Local<v8::Function> process_fun = function_val.As<v8::Function>();
    function.Reset(isolate, process_fun);
    return true;
}
