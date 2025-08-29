//
// Created by Abby on 8/20/2025.
//

#ifndef INC_3D_REND_ENTITIES_H
#define INC_3D_REND_ENTITIES_H

#include <format>
#include <Eigen/Dense>
#include <SDL3/SDL.h>
#include<iostream>
#include<iomanip>
#include<fstream>

using namespace Eigen;

typedef struct Triangle_Struct {
    int vert_indices[3];
    Eigen::Vector3d vert1, vert2, vert3;
    Eigen::Vector3d normal;
    double distance;
    double get_distance() { return (vert1(2)+vert2(2)+vert3(2) );}
}Triangle;

typedef struct SDL_Mesh {
    std::vector<SDL_Vertex> verts;
    std::vector<int> indices;
    double scale;
} SDL_Mesh;

typedef struct Physics_Mesh {
    Eigen::Matrix<double, 3, Dynamic> verts;
    Eigen::Matrix<double, 3, Dynamic> verts_trans; // mesh after world transformations, rotations, scaling, and translations
    std::vector<Vector4i> indices; // index of {vec, vec, vec, norm} of each triangle
    std::vector<Vector4i> indices_culled;
    Eigen::Matrix<double, 3, Dynamic> normals;
    Eigen::Matrix<double, 3, Dynamic> normals_trans;
    void sort_indices();
    std::vector<Triangle> Triangles;
} Phys_Mesh;



class Entity {
public:
    std::string path_to_obj;
    Eigen::Vector3d position;
    Quaternion<double> orientation = Quaternion<double>::Identity();
    Phys_Mesh phys_mesh;
    SDL_Mesh render_mesh;

    void update(Quaternion<double> rotation, double dt);
    int load_obj_mesh(std::string path); // returns 1 if failed
};

class Camera { //WIP
    Eigen::Vector3d position;
    Eigen::Quaternion<double> rotation;
    double FOV = 90;
    double aspect_ratio = 16.0 / 9.0;
    double z_near = 0.1;
    double z_far = 100.0;
};


#endif //INC_3D_REND_ENTITIES_H