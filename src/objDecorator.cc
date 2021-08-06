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

void WAMRModule::Decorate(Napi::Object obj) {

}

void WAMRInstance::Decorate(Napi::Object obj) {

}