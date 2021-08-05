#include <napi.h>
#include "wasm_c_api.h"
#include "objWrapper.h"

wasm_engine_t *g_engine;
wasm_store_t *g_store;

Napi::Object wasm_runtime_init(const Napi::CallbackInfo& info) {
    napi_status status;
    Napi::Env env = info.Env();

    wasm_engine_t *engine = wasm_engine_new();
    wasm_store_t *store = wasm_store_new(engine);

    /* assign to the global variables, maybe we can wrap wamr instance as a JSObject */
    g_engine = engine;
    g_store = store;

    Napi::Object wamr_inst = Napi::Object::New(env);
    wamr_inst["ready"] = Napi::Boolean::New(env, false);

    auto engine_ref = Napi::Object::New(env);
    auto store_ref = Napi::Object::New(env);

    status = napi_wrap(napi_env(env),
                       napi_value(engine_ref),
                       engine, NULL, NULL, NULL);
    if (status != napi_ok) return wamr_inst;

    status = napi_wrap(napi_env(env),
                       napi_value(store_ref),
                       store, NULL, NULL, NULL);
    if (status != napi_ok) return wamr_inst;

    wamr_inst["engine"] = engine_ref;
    wamr_inst["store"] = store_ref;
    wamr_inst["ready"] = Napi::Boolean::New(env, true);

    return wamr_inst;
}

Napi::Value wasm_runtime_load(const Napi::CallbackInfo& info) {
    napi_status status;
    Napi::Env env = info.Env();

    if (info.Length() != 1) {
        Napi::Error::New(info.Env(), "Expected exactly one argument")
            .ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }

    if (!info[0].IsBuffer()) {
        Napi::Error::New(info.Env(), "Expected a Buffer")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Buffer<char> buf = info[0].As<Napi::Buffer<char>>();

    wasm_byte_vec_t binary;
    wasm_byte_vec_new_uninitialized(&binary, buf.Length());
    memcpy(binary.data, buf.Data(), buf.Length());

    wasm_module_t* module = wasm_module_new(g_store, &binary);

    auto module_ref = Napi::Object::New(env);

    status = napi_wrap(napi_env(env),
                       napi_value(module_ref),
                       module, NULL, NULL, NULL);
    if (status != napi_ok) return env.Undefined();

    return module_ref;
}

Napi::Value wasm_runtime_instantiate(const Napi::CallbackInfo& info) {
    napi_status status;
    Napi::Env env = info.Env();
    wasm_module_t* module = nullptr;

    if (info.Length() != 1) {
        Napi::Error::New(info.Env(), "Expected exactly one argument")
            .ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }

    Napi::Object module_ref = info[0].As<Napi::Object>();

    status = napi_unwrap(napi_env(env), module_ref, (void**)&module);
    if (status != napi_ok) return env.Undefined();

    wasm_instance_t* instance =
        wasm_instance_new(g_store, module, nullptr, nullptr);

    auto instance_ref = Napi::Object::New(env);

    status = napi_wrap(napi_env(env),
                       napi_value(instance_ref),
                       instance, NULL, NULL, NULL);
    if (status != napi_ok) return env.Undefined();

    return instance_ref;
}

Napi::Value wasm_runtime_lookup_function(const Napi::CallbackInfo& info) {
    napi_status status;
    Napi::Env env = info.Env();
    wasm_instance_t* instance = nullptr;

    if (info.Length() != 2) {
        Napi::Error::New(info.Env(), "Expected exactly two argument")
            .ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }

    Napi::Object inst_ref = info[0].As<Napi::Object>();

    status = napi_unwrap(napi_env(env), inst_ref, (void**)&instance);
    if (status != napi_ok) return env.Undefined();

    wasm_extern_vec_t exports;
    wasm_instance_exports(instance, &exports);

    wasm_func_t *func = wasm_extern_as_func(exports.data[1]);

    auto func_ref = Napi::Object::New(env);

    status = napi_wrap(napi_env(env),
                       napi_value(func_ref),
                       func, NULL, NULL, NULL);
    if (status != napi_ok) return env.Undefined();

    wasm_val_t args[2] = { WASM_I32_VAL(3), WASM_I32_VAL(4) };
    wasm_val_t results[1] = { WASM_INIT_VAL };
    wasm_func_call(func, args, results);

    return func_ref;
}

Napi::Object wasm_runtime_call_wasm(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

}

Napi::Value wasm_runtime_deinstantiate(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

}

Napi::Value wasm_runtime_unload(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

}

Napi::Value wasm_runtime_destroy(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set(Napi::String::New(env, "wasm_runtime_init"),
                Napi::Function::New(env, wasm_runtime_init));
    exports.Set(Napi::String::New(env, "wasm_runtime_load"),
                Napi::Function::New(env, wasm_runtime_load));
    exports.Set(Napi::String::New(env, "wasm_runtime_instantiate"),
                Napi::Function::New(env, wasm_runtime_instantiate));
    exports.Set(Napi::String::New(env, "wasm_runtime_lookup_function"),
                Napi::Function::New(env, wasm_runtime_lookup_function));
    return exports;
}

NODE_API_MODULE(wamr, Init)
