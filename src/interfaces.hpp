#pragma once

struct IUpdatable {
    virtual void update() = 0;
};

struct ITickable{
    virtual void tick() = 0;
};
