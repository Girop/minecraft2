#pragma once

struct IUpdatable {
    virtual void update() = 0;
    virtual ~IUpdatable() = default;
};

struct ITickable{
    virtual void tick() = 0;
    virtual ~ITickable() = default;
};

