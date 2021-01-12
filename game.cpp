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

// Function declarations
vector<Tank*> get_tank_collision_candidates(float x, float y);
void remove_tank_from_grid(Tank& tank);
void add_tank_to_grid(Tank& tank);
vec2 get_tank_grid_coordinate(float x, float y);
bool is_outside_of_screen(float x, float y);

//Global performance timer
#define REF_PERFORMANCE 20254.3 //UPDATE THIS WITH YOUR REFERENCE PERFORMANCE (see console after 2k frames)
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

const static int grid_col_width = 14; // Tank width, using tank_size.x would give an error on the grid array so had to use ugly number
const static int grid_col_height = 18; // Tank height, using tank_size.y would give an error on the grid array so had to use ugly number
const static int grid_col_amount = (SCRWIDTH / grid_col_width) + 1;
const static int grid_row_amount = (SCRHEIGHT / grid_col_height) + 1;

// Grid of tanks used for collisions
vector<Tank*> grid[grid_col_amount][grid_row_amount];

std::mutex myMutex;
const static int amount_of_threads = thread::hardware_concurrency();
ThreadPool thread_pool(amount_of_threads);

// -----------------------------------------------------------
// Initialize the application
// -----------------------------------------------------------
void Game::init()
{   
    frame_count_font = new Font("assets/digital_small.png", "ABCDEFGHIJKLMNOPQRSTUVWXYZ:?!=-0123456789.");

    tanks.reserve(NUM_TANKS_BLUE + NUM_TANKS_RED);
    alive_red_tanks.reserve(NUM_TANKS_RED);
    alive_blue_tanks.reserve(NUM_TANKS_BLUE);

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
        Tank tank(start_blue_x + ((i % max_rows) * spacing), start_blue_y + ((i / max_rows) * spacing), BLUE, &tank_blue, &smoke, 1200, 600, tank_radius, TANK_MAX_HEALTH, TANK_MAX_SPEED);
        tanks.push_back(tank);
        alive_blue_tanks.push_back(&tanks.at(i));
        add_tank_to_grid(tank);
    }
    //Spawn red tanks
    for (int i = 0; i < NUM_TANKS_RED; i++)
    {
        Tank tank(start_red_x + ((i % max_rows) * spacing), start_red_y + ((i / max_rows) * spacing), RED, &tank_red, &smoke, 80, 80, tank_radius, TANK_MAX_HEALTH, TANK_MAX_SPEED);
        tanks.push_back(tank);
        alive_red_tanks.push_back(&tanks.at(i + NUM_TANKS_BLUE));
        add_tank_to_grid(tank);
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
    const allignments enemy_alignment = current_tank.allignment == BLUE ? RED : BLUE;
    vector<Tank*>& enemies = enemy_alignment == BLUE ? alive_blue_tanks : alive_red_tanks;

    float closest_distance = numeric_limits<float>::infinity();
    int closest_index = 0;

    // Margin outside of screen
    int margin = 50;
    
    for (int i = 0; i < enemies.size(); i++)
    {
        Tank& enemy = *enemies.at(i);

        // Don't scan tanks outside of the screen which has huge performance impact 
        if (enemy.get_position().x < 0 - margin || enemy.get_position().x > SCRWIDTH + margin
            || enemy.get_position().y < 0 - margin || enemy.get_position().y > SCRHEIGHT + margin)
        {
            continue;
        }

        float sqr_dist = fabsf((enemy.get_position() - current_tank.get_position()).sqr_length());
        if (sqr_dist < closest_distance)
        {
            closest_distance = sqr_dist;
            closest_index = i;
        }
    }

    return *enemies.at(closest_index);
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
    update_tanks();

    //Update smoke plumes
    for (Smoke& smoke : smokes)
    {
        smoke.tick();
    }

    //Update rockets
    for (Rocket& rocket : rockets)
    {
        rocket.tick();

        vector<Tank*>& tanks_to_loop_through = rocket.allignment == BLUE ? alive_red_tanks : alive_blue_tanks;

        //Check if rocket collides with enemy tank, spawn explosion and if tank is destroyed spawn a smoke plume
        for (int i = 0; i < tanks_to_loop_through.size(); i++)
        {
            Tank& tank = *tanks_to_loop_through.at(i);

            if (rocket.intersects(tank.position, tank.collision_radius))
            {
                explosions.push_back(Explosion(&explosion, tank.position));

                if (tank.hit(ROCKET_HIT_VALUE))
                {
                    smokes.push_back(Smoke(smoke, tank.position - vec2(0, 48)));
                    delete_dead_tank(tank.allignment, i);
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
        for (int side = 0; side < 2; side++)
        {
            vector<Tank*>& tanks_to_loop_through = side == 0 ? alive_blue_tanks : alive_red_tanks;

            for (int i = 0; i < tanks_to_loop_through.size(); i++) 
            {
                Tank& tank = *tanks_to_loop_through.at(i);

                if (particle_beam.rectangle.intersects_circle(tank.get_position(), tank.get_collision_radius()))
                {
                    if (tank.hit(particle_beam.damage))
                    {
                        smokes.push_back(Smoke(smoke, tank.position - vec2(0, 48)));
                        delete_dead_tank(tank.allignment, i);
                    }
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

void Game::update_tanks() {
    
    int upper_limit = tanks.size();
    int block_size = upper_limit / amount_of_threads;
    int start = 0;
    int end = start + block_size;
    int remaining = upper_limit % amount_of_threads;
    int current_remaining = remaining;

    vector<future<void>*> futures;

    // ========================================= Tank collisions 

    for (int i = 0; i < amount_of_threads; i++)
    {
        // One extra loop for first N amount of threads
        if (current_remaining > 0) {
            end++;
            current_remaining--;
        }

        future<void> fut = thread_pool.enqueue([&, start, end]  
            {

            //Check tank collision and nudge tanks away from each other
            for (int i = start; i < end; i++)
            {
                vector<Tank*>& tanks_to_loop_through = i < alive_blue_tanks.size() ? alive_blue_tanks : alive_red_tanks;
                int offset = i < alive_blue_tanks.size() ? 0 : alive_blue_tanks.size();

                Tank& tank = *tanks_to_loop_through.at(i - offset);
                
                std::unique_lock<std::mutex> lock(myMutex);
                vector<Tank*> collision_candidates = get_tank_collision_candidates(tank.get_position().x, tank.get_position().y);
                lock.unlock();

                //Check tank collision and nudge tanks away from each other
                for (int i = 0; i < collision_candidates.size(); i++)
                {
                    Tank* o_tank = collision_candidates.at(i);

                    if (&tank == o_tank) continue;

                    vec2 dir = tank.get_position() - (*o_tank).get_position();

                    float dir_squared_len = dir.sqr_length();

                    float col_squared_len = (tank.get_collision_radius() + (*o_tank).get_collision_radius());

                    col_squared_len *= col_squared_len;

                    if (dir_squared_len < col_squared_len)
                    {
                        tank.push(dir.normalized(), 1.f);
                    }
                }
            }   

        }); 

        start = end;

        end += block_size;

    }

    for (future<void>* fut : futures)
    {
        (*fut).wait();
    }

    futures.clear();

    start = 0;
    end = start + block_size;
    current_remaining = remaining;

    // ========================================= Tank movement 

    for (int i = 0; i < amount_of_threads; i++)
    {
        // One extra loop for first N amount of threads
        if (current_remaining > 0) 
        {
            end++;
            current_remaining--;
        }

        future<void> fut = thread_pool.enqueue([&, start, end] 
            {

            for (int i = start; i < end; i++)
            {
                vector<Tank*>& tanks_to_loop_through = i < alive_blue_tanks.size() ? alive_blue_tanks : alive_red_tanks;
                int offset = i < alive_blue_tanks.size() ? 0 : alive_blue_tanks.size();

                Tank& tank = *tanks_to_loop_through.at(i - offset);

                std::unique_lock<std::mutex> lock(myMutex);

                // Update grid position and move tank

                remove_tank_from_grid(tank);

                lock.unlock();

                tank.tick();

                lock.lock();

                add_tank_to_grid(tank);
            }

            });

        start = end;

        end += block_size;

    }

    for (future<void>* fut : futures)
    {
        (*fut).wait();
    }


    futures.clear();

    start = 0;
    end = start + block_size;
    current_remaining = remaining;

    // ========================================= Shoot rockets 

    for (int i = 0; i < amount_of_threads; i++)
    {
        // One extra loop for first N amount of threads
        if (current_remaining > 0) {
            end++;
            current_remaining--;
        }

        future<void> fut = thread_pool.enqueue([&, start, end] {

            for (int i = start; i < end; i++)
            {
                vector<Tank*>& tanks_to_loop_through = i < alive_blue_tanks.size() ? alive_blue_tanks : alive_red_tanks;
                int offset = i < alive_blue_tanks.size() ? 0 : alive_blue_tanks.size();

                Tank& tank = *tanks_to_loop_through.at(i - offset);

                //Shoot at closest target if reloaded
                if (tank.rocket_reloaded())
                {
                    std::unique_lock<std::mutex> lock(myMutex);

                    Tank& target = find_closest_enemy(tank);

                    rockets.push_back(Rocket(tank.position, (target.get_position() - tank.position).normalized() * 3, rocket_radius, tank.allignment, ((tank.allignment == RED) ? &rocket_red : &rocket_blue)));

                    lock.unlock();

                    tank.reload_rocket();
                }
            }

            });

        start = end;

        end += block_size;

    }

    for (future<void>* fut : futures)
    {
        (*fut).wait();
    }

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

        vec2 tank_pos = tanks.at(i).get_position();
        // tread marks
        if ((tank_pos.x >= 0) && (tank_pos.x < SCRWIDTH) && (tank_pos.y >= 0) && (tank_pos.y < SCRHEIGHT))
            background.get_buffer()[(int)tank_pos.x + (int)tank_pos.y * SCRWIDTH] = sub_blend(background.get_buffer()[(int)tank_pos.x + (int)tank_pos.y * SCRWIDTH], 0x808080);
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
        const int NUM_TANKS = ((t < 1) ? alive_blue_tanks.size() : alive_red_tanks.size());

        const int begin = ((t < 1) ?  0 :  0);

        std::vector<Tank*> sorted_tanks;

        if (t < 1) 
        {
         sorted_tanks = alive_blue_tanks;
         std::atomic<int> threads(amount_of_threads);
         merge_sort(sorted_tanks, begin, begin + NUM_TANKS - 1, threads);
        }

        else 
        {         
         sorted_tanks = alive_red_tanks;
         std::atomic<int> threads = amount_of_threads;
         merge_sort(sorted_tanks, begin, begin + NUM_TANKS - 1, threads);          
        }

        int health_bar_start_x = 0;
        int health_bar_start_y = (t < 1) ? 0 : (SCRHEIGHT - HEALTH_BAR_HEIGHT) - 1;
        int health_bar_end_x = (t < 1) ? health_bar_start_x + (NUM_TANKS_BLUE - alive_blue_tanks.size()) : health_bar_start_x + (NUM_TANKS_RED - alive_red_tanks.size());
        int health_bar_end_y = (t < 1) ? HEALTH_BAR_HEIGHT : SCRHEIGHT - 1;

        int sorted_health_bars_start_x = health_bar_end_x;

        screen->bar(health_bar_start_x, health_bar_start_y, health_bar_end_x, health_bar_end_y, REDMASK);

        for (int i = 0; i < NUM_TANKS; i++)
        {
            health_bar_start_x = sorted_health_bars_start_x + (i * (HEALTH_BAR_WIDTH + HEALTH_BAR_SPACING) + HEALTH_BARS_OFFSET_X);
            health_bar_start_y = (t < 1) ? 0 : (SCRHEIGHT - HEALTH_BAR_HEIGHT) - 1;
            health_bar_end_x = health_bar_start_x + HEALTH_BAR_WIDTH;
            health_bar_end_y = (t < 1) ? HEALTH_BAR_HEIGHT : SCRHEIGHT - 1;
            
            screen->bar(health_bar_start_x, health_bar_start_y, health_bar_end_x, health_bar_end_y, REDMASK);
            screen->bar(health_bar_start_x, health_bar_start_y + (int)((double)HEALTH_BAR_HEIGHT * (1 - ((double)sorted_tanks.at(i)->health / (double)TANK_MAX_HEALTH))), health_bar_end_x, health_bar_end_y, GREENMASK);
        }
    }
}

// merges two subvectors of vector.
void Tmpl8::Game::merge(std::vector<Tank*>& sorted_tanks, int start, int middle, int end) {

    int save_value = ((middle - start) + 1);
    int save_value2 = (end - middle);
    std::vector<Tank*> left_vector(save_value);
    std::vector<Tank*> right_vector(save_value2);

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
void Tmpl8::Game::merge_sort(std::vector<Tank*>& sorted_tanks, int start, int end, std::atomic<int>& threads) {

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

// -----------------------------------------------------------
// Get tanks in the 8 grid squares around a
// specific coordinate along with the square itself, 
// which is enough for tank and rocket collision range. 
// -----------------------------------------------------------
vector<Tank*> get_tank_collision_candidates(float x, float y) 
{
    vector<Tank*> candidate_tanks;

    // Dont count tanks that are outside the screen
    if (is_outside_of_screen(x, y))
        return candidate_tanks;

    vec2 coord = get_tank_grid_coordinate(x, y);
    int col = coord.x;
    int row = coord.y;

    for (int offset_x = -1, offset_y = -1, i = 0; i < 9; i++, offset_x++) 
    {
        // Offset counts from -1 to 1 (top left to top right, mid left to mid right, bottom left to bottom right)
        if (offset_x == 2) 
        {
            offset_x = -1;
            offset_y++;
        }

        // Prevent index out of bounds errors
        if (col + offset_x < 0 || row + offset_y < 0
            || col + offset_x >= grid_col_amount
            || row + offset_y >= grid_row_amount)
        {
            continue;
        }

        for (Tank* tank : grid[col + offset_x][row + offset_y])
        {
            candidate_tanks.push_back(tank);
        }
        
    }

    return candidate_tanks;
}


// -----------------------------------------------------------
// Remove a tank from the grid 
// -----------------------------------------------------------
void remove_tank_from_grid(Tank& tank) 
{
    // Dont count tanks that are outside the screen 
    if (is_outside_of_screen(tank.get_position().x, tank.get_position().y))
        return;

    vec2 coord = get_tank_grid_coordinate(tank.get_position().x, tank.get_position().y);
    int col = coord.x;
    int row = coord.y;

    vector<Tank*>& v = grid[col][row];

    v.erase(std::remove(v.begin(), v.end(), &tank), v.end());
}


// -----------------------------------------------------------
// Add a tank to the grid 
// -----------------------------------------------------------
void add_tank_to_grid(Tank& tank) 
{
    // Dont count tanks that are outside the screen
    if (is_outside_of_screen(tank.get_position().x, tank.get_position().y))
        return;

    vec2 coord = get_tank_grid_coordinate(tank.get_position().x, tank.get_position().y);
    int col = coord.x;
    int row = coord.y;

    grid[col][row].push_back(&tank);
}

// -----------------------------------------------------------
// Returns a vec2 with the col (x) and row (y) of this tank in the grid
// Warning: it can return out of bounds values if the tank is outside the screen
// -----------------------------------------------------------
vec2 get_tank_grid_coordinate(float x, float y) 
{
    int col = x / grid_col_width;
    int row = y / grid_col_height;

    vec2 vector(col, row);
    return vector;
}

// -----------------------------------------------------------
// Check if a coordinate is outside the screen 
// -----------------------------------------------------------
bool is_outside_of_screen(float x, float y)
{
    return x < 0 || y < 0 || x > SCRWIDTH || y > SCRHEIGHT;
}

// -----------------------------------------------------------
// Remove a dead tank from the list of alive tanks 
// -----------------------------------------------------------
void Tmpl8::Game::delete_dead_tank(allignments alignment, int index) {
    alignment == BLUE ? alive_blue_tanks.erase(alive_blue_tanks.begin() + index) : alive_red_tanks.erase(alive_red_tanks.begin() + index);
}