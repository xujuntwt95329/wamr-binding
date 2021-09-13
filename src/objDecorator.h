#pragma once
#include <napi.h>

class objDecorator
{
private:

protected:
    virtual void Decorate(Napi::Env env, Napi::Object obj) {};

public:
    objDecorator() { };
    virtual ~objDecorator() {};
};