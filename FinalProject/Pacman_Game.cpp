/*
Author: Zechuan Ding
Class: ECE4122
Last Date Modified: November 29, 2020
Description:
Pac-Man Game
*/
//#include <GL/glew.h> // Include the GLEW header file
#define _CRT_SECURE_NO_WARNINGS
#include <GL/glut.h> // Include the GLUT header file
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <list>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include <ctime>
#define PI 3.14159265358
#define ESC 27
using namespace std;

enum Color {RED, CYAN, ORANGE, PINK};       // Four possible colors of ghosts
enum Direction {UP, DOWN, LEFT, RIGHT};     // Four possible directions of ghosts
enum Mode {CHASE, SCATTER, WAIT, FRIGHTENED, START};        // Five possible modes of ghosts
enum Game_Mode {READY, RUN, WIN, LOSE};

// Camera position
const float radius = 25;            // Distance of camera from origin
const float height = 25;            // Height of camera
const int step = 16;                // Number of steps between grid points
int theta = 270;                                // Angle relative to x-axis
float x = radius * cos(theta * PI / 180);       // x coordinate of camera
float y = radius * sin(theta * PI / 180);       // y coordinate of camera

volatile Game_Mode game_mode = READY;

const float wall_radius = 0.25f;     // Radius of wall
const float coin_radius = 0.15f;     // Radius of coin
GLUquadricObj* quadratic(nullptr);
void display_text();                 // Function to display texts

mutex waitlist_mutex;

#ifdef _WIN32
static DWORD powerup_start_time;
#else
static struct timeval powerup_start_time;
#endif


