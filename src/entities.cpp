//
// Created by Abby on 8/20/2025.
//

#include "entities.h"

#include <random>
#include<Eigen/StdVector>


void Entity::load_obj_mesh(std::string file_name) {
    double mesh_scale = 1;
    path_to_obj =  "..\\src\\" + file_name;
    std::cout << "Loading mesh file: " << path_to_obj << std::endl;
    char delimiter = ' ';

    std::vector<Eigen::Vector3d> temp_vertices; // WARNING: Eigen::Vectors in c++ vectors can have issues with alignment if compiling in C++ 17 or prior
    std::vector<Eigen::Vector3d> temp_normals;
    std::ifstream loaded_file;
    loaded_file.open(path_to_obj);
    if (!loaded_file.is_open()) {
        std::cerr << "ERROR: Failed to open file " << path_to_obj << std::endl;
    }

    std::string line;
    while (std::getline(loaded_file, line)) {
        std::istringstream whole_line(line);
        //if (line.find(face_delimiter) != std::string::npos) {}
        Vector3d xyz;
        Vector4i inds;

        while (getline(whole_line, line, delimiter)) {
            if (line == "v") {
                getline(whole_line, line, delimiter);
                xyz[0] = stod(line);
                getline(whole_line, line, delimiter);
                xyz[1] = stod(line);
                getline(whole_line, line, delimiter);
                xyz[2] = stod(line);
                temp_vertices.push_back(xyz);
            } //load vertex
            else if (line == "vn") {
                getline(whole_line, line, delimiter);
                xyz[0] = stod(line);
                getline(whole_line, line, delimiter);
                xyz[1] = stod(line);
                getline(whole_line, line, delimiter);
                xyz[2] = stod(line);
                temp_normals.push_back(xyz);

            } //load normal
            else if (line == "vt") {}
            else if (line == "s") {
                getline(whole_line, line, delimiter);
                if (std::stod(line) != 0) mesh_scale = std::stod(line);
            }
            else if (line == "f") {
                int vert_ind = 0;
                //int norm_ind = 0;
                bool got_normal = false;
                std::istringstream sub_string(line);
                while (getline(whole_line, line, delimiter)) {
                    inds[vert_ind] = std::stoi(&line[0]) - 1; // .obj files start index from 1 not 0
                    if (!got_normal) { // need to grab index of normal vector
                        size_t last_index = line.find_last_of("/");
                        inds[3] = std::stoi(&line[last_index+1])-1;
                        got_normal = true;
                    }
                    vert_ind++;
                    if (vert_ind >= 3) { // at 3 vertex indices, pushback the first 3, and if there are more than 3 create another set of 3
                        vert_ind %= 2;
                        phys_mesh.indices.push_back(inds);
                    }
                }
                phys_mesh.indices.push_back(inds);
            } //load face


        }
    }

    loaded_file.close();
    std::cout << "Mesh file: \'" << path_to_obj << "\' loaded successfully" << std::endl;

    for (int i = 0; i < phys_mesh.indices.size(); i++) {
        //std::cout << "index: " << i+1 << "\nVertices: " << phys_mesh.indices[i](seq(0,2)).transpose() << "\nFace: " << phys_mesh.indices[i](3) << std::endl;
        render_mesh.indices.emplace_back(phys_mesh.indices[i](0));
        render_mesh.indices.emplace_back(phys_mesh.indices[i](1));
        render_mesh.indices.emplace_back(phys_mesh.indices[i](2));
    }

    phys_mesh.verts.resize(3, static_cast<signed long long>(temp_vertices.size()));
    phys_mesh.verts_trans.resizeLike(phys_mesh.verts);
    for (int i = 0; i < phys_mesh.verts.row(0).size(); i++) {
        phys_mesh.verts.col(i) = temp_vertices[i];
    }
    phys_mesh.verts *= mesh_scale;

    phys_mesh.normals.resize( 3, static_cast<signed long long>(temp_normals.size()) );
    phys_mesh.normals_trans.resize( 3, static_cast<signed long long>(temp_normals.size()) );
    for ( int i = 0; i < phys_mesh.normals.row(0).size(); i++ ) {
        phys_mesh.normals.col(i) = temp_normals[i];
    }

    std::cout << phys_mesh.verts.row(0).size() << " vertices loaded" << std::endl;
    std::cout << temp_normals.size() << " normals loaded \n" << std::endl;
    render_mesh.verts.resize(phys_mesh.verts.row(0).size());

    // adding random colors to vertices as temporary stand-in for adding texture
    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    for (int i=0; i<render_mesh.verts.size(); i++) {
        render_mesh.verts[i].color = SDL_FColor(distribution(generator),distribution(generator),distribution(generator),1);
    }
}


void Entity::update(Quaternion<double> rotation, const double dt) {
    rotation.normalize();
    orientation.normalize();
    rotation *= orientation;
    orientation = orientation.slerp(dt, rotation);
    phys_mesh.verts_trans = orientation.toRotationMatrix() * phys_mesh.verts;
    phys_mesh.normals_trans = orientation.toRotationMatrix() * phys_mesh.normals;
    phys_mesh.verts_trans.colwise() += position;
    phys_mesh.verts_trans *= render_mesh.scale;
    for (int i=0; i<render_mesh.verts.size(); i++) {
        render_mesh.verts[i].position.x = phys_mesh.verts_trans(0,i);
        render_mesh.verts[i].position.y = phys_mesh.verts_trans(1,i);
    }

    //Eigen::Index index;
    ////reorder indices according to avg triangle distance, WIP
    //int triangle_index[3];
    //double triangle_distance;
    //for (int i = 0; i < phys_mesh.indices.size(); i++) {
    //    triangle_index[0] = phys_mesh.indices[i](0);
    //    triangle_index[1] = phys_mesh.indices[i](1);
    //    triangle_index[2] = phys_mesh.indices[i](2);
    //    triangle_distance = ;
    //}


    for (int i = 0; i < phys_mesh.indices.size(); i++) { // backface culling
         int norm_index = phys_mesh.indices[i](3);
        if (phys_mesh.normals_trans( 2, norm_index ) < 0) { // if face is pointing away, blank the render index
            render_mesh.indices[3*i] = 0;
            render_mesh.indices[3*i+1] = 0;
            render_mesh.indices[3*i+2] = 0;
        }
        else {
            render_mesh.indices[3*i] = (phys_mesh.indices[i](0));
            render_mesh.indices[3*i+1] = (phys_mesh.indices[i](1));
            render_mesh.indices[3*i+2] = (phys_mesh.indices[i](2));
        }
    }

}