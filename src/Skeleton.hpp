#pragma once

#include <vector>
#include <iostream>

#include <glm/glm.hpp>

struct Bone {
    Bone(std::string name, std::shared_ptr<Bone> parent, glm::mat4 transformation)
        : name(name), parent(parent), transformation(transformation) {}
    
    std::string name;
    std::vector<std::shared_ptr<Bone>> children;
    std::weak_ptr<Bone> parent;
    glm::mat4 transformation;
    
    void print(int indent = 0) {
        for (int i = 0; i < indent; i++) {
            std::cout << " ";
        }
        
        std::cout << name << std::endl;
        
        for (const auto& child : children) {
            child->print(indent + 2);
        }
    }
};

class Skeleton {
    
public:
    Skeleton(std::shared_ptr<Bone> root)  : root(root) {}
    
    std::shared_ptr<Bone> getRoot() { return root; }
    
private:
    std::shared_ptr<Bone> root;
    
};
