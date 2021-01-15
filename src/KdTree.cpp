#include <limits.h>
#include <algorithm>
#include <set>

#include "KdTree.hpp"

KdTree::KdTree(std::vector<std::shared_ptr<Model>> models)
{
  std::vector<KdTreeTriangleBuildData> triDataList;
  for (auto& model : models) {
    for (auto& mesh : model->getMeshes()) {
      // Create the model matrix to transform triangles with
      auto modelMatrix = glm::scale(glm::mat4(1.0f), model->scale);
      modelMatrix *= glm::toMat4(model->rotation);
      modelMatrix = glm::translate(modelMatrix, model->position);

      auto tris = mesh->getAllTriangles();
      for (auto& tri: tris) {
        for (int i = 0; i < 3; i++) {
          // Transform the triangle into world space
          glm::vec4 transformedTriangle = modelMatrix * glm::vec4(tri[i].x, tri[i].y, tri[i].z, 1);
          tri[i].x = transformedTriangle.x;
          tri[i].y = transformedTriangle.y;
          tri[i].z = transformedTriangle.z;
        }

        KdTreeTriangleBuildData triData;
        triData.triangle = tri;
        triData.bounds = getBoundingBox(tri);
        triDataList.push_back(triData);
      }
    }
  }
  
  root = new KdTreeNode();
  buildSubtree(root, triDataList, MAX_DEPTH);
}

KdTreeRaycastHit KdTree::raycast(glm::vec3 originPoint, glm::vec3 direction, float maxDistance) {

  KdTreeRaycastHit hit = {};
  hit.distance = INFINITY;
  raycastVisit(root, originPoint, direction, maxDistance, hit);
  return hit;
}

void KdTree::buildSubtree(KdTreeNode* current, std::vector<KdTreeTriangleBuildData> triangles, int depth) {
  if (triangles.size() <= MAX_PRIMITIVES_PER_LEAF || depth == 0) {
    // Allocate a new vector containing the triangles on the heap to ensure that nodes only
    // store a pointer instead of the vector header containing multiple pointers, length etc.
    current->data = new std::vector<KdTreeTriangle>(triangles.size());

    for (int i = 0; i < triangles.size(); i++) {
      (*current->data)[i] = triangles[i].triangle;
    }
    return;
  }

  current->dimension = getLongestDimension(getBoundingBox(triangles));
  current->splitPlane = getMedianInDimension(triangles, current->dimension);

  std::vector<KdTreeTriangleBuildData> leftTriangles;
  std::vector<KdTreeTriangleBuildData> rightTriangles;
  splitTrianglesByPlane(current->dimension, current->splitPlane, triangles, leftTriangles, rightTriangles);

  current->successors[0] = new KdTreeNode();
  buildSubtree(current->successors[0], leftTriangles, depth - 1);
  current->successors[1] = new KdTreeNode();
  buildSubtree(current->successors[1], rightTriangles, depth - 1);
}

void KdTree::raycastVisit(KdTreeNode* current, glm::vec3 originPoint, glm::vec3 direction, float maxDistance, KdTreeRaycastHit& hit) {
  if (current == nullptr)
    return;

  int dimension = current->dimension;
  int first = originPoint[dimension] > current->splitPlane;

  if (current->isLeaf()) {
    auto& trianglesInNode = *current->data;
    for (auto& triangle: trianglesInNode) {
      float t = intersectTriangle(triangle, originPoint, direction, maxDistance);
      if (t >= 0 && t <= hit.distance && t <= maxDistance) {
        hit.triangle = triangle;
        hit.point = originPoint + direction * t;
        hit.distance = t;
      }
    }
  }

  if (direction[dimension] == 0.0f) {
    // line segment parallel to splitting plane, visit near side only
    raycastVisit(current->successors[first], originPoint, direction, maxDistance, hit);
  } else {
    // find t value for intersection
    float t = (current->splitPlane - originPoint[dimension]) / direction[dimension];
    if (0.0f <= t && t < maxDistance) {
      raycastVisit(current->successors[first], originPoint, direction, maxDistance, hit);
      raycastVisit(current->successors[first^1], originPoint, direction, maxDistance, hit);
    } else {
      raycastVisit(current->successors[first], originPoint, direction, maxDistance, hit);
    }
  }
}

void KdTree::getMinMaxInDimension(KdTreeTriangle triangle, int dimension, float& min, float& max) {
  min = triangle[0][dimension];
  max = triangle[0][dimension];
  
  if (triangle[1][dimension] < min)
    min = triangle[1][dimension];
  if (triangle[1][dimension] > max)
    max = triangle[1][dimension];

  if (triangle[2][dimension] < min)
    min = triangle[2][dimension];
  if (triangle[2][dimension] > max)
    max = triangle[2][dimension];
}