// Map of pac-man game maze
// 1 represents wall, 0 represents coin
// 2 represents spaces without coin, 3 represents power ups
short map[22][19] =
{
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1},
    {1,3,1,1,0,1,1,1,0,1,0,1,1,1,0,1,1,3,1},
    {1,0,1,1,0,1,1,1,0,1,0,1,1,1,0,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,0,1,0,1,1,1,1,1,0,1,0,1,1,0,1},
    {1,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,1},
    {1,1,1,1,0,1,1,1,2,1,2,1,1,1,0,1,1,1,1},
    {2,2,2,1,0,1,2,2,2,2,2,2,2,1,0,1,2,2,2},
    {1,1,1,1,0,1,2,1,1,1,1,1,2,1,0,1,1,1,1},
    {2,2,2,2,0,2,2,1,2,2,2,1,2,2,0,2,2,2,2},
    {1,1,1,1,0,1,2,1,1,1,1,1,2,1,0,1,1,1,1},
    {2,2,2,1,0,1,2,2,2,2,2,2,2,1,0,1,2,2,2},
    {1,1,1,1,0,1,2,1,1,1,1,1,2,1,0,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,0,1,1,1,0,1,0,1,1,1,0,1,1,0,1},
    {1,3,0,1,0,0,0,0,0,2,0,0,0,0,0,1,0,3,1},
    {1,1,0,1,0,1,0,1,1,1,1,1,0,1,0,1,0,1,1},
    {1,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,1},
    {1,0,1,1,1,1,1,1,0,1,0,1,1,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

class ECE_Ghost;

class ECE_Pacman {
public:
    friend class ECE_Ghost;
    friend void display_text();

    ECE_Pacman(int _step, short(*_p)[22][19]) {
        // Intialize pacman object
        p = _p;
        step = _step;
        reset_postion();
        vx = 1;
        vy = 0;
        score = 0;
    }
    void reset_postion() {
        // Reset pacman's position
        mapx = 9;
        mapy = 16;
        xpos = 9 * step;
        ypos = 16 * step;
    }

    void draw() {
        // Member function to draw pac-man
        glColor3f(1.0, 1.0, 0.0);
        glPushMatrix();
        glTranslatef((float)xpos / step - 9, 10 - (float)ypos / step, 2);
        glTranslatef(0, 0, body_radius / 2);
        glutSolidSphere(body_radius, 20, 20);
        glPopMatrix();
    }

    void set_next(int _x, int _y) {
        next_x = _x;
        next_y = _y;
    }

    void update() {
        if (powered_up) {
            float powerup_time;
            #ifdef _WIN32
                powerup_time = (float)(GetTickCount() - powerup_start_time) / 1000.0;
            #else
                // Figure out time elapsed since last call to idle function
                struct timeval time_now;
                gettimeofday(&time_now, NULL);
                powerup_time = (float)(time_now.tv_sec - powerup_start_time.tv_sec) +
                1.0e-6 * (time_now.tv_usec - powerup_start_time.tv_usec);
            #endif
            if (powerup_time >= 5) {
                // Lose power after 5 seconds
                powered_up = false;
            }
        }
        if (pos_check()) {
            xpos += (int)(speed * vx);
            ypos += (int)(speed * vy);
        }
    }

    bool pos_check() {
        // Return a bool value indicating whether pacman can move

        if (xpos % step == 0 && ypos % step == 0) {
            // If pacman on map grid points
            mapx = xpos / step;
            mapy = ypos / step;
            //printf("on grid: %d, %d\n", mapx, mapy);

            if ((*p)[mapy][mapx] == 0) {
                // ======== if that tile is a coin  ======== 
                item_count--;
                (*p)[mapy][mapx] = 2;//eat it
                score += 10;
            }

            if ((*p)[mapy][mapx] == 3) {
                // ======== if that tile is a power up ======== 
                item_count--;
                #ifdef _WIN32
                    powerup_start_time = GetTickCount();                  
                #else
                    gettimeofday(&powerup_start_time, NULL);
                #endif
                powered_up = true;
                (*p)[mapy][mapx] = 2;//eat it
                score += 50;

            }

            if (item_count <= 0) {
                game_mode = WIN;
                return false;
            }

            // Teleport through side channels
            if (mapx == 0 && vx == -1) {
                xpos = 18 * step;
                return true;
            }
            else if (mapx == 18 && vx == 1) {
                xpos = 0;
                return true;
            }

            if ((*p)[mapy + next_y][mapx + next_x] == 1) {
                // If pacman cannot turn
                if ((*p)[mapy + vy][mapx + vx] == 1) {
                    // If wall ahead
                    return false;
                }
                else {
                    // If no wall ahead
                    return true;
                }
            }
            else {
                // If pacman can turn, change current direction to next direction
                vx = next_x;
                vy = next_y;
                return true;
            }

        }
        else {
            if (vy + next_y == 0 && vx + next_x == 0) {
                // If next direction is opposite to current direction, reverse current direction
                vx = next_x;
                vy = next_y;
                return true;
            }
        }
        return true;
    }

    
private:
    int xpos, ypos;
    int vx, vy;
    int next_x, next_y;
    int score;
    const float body_radius = 0.6f;
    bool powered_up = false;
    int life = 3;
    int item_count = 156;
    short(*p)[22][19]; // Pointer to map
    int mapx, mapy; // relative position in the map
    int speed = 2;
    int step; // Number of steps between grid points

};

list<ECE_Ghost*> waitlist;
void init_waitlist();

class ECE_Ghost {
public:
    friend class ECE_Pacman;
    friend void init_waitlist();
    friend void display_text();

    ECE_Ghost(Color _color, int _step, short(*_p)[22][19], ECE_Pacman* _pacman) {
        // Initialize ghost's color, step length, map pointer, and collide distance
        color = _color;
        step = _step;
        p = _p;
        pacman = _pacman;
        reset_position();
        collide_dist = pow((body_radius + pacman->body_radius) * step, 2);
    }
    void reset_position() {
        // Reset (only one) ghost's position, speed, and mode
        mode = WAIT;
        vx = vy = 0;
        switch (color) {
        case RED:
            xposf = xpos = 9 * step;
            yposf = ypos = 9 * step;
            break;
        case CYAN:
            xposf = xpos = 8 * step;
            yposf = ypos = 10 * step;
            break;
        case PINK:
            xposf = xpos = 9 * step;
            yposf = ypos = 10 * step;
            break;
        case ORANGE:
            xposf = xpos = 10 * step;
            yposf = ypos = 10 * step;
            break;
        }

    }
    void draw() {
        // Member function to draw ghost
        if (mode == FRIGHTENED)
            glColor3f(1.0f, 1.0f, 1.0f);
        else {
            if (color == RED)
                glColor3f(1.0f, 0.0f, 0.0f);
            else if (color == CYAN)
                glColor3f(0.18f, 0.96f, 0.81f);
            else if (color == ORANGE)
                glColor3f(1.0f, 0.6f, 0.13f);
            else if (color == PINK)
                glColor3f(1.0f, 0.75f, 0.9f);
        }
        glPushMatrix();
        glTranslatef((float)xpos / step - 9, 10 - (float)ypos / step, 2);
        gluCylinder(quadratic, body_radius, body_radius, body_height, 25, 25);
        glTranslatef(0, 0, body_height);
        glutSolidSphere(body_radius, 20, 20);
        glPopMatrix();
        
        /* Draw target position
        glPushMatrix();
        glTranslatef(tarx - 9, 10 - tary, 2);
        gluCylinder(quadratic, 0.1, 0.1, 1, 10, 10);
        glPopMatrix(); */
    }

    void update(float dt) {

        // Wait until wait time is reached
        if (mode == WAIT) {
            #ifdef _WIN32
            elapsed_time = (float)(GetTickCount() - wait_start) / 1000.0;
            #else
                gettimeofday(&current_time, NULL);
                elapsed_time = (float)(current_time.tv_sec - wait_start.tv_sec) +
                    1.0e-6 * (current_time.tv_usec - wait_start.tv_usec);
            #endif
            if (elapsed_time > wait_time) {
                //printf("%f, %f\n", elapsed_time, wait_time);
                waitlist_mutex.lock();
                if (!waitlist.empty())
                    waitlist.pop_front();
                waitlist_mutex.unlock();
                //printf("SET OFF!\n");
                mode = START;
            }
        }
        
        // Animation to move pacman to starting point
        if (mode == START) {
            if (yposf == 10 * step) {
                // Converging to (9, 10)
                if (xposf == 8 * step)
                    vx = 1;
                else if (xposf == 10 * step)
                    vx = -1;
            }

            if (abs(xposf - 9 * step) <= 3) {
                // Moving up to (9, 8)
                if (yposf > 8 * step) {
                    xposf = 9 * step;
                    
                    vx = 0;
                    vy = -1;
                }
                else {
                    xposf = 9 * step;
                    yposf = 8 * step;
                    if (!pacman->powered_up) {
                        mode = CHASE;
                    }
                    else {
                        mode = FRIGHTENED;
                    }
                }
            }
        }

        if (pacman->powered_up) {
            if (mode == CHASE) {
                // Change direction immediately when frightened
                vx = -vx;
                vy = -vy;
                xposf += dt * speed * vx;
                yposf += dt * speed * vy;
                mode = FRIGHTENED;
            }
        }
        else {
            if (mode == FRIGHTENED) {
                mode = CHASE;
            }
        }

        if (pow(xpos - pacman->xpos, 2) + pow(ypos - pacman->ypos, 2) < collide_dist) {
            if (mode == CHASE) {
                // ======== IF A GHOST EATS PACMAN ========
                pacman->life -= 1;
                if (pacman->life <= 0) {
                    game_mode = LOSE;
                    return;
                }
                pacman->reset_postion();
                pacman->vx = pacman->vy = pacman->next_x = pacman->next_y = 0;
                for (int i = 0; i < 4; i++) {
                    Ghosts[i]->reset_position();
                }
                init_waitlist();

            }
            else if (mode == FRIGHTENED){
                // ======== IF PACMAN EATS A GHOST ========
                reset_position();
                mode = WAIT;
#ifdef _WIN32
                this->wait_start = GetTickCount();
#else
                gettimeofday(&(this->wait_start), NULL);
#endif
                waitlist_mutex.lock();
                if (!waitlist.empty()) {
                    
                    static float diff;
                    #ifdef _WIN32
                    diff = (this->wait_start - waitlist.back()->wait_start)/1000.0f;
                    #else
                    diff = (float)(wait_start.tv_sec - waitlist.back()->wait_start.tv_sec) +
                        1.0e-6 * (wait_start.tv_usec - waitlist.back()->wait_start.tv_usec);
                    #endif
                    //printf("difference = %d - %d = %f\n", this->wait_start, waitlist.back()->wait_start,  diff);
                    wait_time = (diff < 2) ? (7 - diff) : 5;
                    
                }
                else {
                    wait_time = 5;
                }
                //printf("%d's wait time is %f\n", color, wait_time);
                waitlist.push_back(this);
                pacman->score += pow(2, waitlist.size()) * 200;
                waitlist_mutex.unlock();
            }    
        }

        if (xpos % step == 0 && ypos % step == 0) {
            // If ghost on map grid point
            mapx = xpos / step;
            mapy = ypos / step;

            if(mode == CHASE)
                set_target();

            if (mode == CHASE || mode == FRIGHTENED) {
                // CHASE: go through all four possible directions and find the best direction
                // FRIGHTENED: choose a random direction
                min_distance = 1000;
                if ((*p)[mapy][mapx - 1] != 1 && vx != 1) {
                    if (mode == CHASE)
                        next_dis = pow(mapx - 1 - tarx, 2) + pow(mapy - tary, 2);
                    else if (mode == FRIGHTENED)
                        next_dis = rand() % 1000;

                    if (next_dis <= min_distance) {
                        min_distance = next_dis;
                        dir = LEFT;
                    }
                }

                if ((*p)[mapy][mapx + 1] != 1 && vx != -1) {
                    if (mode == CHASE)
                        next_dis = pow(mapx + 1 - tarx, 2) + pow(mapy - tary, 2);
                    else if (mode == FRIGHTENED)
                        next_dis = rand() % 1000;

                    if (next_dis <= min_distance) {
                        min_distance = next_dis;
                        dir = RIGHT;
                    }
                }

                if ((*p)[mapy + 1][mapx] != 1 && vy != -1) {
                    if (mode == CHASE)
                        next_dis = pow(mapx - tarx, 2) + pow(mapy + 1 - tary, 2);
                    else if (mode == FRIGHTENED)
                        next_dis = rand() % 1000;

                    if (next_dis <= min_distance) {
                        min_distance = next_dis;
                        dir = DOWN;
                    }
                }

                if ((*p)[mapy - 1][mapx] != 1 && vy != 1) {
                    if (mode == CHASE)
                        next_dis = pow(mapx - tarx, 2) + pow(mapy - 1 - tary, 2);
                    else if (mode == FRIGHTENED)
                        next_dis = rand() % 1000;

                    if (next_dis <= min_distance) {
                        min_distance = next_dis;
                        dir = UP;
                    }
                }

                // Set actual direction
                switch (dir) {
                case UP:
                    vx = 0;
                    vy = -1;
                    break;
                case DOWN:
                    vx = 0;
                    vy = 1;
                    break;
                case LEFT:
                    vx = -1;
                    vy = 0;
                    break;
                case RIGHT:
                    vx = 1;
                    vy = 0;
                    break;
                }
            }
        }

        xposf += dt * speed * vx;
        yposf += dt * speed * vy;
        if (xposf >= 18 * step && vx == 1)
            xposf = 0;
        else if (xposf <= 0 && vx == -1)
            xposf = 18 * step;
        xpos = round(xposf);
        ypos = round(yposf);
    }

    void set_target() {
        pac_mapx = (pacman->xpos) / step;
        pac_mapy = (pacman->ypos) / step;
        switch (color) {
        case RED:
            // Set target to pacman
            tarx = pac_mapx;
            tary = pac_mapy;
            break;
        case PINK:
            // Set target to four tiles in front of pacman
            if (pacman->vy == -1) 
                tarx = pac_mapx - 4;
            else 
                tarx = pac_mapx + 4 * pacman->vx;
            tary = pac_mapy + 4 * pacman->vy;
            break;
        case CYAN:
            // Set target to position that is symmetric of Blinky about pacman
            tarx = 2 * pac_mapx - (Ghosts[0]->xpos) / step;
            tary = 2 * pac_mapy - (Ghosts[0]->ypos) / step;
            break;
        case ORANGE:
            if (pow(mapx - pac_mapx, 2) + pow(mapy - pac_mapy, 2) > 64){
                // Set target to pacman if pacman is eight tiles away
                tarx = pac_mapx;
                tary = pac_mapy;
            }
            else {
                // Set target to left bottom if it is within eight tiles
                tarx = 0;
                tary = 22;
            }
            break;
        }
    }
    void set_list_ptr(ECE_Ghost** ghost) {
        Ghosts = ghost;
    }
private:

    // Start and end time when waiting
#ifdef _WIN32
    DWORD wait_start;
#else
    struct timeval wait_start;
    struct timeval current_time;
#endif

    ECE_Pacman* pacman;
    ECE_Ghost** Ghosts;
    short(*p)[22][19];
    float speed = 24;
    int xpos, ypos; // Position of ghost
    float xposf, yposf;
    Direction dir;
    int vx, vy;
    int mapx, mapy; // relative position in the map coordinates
    int tarx, tary; // target position in the map coordinates
    int pac_mapx, pac_mapy;
    Color color;
    Mode mode;
    float elapsed_time, wait_time;
    int step;
    int min_distance = 1000;
    float collide_dist;
    int next_dis;
    const float body_radius = 0.48f;
    const float body_height = 0.52f;
};

ECE_Pacman pacman(step, &map);

ECE_Ghost Blinky(RED, step, &map, &pacman);
ECE_Ghost Inky(CYAN, step, &map, &pacman);
ECE_Ghost Pinky(PINK, step, &map, &pacman);
ECE_Ghost Clyde(ORANGE, step, &map, &pacman);
ECE_Ghost* ghost_list[4] = { &Blinky, &Pinky, &Inky, &Clyde};

void init_waitlist() {
    waitlist_mutex.lock();
    waitlist.clear();
    for (int i = 0; i < 4; i++) {
        waitlist.push_back(ghost_list[i]);
        #ifdef _WIN32
        ghost_list[i]->wait_start = GetTickCount();
        #else
        gettimeofday(&(ghost_list[i]->wait_start), NULL);
        #endif
        ghost_list[i]->wait_time = 2 * i + 1;
    }
    waitlist_mutex.unlock();
}

void display_text() {
    static char score_char[40];
    static char life_char[10];
    static char ready_char[] = "PRESS ANY KEY TO START";
    static char lose_char[] = "GAME OVER!";
    static char win_char[] = "YOU WON!";

    sprintf_s(score_char, sizeof(score_char), "SCORE: %d", pacman.score);
    sprintf_s(life_char, sizeof(life_char), "LIFE: %d", pacman.life);

    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos3f(-20, 10, 8);
    for (int i = 0; i < 15; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, score_char[i]);
    }
    glRasterPos3f(-20, 6, 8);
    for (int i = 0; i < 10; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, life_char[i]);
    }

    glRasterPos3f(-20, 2, 8);
    if (game_mode == READY) {
        for (int i = 0; i < 23; i++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, ready_char[i]);
        }
    }
    else if (game_mode == WIN) {
        for (int i = 0; i < 9; i++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, win_char[i]);
        }
    }
    else if (game_mode == LOSE) {
        for (int i = 0; i < 11; i++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, lose_char[i]);
        }
    }

}


