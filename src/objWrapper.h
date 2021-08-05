#pragma once

#include <napi.h>
#include "wasm_c_api.h"

// class WAMRObject : public Napi::ObjectWrap<WAMRObject> {
//  public:
//     static Napi::Object Init(Napi::Env env, Napi::Object exports) {
//         auto obj = Napi::Object::New(env);
//         obj["hello"] = "world";
//     }

//     WAMRObject(const Napi::CallbackInfo& info) : Napi::ObjectWrap<WAMRObject>(info) {

//     }

//     void set_data(void *data) {
//         data_ = data;
//     }

//     void *get_data() {
//         return data_;
//     }

//     private:
//     void* data_;
// };

class WAMRRuntime : public Napi::ObjectWrap<WAMRRuntime> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports) {
        Napi::Function func = DefineClass(env, "WAMRRuntime", {
            InstanceMethod<&WAMRRuntime::load>("load"),
            InstanceMethod<&WAMRRuntime::instantiate>("instantiate"),
            InstanceMethod<&WAMRRuntime::lookupFunction>("lookupFunction"),
            InstanceMethod<&WAMRRuntime::executeFunction>("executeFunction")
        });

        exports.Set("wamr", func);

        return exports;
    }

    WAMRRuntime(const Napi::CallbackInfo& info) : Napi::ObjectWrap<WAMRRuntime>(info) {
        this->engine_ = wasm_engine_new();
        this->store_ = wasm_store_new(engine_);
    }

    Napi::Value load(const Napi::CallbackInfo& info) {
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

        wasm_module_t* module = wasm_module_new(store_, &binary);

        auto module_ref = Napi::Object::New(env);

        status = napi_wrap(napi_env(env),
                           napi_value(module_ref),
                           module, NULL, NULL, NULL);
        if (status != napi_ok) return env.Undefined();

        return module_ref;
    }

    Napi::Value instantiate(const Napi::CallbackInfo& info) {
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
            wasm_instance_new(store_, module, nullptr, nullptr);

        auto instance_ref = Napi::Object::New(env);

        status = napi_wrap(napi_env(env),
                           napi_value(instance_ref),
                           instance, NULL, NULL, NULL);
        if (status != napi_ok) return env.Undefined();

        return instance_ref;
    }

    Napi::Value lookupFunction(const Napi::CallbackInfo& info) {
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

    Napi::Value executeFunction(const Napi::CallbackInfo& info) {

    }

private:
    wasm_engine_t *engine_;
    wasm_store_t *store_;
};