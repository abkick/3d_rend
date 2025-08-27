//
// Created by Abby on 8/19/2025.
//

#ifndef INC_3D_REND_SIMULATION_H
#define INC_3D_REND_SIMULATION_H

#include <chrono>
#include <SDL3/SDL.h>

#include <SDL3/SDL_thread.h>
#include <Eigen/Dense>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

#include "entities.h"

class SIM {
public:
    std::atomic<bool> running = true;
    std::atomic<bool> ready = false;
    Uint64 time;
    double dt;
    SDL_Event event;
    SDL_Window *window = nullptr;
    int window_height, window_width;
    SDL_Renderer *renderer = nullptr;
    SDL_Texture *texture = nullptr;
    SDL_Thread *phys_thread;
    int data = 101;

    Entity cube;
    Entity monkey;
    Entity abby;

    void initialize();
    void update_phys();
    void update_render();
    void handle_events();
    void run_main();

    ~SIM();
};




#endif //INC_3D_REND_SIMULATION_H