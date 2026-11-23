#include "GameEngine.h"
#include <optional>
#include <cmath>
#include <cstdlib>
#include <ctime>

GameEngine::GameEngine(int width, int height, const std::string& title)
    : m_screenWidth(width), m_screenHeight(height) {
    
    sf::ContextSettings settings;
    settings.antiAliasingLevel = 8;

    m_window = new sf::RenderWindow(
        sf::VideoMode({static_cast<unsigned int>(width), static_cast<unsigned int>(height)}),
        title,
        sf::Style::Default,
        sf::State::Windowed,
        settings
    );
    m_window->setFramerateLimit(60);

    // Initialize Player starting state
    m_player.x = width / 2.0;
    m_player.y = height / 2.0;
    m_player.vx = 0.0;
    m_player.vy = 0.0;
    m_player.angle = 0.0;
    m_player.hit = 0;
    
    num_pts=0;


   //Initialize gas particles
   m_gasParticles.reserve(40000);
   double dx = static_cast<double>(m_screenWidth)/200;
   double dy = static_cast<double>(m_screenHeight)/200;
   for (int row = 0; row<200; row++){
	for (int col=0; col<200; col++){
	   GasParticle p; 
           p.x=dx*col;
           p.y=dy*row;
           p.vx=0.0;  p.vy=0.0;
           p.ax=0.0;  p.ay=0.0;
           m_gasParticles.push_back(p);	
        }
   }



    // Seed the random number generator
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
        // Load default Mac system font for title and start prompt
    if (!m_font.openFromFile("Arial.ttf")) {
            // Fallback if font path differs
}

    // Load and start looping background music
    if (m_bgMusic.openFromFile("AsteroidsNice.mp3")) {
        m_bgMusic.setLooping(true);
        m_bgMusic.setVolume(20.0f); // 0 to 100
        m_bgMusic.play();
    }
    if (m_engineBuffer.loadFromFile("Engine.mp3")) {
        m_engineSound.setVolume(70.0f);
    }

}

GameEngine::~GameEngine() {
    delete m_window;
}

void GameEngine::run() {
    while (m_window->isOpen()) {
        processEvents();
        update();
        render();
    }
}

void GameEngine::processEvents() {
    while (const std::optional<sf::Event> event = m_window->pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            m_window->close();
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
	    if (m_state == GameState::START_SCREEN && keyPressed->code == sf::Keyboard::Key::Space) {
        	m_state = GameState::PLAYING;
        	m_asteroidTimer.restart(); // Reset timer so asteroids don't spawn immediately upon starting
    		}
            if (keyPressed->code == sf::Keyboard::Key::Up    || keyPressed->code == sf::Keyboard::Key::W) m_inputState.upPressed = true;
            if (keyPressed->code == sf::Keyboard::Key::Down  || keyPressed->code == sf::Keyboard::Key::S) m_inputState.downPressed = true;
            if (keyPressed->code == sf::Keyboard::Key::Left  || keyPressed->code == sf::Keyboard::Key::A) m_inputState.leftPressed = true;
            if (keyPressed->code == sf::Keyboard::Key::Right || keyPressed->code == sf::Keyboard::Key::D) m_inputState.rightPressed = true;
            if (keyPressed->code == sf::Keyboard::Key::Space) m_inputState.spacePressed = true;
        }

        if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>()) {
            if (keyReleased->code == sf::Keyboard::Key::Up    || keyReleased->code == sf::Keyboard::Key::W) m_inputState.upPressed = false;
            if (keyReleased->code == sf::Keyboard::Key::Down  || keyReleased->code == sf::Keyboard::Key::S) m_inputState.downPressed = false;
            if (keyReleased->code == sf::Keyboard::Key::Left  || keyReleased->code == sf::Keyboard::Key::A) m_inputState.leftPressed = false;
            if (keyReleased->code == sf::Keyboard::Key::Right || keyReleased->code == sf::Keyboard::Key::D) m_inputState.rightPressed = false;
            if (keyReleased->code == sf::Keyboard::Key::Space) m_inputState.spacePressed = false;
        }
    }
}

