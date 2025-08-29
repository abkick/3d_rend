//
// Created by Abby on 8/20/2025.
//

#include "entities.h"

#include <filesystem>
#include <random>
#include<Eigen/StdVector>


//template <typename T>
//std::vector<size_t> sort_indexes(const std::vector<T> &v) {
//    // initialize original index locations
//    std::vector<size_t> idx(v.size());
//    iota(idx.begin(), idx.end(), 0);
//    // sort indexes based on comparing values in v
//    // using std::stable_sort instead of std::sort
//    // to avoid unnecessary index re-orderings
//    // when v contains elements of equal values
//    stable_sort(idx.begin(), idx.end(),
//         [&v](size_t i1, size_t i2) {return v[i1] < v[i2];});
//    return idx;
//}

int Entity::load_obj_mesh(std::string file_name) {
    double mesh_scale = 1;
    path_to_obj =  R"(..\src\mesh\)" + file_name;
    std::cout << "Loading mesh file: " << path_to_obj << std::endl;
    char delimiter = ' ';

    std::vector<Eigen::Vector3d> temp_vertices; // WARNING: Eigen::Vectors in c++ vectors can have issues with alignment if compiling in C++ 17 or prior
    std::vector<Eigen::Vector3d> temp_normals;
    std::ifstream loaded_file;
    loaded_file.open(path_to_obj);
    if (!loaded_file.is_open()) {
        std::cerr << "ERROR: Failed to open file " << path_to_obj << std::endl;
        return 1;
    }

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
                getline(whole_line, line, delimiter);
                xyz[1] = stod(line);
                getline(whole_line, line, delimiter);
                xyz[2] = stod(line);
                temp_vertices.push_back(xyz);
            }
            else if (line == "vn") { //load vertex normals
                getline(whole_line, line, delimiter);
                xyz[0] = stod(line);
                getline(whole_line, line, delimiter);
                xyz[1] = stod(line);
                getline(whole_line, line, delimiter);
                xyz[2] = stod(line);
                temp_normals.push_back(xyz);

            }
            else if (line == "vt") {}
            else if (line == "s") {}
            else if (line == "f") {  //load face, works with faces delimited with "/"
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
            }
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

    for (int i=0; i<render_mesh.verts.size(); i++) {
        render_mesh.verts[i].position.x = static_cast<float>(phys_mesh.verts_trans(0, i));
        render_mesh.verts[i].position.y = static_cast<float>(phys_mesh.verts_trans(1, i));
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


    Eigen::Vector<double, Dynamic> distance_by_index;
    distance_by_index.resize(phys_mesh.indices_culled.size());
    Eigen::Matrix<int, 5, Dynamic> triangle_indices;
    triangle_indices.resize(5,render_mesh.indices.size());

    for (int i=0; i<phys_mesh.indices_culled.size(); i++) {
        triangle_indices(seq(0,3), i) = phys_mesh.indices_culled[i];
        triangle_indices(4,i) = phys_mesh.verts_trans(2, triangle_indices(0,i));  // the z value of each vertex in the index is summed
        triangle_indices(4,i) += phys_mesh.verts_trans(2,triangle_indices(1,i)); // so triangle_indices(4, i) is the summed z_distance of each indexed triangle, I should just make a triangle struct...
        triangle_indices(4,i) += phys_mesh.verts_trans(2,triangle_indices(2,i));
        distance_by_index(i) = triangle_indices(4,i); // store z distance,      oh, crap, i converted my distances to integers.... ok, time to replace with a triangle struct
    }
    std::cout << "triangle dist:" << triangle_indices(4,2) << std::endl;
    //////////////////////////////////////////////////
    /// Sort the culled triangle indices by their z distance


    double temp;
    Eigen::Vector<double, Dynamic> temp_distance_by_index;
    temp_distance_by_index.resizeLike(distance_by_index);
    temp_distance_by_index = distance_by_index;
    std::vector<int> arranged_inds;
    int max_index = 0;
    double max = distance_by_index.maxCoeff(&max_index);
    std::cout << max << std::endl;
    for (int i = 0; i < distance_by_index.size(); i++) {
        int min_index = 0;
        if (max != temp_distance_by_index.minCoeff(&min_index)) {
            arranged_inds.push_back(min_index);
            temp_distance_by_index(min_index) = max + 10;
            std::swap(phys_mesh.indices_culled[i], phys_mesh.indices_culled[min_index]);
        }
        else {
            arranged_inds.push_back(max_index);
        }
    }


   /////////////////////////////////////////////////////
   ///  convert the phys_mesh to the SDL format (render_mesh)
    render_mesh.indices.clear();
    for (int i = 0; i < arranged_inds.size(); i++) {
        render_mesh.indices.emplace_back() = (phys_mesh.indices_culled[i](0));
        render_mesh.indices.emplace_back() = (phys_mesh.indices_culled[i](1));
        render_mesh.indices.emplace_back() = (phys_mesh.indices_culled[i](2));
    }

    std::cout << distance_by_index(arranged_inds).transpose() << "\n";
    for (auto ind : arranged_inds) {
        std::cout << ind << "\t";
    }
    std::cout << std::endl;
}