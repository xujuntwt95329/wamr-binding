#include <napi.h>
#include "objWrapper.h"
#include "errMsg.h"

#include <vector>

/* wrapper for wamr engine */
Napi::Object WAMRRuntime::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "WAMRRuntime", {
        InstanceMethod<&WAMRRuntime::load>("load"),
        InstanceMethod<&WAMRRuntime::instantiate>("instantiate"),
        InstanceMethod<&WAMRRuntime::lookupFunction>("lookupFunction"),
        InstanceMethod<&WAMRRuntime::executeFunction>("executeFunction"),
        InstanceMethod<&WAMRRuntime::deinstantiate>("deinstantiate"),
        InstanceMethod<&WAMRRuntime::unload>("unload")
    });

    constructor_ = Napi::Persistent(func);
    constructor_.SuppressDestruct();
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
    instanceObj["module"] = info[0];

    return instanceObj;
}

Napi::Value WAMRRuntime::lookupFunction(const Napi::CallbackInfo& info) {
    napi_status status;
    Napi::Env env = info.Env();
    wasm_module_t* module = nullptr;
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
    module = WAMRModule::Extract(info[0].As<Napi::Object>().Get("module"));

    const char *func_name = std::string(info[1].As<Napi::String>()).c_str();

    wasm_exporttype_vec_t exporttypes = {0};
    unsigned int export_item_idx = -1;

    wasm_module_exports(module, &exporttypes);
    for (unsigned int i = 0; i < exporttypes.num_elems; i++) {
        wasm_exporttype_t *exporttype = exporttypes.data[i];
        if (strncmp(func_name, (const char*)(wasm_exporttype_name(exporttype)->data), strlen(func_name)) == 0) {
            export_item_idx = i;
            break;
        }
    }

    if (export_item_idx == -1) {
        return info.Env().Undefined();
    }

    wasm_extern_vec_t exports;
    wasm_instance_exports(instance, &exports);

    wasm_func_t *func = wasm_extern_as_func(exports.data[export_item_idx]);

    auto functionObj = WAMRFunction::New(env, func);
    functionObj["instance"] = info[0];

    return functionObj;
}

static wasm_val_t jsValueToWasmValue(const Napi::Number jsValue, wasm_valtype_t *varType) {
    switch (wasm_valtype_kind(varType)) {
        case WASM_I32:
            return WASM_I32_VAL(int32_t(jsValue));
        break;
        case WASM_I64:
            return WASM_I64_VAL(int64_t(jsValue));
        break;
        case WASM_F32:
            return WASM_F32_VAL(float(jsValue));
        break;
        case WASM_F64:
            return WASM_F64_VAL(double(jsValue));
        break;
        default:

        break;
    }
}

static double wasmValueToDouble(wasm_val_t wasmValue, wasm_valtype_t *varType) {
    switch (wasm_valtype_kind(varType)) {
        case WASM_I32:
            return (double)wasmValue.of.i32;
        break;
        case WASM_I64:
            return (double)wasmValue.of.i64;
        break;
        case WASM_F32:
            return (double)wasmValue.of.f32;
        break;
        case WASM_F64:
            return (double)wasmValue.of.f64;
        break;
        default:

        break;
    }
}

Napi::Value WAMRRuntime::executeFunction(const Napi::CallbackInfo& info) {
    napi_status status;
    Napi::Env env = info.Env();
    wasm_func_t *func = nullptr;

    if (info.Length() != 2) {
        Napi::Error::New(info.Env(), "Expected exactly two argument")
            .ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }

    if (!WAMRFunction::IsClassOf(info[0])) {
        Napi::Error::New(info.Env(), "Expect WAMRFunction object")
            .ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }

    func = WAMRFunction::Extract(info[0]);
    size_t expect_param_size = wasm_func_param_arity(func);

    if (!info[1].IsArray()) {
        Napi::Error::New(info.Env(), "The arguments must be an array")
            .ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }

    Napi::Array jsArgArray = info[1].As<Napi::Array>();

    if (jsArgArray.Length() != expect_param_size) {
        char msg[128];
        sprintf(msg, "argument count not match, %ld expected, %d passed\n",
                expect_param_size, jsArgArray.Length());
        Napi::Error::New(info.Env(), msg)
            .ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }

    wasm_functype_t *func_type = wasm_func_type(func);
    const wasm_valtype_vec_t *param_types = wasm_functype_params(func_type);
    const wasm_valtype_vec_t *result_types = wasm_functype_results(func_type);

    std::vector<wasm_val_t> args;
    std::vector<wasm_val_t> results;

    for (size_t i = 0; i < expect_param_size; i++) {
        Napi::Number jsValue = Napi::Value(jsArgArray[i]).ToNumber();
        args.push_back(jsValueToWasmValue(jsValue, param_types->data[i]));
    }

    for (size_t i = 0; i < wasm_func_result_arity(func); i++) {
        results.push_back(WASM_INIT_VAL);
    }

    wasm_func_call(func, args.data(), results.data());

    auto jsResults = Napi::Array::New(env, results.size());
    for (size_t i = 0; i < results.size(); i++) {
        jsResults[i] =
            Napi::Number::New(env, wasmValueToDouble(results[i], result_types->data[i]));
    }

    return jsResults;
}

Napi::Value WAMRRuntime::deinstantiate(const Napi::CallbackInfo& info) {
    wasm_instance_t* instance = nullptr;

    if (info.Length() != 1) {
        Napi::Error::New(info.Env(), "Expected exactly one argument")
            .ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }

    if (!WAMRInstance::IsClassOf(info[0])) {
        Napi::Error::New(info.Env(), "Expect WAMRInstance object")
            .ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }

    instance = WAMRInstance::Extract(info[0]);

    wasm_instance_delete(instance);

    return info.Env().Undefined();
}

Napi::Value WAMRRuntime::unload(const Napi::CallbackInfo& info) {
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

    wasm_module_delete(module);

    return info.Env().Undefined();
}

void WAMRRuntime::Finalize(Napi::Env env) {
    wasm_store_delete(store_);
    wasm_engine_delete(engine_);
}


/* wrapper for wamr module */
void WAMRModule::Init(Napi::Env env, Napi::Object &exports) {
    Napi::Function func = DefineClass(env, "WAMRModule", {

    });

    constructor_ = Napi::Persistent(func);
    constructor_.SuppressDestruct();
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
    constructor_.SuppressDestruct();
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


/* wrapper for wamr function instance */
void WAMRFunction::Init(Napi::Env env, Napi::Object &exports) {
    Napi::Function func = DefineClass(env, "WAMRFunction", {

    });

    constructor_ = Napi::Persistent(func);
    constructor_.SuppressDestruct();
    exports.Set("WAMRFunction", func);
}

Napi::Object WAMRFunction::New(Napi::Env env, wasm_func_t *func) {
    return constructor_.New({Napi::External<wasm_func_t>::New(env, func)});
}

bool WAMRFunction::IsClassOf(const Napi::Value &value) {
    return value.As<Napi::Object>().InstanceOf(constructor_.Value());
}

wasm_func_t *WAMRFunction::Extract(const Napi::Value &value) {
    return Unwrap(value.As<Napi::Object>())->function_;
}

WAMRFunction::WAMRFunction(const Napi::CallbackInfo& info) : ObjectWrap(info) {
    Napi::Env env = info.Env();
    if (!info.IsConstructCall() || info.Length() == 0 || !info[0].IsExternal()) {
        throw Napi::TypeError::New(env, ErrMsg::Class::WAMRFunction::constructor);
    }
    auto external = info[0].As<Napi::External<wasm_func_t>>();
    function_ = external.Data();
}