void init(void)
{
    if (!quadratic)
        quadratic = gluNewQuadric();
}

//----------------------------------------------------------------------
// Reshape callback
//
// Window size has been set/changed to w by h pixels. Set the camera
// perspective to 45 degree vertical field of view, a window aspect
// ratio of w/h, a near clipping plane at depth 1, and a far clipping
// plane at depth 100. The viewport is the entire window.
//
//----------------------------------------------------------------------
void changeSize(int w, int h)
{
    float ratio = ((float)w) / ((float)h); // window aspect ratio
    glMatrixMode(GL_PROJECTION); // projection matrix is active
    glLoadIdentity(); // reset the projection
    gluPerspective(45.0, ratio, 0.1, 100.0); // perspective transformation
    glMatrixMode(GL_MODELVIEW); // return to modelview mode
    glViewport(0, 0, w, h); // set viewport (drawing area) to entire window
}


void drawMap() {
    for (int i = 0; i < 22; i++) {
        for (int j = 0; j < 19; j++) {
            glPushMatrix();
            glTranslatef(j - 9, 10 - i, 2);
            if (map[i][j] == 0)
            {
                // Draw coins
                glColor3f(1.0, 1.0, 1.0);
                glutSolidSphere(coin_radius, 20, 20);
            }
            else if (map[i][j] == 1)
            {
                // Draw connection on walls
                glColor3f(0.1f, 0.61f, 0.95f);
                glutSolidSphere(wall_radius, 20, 20);
                if (j < 18 && map[i][j + 1] == 1) {
                    // Draw horizontal walls
                    glPushMatrix();
                    glRotatef(90, 0.0, 1.0, 0.0);
                    if (i == 9 && (j == 8 || j == 9)) 
                        glColor3f(1.0, 1.0, 1.0); // Set door to white
                    gluCylinder(quadratic, wall_radius, wall_radius, 1, 25, 25);
                    glPopMatrix();
                }
                if (i < 21 && map[i + 1][j] == 1) {
                    // Draw vertical walls
                    if (i == 2 && (j == 6 || j == 12)) {
                        glPopMatrix();
                        continue;
                    }
                    glPushMatrix();
                    glRotatef(90, 1.0, 0.0, 0.0);
                    gluCylinder(quadratic, wall_radius, wall_radius, 1, 25, 25);
                    glPopMatrix();
                }
            }

            else if (map[i][j] == 3)
            {
                // Draw power-ups
                glColor3f(0.92, 0.74, 0.32);
                gluDisk(quadratic, 0, 0.35, 25, 25);
            }

            glPopMatrix();
        }
    }
}


