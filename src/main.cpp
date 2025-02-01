#include "engine.hpp"


int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) 
{
    Engine engine {}; 
    engine.run();
    engine.shutdown();
    return 0;
} 

