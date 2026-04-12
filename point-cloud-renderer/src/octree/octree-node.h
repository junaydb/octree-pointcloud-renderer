#pragma once

#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include <boundingbox/boundingbox.h>
#include <buffers/buffers.h>
#include <point-cloud/point-cloud.h>
#include <view/view.h>

class OctreeNode {
 public:
  OctreeNode();
  ~OctreeNode();

  OctreeNode(OctreeNode&& other) noexcept;

  static OctreeNode buildOctree(const PointCloud& pointCloud,
                                unsigned int pointBudget,
                                unsigned int minPointsPerNode,
                                const View& view);

  static unsigned int getTotalNodes();
  static unsigned int getMaxDepth();
  static unsigned int getPointDrawCount();

  void insert(const glm::vec3* position, const glm::u8vec3* colour);
  void buffer();
  void draw(const glm::mat4& modelViewMat);
  void drawLevel(unsigned int level);
  void bufferDebug();
  void drawDebug();
  void drawDebugAll();

 private:
  static constexpr unsigned int initialDepth = 0;
  static constexpr unsigned int resolution = 256;
  static constexpr float minScreenSize = 1.f;

  Buffers buffers;
  BoundingBox bbox;
  OctreeNode* children[8];
  unsigned char activeChildren;  // bitmask: 1 bit per octant
  unsigned int depth;
  std::vector<glm::vec3> overflowPositions;
  std::vector<glm::u8vec3> overflowColours;
  std::unordered_map<int, std::pair<glm::vec3, glm::u8vec3>> grid;
  float cellSize;
  float screenProjectedSize;
  bool isBuffered;
  bool isDrawn;

  unsigned int vao;

  static View view;
  static unsigned int totalNodes;
  static unsigned int maxDepth;
  static unsigned int pointDrawCount;
  static unsigned int frameBudget;
  static unsigned int minPointsPerNode;
  static std::vector<OctreeNode*> collectedNodes;

  OctreeNode(BoundingBox bbox, unsigned int depth);

  bool isChildActive(unsigned int idx) const;
  void activateChild(unsigned int idx);

  unsigned int getChildNodeIndex(const glm::vec3* position) const;
  void createChildNode(unsigned int idx);
  void collect(const glm::mat4& modelViewMat);
  void bufferNode(OctreeNode* node);
  void deleteChildren();

  static bool compareByScreenProjectedSize(OctreeNode* node1, OctreeNode* node2);
};