void Blinky_update() {
    #ifdef _WIN32
        static DWORD Blinky_idle_time;
        static DWORD Blinky_time_now;
    #else
        static struct timeval Blinky_idle_time;
        static struct timeval Blinky_time_now;
    #endif
    static float dt;
    while (1) {
        if (game_mode == RUN) {
#ifdef _WIN32
            Blinky_time_now = GetTickCount();
            dt = (float)(Blinky_time_now - Blinky_idle_time) / 1000.0;
#else
            gettimeofday(&Blinky_time_now, NULL);
            dt = (float)(Blinky_time_now.tv_sec - Blinky_idle_time.tv_sec) +
                1.0e-6 * (Blinky_time_now.tv_usec - Blinky_idle_time.tv_usec);
#endif
            Blinky.update(dt);
            Blinky_idle_time = Blinky_time_now;
        }
        else {
            this_thread::sleep_for(chrono::milliseconds(50));
        }
    }
}

void Inky_update() {
#ifdef _WIN32
    static DWORD Inky_idle_time;
    static DWORD Inky_time_now;
#else
    static struct timeval Inky_idle_time;
    static struct timeval Inky_time_now;
#endif
    static float dt;
    while (1) {
        if (game_mode == RUN) {
#ifdef _WIN32
            Inky_time_now = GetTickCount();
            dt = (float)(Inky_time_now - Inky_idle_time) / 1000.0;
#else
            gettimeofday(&Inky_time_now, NULL);
            dt = (float)(Inky_time_now.tv_sec - Inky_idle_time.tv_sec) +
                1.0e-6 * (Inky_time_now.tv_usec - Inky_idle_time.tv_usec);
#endif
            Inky.update(dt);
            Inky_idle_time = Inky_time_now;
        }
        else {
            this_thread::sleep_for(chrono::milliseconds(50));
        }
    }
}

