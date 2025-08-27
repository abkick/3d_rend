//
// Created by Abby on 8/19/2025.
//

#include "simulation.h"

void SIM::initialize() {
    std::cout << "initializing" << std::endl;
    SDL_Init(SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK | SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    if (!SDL_CreateWindowAndRenderer("Abby's 3d render", 1000, 1000, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
    }


    cube.load_obj_mesh("cube.obj");
    monkey.load_obj_mesh("monkey.obj");
    abby.load_obj_mesh("Abby.obj");

    SDL_GetWindowSizeInPixels(window,&window_width, &window_height);
    cube.position = Eigen::Vector3d(window_width>>1, window_height>>1, 0); // setting position as middle of window
    cube.render_mesh.scale = 100;

    time = 0;
    dt = (SDL_GetTicks() - time) * 1e-3;
}

void SIM::update_phys() {
    dt = (SDL_GetTicks() - time) * 1e-3;
    time = SDL_GetTicks();
    cube.update(Quaternion<double>(AngleAxisd(1.2,Eigen::Vector3d(0.9,0.5,0.3).normalized().eval())), dt);
};


void SIM::update_render() {
    int window_height, window_width;
    SDL_GetWindowSizeInPixels(window,&window_width, &window_height);
    cube.position = Eigen::Vector3d(window_width>>1, window_height>>1, 0); // setting position as middle of window
    for (auto &vert : cube.render_mesh.verts) { // flipping vertically due to how SDL renders top down
        vert.position.y *= -1;
        vert.position.y += window_height;
    }
    //std::cout << "x: " << cube.render_mesh.verts[0].position.x << "  y: " << cube.render_mesh.verts[0].position.y << std::endl;

    SDL_SetRenderDrawColor(renderer, 0x70, 0x00, 0x70, 0x00);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderGeometry(renderer, NULL, cube.render_mesh.verts.data(), cube.render_mesh.verts.size(), cube.render_mesh.indices.data(), cube.render_mesh.indices.size());
    SDL_RenderPresent(renderer);

}

void SIM::handle_events() {
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT: running = false;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED: running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                switch (event.key.key) {
                    case SDLK_ESCAPE: running = false; break;
                    case SDLK_1: ; break;
                    case SDLK_2: ; break;
                    case SDLK_3: ; break;
                    default: break;
                }

            case SDL_EVENT_KEY_UP: break;

            default: break;
        }
    }
}


void SIM::run_main() {
    std::cout << "starting sim" << std::endl;
    while (running) {
        update_phys();
        handle_events();
        update_render();
        SDL_Delay(40);
    }
}

SIM::~SIM() {
    //SDL_WaitThread(phys_thread, nullptr);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    texture = nullptr;
    renderer = nullptr;
    window = nullptr;
    SDL_Quit();
}