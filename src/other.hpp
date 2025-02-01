#pragma once
#include <cstdint>


enum class BlockType : uint8_t {
    Stone, 
    Dirt,
};

struct Chunk consturct_chunk();
