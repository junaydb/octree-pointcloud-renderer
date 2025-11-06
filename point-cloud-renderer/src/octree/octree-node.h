#pragma once

#define MIN_SCREEN_SIZE 1.f
#define INITIAL_DEPTH 0

#include <vector>
#include <unordered_map>
#include <boundingbox/boundingbox.h>
#include <point-cloud/point-cloud.h>
#include <view/view.h>

struct Point {
  const glm::vec3* position;
  const glm::u8vec3* colour;
};

class OctreeNode {
  friend bool compareByScreenProjectedSize(OctreeNode* node1,
                                           OctreeNode* node2);

 public:
  Buffers buffers;

  OctreeNode();

  static OctreeNode buildOctree(const PointCloud& pointCloud,
                                unsigned int pointBudget,
                                unsigned int bufferBudget,
                                unsigned int minPointsPerNode,
                                const View& view);

  static unsigned int getTotalNodes();
  static unsigned int getMaxDepth();
  static unsigned int getPointDrawCount();

  void insert(const glm::vec3* position, const glm::u8vec3* colour);
  void buffer();
  void draw(const glm::vec3& cameraPos, const glm::mat4& modelViewMat);
  void drawLevel(unsigned int level);
  void bufferDebug();
  void drawDebug();
  void drawDebugAll();

 private:
  BoundingBox bbox;
  OctreeNode* children[8];
  unsigned char activeChildren;  // 0b00000000 --> 8 states, 1 for each octant
  unsigned int depth;
  std::vector<Point> overflowPoints;
  std::unordered_map<int, Point> grid;
  float cellSize;  // size of each cell in the spatial hash grid
  float screenProjectedSize;
  float distance;
  bool isBuffered;
  bool isDrawn;

  unsigned int vao;

  static View view;
  static unsigned int totalNodes;
  static unsigned int maxDepth;
  static unsigned int pointDrawCount;
  static unsigned int frameBudget;
  static unsigned int pointBufferCount;
  static unsigned int bufferBudget;
  static unsigned int minPointsPerNode;
  static std::vector<OctreeNode*> collectedNodes;

  static const unsigned int resolution = 256;
  static constexpr float minNodeSize = 5.f;

  OctreeNode(BoundingBox bbox, unsigned int depth);

  unsigned int getChildNodeIndex(const glm::vec3* position) const;
  void createChildNode(unsigned int idx);
  void collect(const glm::vec3& cameraPos, const glm::mat4& modelViewMat);
  void bufferNode(OctreeNode* node);
  void drawClose(const glm::vec3& cameraPos, const glm::mat4& modelViewMat);
};
