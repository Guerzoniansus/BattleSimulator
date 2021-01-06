#include "precomp.h" // include (only) this in every .cpp file

#define NUM_TANKS_BLUE 1279
#define NUM_TANKS_RED 1279

#define TANK_MAX_HEALTH 1000
#define ROCKET_HIT_VALUE 60
#define PARTICLE_BEAM_HIT_VALUE 50

#define TANK_MAX_SPEED 1.5

#define HEALTH_BARS_OFFSET_X 0
#define HEALTH_BAR_HEIGHT 70
#define HEALTH_BAR_WIDTH 1
#define HEALTH_BAR_SPACING 0

#define MAX_FRAMES 2000

//Global performance timer
#define REF_PERFORMANCE 73466 //UPDATE THIS WITH YOUR REFERENCE PERFORMANCE (see console after 2k frames)
static timer perf_timer;
static float duration;

//Load sprite files and initialize sprites
static Surface* background_img = new Surface("assets/Background_Grass.png");
static Surface* tank_red_img = new Surface("assets/Tank_Proj2.png");
static Surface* tank_blue_img = new Surface("assets/Tank_Blue_Proj2.png");
static Surface* rocket_red_img = new Surface("assets/Rocket_Proj2.png");
static Surface* rocket_blue_img = new Surface("assets/Rocket_Blue_Proj2.png");
static Surface* particle_beam_img = new Surface("assets/Particle_Beam.png");
static Surface* smoke_img = new Surface("assets/Smoke.png");
static Surface* explosion_img = new Surface("assets/Explosion.png");

static Sprite background(background_img, 1);
static Sprite tank_red(tank_red_img, 12);
static Sprite tank_blue(tank_blue_img, 12);
static Sprite rocket_red(rocket_red_img, 12);
static Sprite rocket_blue(rocket_blue_img, 12);
static Sprite smoke(smoke_img, 4);
static Sprite explosion(explosion_img, 9);
static Sprite particle_beam_sprite(particle_beam_img, 3);

const static vec2 tank_size(14, 18);
const static vec2 rocket_size(25, 24);

const static float tank_radius = 8.5f;
const static float rocket_radius = 10.f;

const static int amount_of_threads = thread::hardware_concurrency();
ThreadPool thread_pool(amount_of_threads);

// -----------------------------------------------------------
// Initialize the application
// -----------------------------------------------------------
void Game::init()
{
    frame_count_font = new Font("assets/digital_small.png", "ABCDEFGHIJKLMNOPQRSTUVWXYZ:?!=-0123456789.");

    tanks.reserve(NUM_TANKS_BLUE + NUM_TANKS_RED);

    uint rows = (uint)sqrt(NUM_TANKS_BLUE + NUM_TANKS_RED);
    uint max_rows = 12;

    float start_blue_x = tank_size.x + 10.0f;
    float start_blue_y = tank_size.y + 80.0f;

    float start_red_x = 980.0f;
    float start_red_y = 100.0f;

    float spacing = 15.0f;

    //Spawn blue tanks
    for (int i = 0; i < NUM_TANKS_BLUE; i++)
    {
        tanks.push_back(Tank(start_blue_x + ((i % max_rows) * spacing), start_blue_y + ((i / max_rows) * spacing), BLUE, &tank_blue, &smoke, 1200, 600, tank_radius, TANK_MAX_HEALTH, TANK_MAX_SPEED));
    }
    //Spawn red tanks
    for (int i = 0; i < NUM_TANKS_RED; i++)
    {
        tanks.push_back(Tank(start_red_x + ((i % max_rows) * spacing), start_red_y + ((i / max_rows) * spacing), RED, &tank_red, &smoke, 80, 80, tank_radius, TANK_MAX_HEALTH, TANK_MAX_SPEED));
    }

    particle_beams.push_back(Particle_beam(vec2(SCRWIDTH / 2, SCRHEIGHT / 2), vec2(100, 50), &particle_beam_sprite, PARTICLE_BEAM_HIT_VALUE));
    particle_beams.push_back(Particle_beam(vec2(80, 80), vec2(100, 50), &particle_beam_sprite, PARTICLE_BEAM_HIT_VALUE));
    particle_beams.push_back(Particle_beam(vec2(1200, 600), vec2(100, 50), &particle_beam_sprite, PARTICLE_BEAM_HIT_VALUE));
}

