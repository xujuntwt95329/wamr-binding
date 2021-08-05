#pragma once

#include <napi.h>

class WAMRObject : public Napi::ObjectWrap<WAMRObject> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  WAMRObject(const Napi::CallbackInfo& info) : Napi::ObjectWrap<WAMRObject>(info) {

  }

  void set_data(void *data) {
      data_ = data;
  }

  void *get_data() {
      return data_;
  }

 private:
   void* data_;
};
