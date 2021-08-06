#pragma once
#include <napi.h>

class objDecorator
{
private:

protected:
    virtual void Decorate(Napi::Object obj) {};

public:
    objDecorator() { };
    virtual ~objDecorator() {};
};