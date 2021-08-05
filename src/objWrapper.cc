#include <napi.h>
#include "objWrapper.h"
#include "errMsg.h"

/* wrapper for wamr engine */
Napi::Object WAMRRuntime::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "WAMRRuntime", {
        InstanceMethod<&WAMRRuntime::load>("load"),
        InstanceMethod<&WAMRRuntime::instantiate>("instantiate"),
        InstanceMethod<&WAMRRuntime::lookupFunction>("lookupFunction"),
        InstanceMethod<&WAMRRuntime::executeFunction>("executeFunction")
    });

    exports.Set("wamr", func);

    return exports;
}

WAMRRuntime::WAMRRuntime(const Napi::CallbackInfo& info) : Napi::ObjectWrap<WAMRRuntime>(info) {
    this->engine_ = wasm_engine_new();
    this->store_ = wasm_store_new(engine_);
}

Napi::Value WAMRRuntime::load(const Napi::CallbackInfo& info) {
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

    auto moduleObj = WAMRModule::New(env, module);

    return moduleObj;
}

Napi::Value WAMRRuntime::instantiate(const Napi::CallbackInfo& info) {
    napi_status status;
    Napi::Env env = info.Env();
    wasm_module_t* module = nullptr;

    if (info.Length() != 1) {
        Napi::Error::New(info.Env(), "Expected exactly one argument")
            .ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }

    if (!WAMRModule::IsClassOf(info[0])) {
        Napi::Error::New(info.Env(), "Expect WAMRModule object")
            .ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }

    module = WAMRModule::Extract(info[0]);

    wasm_instance_t* instance =
        wasm_instance_new(store_, module, nullptr, nullptr);

    auto instanceObj = WAMRInstance::New(env, instance);

    return instanceObj;
}

Napi::Value WAMRRuntime::lookupFunction(const Napi::CallbackInfo& info) {
    napi_status status;
    Napi::Env env = info.Env();
    wasm_instance_t* instance = nullptr;

    if (info.Length() != 2) {
        Napi::Error::New(info.Env(), "Expected exactly two argument")
            .ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }

    if (!WAMRInstance::IsClassOf(info[0])) {
        Napi::Error::New(info.Env(), "Expect WAMRInstance object")
            .ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }

    instance = WAMRInstance::Extract(info[0]);

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

/* wrapper for wamr module */
void WAMRModule::Init(Napi::Env env, Napi::Object &exports) {
    Napi::Function func = DefineClass(env, "WAMRModule", {

    });

    constructor_ = Napi::Persistent(func);
    exports.Set("WAMRModule", func);
}

Napi::Object WAMRModule::New(Napi::Env env, wasm_module_t *mod) {
    return constructor_.New({Napi::External<wasm_module_t>::New(env, mod)});
}

bool WAMRModule::IsClassOf(const Napi::Value &value) {
    return value.As<Napi::Object>().InstanceOf(constructor_.Value());
}

wasm_module_t *WAMRModule::Extract(const Napi::Value &value) {
    return Unwrap(value.As<Napi::Object>())->module_;
}

WAMRModule::WAMRModule(const Napi::CallbackInfo& info) : ObjectWrap(info) {
    Napi::Env env = info.Env();
    if (!info.IsConstructCall() || info.Length() == 0 || !info[0].IsExternal()) {
        throw Napi::TypeError::New(env, ErrMsg::Class::WAMRModule::constructor);
    }
    auto external = info[0].As<Napi::External<wasm_module_t>>();
    module_ = external.Data();
}

/* wrapper for wamr module instance */
void WAMRInstance::Init(Napi::Env env, Napi::Object &exports) {
    Napi::Function func = DefineClass(env, "WAMRInstance", {

    });

    constructor_ = Napi::Persistent(func);
    exports.Set("WAMRInstance", func);
}

Napi::Object WAMRInstance::New(Napi::Env env, wasm_instance_t *inst) {
    return constructor_.New({Napi::External<wasm_instance_t>::New(env, inst)});
}

bool WAMRInstance::IsClassOf(const Napi::Value &value) {
    return value.As<Napi::Object>().InstanceOf(constructor_.Value());
}

wasm_instance_t *WAMRInstance::Extract(const Napi::Value &value) {
    return Unwrap(value.As<Napi::Object>())->instance_;
}

WAMRInstance::WAMRInstance(const Napi::CallbackInfo& info) : ObjectWrap(info) {
    Napi::Env env = info.Env();
    if (!info.IsConstructCall() || info.Length() == 0 || !info[0].IsExternal()) {
        throw Napi::TypeError::New(env, ErrMsg::Class::WAMRInstance::constructor);
    }
    auto external = info[0].As<Napi::External<wasm_instance_t>>();
    instance_ = external.Data();
}