// -----------------------------------------------------------
// Close down application
// -----------------------------------------------------------
void Game::shutdown()
{
}

// -----------------------------------------------------------
// Iterates through all tanks and returns the closest enemy tank for the given tank
// -----------------------------------------------------------
Tank& Game::find_closest_enemy(Tank& current_tank)
{
    float closest_distance = numeric_limits<float>::infinity();
    int closest_index = 0;

    for (int i = 0; i < tanks.size(); i++)
    {
        if (tanks.at(i).allignment != current_tank.allignment && tanks.at(i).active)
        {
            float sqrDist = fabsf((tanks.at(i).get_position() - current_tank.get_position()).sqr_length());
            if (sqrDist < closest_distance)
            {
                closest_distance = sqrDist;
                closest_index = i;
            }
        }
    }

    return tanks.at(closest_index);
}

// -----------------------------------------------------------
// Update the game state:
// Move all objects
// Update sprite frames
// Collision detection
// Targeting etc..
// -----------------------------------------------------------
void Game::update(float deltaTime)
{
    //Update tanks
    for (Tank& tank : tanks)
    {
        if (tank.active)
        {
            //Check tank collision and nudge tanks away from each other
            for (Tank& oTank : tanks)
            {
                if (&tank == &oTank) continue;

                vec2 dir = tank.get_position() - oTank.get_position();
                float dirSquaredLen = dir.sqr_length();

                float colSquaredLen = (tank.get_collision_radius() + oTank.get_collision_radius());
                colSquaredLen *= colSquaredLen;

                if (dirSquaredLen < colSquaredLen)
                {
                    tank.push(dir.normalized(), 1.f);
                }
            }

            //Move tanks according to speed and nudges (see above) also reload
            tank.tick();

            //Shoot at closest target if reloaded
            if (tank.rocket_reloaded())
            {
                Tank& target = find_closest_enemy(tank);

                rockets.push_back(Rocket(tank.position, (target.get_position() - tank.position).normalized() * 3, rocket_radius, tank.allignment, ((tank.allignment == RED) ? &rocket_red : &rocket_blue)));

                tank.reload_rocket();
            }
        }
    }

    //Update smoke plumes
    for (Smoke& smoke : smokes)
    {
        smoke.tick();
    }

    //Update rockets
    for (Rocket& rocket : rockets)
    {
        rocket.tick();

        //Check if rocket collides with enemy tank, spawn explosion and if tank is destroyed spawn a smoke plume
        for (Tank& tank : tanks)
        {
            if (tank.active && (tank.allignment != rocket.allignment) && rocket.intersects(tank.position, tank.collision_radius))
            {
                explosions.push_back(Explosion(&explosion, tank.position));

                if (tank.hit(ROCKET_HIT_VALUE))
                {
                    smokes.push_back(Smoke(smoke, tank.position - vec2(0, 48)));
                }

                rocket.active = false;
                break;
            }
        }
    }

    //Remove exploded rockets with remove erase idiom
    rockets.erase(std::remove_if(rockets.begin(), rockets.end(), [](const Rocket& rocket) { return !rocket.active; }), rockets.end());

    //Update particle beams
    for (Particle_beam& particle_beam : particle_beams)
    {
        particle_beam.tick(tanks);

        //Damage all tanks within the damage window of the beam (the window is an axis-aligned bounding box)
        for (Tank& tank : tanks)
        {
            if (tank.active && particle_beam.rectangle.intersects_circle(tank.get_position(), tank.get_collision_radius()))
            {
                if (tank.hit(particle_beam.damage))
                {
                    smokes.push_back(Smoke(smoke, tank.position - vec2(0, 48)));
                }
            }
        }
    }

    //Update explosion sprites and remove when done with remove erase idiom
    for (Explosion& explosion : explosions)
    {
        explosion.tick();
    }

    explosions.erase(std::remove_if(explosions.begin(), explosions.end(), [](const Explosion& explosion) { return explosion.done(); }), explosions.end());
}

