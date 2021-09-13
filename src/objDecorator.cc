#include <napi.h>
#include "wasm_c_api.h"
#include "objDecorator.h"
#include "objWrapper.h"
extern "C" {
/* shadow the C++ keyword "export" */
#define export extern_export

#if WASM_ENABLE_INTERP != 0
#include "../interpreter/wasm_runtime.h"
#endif
#if WASM_ENABLE_AOT != 0
#include "../aot/aot_runtime.h"
#endif

#if WASM_ENABLE_THREAD_MGR != 0
#include "../libraries/thread-mgr/thread_manager.h"
#endif

#undef export
}

void WAMRModule::Decorate(Napi::Env env, Napi::Object obj) {
    /* module type */
    int module_type = (*module_)->module_type;
    obj["type"] = Napi::String::New(env,
        module_type == Wasm_Module_Bytecode ? "Bytecode" : "AoT");

    if (module_type == Wasm_Module_Bytecode) {
        auto wasm_module = reinterpret_cast<WASMModule *>(*module_);
    }
    else if (module_type == Wasm_Module_AoT) {
        auto aot_module = reinterpret_cast<AOTModule *>(*module_);
    }

}

void WAMRInstance::Decorate(Napi::Env env, Napi::Object obj) {

}

Napi::Value WAMRModule::getType(const Napi::CallbackInfo& info) {
    printf("type: %d\n", (*module_)->module_type);
    return Napi::String::New(info.Env(),
        (*module_)->module_type == Wasm_Module_Bytecode ? "Bytecode" : "AoT");
}

// Napi::Value WAMRModule::getExport(const Napi::CallbackInfo& info) {

// }