void GameEngine::update() {

// First check if game started
    if (m_state == GameState::START_SCREEN) return;
//Check if game is over
    if (m_player.hit >= 4) {
    	m_state = GameState::GAME_OVER;
	explosion_frames=150;
    }
    if (m_state != GameState::PLAYING) return;


   //Give player points if still alive?
   num_pts++;
   //Increase game timer
   game_timer++;

    // ==========================================
    // 1. PLAYER ROCKET PHYSICS
    // ==========================================
    if (m_inputState.leftPressed)  m_player.angle -= m_rotationSpeed;
    if (m_inputState.rightPressed) m_player.angle += m_rotationSpeed;

    m_player.ax = 0.0;
    m_player.ay = 0.0;
    if (m_inputState.upPressed) {
        double angleRadians = m_player.angle * (M_PI / 180.0);
        m_player.ax = std::cos(angleRadians - M_PI / 2.0) * m_thrustPower;
        m_player.ay = std::sin(angleRadians - M_PI / 2.0) * m_thrustPower;
        m_engineSound.play();
    }
    else {m_engineSound.stop();}
    if (m_player.invinsible_frames>0){m_player.invinsible_frames--;};


        //Here we will account for the each asteroids gravity affecting the spacecraft.
 double dx,dy,dist0, Fg, speed, beta;
            for (size_t i = 0; i < m_asteroids.size(); ++i) {
			
		
	dx = m_player.x - m_asteroids[i].x;
        dy = m_player.y - m_asteroids[i].y;

            // Minimum Image Convention (Shortest distance across periodic boundaries)
                if (dx > m_screenWidth * 0.5)       dx -= m_screenWidth;
                else if (dx < -m_screenWidth * 0.5) dx += m_screenWidth;
                if (dy > m_screenHeight * 0.5)       dy -= m_screenHeight;
                else if (dy < -m_screenHeight * 0.5) dy += m_screenHeight;

        dist0= std::hypot(dx,dy);

       Fg=3.0/(dist0*dist0+30.0); //Make gravity pull harder on ship for some reason
        
       m_player.ax+=(-dx/dist0)*Fg*m_asteroids[i].radius;
       m_player.ay+=(-dy/dist0)*Fg*m_asteroids[i].radius;

       //Also detect if player is hit
       if (dist0<(5.0+m_asteroids[i].radius) && m_player.invinsible_frames<=0) {
           m_player.hit++;
           m_player.invinsible_frames=120;
           //Make it bounce hard?
           double temp_vx = m_player.vx;
           double temp_vy = m_player.vy;
           
           m_player.vx = m_asteroids[i].vx;
           m_player.vy = m_asteroids[i].vy;
           m_asteroids[i].vx=temp_vx;
           m_asteroids[i].vy=temp_vy;

       }
       }
       //Gravity control
       speed = std::hypot(m_player.vx,m_player.vy);

       //Add air friction (even though we in space...)
       beta=0.005;
       m_player.ax+=-beta*speed*m_player.vx;
       m_player.ay+=-beta*speed*m_player.vy;
       m_player.vx+=m_player.ax;
       m_player.vy+=m_player.ay;


	m_player.x += m_player.vx;
        m_player.y += m_player.vy;

    // Player Periodic Boundary Conditions
    if (m_player.x > m_screenWidth)       m_player.x = 0.0;
    else if (m_player.x < 0.0)            m_player.x = static_cast<double>(m_screenWidth);
    if (m_player.y > m_screenHeight)      m_player.y = 0.0;
    else if (m_player.y < 0.0)            m_player.y = static_cast<double>(m_screenHeight);

    // ==========================================
    // 2. ASTEROID GENERATION (SPAWNING TIMER)
    // ==========================================
    if (m_asteroidTimer.getElapsedTime().asSeconds() >= m_spawnInterval && m_asteroids.size() < 220) {
        Asteroid newAstro;
        newAstro.radius = 2.5 + static_cast<double>(std::rand() % 20);
        newAstro.ax = 0.0;
        newAstro.ay = 0.0;
	newAstro.invincibility_frames=10;
        
	// Give each asteroid a slightly different rocky tint around (100, 100, 100 +/- 20)
	newAstro.r = 100 + (std::rand() % 21) - 10;
	newAstro.g = 100 + (std::rand() % 21) - 10;
	newAstro.b = 100 + (std::rand() % 41) - 20;


        int spawn_dir = std::rand() % 4; // 0=top, 1=bottom, 2=left, 3=right
        if (spawn_dir < 2) {
            newAstro.x = static_cast<double>(std::rand() % m_screenWidth);
            newAstro.y = spawn_dir * static_cast<double>(m_screenHeight);
            newAstro.vx = -0.5 + (std::rand() % 1);
            newAstro.vy = 0.5 + (std::rand() % 1);
            if (spawn_dir == 1) {
                newAstro.vy *= -1.0;
            }
        } else {
            newAstro.x = (spawn_dir - 2) * static_cast<double>(m_screenWidth);
            newAstro.y = static_cast<double>(std::rand() % m_screenHeight);
            newAstro.vx = 0.5 + (std::rand() % 1);
            newAstro.vy = -0.5 + (std::rand() % 1);
            if (spawn_dir == 3) {
                newAstro.vx *= -1.0;
            }
        }

        m_asteroids.push_back(newAstro);
        m_asteroidTimer.restart();
    }

   //  ===========================================
   // 3.0  Bullet generation
   // ============================================
   
   //First check if space pressed
    if (m_inputState.spacePressed) {
	//Then check if enough time has passed for new bullet;
	if (game_timer-last_bullet_time>20) {
	   last_bullet_time=game_timer;
           Bullet newBullet;
           newBullet.x = m_player.x;
	   newBullet.y = m_player.y;
           newBullet.vx = 4.0*std::sin(3.1415*m_player.angle/180.0);
           newBullet.vy = -4.0*std::cos(3.1415*m_player.angle/180.0);
	   m_bullets.push_back(newBullet);


	   //create recoil
           m_player.vx += -0.1*newBullet.vx;
           m_player.vy += -0.1*newBullet.vy;	
	   }
	} 
    //Now update bullet position
    for (int i=0; i<m_bullets.size();i++){
	m_bullets[i].x += m_bullets[i].vx;
	m_bullets[i].y += m_bullets[i].vy;

       //Let's get rid of bullet if it leaves the screen?
       if (m_bullets[i].x > m_screenWidth || m_bullets[i].x < 0) {m_bullets.erase(m_bullets.begin() + i);}
       if (m_bullets[i].y > m_screenHeight || m_bullets[i].y < 0) {m_bullets.erase(m_bullets.begin() + i);}
	

       //Now check if they collided with asteroid
       double hit_radius; 
       for (int j=0; j<m_asteroids.size(); j++){
	hit_radius = std::hypot(m_asteroids[j].x-m_bullets[i].x,m_asteroids[j].y-m_bullets[i].y);
        if (hit_radius<m_asteroids[j].radius) {
		//Make explosion? 
		//Disapear asteroid and bullet
                m_asteroids.erase(m_asteroids.begin()+j);
		m_bullets.erase(m_bullets.begin()+i);
                num_pts=num_pts-50;
		}
	}
}

    // ==========================================
    // 3. ASTEROID COLLISION & PHYSICS UPDATE
    // ==========================================
    // Reset accelerations before calculating pair forces
    for (size_t i = 0; i < m_asteroids.size(); ++i) {
        m_asteroids[i].ax = 0.0;
        m_asteroids[i].ay = 0.0;
    }

    // Pairwise collision loop (Ready for your viscoelastic force equations)
    double distance;
    double spring_k = 0.05;
    double spring_accel,vx_rel, vy_rel, v_radial;
    double damping = 0.1;
    double v_rel;
    std::vector<Asteroid> newFragments;
    int new_fragments_this_step=0;   
 
    for (size_t i = 0; i < m_asteroids.size(); ++i) {
    if (m_asteroids[i].invincibility_frames>0){m_asteroids[i].invincibility_frames--;};
    if (m_asteroids[i].invincibility_frames==0) {
     //Now proceed with asteroid-asteroid interaction
        for (size_t j = i + 1; j < m_asteroids.size(); ++j) {
            dx = m_asteroids[j].x - m_asteroids[i].x;
            dy = m_asteroids[j].y - m_asteroids[i].y;
           
            // Minimum Image Convention (Shortest distance across periodic boundaries)
	if (dx > m_screenWidth * 0.5)       dx -= m_screenWidth;
	else if (dx < -m_screenWidth * 0.5) dx += m_screenWidth;
	if (dy > m_screenHeight * 0.5)       dy -= m_screenHeight;
	else if (dy < -m_screenHeight * 0.5) dy += m_screenHeight;
  
            distance = std::hypot(dx, dy);

            if (distance < m_asteroids[i].radius + m_asteroids[j].radius) {
                spring_accel = spring_k*(m_asteroids[i].radius + m_asteroids[j].radius-distance);
                //Find relative velocity to calculate damping
                vx_rel=m_asteroids[j].vx - m_asteroids[i].vx;
                vy_rel=m_asteroids[j].vy - m_asteroids[i].vy;
		v_rel=std::hypot(vx_rel,vy_rel);
                v_radial = vx_rel * (dx / distance) + vy_rel * (dy / distance); //radial velocity component
                //damping=v_rel/(v_factor+v_rel);
                spring_accel -=damping*v_radial;
		if (spring_accel>0.2){spring_accel=0.2;}
		if (spring_accel<0){spring_accel=0;}
                m_asteroids[i].ax += spring_accel*(-dx/distance);
                m_asteroids[i].ay += spring_accel*(-dy/distance);
		m_asteroids[j].ax += spring_accel*(dx/distance);
		m_asteroids[j].ay += spring_accel*(dy/distance);
               
                //Check if they just collided and add fragments if so
                if ((m_asteroids[i].radius + m_asteroids[j].radius - distance <6.0)&&v_radial<-2.7){
                        //Also only spawn new asteroids if there are less than 50 total
                   if (m_asteroids.size() < 51 &&new_fragments_this_step==0){
			m_asteroids[i].radius *=0.9;
			m_asteroids[j].radius *=0.9;
                        Asteroid frag1 =m_asteroids[i];
			Asteroid frag2 =m_asteroids[j];
                        double gamma=std::atan2(vx_rel,vy_rel); //imapct angle
			new_fragments_this_step=1.0;

			// Override only the properties that change
			frag1.radius = 2.5;
			double offset=1.65*m_asteroids[i].radius+frag1.radius;
			frag1.x += offset*std::sin(gamma);
			frag1.y += offset*std::cos(gamma);
			frag1.vx *=0.5*sin(gamma);
			frag1.vy *= 0.5*cos(gamma);
			frag1.invincibility_frames=0;

			frag2.radius = 5.0;
			/*frag2.x = midX - perpX * offsetDist;
			frag2.y = midY - perpY * offsetDist;
			frag2.vx = midVx - perpX * ejectSpeed;
			frag2.vy = midVy - perpY * ejectSpeed;*/
			//If fragment is inside another asteroid, delete it
			bool overlapping = false;
			for (int kk=0;kk<m_asteroids.size();kk++){
				double dx2=frag1.x-m_asteroids[kk].x;
				double dy2=frag1.y-m_asteroids[kk].y;
				double dist1=std::hypot(dx2,dy2);
				if (dist1<(3.0+m_asteroids[kk].radius)){ overlapping= true;}
				}
			if (!overlapping){
				newFragments.push_back(frag1);
				}
			//newFragments.push_back(frag2);
		     }
		} 

              }

                //Do I go crazy with the cheese wiz and add gravity?
                double Fg_mag=0.85/(distance*distance+50.0);
                m_asteroids[i].ax += Fg_mag*(dx/distance)*m_asteroids[j].radius;
                m_asteroids[i].ay += Fg_mag*(dy/distance)*m_asteroids[j].radius;
                m_asteroids[j].ax += Fg_mag*(-dx/distance)*m_asteroids[i].radius;
                m_asteroids[j].ay += Fg_mag*(-dy/distance)*m_asteroids[i].radius;
          }
        }
    }
   //If a new fragment is inside another pre-existing asteroid, delete it
   
   m_asteroids.insert(m_asteroids.end(), newFragments.begin(), newFragments.end());

    // Integrate velocity and position for all asteroids
    for (size_t i = 0; i < m_asteroids.size(); ++i) {
	//Need a touch of damping on the asteroids acceleration
        m_asteroids[i].ax += -0.3*beta*m_asteroids[i].vx*std::hypot(m_asteroids[i].vx,m_asteroids[i].vy);
	m_asteroids[i].ay += -0.3*beta*m_asteroids[i].vy*std::hypot(m_asteroids[i].vx,m_asteroids[i].vy);
        m_asteroids[i].vx += m_asteroids[i].ax;
        m_asteroids[i].vy += m_asteroids[i].ay;
        m_asteroids[i].x  += m_asteroids[i].vx;
        m_asteroids[i].y  += m_asteroids[i].vy;
	
	//Set a maximum asteroid speed
        speed = std::hypot(m_asteroids[i].vx,m_asteroids[i].vy);
	if (speed>3.0) {
	double damp=3.0/speed;
	m_asteroids[i].vx *=damp;
	m_asteroids[i].vy *=damp;


	}
        // Asteroid Periodic Boundary Conditions
        if (m_asteroids[i].x > m_screenWidth)       m_asteroids[i].x = 0.0;
        else if (m_asteroids[i].x < 0.0)            m_asteroids[i].x = static_cast<double>(m_screenWidth);
        if (m_asteroids[i].y > m_screenHeight)      m_asteroids[i].y = 0.0;
        else if (m_asteroids[i].y < 0.0)            m_asteroids[i].y = static_cast<double>(m_screenHeight);
    }

	//Now let's evolve the gas particles
   for (int i=0; i<40000;i++){
      //Only do acceleration update on points every half second (30 steps)
      if (game_timer%15 ==0){
      m_gasParticles[i].ax=0.0; m_gasParticles[i].ay=0.0;
      for (int j=0; j<m_asteroids.size(); j++){
            double dx=m_gasParticles[i].x-m_asteroids[j].x;
 	    double dy=m_gasParticles[i].y-m_asteroids[j].y;
            double dist_sq=dx*dx+dy*dy;
            double grav=m_asteroids[j].radius*0.5/(70.0+dist_sq);
            //idea, if particle is inside asteroid, grav pushes it out?
              
              m_gasParticles[i].ax += -1.0*std::copysign(1.0,dx)*(dx*dx/dist_sq)*grav;
              m_gasParticles[i].ay += -1.0*std::copysign(1.0,dx)*(dy*dy/dist_sq)*grav;
          
          //If collided they bounce off?
          if (dist_sq<m_asteroids[j].radius*m_asteroids[j].radius*1.7) {
              double v_orbit = std::sqrt(0.5*m_asteroids[j].radius/std::sqrt(dist_sq));
              double vx_bounce=m_asteroids[j].vx +2.2*(std::copysign(1.0,dy)*dy*dy/dist_sq)*v_orbit;
              double vy_bounce=m_asteroids[j].vy +2.2*(std::copysign(-1.0,dy)*dx*dx/dist_sq)*v_orbit;
              m_gasParticles[i].vx += 0.66*(vx_bounce-m_gasParticles[i].vx);
              m_gasParticles[i].vy += 0.66*(vy_bounce-m_gasParticles[i].vy);
              //m_gasParticles[i].ax = 0.0;
              //m_gasParticles[i].ay = 0.0;
              //break;
          }
          
	}
	}
	
      double speed_g = m_gasParticles[i].vx*m_gasParticles[i].vx+m_gasParticles[i].vy*m_gasParticles[i].vy;
      m_gasParticles[i].ax += -0.0005*m_gasParticles[i].vx*speed_g;
      m_gasParticles[i].ay += -0.0005*m_gasParticles[i].vy*speed_g;
      m_gasParticles[i].vx += m_gasParticles[i].ax;
      m_gasParticles[i].vy += m_gasParticles[i].ay;
      m_gasParticles[i].x += m_gasParticles[i].vx;
      m_gasParticles[i].y += m_gasParticles[i].vy;
      
        if (m_gasParticles[i].x > m_screenWidth)       {m_gasParticles[i].x = 0.0;}
        else if (m_gasParticles[i].x < 0.0)            {m_gasParticles[i].x = static_cast<double>(m_screenWidth);}
        if (m_gasParticles[i].y > m_screenHeight)      {m_gasParticles[i].y = 0.0;}
        else if (m_gasParticles[i].y < 0.0)            {m_gasParticles[i].y = static_cast<double>(m_screenHeight);}

   }


}

