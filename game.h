#pragma once

namespace Tmpl8
{
//forward declarations
class Tank;
class Rocket;
class Smoke;
class Particle_beam;

class Game
{
  public:
    void set_target(Surface* surface) { screen = surface; }
    void init();
    void shutdown();
    void update(float deltaTime);
    void update_tanks();
    void update_rockets();
    void update_smokes();
    void draw();
    void tick(float deltaTime);
    void draw_tanks();
    void draw_rockets();
    void draw_smokes();
    void draw_explosions();

    vector<Tank*> get_tank_collision_candidates(float x, float y);
    void remove_tank_from_grid(Tank& tank);
    void add_tank_to_grid(Tank& tank);
    vec2 get_tank_grid_coordinate(float x, float y);
    bool is_outside_of_screen(float x, float y);
    bool is_negatively_outside_of_screen(float x, float y);
    
    void measure_performance();

    Tank& find_closest_enemy(Tank& current_tank);

    void mouse_up(int button)
    { /* implement if you want to detect mouse button presses */
    }

    void mouse_down(int button)
    { /* implement if you want to detect mouse button presses */
    }

    void mouse_move(int x, int y)
    { /* implement if you want to detect mouse movement */
    }

    void key_up(int key)
    { /* implement if you want to handle keys */
    }

    void key_down(int key)
    { /* implement if you want to handle keys */
    }

    void Tmpl8::Game::delete_dead_tank(allignments alignment, int index);
    void Tmpl8::Game::delete_dead_tank(Tank* tank);

  private:
    Surface* screen;

    vector<Tank> tanks;
    vector<Tank*> alive_blue_tanks;
    vector<Tank*> alive_red_tanks;
    vector<Rocket> rockets;
    vector<Smoke> smokes;
    vector<Explosion> explosions;
    vector<Particle_beam> particle_beams;

    Font* frame_count_font;
    long long frame_count = 0;

    bool lock_update = false;
};

}; // namespace Tmpl8