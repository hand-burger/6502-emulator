#include "frontend.h"
#include <iostream>
#include <cstring>

Frontend::Frontend() : window(nullptr), renderer(nullptr), texture(nullptr) {}

Frontend::~Frontend() {
    shutdown();
}

bool Frontend::init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
        std::fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return false;
    }
    window = SDL_CreateWindow("6502 Snake", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W * SCALE, H * SCALE, SDL_WINDOW_SHOWN);
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::fprintf(stderr, "SDL_CreateRenderer error: %s\n", SDL_GetError());
        return false;
    }
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, W, H);
    if (!texture) {
        std::fprintf(stderr, "SDL_CreateTexture error: %s\n", SDL_GetError());
        return false;
    }
    last_frame.assign(W * H, 0xFF); // force initial upload
    return true;
}

void Frontend::shutdown() {
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

void Frontend::map_to_rgb(Byte v, Byte& r, Byte& g, Byte& b) {
    if (v == 0) { r = g = b = 0; }
    else if (v == 1) { r = g = b = 255; }
    else { r = g = b = (Byte)(v * 16); }
}

void Frontend::draw_if_changed(const Byte* screen) {
    bool changed = false;
    for (int i = 0; i < W * H; ++i) {
        if (last_frame[i] != screen[i]) {
            changed = true;
            break;
        }
    }
    if (!changed) return;

    void* pixels = nullptr;
    int pitch = 0;
    if (SDL_LockTexture(texture, nullptr, &pixels, &pitch) != 0) return;

    for (int y = 0; y < H; ++y) {
        uint8_t* row = (uint8_t*)pixels + y * pitch;
        for (int x = 0; x < W; ++x) {
            Byte r, g, b;
            map_to_rgb(screen[y * W + x], r, g, b);
            row[x * 3 + 0] = r;
            row[x * 3 + 1] = g;
            row[x * 3 + 2] = b;
        }
    }
    SDL_UnlockTexture(texture);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
    std::memcpy(last_frame.data(), screen, W * H);
}

bool Frontend::handle_events(cpu& cpu) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) return false;
        if (ev.type == SDL_KEYDOWN) {
            switch (ev.key.keysym.sym) {
                case SDLK_ESCAPE: return false;
                case SDLK_w: cpu.write(0x00FF, 'w'); break;
                case SDLK_a: cpu.write(0x00FF, 'a'); break;
                case SDLK_s: cpu.write(0x00FF, 's'); break;
                case SDLK_d: cpu.write(0x00FF, 'd'); break;
                default: break;
            }
        }
    }
    return true;
}
