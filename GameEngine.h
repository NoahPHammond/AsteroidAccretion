#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <SFML/Audio.hpp>

// GameState
enum class GameState {
    START_SCREEN,
    PLAYING,
    GAME_OVER
};


struct InputState {
    bool upPressed = false;
    bool downPressed = false;
    bool leftPressed = false;
    bool rightPressed = false;
    bool spacePressed = false;
};

// Pure physics struct for the player rocket
struct Player {
    double x = 400.0;
    double y = 300.0;
    double vx = 0.0;
    double vy = 0.0;
    double ax = 0.0;
    double ay = 0.0;
    double angle = 0.0;   // Orientation in degrees (0 = pointing up)
    float width = 20.0;
    float height = 30.0;
    int    hit    = 0;
    int    invinsible_frames = 0;
};

// Pure physics struct for asteroids
struct Asteroid {
    double x = 0.0;
    double y = 0.0;
    double vx = 0.0;
    double vy = 0.0;
    double ax = 0.0;
    double ay = 0.0;
    double radius = 25.0;
    int r = 100;
    int g = 100;
    int b = 100;
    int invincibility_frames=100;
};

//Bullets
struct Bullet {
	double x =0.0;
	double y =0.0;
	double vx =0.0;
	double vy=0.0;
	double width=2.0;
        long int spawn_time=0.0; //Will save time when bullet is generated
};

struct GasParticle {
    double x = 0.0;
    double y = 0.0;
    double vx = 0.0;
    double vy = 0.0;
    double ax = 0.0;
    double ay = 0.0;
};


class GameEngine {
public:
    GameEngine(int width, int height, const std::string& title);
    ~GameEngine();

    void run();
    int num_pts=0;
    long int game_timer=0;
    long int last_bullet_time=0;
private:
    void processEvents();
    void update();
    void render();

    sf::RenderWindow* m_window;
    int m_screenWidth;
    int m_screenHeight;

    InputState m_inputState;
    Player m_player;
    std::vector<Asteroid> m_asteroids;

    //Bullets and Asteroids in vector of flexible size
    std::vector<Bullet> m_bullets;
 
//Now adding gas particles
std::vector<GasParticle> m_gasParticles;


    GameState m_state = GameState::START_SCREEN;
    sf::Font m_font;
    // ... your existing window, player, asteroids, and font members ...

    // Audio members (kept alive for the entire game)
    sf::Music m_bgMusic;
    sf::SoundBuffer m_engineBuffer;
    sf::Sound m_engineSound{m_engineBuffer};
    //sf::SoundBuffer m_laserBuffer;
    //sf::Sound m_laserSound{m_laserBuffer};

    //sf::SoundBuffer m_explosionBuffer;
    //sf::Sound m_explosionSound{m_explosionBuffer};

    // Simulation tuning parameters
    double m_rotationSpeed = 3.5;
    double m_thrustPower = 0.23;
    double m_drag = 0.97;

    // Asteroid spawning controls
    sf::Clock m_asteroidTimer;
    float m_spawnInterval = 2.0f;

   //End game
   int explosion_frames =0;
};

#endif // GAME_ENGINE_H