void Game::draw()
{
    // clear the graphics window
    screen->clear(0);

    //Draw background
    background.draw(screen, 0, 0);

    //Draw sprites
    for (int i = 0; i < NUM_TANKS_BLUE + NUM_TANKS_RED; i++)
    {
        tanks.at(i).draw(screen);

        vec2 tPos = tanks.at(i).get_position();
        // tread marks
        if ((tPos.x >= 0) && (tPos.x < SCRWIDTH) && (tPos.y >= 0) && (tPos.y < SCRHEIGHT))
            background.get_buffer()[(int)tPos.x + (int)tPos.y * SCRWIDTH] = sub_blend(background.get_buffer()[(int)tPos.x + (int)tPos.y * SCRWIDTH], 0x808080);
    }

    for (Rocket& rocket : rockets)
    {
        rocket.draw(screen);
    }

    for (Smoke& smoke : smokes)
    {
        smoke.draw(screen);
    }

    for (Particle_beam& particle_beam : particle_beams)
    {
        particle_beam.draw(screen);
    }

    for (Explosion& explosion : explosions)
    {
        explosion.draw(screen);
    }

    //Draw sorted health bars
    for (int t = 0; t < 2; t++)
    {
        const int NUM_TANKS = ((t < 1) ? NUM_TANKS_BLUE : NUM_TANKS_RED);

        const int begin = ((t < 1) ? 0 : NUM_TANKS_BLUE);

        std::vector<const Tank*> sorted_tanks(NUM_TANKS);

        if (t < 1) {
            for (int i = 0; i < NUM_TANKS; i++)
            {
                sorted_tanks.at(i) = &tanks.at(i);
            }
            std::atomic<int> threads(amount_of_threads);
            merge_sort(sorted_tanks, begin, begin + NUM_TANKS - 1, threads);
        }
        else {
            for (int i = 0; i < NUM_TANKS; i++)
            {
                int save_value = i + begin;
                sorted_tanks.at(i) = &tanks.at(save_value);
            }
            std::atomic<int> threads = amount_of_threads;
            merge_sort(sorted_tanks, begin - NUM_TANKS, begin - 1, threads);
        }
        for (int i = 0; i < NUM_TANKS; i++)
        {
            int health_bar_start_x = i * (HEALTH_BAR_WIDTH + HEALTH_BAR_SPACING) + HEALTH_BARS_OFFSET_X;
            int health_bar_start_y = (t < 1) ? 0 : (SCRHEIGHT - HEALTH_BAR_HEIGHT) - 1;
            int health_bar_end_x = health_bar_start_x + HEALTH_BAR_WIDTH;
            int health_bar_end_y = (t < 1) ? HEALTH_BAR_HEIGHT : SCRHEIGHT - 1;

            screen->bar(health_bar_start_x, health_bar_start_y, health_bar_end_x, health_bar_end_y, REDMASK);
            screen->bar(health_bar_start_x, health_bar_start_y + (int)((double)HEALTH_BAR_HEIGHT * (1 - ((double)sorted_tanks.at(i)->health / (double)TANK_MAX_HEALTH))), health_bar_end_x, health_bar_end_y, GREENMASK);
        }
    }
}

