#pragma once

#include <cstdio>
#include <iostream>

class App
{
private:
    Bridge * _bridge;

public:
    App() : _bridge(0) {}
    ~App() { delBridge(); }
    void delBridge()
    {
        delete _bridge;
        _bridge = 0;
    }
    void setBridge(Bridge * cb)
    {
        delBridge();
        _bridge = cb;
    }
    void cb()
    {
        if (_bridge)
            _bridge->cb();
    }
};
