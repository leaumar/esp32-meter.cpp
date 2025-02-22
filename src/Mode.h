#pragma once

class Mode
{
public:
    virtual ~Mode() {}
    virtual void init() = 0;
    virtual void loop() = 0;
};