void Pinky_update() {
#ifdef _WIN32
    static DWORD Pinky_idle_time;
    static DWORD Pinky_time_now;
#else
    static struct timeval Pinky_idle_time;
    static struct timeval Pinky_time_now;
#endif
    static float dt;
    while (1) {
        if (game_mode == RUN) {
#ifdef _WIN32
            Pinky_time_now = GetTickCount();
            dt = (float)(Pinky_time_now - Pinky_idle_time) / 1000.0;
#else
            gettimeofday(&Pinky_time_now, NULL);
            dt = (float)(Pinky_time_now.tv_sec - Pinky_idle_time.tv_sec) +
                1.0e-6 * (Pinky_time_now.tv_usec - Pinky_idle_time.tv_usec);
#endif
            Pinky.update(dt);
            Pinky_idle_time = Pinky_time_now;
        }
        else {
            this_thread::sleep_for(chrono::milliseconds(50));
        }
    }
}

void Clyde_update() {
#ifdef _WIN32
    static DWORD Clyde_idle_time;
    static DWORD Clyde_time_now;
#else
    static struct timeval Clyde_idle_time;
    static struct timeval Clyde_time_now;
#endif
    static float dt;
    while (1) {
        if (game_mode == RUN) {
#ifdef _WIN32
            Clyde_time_now = GetTickCount();
            dt = (float)(Clyde_time_now - Clyde_idle_time) / 1000.0;
#else
            gettimeofday(&Clyde_time_now, NULL);
            dt = (float)(Clyde_time_now.tv_sec - Clyde_idle_time.tv_sec) +
                1.0e-6 * (Clyde_time_now.tv_usec - Clyde_idle_time.tv_usec);
#endif
            Clyde.update(dt);
            Clyde_idle_time = Clyde_time_now;
        }
        else {
            this_thread::sleep_for(chrono::milliseconds(50));
        }
    }
}