void GameEngine::render() {
  
    m_window->clear(sf::Color::Black);
   //Draw start screen if on start screen
       if (m_state == GameState::START_SCREEN) {
        // Game Title
        sf::Text titleText(m_font, "ASTEROIDS", 54);
        titleText.setFillColor(sf::Color::White);
        sf::FloatRect titleBounds = titleText.getLocalBounds();
        titleText.setOrigin({titleBounds.position.x + titleBounds.size.x / 2.0f, titleBounds.position.y + titleBounds.size.y / 2.0f});
        titleText.setPosition({m_screenWidth / 2.0f, m_screenHeight * 0.25f});
        m_window->draw(titleText);

        // Start Button Box
        sf::RectangleShape buttonBox({280.0f, 50.0f});
        buttonBox.setOrigin({140.0f, 25.0f});
        buttonBox.setPosition({m_screenWidth / 2.0f, m_screenHeight * 0.72f});
        buttonBox.setFillColor(sf::Color(20, 40, 20));
        buttonBox.setOutlineColor(sf::Color::Green);
        buttonBox.setOutlineThickness(2.0f);
        m_window->draw(buttonBox);

        // Start Button Label
        sf::Text startText(m_font, "PRESS SPACE TO START", 20);
        startText.setFillColor(sf::Color::Green);
        sf::FloatRect startBounds = startText.getLocalBounds();
        startText.setOrigin({startBounds.position.x + startBounds.size.x / 2.0f, startBounds.position.y + startBounds.size.y / 2.0f});
        startText.setPosition({m_screenWidth / 2.0f, m_screenHeight * 0.72f});
        m_window->draw(startText);
    }
  
  
    //0. Draw stary background?
    
    float star_x[]={30,600,420,150,380,410,699,75,302,202,227,500,400,300,200,100,10,500,60,488,155,559,630,70,450,285};
    float star_y[]={200,300,100,449,220,55,99,306,579,293,122,39,9,588,378,264,236,414,501,174,114,209,388,523,340,410};
    sf::CircleShape tall(60.f, 4), wide(60.f, 4);
    for (int i=0; i<26; i++){
    tall.setPosition({star_x[i], star_y[i]});
    wide.setPosition({star_x[i], star_y[i]});
    tall.setScale({0.4f, 1.5f});
    wide.setScale({1.0f, 0.4f});
    tall.setFillColor(sf::Color(255,255,0,(100+std::rand()%100)));
    wide.setFillColor(sf::Color::Yellow);
    tall.setRadius(3.0f); wide.setRadius(3.0f);
    tall.setOrigin({3.0f, 3.0f});
    wide.setOrigin({3.0f, 3.0f});
    m_window ->draw(tall);
    m_window ->draw(wide);
    }

    // ==========================================
    // 0. DRAW SWIRLING GAS NEBULA (ADDITIVE DENSITY)
    // ==========================================
    sf::VertexArray gasVertices(sf::PrimitiveType::Points, m_gasParticles.size());
    float puffHalfSize = 6.0f; // 8x8 pixel translucent cloud puffs
       sf::Color cloudColor(180, 60, 255, 6);   // Ultra-low alpha for smooth additive mist
       sf::Color pointColor(235, 130, 255, 45); // Brighter pinpoint core

       sf::VertexArray gasClouds(sf::PrimitiveType::Triangles, m_gasParticles.size() * 6);

    for (size_t i = 0; i < m_gasParticles.size(); ++i) {
        float px = static_cast<float>(m_gasParticles[i].x);
        float py = static_cast<float>(m_gasParticles[i].y);
        gasVertices[i].position = {px,py};
        // Translucent violet-pink; overlapping particles sum via BlendAdd
        gasVertices[i].color = sf::Color(200, 70, 255, 130);
        
        //Also do points as larger transparent triangles
        size_t idx = i * 6;
        gasClouds[idx + 0] = {{px - puffHalfSize, py - puffHalfSize}, cloudColor};
        gasClouds[idx + 1] = {{px + puffHalfSize, py - puffHalfSize}, cloudColor};
                gasClouds[idx + 2] = {{px + puffHalfSize, py + puffHalfSize}, cloudColor};

                gasClouds[idx + 3] = {{px - puffHalfSize, py - puffHalfSize}, cloudColor};
                gasClouds[idx + 4] = {{px + puffHalfSize, py + puffHalfSize}, cloudColor};
                gasClouds[idx + 5] = {{px - puffHalfSize, py + puffHalfSize}, cloudColor};
        
    }

    m_window->draw(gasVertices, sf::BlendAdd);
    m_window->draw(gasClouds, sf::BlendAdd);
    /*for (size_t i = 0; i < m_gasParticles.size(); ++i) {
        gasVertices[i].position = {
            static_cast<float>(-1.0*m_gasParticles[i].y),
            static_cast<float>(m_gasParticles[i].x)
        };
        // Translucent violet-pink; overlapping particles sum via BlendAdd
        gasVertices[i].color = sf::Color(200, 240, 120, 160);
    }
     */

    m_window->draw(gasVertices, sf::BlendAdd);


    // 1. Draw Player Rocket
    sf::RectangleShape playerRect({static_cast<float>(m_player.width), static_cast<float>(m_player.height)});
    //playerRect.setOrigin({static_cast<float>(m_player.width / 2.0), static_cast<float>(m_player.height / 2.0)});
    playerRect.setOrigin({m_player.width/2.0f,m_player.height/2.0f});
    playerRect.setPosition({static_cast<float>(m_player.x), static_cast<float>(m_player.y)});
    playerRect.setRotation(sf::degrees(static_cast<float>(m_player.angle)));
    playerRect.setFillColor(sf::Color::Green);
    if (m_player.hit>0){
	playerRect.setFillColor(sf::Color(80*m_player.hit,255-(80*m_player.hit),2*m_player.invinsible_frames));
	}
    m_window->draw(playerRect);
/*
    sf::ConvexShape playerTop;
    playerTop.setPointCount(3);
    playerTop.setPoint(0,{m_player.width/2.f,-1.f*m_player.height/2.f});
    playerTop.setPoint(1,{m_player.width+1.0f,0.f});
    playerTop.setPoint(2,{-1.f,0.f});
    playerTop.setPosition({static_cast<float>(m_player.x)-m_player.width, static_cast<float>(m_player.y)-m_player.height});
    playerTop.setRotation(sf::degrees(static_cast<float>(m_player.angle)));
    playerTop.setFillColor(sf::Color::Green);
   
      if (m_player.hit>0){
        playerTop.setFillColor(sf::Color(80*m_player.hit,255-(80*m_player.hit),2*m_player.invinsible_frames));
        }

    m_window->draw(playerTop);
*/
    // 2. Draw Thruster Flame
    if (m_inputState.upPressed) {
        sf::RectangleShape thrusterRect({3.5f, 8.0f});
        thrusterRect.setOrigin({2.5f, -static_cast<float>(m_player.height / 2.0)});
        thrusterRect.setPosition({static_cast<float>(m_player.x), static_cast<float>(m_player.y)});
        thrusterRect.setRotation(sf::degrees(static_cast<float>(m_player.angle)));
        thrusterRect.setFillColor(sf::Color::Yellow);
        m_window->draw(thrusterRect);
    }
    if (m_inputState.leftPressed) {
        sf::RectangleShape thrusterRect1({4.0f, 2.0f});
        thrusterRect1.setOrigin({-static_cast<float>(m_player.width / 2.0),8.5});
        thrusterRect1.setPosition({static_cast<float>(m_player.x), static_cast<float>(m_player.y)});
        thrusterRect1.setRotation(sf::degrees(static_cast<float>(m_player.angle)));
        thrusterRect1.setFillColor(sf::Color(250,250,250,190));
        m_window->draw(thrusterRect1);
        
        sf::RectangleShape thrusterRect2({4.0f, 2.0f});
        thrusterRect2.setOrigin({6.0f+static_cast<float>(m_player.width / 2.0),-2.0});
        thrusterRect2.setPosition({static_cast<float>(m_player.x), static_cast<float>(m_player.y)});
        thrusterRect2.setRotation(sf::degrees(static_cast<float>(m_player.angle)));
        thrusterRect2.setFillColor(sf::Color(250,250,250,190));
        m_window->draw(thrusterRect2);
        
    }
    if (m_inputState.rightPressed) {
        sf::RectangleShape thrusterRect1({4.0f, 2.0f});
        thrusterRect1.setOrigin({-static_cast<float>(m_player.width / 2.0),-2.0});
        thrusterRect1.setPosition({static_cast<float>(m_player.x), static_cast<float>(m_player.y)});
        thrusterRect1.setRotation(sf::degrees(static_cast<float>(m_player.angle)));
        thrusterRect1.setFillColor(sf::Color(250,250,250,190));
        m_window->draw(thrusterRect1);
        
        sf::RectangleShape thrusterRect2({4.0f, 2.0f});
        thrusterRect2.setOrigin({6.0f+static_cast<float>(m_player.width / 2.0),8.5});
        thrusterRect2.setPosition({static_cast<float>(m_player.x), static_cast<float>(m_player.y)});
        thrusterRect2.setRotation(sf::degrees(static_cast<float>(m_player.angle)));
        thrusterRect2.setFillColor(sf::Color(250,250,250,190));
        m_window->draw(thrusterRect2);
    }

    // 3. Draw All Asteroids
        sf::CircleShape astroCircle;
    astroCircle.setOutlineColor(sf::Color(160, 160, 160));
    astroCircle.setOutlineThickness(1.5f);

    for (size_t i = 0; i < m_asteroids.size(); ++i) {
        float r = static_cast<float>(m_asteroids[i].radius);
        astroCircle.setRadius(r);
        astroCircle.setOrigin({r, r});
        astroCircle.setPosition({static_cast<float>(m_asteroids[i].x), static_cast<float>(m_asteroids[i].y)});
        astroCircle.setFillColor(sf::Color(
            static_cast<std::uint8_t>(m_asteroids[i].r),
            static_cast<std::uint8_t>(m_asteroids[i].g),
            static_cast<std::uint8_t>(m_asteroids[i].b)
        ));

        m_window->draw(astroCircle);
    }
    //3.2 Draw bullets? 
    sf::RectangleShape bullet_sq({2.5f,2.5f});
    for (int i=0; i < m_bullets.size(); i++){
	bullet_sq.setPosition({static_cast<float>(m_bullets[i].x), static_cast<float>(m_bullets[i].y)});
	bullet_sq.setFillColor(sf::Color::White);
        m_window->draw(bullet_sq);
	}

    // ==========================================
    // 4. HEALTH STATUS BAR (TOP-LEFT HUD)
    // ==========================================
    if (m_state == GameState::PLAYING || m_state == GameState::GAME_OVER) {
        float maxBarWidth = 160.0f;
        float barHeight = 16.0f;
        float healthFraction = std::max(0.0f, (4.0f - static_cast<float>(m_player.hit)) / 4.0f);

        // Dark background frame
        sf::RectangleShape healthBg({maxBarWidth, barHeight});
        healthBg.setPosition({20.0f, 20.0f});
        healthBg.setFillColor(sf::Color(40, 40, 40));
        healthBg.setOutlineColor(sf::Color::White);
        healthBg.setOutlineThickness(2.0f);
        m_window->draw(healthBg);

        // Active green health fill
        sf::RectangleShape healthFill({maxBarWidth * healthFraction, barHeight});
        healthFill.setPosition({20.0f, 20.0f});
        healthFill.setFillColor(sf::Color::Green);
        m_window->draw(healthFill);
        
        int points2;
        points2=10*(num_pts/100);
        sf::Text Points(m_font, "Points = "+ std::to_string(points2), 32);
       // sf::Text Points(m_font, "Points = "+ std::to_string(m_bullets.size()), 32);
        Points.setFillColor(sf::Color::Green);
        Points.setPosition({20.0f,60.0f});
        m_window->draw(Points);

    }

    // ==========================================
    // 5. GAME OVER EXPLOSION & BANNER
    // ==========================================
    if (m_state == GameState::GAME_OVER) {

        // Draw 12 overlapping circles that pop in and out around the ship
        sf::CircleShape puff;
        if (explosion_frames>0){
	explosion_frames--;
        for (int k = 0; k < 6; ++k) {
            float radius = 10.0f + static_cast<float>(std::rand() % 35);
            float offsetX = static_cast<float>((std::rand() % 71) - 35);
            float offsetY = static_cast<float>((std::rand() % 71) - 35);

            puff.setRadius(radius);
            puff.setOrigin({radius, radius});
            puff.setPosition({
                static_cast<float>(m_player.x) + offsetX,
                static_cast<float>(m_player.y) + offsetY
            });
	    int c1=std::rand()%150;
	    int c2=std::rand()%50;
	    int c3=std::rand()%100;
            puff.setFillColor(sf::Color(150+c1,10+c2,50+c3,explosion_frames));
            m_window->draw(puff);
        }
	}
        // Game Over Title
        if (explosion_frames>0){explosion_frames--;}
        sf::Text gameOverText(m_font, "GAME OVER", 54);
        gameOverText.setFillColor(sf::Color::Red);
        sf::FloatRect goBounds = gameOverText.getLocalBounds();
        gameOverText.setOrigin({goBounds.position.x + goBounds.size.x / 2.0f, goBounds.position.y + goBounds.size.y / 2.0f});
        gameOverText.setPosition({m_screenWidth / 2.0f, m_screenHeight * 0.25f});
        m_window->draw(gameOverText);

        // Restart Prompt
        //sf::Text restartText(m_font, "PRESS SPACE TO RESTART", 20);
        //restartText.setFillColor(sf::Color::White);
        //sf::FloatRect reBounds = restartText.getLocalBounds();
        //restartText.setOrigin({reBounds.position.x + reBounds.size.x / 2.0f, reBounds.position.y + reBounds.size.y / 2.0f});
        //restartText.setPosition({m_screenWidth / 2.0f, m_screenHeight * 0.75f});
        //m_window->draw(restartText);
    }


    m_window->display();
}

