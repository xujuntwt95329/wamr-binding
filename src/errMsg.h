#pragma once

namespace ErrMsg {
    namespace Class {
        namespace WAMRModule {
            static const char *constructor = "WAMRModule.constructor needs to be called with new (external: Napi::External<wasm_module_t>)";
        }
        namespace WAMRInstance {
            static const char *constructor = "WAMRInstance.constructor needs to be called with new (external: Napi::External<wasm_instance_t>)";
        }
    }
}
