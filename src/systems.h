#pragma once

#include "input.h"
#include "audio.h"
#include "events.h"
#include "asset_loader.h"
#include "assets.h"
#include "memory.h"

struct Systems {
    SDL_Renderer* renderer{};
    MemoryAllocators& allocators;
    Input::Input& input;
    Audio& audio;
    AssetLoader& assets;
    EventQueue& event_queue;
};
