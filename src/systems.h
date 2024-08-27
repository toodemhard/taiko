#pragma once

#include "input.h"
#include "audio.h"
#include "events.h"
#include "asset_loader.h"

struct Systems {
    SDL_Renderer* renderer{};
    Input& input;
    Audio& audio;
    AssetLoader& assets;
    EventQueue& event_queue;
};