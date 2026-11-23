#include "GameEngine.h"

int main() {
    // Instantiate your engine with a 800x600 resolution window
    GameEngine game(800, 600, "My Custom SFML Game");
    game.run();
    return 0;
}

