#include "log.hpp"
#include "app.hpp"
#include "utils.hpp"


int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) 
{
    debug("Starting the app");
    try {
        App app {"Minecraft2"};
        app.run();
    } catch (utils::FatalError const& except) {
        error("Fatal error occured at {}", except.where());
    }
    return 0;
} 

