#pragma once
#include <string>
#include "gfx/renderer.hpp"
#include "input.hpp"
#include "window.hpp"
#include "world.hpp"


class App {
public:
    App(std::string name);
    ~App();
    
    App(App const&) = delete;
    App(App&&) = delete;
    App& operator=(App const&) = delete;
    App& operator=(App&&) = delete;
        
    void run();
    
private:
    static constexpr uint8_t FRAMES_PER_SECOND {60};
    static constexpr uint8_t TICKS_PER_FRAME {8};

    std::string name_;
    Window window_;
    InputCollector input_;
    World world_; 
    Renderer renderer_;
};