//----------------------------------------------------------------------
// Update with each idle event
//
// This incrementally moves the camera and requests that the scene be
// redrawn.
//----------------------------------------------------------------------
void update(void)
{
    if (game_mode != RUN) {
        return;
    }

    pacman.update();
    
    // Save time_now for next time
    glutPostRedisplay(); // redisplay everything
}

//----------------------------------------------------------------------
// Draw the entire scene
//
// We first update the camera location based on its distance from the
// origin and its direction.
//----------------------------------------------------------------------
void renderScene(void)
{

    // Clear color and depth buffers
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Reset transformations
    glLoadIdentity();

    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHT0);
    GLfloat light_ambient[] = { 0.0, 0.0, 0.0, 1.0 };
    GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };

    //glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    //glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    //glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glEnable(GL_DEPTH_TEST);

    // Set the camera centered at (x,y,height) and looking along directional
    gluLookAt(
        x, y, height,
        0.0, 0.0, 0.0,
        0.0, 0.0, 1.0);

    // Draw ground - 200x200 square colored black
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_QUADS);
    glVertex3f(-100.0, -100.0, 0.0);
    glVertex3f(-100.0, 100.0, 0.0);
    glVertex3f(100.0, 100.0, 0.0);
    glVertex3f(100.0, -100.0, 0.0);
    glEnd();


    drawMap(); // Draw the maze
    pacman.draw(); // Draw the pacman
    for (int i = 0; i < 4; i++) {
        ghost_list[i]->draw(); // Draw four ghosts
    }

    
    display_text();

    glutSwapBuffers(); // Make it all visible

}

