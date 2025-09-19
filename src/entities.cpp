//
// Created by Abby on 8/20/2025.
//

#include "entities.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <Eigen/StdVector>


template <typename T>
std::vector<size_t> sort_indexes(const std::vector<T> &v) { //Note to self, learn how lamdas work in c++, this function was copied this from stackoverflow user Lukasz Wiklendt,
    std::vector<size_t> idx(v.size());
    iota(idx.begin(), idx.end(), 0);
    stable_sort(idx.begin(), idx.end(),
         [&v](size_t i1, size_t i2) {return v[i1] < v[i2];});
    return idx;
}

int Entity::load_obj_mesh(std::string file_name) {

    path_to_obj =  R"(..\src\mesh\)" + file_name;
    std::cout << "Loading mesh file: " << path_to_obj << std::endl;
    static constexpr char delimiter = ' ';

    std::vector<Eigen::Vector3d> temp_vertices; // WARNING: Eigen::Vectors in c++ vectors can have issues with alignment if using older compliers, so watchout for that
    std::vector<Eigen::Vector3d> temp_normals;
    std::ifstream loaded_file;
    loaded_file.open(path_to_obj);
    if (!loaded_file.is_open()) {
        std::cerr << "ERROR: Failed to open file " << path_to_obj << std::endl;
        return 1;
    }
    double max_x, max_y, max_z;
    double min_x, min_y, min_z;
    std::string line;
    while (std::getline(loaded_file, line)) {
        std::istringstream whole_line(line);
        //if (line.find(face_delimiter) != std::string::npos) {}
        Vector3d xyz;
        Vector4i inds;

        while (getline(whole_line, line, delimiter)) {
            if (line == "v") {  //load vertex
                getline(whole_line, line, delimiter);
                xyz[0] = stod(line);
                if (xyz[0] > max_x) max_x = xyz[0];
                if (xyz[0] < min_x) min_x = xyz[0];
                getline(whole_line, line, delimiter);
                xyz[1] = stod(line);
                if (xyz[1] > max_y) max_y = xyz[1];
                if (xyz[1] < min_y) min_y = xyz[1];
                getline(whole_line, line, delimiter);
                xyz[2] = stod(line);
                if (xyz[2] > max_z) max_z = xyz[2];
                if (xyz[2] < min_z) min_z = xyz[2];
                temp_vertices.push_back(xyz);
            }
            else if (line == "vn") { //load normals
                getline(whole_line, line, delimiter);
                xyz[0] = stod(line);
                getline(whole_line, line, delimiter);
                xyz[1] = stod(line);
                getline(whole_line, line, delimiter);
                xyz[2] = stod(line);
                temp_normals.push_back(xyz);

            }
            else if (line == "vt") { //has 2 texture coordinates, uv

            }
            else if (line == "s") {}
            else if (line == "f") {  //load face, works with faces delimited with "/"
                int vert_ind = 0;
                //int norm_ind = 0;
                bool got_normal = false;
                std::istringstream sub_string(line);
                while (getline(whole_line, line, delimiter)) {
                    inds[vert_ind] = std::stoi(&line[0]) - 1; // .obj files start index from 1 not 0
                    if (!got_normal) { // need to grab index of normal vector
                        size_t last_index = line.find_last_of('/'); // faces can contain texture and normal index that are delimited with '/'
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
            }
        }
    }
    loaded_file.close();
    std::cout << "Mesh file: \'" << path_to_obj << "\' loaded successfully" << std::endl;

    for (int i = 0; i < phys_mesh.indices.size(); i++) {
        render_mesh.indices.emplace_back(phys_mesh.indices[i](0));
        render_mesh.indices.emplace_back(phys_mesh.indices[i](1));
        render_mesh.indices.emplace_back(phys_mesh.indices[i](2));
    }

    std::cout << "Bounds x:{" << min_x << "," << max_x << "} y:{ " << min_y << "," << max_y << "} z:{"  << min_z << "," << max_z << "}" << std::endl;
    Vector3d center = {(min_x+max_x)/2,(min_y+max_y)/2,(min_z+max_y)/2};
    phys_mesh.verts.resize(3, static_cast<signed long long>(temp_vertices.size()));
    phys_mesh.verts_trans.resizeLike(phys_mesh.verts);
    for (int i = 0; i < phys_mesh.verts.row(0).size(); i++) {
        phys_mesh.verts.col(i) = temp_vertices[i]-center;
    }
    std::vector<double> bounds = {max_x-min_x, max_y-min_y, max_z-min_z};
    render_mesh.scale = 600/std::sqrt(bounds[0]*bounds[0] + bounds[1]*bounds[1] + bounds[2]*bounds[2]);
    std::cout << "Scale: " << render_mesh.scale << std::endl;
    phys_mesh.normals.resize( 3, static_cast<signed long long>(temp_normals.size()) );
    phys_mesh.normals_trans.resize( 3, static_cast<signed long long>(temp_normals.size()) );
    for ( int i = 0; i < phys_mesh.normals.row(0).size(); i++ ) {
        phys_mesh.normals.col(i) = temp_normals[i];
    }

    std::cout << phys_mesh.verts.row(0).size() << " vertices loaded" << std::endl;
    std::cout << temp_normals.size() << " normals loaded" << std::endl;
    std::cout << phys_mesh.indices.size() << " triangles loaded\n" << std::endl;
    render_mesh.verts.resize(phys_mesh.verts.row(0).size());

    // adding random colors to vertices as temporary stand-in for adding texture
    std::default_random_engine generator(SDL_GetTicksNS());
    std::uniform_real_distribution<float> distribution(0.0, 1.0);
    for (auto & vert : render_mesh.verts) {
        vert.color = SDL_FColor(distribution(generator),distribution(generator),distribution(generator),1);
    }

    return 0;
}


void Entity::update(Quaternion<double> rotation, const double dt) {
    rotation.normalize();
    orientation.normalize();
    rotation *= orientation; // "rotation" is rotated by the orientation. aka rotation is made to be relative to the objects orientation
    orientation = orientation.slerp(dt, rotation); // similar to scaling the rotation by dt, rotation acts as angular velocity
    phys_mesh.verts_trans = orientation.toRotationMatrix() * phys_mesh.verts;
    phys_mesh.normals_trans = orientation.toRotationMatrix() * phys_mesh.normals;
    phys_mesh.verts_trans *= render_mesh.scale;
    phys_mesh.verts_trans.colwise() += position;

    for (int i=0; i<phys_mesh.verts_trans(0,all).size(); i++) {
        render_mesh.verts[i].position.x = static_cast<float>(phys_mesh.verts_trans(0, i));
        render_mesh.verts[i].position.y = static_cast<float>(phys_mesh.verts_trans(1, i));
        render_mesh.verts[i].color.r = (phys_mesh.verts_trans(2,i))/(render_mesh.scale);
        render_mesh.verts[i].color.b = (-phys_mesh.verts_trans(2,i))/(render_mesh.scale);
    }


    ///////////////////////////////////////////
    /// Back face culling, make a vector of indexes of only the triagles that face in the z direction
    phys_mesh.indices_culled.clear();
    for (int i = 0; i < phys_mesh.indices.size(); i++) { // backface culling
        int norm_index = phys_mesh.indices[i](3);
        if (phys_mesh.normals_trans( 2, norm_index ) > 0) { // if face/normal is pointing toward the screen (z>0), add to new index list
            phys_mesh.indices_culled.emplace_back() = phys_mesh.indices[i];
        }
    }

    std::vector<double> distance_by_index;
    std::vector<Triangle> tris;

    for (int i=0; i<phys_mesh.indices_culled.size(); i++) {
        tris.emplace_back();
        tris[i].vert_indices[0] = phys_mesh.indices_culled[i](0);
        tris[i].vert_indices[1] = phys_mesh.indices_culled[i](1);
        tris[i].vert_indices[2] = phys_mesh.indices_culled[i](2);
        tris[i].distance = phys_mesh.verts_trans(2, tris[i].vert_indices[0]);  // the z value of each vertex in the index is summed
        tris[i].distance += phys_mesh.verts_trans(2,tris[i].vert_indices[1]);
        tris[i].distance += phys_mesh.verts_trans(2,tris[i].vert_indices[2]);
        distance_by_index.emplace_back() = tris[i].distance; // store z distance
    }

    //////////////////////////////////////////////////
    /// Sort the culled triangle indices by their z distance
    std::vector<int> index_ordered_by_distance;
    for (auto i: sort_indexes(distance_by_index)) {
        index_ordered_by_distance.emplace_back(i);
    }

   /////////////////////////////////////////////////////
   ///  convert the phys_mesh to the SDL format (render_mesh)
    render_mesh.indices.clear();
    for (int i = 0; i < index_ordered_by_distance.size(); i++) {
        render_mesh.indices.emplace_back() = (phys_mesh.indices_culled[index_ordered_by_distance[i]](0));
        render_mesh.indices.emplace_back() = (phys_mesh.indices_culled[index_ordered_by_distance[i]](1));
        render_mesh.indices.emplace_back() = (phys_mesh.indices_culled[index_ordered_by_distance[i]](2));
    }

}