#include "app.hpp"



App::App(std::string name):
    name_{std::move(name)},
    window_{name_.c_str(), 800, 600},
    input_{window_.handle()},
    world_{window_.size(), FRAMES_PER_SECOND / static_cast<float>(TICKS_PER_FRAME)},
    renderer_{window_}
{}

App::~App() 
{
    glfwTerminate();
}

void App::run() 
{
    while (not window_.should_close()) 
    {
        window_.poll();
        auto const actions = input_.actions();
        if (std::find(actions.user_actions.begin(), actions.user_actions.end(), Action::Terminate) != actions.user_actions.end()) break;

        for(uint8_t idx{}; idx < TICKS_PER_FRAME; ++idx) 
        {
            world_.tick(actions);
        }
        
        renderer_.draw(world_.to_render());
        input_.update();
    }
}

