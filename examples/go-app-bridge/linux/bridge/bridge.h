#pragma once

#include <cstdio>
#include <iostream>

class Callback {
	public:
        virtual ~Callback() { std::cout << "Callback::Callback()" << std::endl; }
        virtual void status(int status) { std::cout << "Callback::status(" << status << ")" << std::endl; }
};

class Bridge
{
private:
    Callback * _callback;

public:
    Bridge() : _callback(NULL) { std::cout << "Bridge::Bridge()" << std::endl; }
    ~Bridge() { std::cout << "Bridge::~Bridge()" << std::endl; delCallback(); }
    void delCallback()
    {
        delete _callback;
        _callback = NULL;
    }
    void setCallback(Callback * cb)
    {
        delCallback();
        _callback = cb;
    }
    void status(int status)
    {
        if (_callback)
            _callback->status(status);
    }

    int start(int argc, char **argv);
};
