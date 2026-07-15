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
    // The standard easy6502 16-color palette; screen bytes use the low
    // nibble as a color index, matching the ROMs written for it (the
    // bundled snake game picks its apple color from $FE this way).
    static const Byte palette[16][3] = {
        {0x00, 0x00, 0x00}, // 0  black
        {0xFF, 0xFF, 0xFF}, // 1  white
        {0x88, 0x00, 0x00}, // 2  red
        {0xAA, 0xFF, 0xEE}, // 3  cyan
        {0xCC, 0x44, 0xCC}, // 4  purple
        {0x00, 0xCC, 0x55}, // 5  green
        {0x00, 0x00, 0xAA}, // 6  blue
        {0xEE, 0xEE, 0x77}, // 7  yellow
        {0xDD, 0x88, 0x55}, // 8  orange
        {0x66, 0x44, 0x00}, // 9  brown
        {0xFF, 0x77, 0x77}, // 10 light red
        {0x33, 0x33, 0x33}, // 11 dark grey
        {0x77, 0x77, 0x77}, // 12 grey
        {0xAA, 0xFF, 0x66}, // 13 light green
        {0x00, 0x88, 0xFF}, // 14 light blue
        {0xBB, 0xBB, 0xBB}  // 15 light grey
    };
    const Byte* c = palette[v & 0x0F];
    r = c[0];
    g = c[1];
    b = c[2];
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