//----------------------------------------------------------------------
// User-input callbacks
//
// processNormalKeys: ESC, q, and Q cause program to exit
// processNormalKeys: r and R to increase camera angle by 5 degree
//----------------------------------------------------------------------
void processNormalKeys(unsigned char key, int xx, int yy)
{
    if (game_mode == READY) {
        if (key) {
            game_mode = RUN;
            init_waitlist();
            return;
        }
        else
            return;
    }
    if (key == ESC || key == 'q' || key == 'Q')
    {
        exit(0);
    }
    if (key == 'r' || key == 'R')
    {
        theta = (theta + 5) % 360;
        x = radius * cos(theta * PI / 180);
        y = radius * sin(theta * PI / 180);
    }
    if (key == 'e' || key == 'E') {
        theta = 270;
        x = radius * cos(theta * PI / 180);
        y = radius * sin(theta * PI / 180);
    }
    if (key == 'w' || key == 'W')
    {
        theta = (theta + 355) % 360;
        x = radius * cos(theta * PI / 180);
        y = radius * sin(theta * PI / 180);
    }
}

void pressSpecialKey(int key, int xx, int yy)
{
    if (game_mode == READY) {
        if (key) {
            game_mode = RUN;
            init_waitlist();
            return;
        }
        else
            return;
    }
    if (game_mode == RUN) {
        // UP, DOWN, LEFT, and RIGHT arrow keys set the upcoming direction of Pacman
        if (key == GLUT_KEY_UP)
            pacman.set_next(0, -1);
        else if (key == GLUT_KEY_DOWN)
            pacman.set_next(0, 1);
        else if (key == GLUT_KEY_LEFT)
            pacman.set_next(-1, 0);
        else if (key == GLUT_KEY_RIGHT)
            pacman.set_next(1, 0);
    }
}