void KdTree::getMinMaxInDimension(std::vector<KdTreeTriangleBuildData>& triangles, int dimension, float& min, float& max) {
  for (auto& triangle : triangles) {
    float localMin, localMax;
    getMinMaxInDimension(triangle.triangle, dimension, localMin, localMax);
    if (localMin < min)
      min = localMin;
    if (localMax > max)
      max = localMax;
  }
}

KdTreeBoundingBox KdTree::getBoundingBox(KdTreeTriangle triangle) {
  glm::vec3 min(INFINITY);
  glm::vec3 max(-INFINITY);

  for (int d = 0; d < 3; d++) {
    float localMin = INFINITY;
    float localMax = -INFINITY;
    getMinMaxInDimension(triangle, d, localMin, localMax);
    if (localMin < min[d])
      min[d] = localMin;
    if (localMax > max[d])
      max[d] = localMax;
  }
  return KdTreeBoundingBox::fromMinMax(min, max);
}

KdTreeBoundingBox KdTree::getBoundingBox(std::vector<KdTreeTriangleBuildData>& triangles) {
  glm::vec3 min(INFINITY);
  glm::vec3 max(-INFINITY);

  for (auto& triangle : triangles) {
    if (triangle.bounds.min.x < min.x)
      min.x = triangle.bounds.min.x;
    if (triangle.bounds.min.y < min.y)
      min.y = triangle.bounds.min.y;
    if (triangle.bounds.min.z < min.z)
      min.z = triangle.bounds.min.z;
    if (triangle.bounds.max.x > max.x)
      max.x = triangle.bounds.max.x;
    if (triangle.bounds.max.y > max.y)
      max.y = triangle.bounds.max.y;
    if (triangle.bounds.max.z > max.z)
      max.z = triangle.bounds.max.z;
  }

  return KdTreeBoundingBox::fromMinMax(min, max);
}

std::vector<KdTreeBoundingBox> KdTree::createNodeBoundingBoxes(KdTreeNode* current) {
  if (current->isLeaf()) {

    std::vector<KdTreeTriangleBuildData> triangleData;
    for (auto triangle: (*current->data)) {
      KdTreeTriangleBuildData buildData;
      buildData.triangle = triangle;
      buildData.bounds = getBoundingBox(triangle);
      triangleData.push_back(buildData);
    }
    return std::vector<KdTreeBoundingBox> { getBoundingBox(triangleData) };
  }
  auto leftBoundingBoxes = createNodeBoundingBoxes(current->successors[0]);
  auto rightBoundingBoxes = createNodeBoundingBoxes(current->successors[1]);

  std::vector<KdTreeBoundingBox> combinedBounds;
  combinedBounds.insert(combinedBounds.end(), leftBoundingBoxes.begin(), leftBoundingBoxes.end());
  combinedBounds.insert(combinedBounds.end(), rightBoundingBoxes.begin(), rightBoundingBoxes.end());

  glm::vec3 min(INFINITY);
  glm::vec3 max(-INFINITY);
  for (auto& boundingBox: combinedBounds) {
    if (boundingBox.min.x < min.x)
      min.x = boundingBox.min.x;
    if (boundingBox.min.y < min.y)
      min.y = boundingBox.min.y;
    if (boundingBox.min.z < min.z)
      min.z = boundingBox.min.z;
    if (boundingBox.max.x > max.x)
      max.x = boundingBox.max.x;
    if (boundingBox.max.y > max.y)
      max.y = boundingBox.max.y;
    if (boundingBox.max.z > max.z)
      max.z = boundingBox.max.z;
  }


  combinedBounds.push_back(KdTreeBoundingBox::fromMinMax(min, max));

  // The bounding box of an outer node is the combined bounding box of its inner nodes
  return combinedBounds;
}

int KdTree::getLongestDimension(KdTreeBoundingBox bounds) {
  return bounds.size.x >= bounds.size.y
    ? (bounds.size.x >= bounds.size.z ? 0 : 2)
    : (bounds.size.y >= bounds.size.z ? 1 : 2);
}

float KdTree::getMedianInDimension(std::vector<KdTreeTriangleBuildData>& triangles, int dimension) {
  std::vector<float> valuesInDimension(triangles.size());
  for (int i = 0; i < triangles.size(); i++) {
    valuesInDimension[i] = triangles[i].bounds.center[dimension];
  }

  std::nth_element(valuesInDimension.begin(), valuesInDimension.begin() + valuesInDimension.size() / 2, 
    valuesInDimension.end());
  return valuesInDimension[valuesInDimension.size() / 2];
}

void KdTree::splitTrianglesByPlane(
  int dimension,
  float splitPlane,
  std::vector<KdTreeTriangleBuildData>& inTriangles,
  std::vector<KdTreeTriangleBuildData>& outLeftTriangles,
  std::vector<KdTreeTriangleBuildData>& outRightTriangles
) {
  for (auto triangle: inTriangles) {
    bool isSplitPlaneInBounds = splitPlane < triangle.bounds.max[dimension] && splitPlane > triangle.bounds.min[dimension];

    if (isSplitPlaneInBounds || triangle.bounds.min[dimension] <= splitPlane) {
      outLeftTriangles.push_back(triangle);
    }
    if (isSplitPlaneInBounds || triangle.bounds.max[dimension] >= splitPlane) {
      outRightTriangles.push_back(triangle);
    }
  }
}

