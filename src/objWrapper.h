#pragma once

#include <napi.h>
#include "wasm_c_api.h"
#include "objDecorator.h"

class WAMRRuntime : public Napi::ObjectWrap<WAMRRuntime>, public objDecorator {
public:
    static inline Napi::FunctionReference constructor_;

    static Napi::Object Init(Napi::Env env, Napi::Object exports);

    WAMRRuntime(const Napi::CallbackInfo& info);

    Napi::Value load(const Napi::CallbackInfo& info);

    Napi::Value instantiate(const Napi::CallbackInfo& info);

    Napi::Value lookupFunction(const Napi::CallbackInfo& info);

    Napi::Value executeFunction(const Napi::CallbackInfo& info);

    Napi::Value deinstantiate(const Napi::CallbackInfo& info);

    Napi::Value unload(const Napi::CallbackInfo& info);

    virtual void Finalize(Napi::Env env);

private:
    wasm_engine_t *engine_;
    wasm_store_t *store_;
};

class WAMRModule : public Napi::ObjectWrap<WAMRModule>, public objDecorator {
public:
    static inline Napi::FunctionReference constructor_;

    static void Init(Napi::Env, Napi::Object &exports);

    static Napi::Object New(Napi::Env env, wasm_module_t *module);

    static bool IsClassOf(const Napi::Value &value);

    static wasm_module_t *Extract(const Napi::Value &value);

    explicit WAMRModule(const Napi::CallbackInfo& info);

    wasm_module_t *getWAMRModule();

    virtual void Decorate(Napi::Env env, Napi::Object obj);

private:
    wasm_module_t *module_;

    Napi::Value getType(const Napi::CallbackInfo& info);
};

class WAMRInstance : public Napi::ObjectWrap<WAMRInstance>, public objDecorator {
public:
    static inline Napi::FunctionReference constructor_;

    static void Init(Napi::Env, Napi::Object &exports);

    static Napi::Object New(Napi::Env env, wasm_instance_t *instance);

    static bool IsClassOf(const Napi::Value &value);

    static wasm_instance_t *Extract(const Napi::Value &value);

    explicit WAMRInstance(const Napi::CallbackInfo& info);

    wasm_instance_t *getWAMRInstance();

    virtual void Decorate(Napi::Env env, Napi::Object obj);

private:
    wasm_instance_t *instance_;
};

class WAMRFunction : public Napi::ObjectWrap<WAMRFunction>, public objDecorator {
public:
    static inline Napi::FunctionReference constructor_;

    static void Init(Napi::Env, Napi::Object &exports);

    static Napi::Object New(Napi::Env env, wasm_func_t *instance);

    static bool IsClassOf(const Napi::Value &value);

    static wasm_func_t *Extract(const Napi::Value &value);

    explicit WAMRFunction(const Napi::CallbackInfo& info);

    wasm_func_t *getWAMRFunction();

private:
    wasm_func_t *function_;
};
