#include "nes.h"
#include "nes_frontend.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <rom_file.nes>" << std::endl;
        std::cout << "Note: This is a basic NES emulator. For testing without a ROM, a test pattern will be displayed." << std::endl;
    }
    
    NES nes;
    NESFrontend frontend;
    
    // Initialize frontend
    if (!frontend.init()) {
        std::cerr << "Failed to initialize frontend" << std::endl;
        return 1;
    }
    
    // Load ROM if provided
    if (argc >= 2) {
        std::string romPath = argv[1];
        if (!nes.loadCartridge(romPath)) {
            std::cerr << "Failed to load ROM: " << romPath << std::endl;
            std::cerr << "Continuing with test pattern..." << std::endl;
        }
    }
    
    // Reset the system
    nes.reset();
    
    std::cout << "NES Emulator started. Controls:" << std::endl;
    std::cout << "  Arrow keys: D-pad" << std::endl;
    std::cout << "  Z: A button" << std::endl;
    std::cout << "  X: B button" << std::endl;
    std::cout << "  Tab: Select" << std::endl;
    std::cout << "  Enter: Start" << std::endl;
    std::cout << "  Escape: Quit" << std::endl;
    
    bool running = true;
    while (running) {
        // Handle input
        running = frontend.handleEvents(nes);
        
        // Step the emulator
        nes.step();
        
        // Render if frame is ready
        if (nes.isFrameReady()) {
            frontend.render(nes.getFrameBuffer());
            nes.clearFrameReady(); // Clear the frame ready flag for the next frame
        }
        
        // Simple frame limiting
        SDL_Delay(16); // ~60 FPS
    }
    
    return 0;
}