//----------------------------------------------------------------------
// Main program  - standard GLUT initializations and callbacks
//----------------------------------------------------------------------
int main(int argc, char** argv)
{
    printf("\n\
-----------------------------------------------------------------------\n\
 3D Pac-Man Maze Program:\n\
  - Press any key to start game\n\
  - Press R or W to rotate map\n\
  - Press E to reset map\n\
  - q or ESC to quit\n\
-----------------------------------------------------------------------\n");

    srand(time(NULL));
    for (int i = 0; i < 4; i++) {
        // Assign ghost_list pointer to each ghost
        ghost_list[i]->set_list_ptr(ghost_list); 
    }

    // general initializations
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(800, 400);
    glutCreateWindow("Pacman");
    init();

    // register callbacks
    glutReshapeFunc(changeSize); // window reshape callback
    glutDisplayFunc(renderScene); // (re)display callback
    glutIdleFunc(update); // incremental update
    glutIgnoreKeyRepeat(1); // ignore key repeat when holding key down
    glutKeyboardFunc(processNormalKeys); // process standard key clicks
    glutSpecialFunc(pressSpecialKey); // process special key pressed

    thread Blinky_thread(Blinky_update);
    thread Inky_thread(Inky_update);
    thread Pinky_thread(Pinky_update);
    thread Clyde_thread(Clyde_update);

    // enter GLUT event processing cycle
    glutMainLoop();
    // Start ghost threads
    Blinky_thread.join();
    Inky_thread.join();
    Pinky_thread.join();
    Clyde_thread.join();

    return 0; // this is just to keep the compiler happy
}