// merges two subvectors of vector.
void Tmpl8::Game::merge(std::vector<const Tank*>& sorted_tanks, int start, int middle, int end) {

    int save_value = ((middle - start) + 1);
    int save_value2 = (end - middle);
    std::vector<const Tank*> left_vector(save_value);
    std::vector<const Tank*> right_vector(save_value2);

    // fill in left vector
    for (int i = 0; i < left_vector.size(); ++i) {
        save_value = start + i;
        left_vector[i] = sorted_tanks[save_value];
    }
    // fill in right vector
    for (int i = 0; i < right_vector.size(); ++i) {
        save_value = (middle + 1 + i);
        right_vector[i] = sorted_tanks[save_value];
    }

    /* Merge the temp vectors */

    // initial indexes of first and second subvector
    int left_index = 0, right_index = 0;

    // the index we will start at when adding the subvector back into the main vector
    int current_index = start;

    // compare each index of the subvector adding the lowest value to the currentIndex
    while (left_index < left_vector.size() && right_index < right_vector.size()) {
        if (left_vector[left_index]->health <= right_vector[right_index]->health) {
            sorted_tanks[current_index] = left_vector[left_index];
            left_index++;
        }
        else {
            sorted_tanks[current_index] = right_vector[right_index];
            right_index++;
        }
        current_index++;
    }

    // copy remaining elements of left_vector if any
    while (left_index < left_vector.size()) sorted_tanks[current_index++] = left_vector[left_index++];

    // copy remaining elements of right_vector if any
    while (right_index < right_vector.size()) sorted_tanks[current_index++] = right_vector[right_index++];
}

// main function that sorts array[start..end] using merge()
void Tmpl8::Game::merge_sort(std::vector<const Tank*>& sorted_tanks, int start, int end, std::atomic<int>& threads) {

    if (start < end) {
        // find the middle point
        int middle = (start + end) / 2;

        if (threads >= 1) {
            threads --;

            future<void> future_left = thread_pool.enqueue([&] {
                merge_sort(sorted_tanks, start, middle, threads);
                });

            merge_sort(sorted_tanks, middle + 1, end, threads);

            future_left.wait();

            threads++;

            merge(sorted_tanks, start, middle, end);
        }

        else {
            merge_sort(sorted_tanks, start, middle, threads);
            merge_sort(sorted_tanks, middle + 1, end, threads);

            merge(sorted_tanks, start, middle, end);
        }
    }

}

// -----------------------------------------------------------
// When we reach MAX_FRAMES print the duration and speedup multiplier
// Updating REF_PERFORMANCE at the top of this file with the value
// on your machine gives you an idea of the speedup your optimizations give
// -----------------------------------------------------------
void Tmpl8::Game::measure_performance()
{
    char buffer[128];
    if (frame_count >= MAX_FRAMES)
    {
        if (!lock_update)
        {
            duration = perf_timer.elapsed();
            cout << "Duration was: " << duration << " (Replace REF_PERFORMANCE with this value)" << endl;
            lock_update = true;
        }

        frame_count--;
    }

    if (lock_update)
    {
        screen->bar(420, 170, 870, 430, 0x030000);
        int ms = (int)duration % 1000, sec = ((int)duration / 1000) % 60, min = ((int)duration / 60000);
        sprintf(buffer, "%02i:%02i:%03i", min, sec, ms);
        frame_count_font->centre(screen, buffer, 200);
        sprintf(buffer, "SPEEDUP: %4.1f", REF_PERFORMANCE / duration);
        frame_count_font->centre(screen, buffer, 340);
    }
}

// -----------------------------------------------------------
// Main application tick function
// -----------------------------------------------------------
void Game::tick(float deltaTime)
{
    if (!lock_update)
    {
        update(deltaTime);
    }
    draw();

    measure_performance();

    // print something in the graphics window
    //screen->Print("hello world", 2, 2, 0xffffff);

    // print something to the text window
    //cout << "This goes to the console window." << std::endl;

    //Print frame count
    frame_count++;
    string frame_count_string = "FRAME: " + std::to_string(frame_count);
    frame_count_font->print(screen, frame_count_string.c_str(), 350, 580);
}