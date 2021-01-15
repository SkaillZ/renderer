#pragma once

#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <memory>

#include <glm/glm.hpp>

#include "Model.hpp"

typedef std::array<glm::vec3, 3> KdTreeTriangle;

const int MAX_PRIMITIVES_PER_LEAF = 300;
const int MAX_DEPTH = 15;

struct KdTreeBoundingBox {
  glm::vec3 min;
  glm::vec3 max;
  glm::vec3 center;
  glm::vec3 size;

  static KdTreeBoundingBox fromMinMax(glm::vec3 min, glm::vec3 max) {
    KdTreeBoundingBox bounds;
    bounds.min = min;
    bounds.max = max;
    bounds.center = (min + max) * 0.5f;
    bounds.size = max - min;
    return bounds;
  }
};

struct KdTreeTriangleBuildData {
  KdTreeTriangle triangle;
  KdTreeBoundingBox bounds;
};

struct KdTreeNode {
  float splitPlane;
  int dimension;
  
  KdTreeNode* successors[2];
  std::vector<KdTreeTriangle>* data;

  inline bool isLeaf() { return data != nullptr; }
};

struct KdTreeRaycastHit {
  KdTreeTriangle triangle;
  glm::vec3 point;
  float distance;
};

class KdTree {
    
public:
  KdTree(std::vector<std::shared_ptr<Model>> models);

  std::shared_ptr<Model> createLineModelForBoundingBoxes(
    VulkanDevice& device,
    std::shared_ptr<PipelineSettings> pipelineSettings, 
    std::shared_ptr<Uniforms<LocalTransform>> uniforms);
  std::shared_ptr<Model> createHitTriangleModel(
    VulkanDevice& device,
    std::shared_ptr<PipelineSettings> pipelineSettings, 
    std::shared_ptr<Uniforms<LocalTransform>> uniforms);

  KdTreeRaycastHit raycast(glm::vec3 originPoint, glm::vec3 direction, float maxDistance);

private:
  void buildSubtree(KdTreeNode* current, std::vector<KdTreeTriangleBuildData> triangles, int depth);

  void raycastVisit(KdTreeNode* current, glm::vec3 originPoint, glm::vec3 direction, float maxDistance, KdTreeRaycastHit& hit);

  inline void getMinMaxInDimension(KdTreeTriangle triangle, int dimension, float& min, float& max);
  inline void getMinMaxInDimension(std::vector<KdTreeTriangleBuildData>& triangles, int dimension, float& min, float& max);
  inline KdTreeBoundingBox getBoundingBox(KdTreeTriangle triangle);
  inline KdTreeBoundingBox getBoundingBox(std::vector<KdTreeTriangleBuildData>& triangles);
  std::vector<KdTreeBoundingBox> createNodeBoundingBoxes(KdTreeNode* current);

  int getLongestDimension(KdTreeBoundingBox bounds);
  float getMedianInDimension(std::vector<KdTreeTriangleBuildData>& triangles, int dimension);
  void splitTrianglesByPlane(
    int dimension,
    float splitPlane,
    std::vector<KdTreeTriangleBuildData>& inTriangles,
    std::vector<KdTreeTriangleBuildData>& outLeftTriangles,
    std::vector<KdTreeTriangleBuildData>& outRightTriangles);
  float intersectTriangle(KdTreeTriangle& triangle, glm::vec3 point, glm::vec3 direction, float maxDistance);

  KdTreeNode* root;
};