float KdTree::intersectTriangle(KdTreeTriangle& triangle, glm::vec3 point, glm::vec3 direction, float maxDistance) {

  // Compute the normal of the triangle
	glm::vec3 AB = triangle[1] - triangle[0];
	glm::vec3 AC = triangle[2] - triangle[0];
	glm::vec3 n = glm::cross(AB, AC);
	n = glm::normalize(n);

	float d = glm::dot(n, triangle[0]);
	float denom = glm::dot(n, direction);
	if (denom == 0) {
		return -1;
	}
	float t = (d - glm::dot(n, point)) / denom;

	// Intersection point with plane
	glm::vec3 Q = point + direction * t;

	// Check if the intersection point on the containing plane
  // is inside the triangle
	glm::vec3 BC = triangle[2] - triangle[1];
	glm::vec3 CA = triangle[0] - triangle[2];
	glm::vec3 AQ = Q - triangle[0];
	glm::vec3 BQ = Q - triangle[1];
	glm::vec3 CQ = Q - triangle[2];

	if (glm::dot(glm::cross(AB, AQ), n) < 0
		|| glm::dot(glm::cross(BC, BQ), n) < 0
    || glm::dot(glm::cross(CA, CQ), n) < 0) {
		return -1; // No intersection
	}

	return t;
}

std::shared_ptr<Model> KdTree::createLineModelForBoundingBoxes(
  VulkanDevice& device,
  std::shared_ptr<PipelineSettings> pipelineSettings, 
  std::shared_ptr<Uniforms<LocalTransform>> uniforms
) {
  std::vector<KdTreeBoundingBox> boundingBoxes = createNodeBoundingBoxes(root);

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  std::vector<glm::vec3> currentVertexPositions;
  uint32_t currentStartIndex = 0;
  for (auto boundingBox : boundingBoxes) {
    currentVertexPositions.clear();

    auto min = boundingBox.min;
    auto max = boundingBox.max;
    
    currentVertexPositions.insert(currentVertexPositions.end(), {
      min,
      glm::vec3(min.x, min.y, max.z),
      glm::vec3(max.x, min.y, max.z),
      glm::vec3(max.x, min.y, min.z),
      glm::vec3(min.x, max.y, max.z),
      glm::vec3(min.x, max.y, min.z),
      glm::vec3(max.x, max.y, min.z),
      max
    });

    for (auto vertexPos : currentVertexPositions) {
      Vertex vert = {};
      vert.pos = vertexPos;
      vertices.push_back(vert);
    }

    indices.insert(indices.end(), {
      currentStartIndex + 0, currentStartIndex + 1,
      currentStartIndex + 1, currentStartIndex + 2,
      currentStartIndex + 2, currentStartIndex + 3,
      currentStartIndex + 3, currentStartIndex + 0,
      currentStartIndex + 0, currentStartIndex + 5,
      currentStartIndex + 5, currentStartIndex + 4,
      currentStartIndex + 4, currentStartIndex + 7,
      currentStartIndex + 7, currentStartIndex + 6,
      currentStartIndex + 6, currentStartIndex + 5,
      currentStartIndex + 6, currentStartIndex + 3,
      currentStartIndex + 7, currentStartIndex + 2
    });
    
    currentStartIndex += 8;
  }

  std::unordered_map<std::string, MeshBoneData> emptyBoneData;
  std::vector<std::shared_ptr<Mesh>> meshes;
  meshes.push_back(std::make_shared<Mesh>(device, vertices, indices, emptyBoneData));

  std::cout << " created bounding box visualization. " << std::endl;

  std::unordered_map<std::string, Animation> emptyAnimations;
  return std::make_shared<Model>(meshes, emptyAnimations, pipelineSettings, uniforms, nullptr, device);

}

std::shared_ptr<Model> KdTree::createHitTriangleModel(
  VulkanDevice& device,
  std::shared_ptr<PipelineSettings> pipelineSettings, 
  std::shared_ptr<Uniforms<LocalTransform>> uniforms
) {
  std::vector<Vertex> vertices = { Vertex(), Vertex(), Vertex() };
  std::vector<uint32_t> indices = { 0, 1, 2 };

  std::unordered_map<std::string, MeshBoneData> emptyBoneData;
  std::vector<std::shared_ptr<Mesh>> meshes;
  meshes.push_back(std::make_shared<Mesh>(device, vertices, indices, emptyBoneData));

  std::unordered_map<std::string, Animation> emptyAnimations;
  return std::make_shared<Model>(meshes, emptyAnimations, pipelineSettings, uniforms, nullptr, device);
}

