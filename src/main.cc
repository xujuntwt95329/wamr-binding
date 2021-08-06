#include <napi.h>
#include "wasm_c_api.h"
#include "objWrapper.h"

using namespace Napi;

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    WAMRRuntime::Init(env, exports);
    WAMRModule::Init(env, exports);
    WAMRInstance::Init(env, exports);
    WAMRFunction::Init(env, exports);

    return exports;
}

NODE_API_MODULE(wamr, Init)
