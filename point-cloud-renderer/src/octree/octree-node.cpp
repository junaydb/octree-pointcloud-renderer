#include <algorithm>
#include <cmath>
#include <queue>
#include <utility>

#include <octree/octree-node.h>

OctreeNode::OctreeNode()
    : children{nullptr},
      activeChildren(0),
      depth(0),
      cellSize(0),
      screenProjectedSize(0),
      isBuffered(false),
      isDrawn(false),
      vao(0) {
}

OctreeNode::OctreeNode(BoundingBox bbox, unsigned int depth)
    : bbox(bbox),
      children{nullptr},
      activeChildren(0),
      depth(depth),
      cellSize(bbox.getScale() / resolution),
      screenProjectedSize(0),
      isBuffered(false),
      isDrawn(false),
      vao(0) {
  if (depth > maxDepth) {
    maxDepth = depth;
  }
}

OctreeNode::~OctreeNode() {
  deleteChildren();
  if (vao != 0) {
    glDeleteVertexArrays(1, &vao);
  }
}

OctreeNode::OctreeNode(OctreeNode&& other) noexcept
    : buffers(std::move(other.buffers)),
      bbox(other.bbox),
      activeChildren(other.activeChildren),
      depth(other.depth),
      overflowPositions(std::move(other.overflowPositions)),
      overflowColours(std::move(other.overflowColours)),
      grid(std::move(other.grid)),
      cellSize(other.cellSize),
      screenProjectedSize(other.screenProjectedSize),
      isBuffered(other.isBuffered),
      isDrawn(other.isDrawn),
      vao(other.vao) {
  for (int i = 0; i < 8; i++) {
    children[i] = other.children[i];
    other.children[i] = nullptr;
  }
  other.activeChildren = 0;
  other.vao = 0;
}

void OctreeNode::deleteChildren() {
  for (int i = 0; i < 8; i++) {
    delete children[i];
    children[i] = nullptr;
  }
  activeChildren = 0;
}

bool OctreeNode::isChildActive(unsigned int idx) const {
  return (activeChildren & (1 << idx)) != 0;
}

void OctreeNode::activateChild(unsigned int idx) {
  activeChildren |= (1 << idx);
}

bool OctreeNode::compareByScreenProjectedSize(OctreeNode* node1,
                                              OctreeNode* node2) {
  return node1->screenProjectedSize > node2->screenProjectedSize;
}

OctreeNode OctreeNode::buildOctree(const PointCloud& pointCloud,
                                   unsigned int pointBudget,
                                   unsigned int minPointsPerNode,
                                   const View& view) {
  OctreeNode::frameBudget = pointBudget;
  OctreeNode::minPointsPerNode = minPointsPerNode;
  OctreeNode::view = view;

  OctreeNode root(pointCloud.getBoundingBox(), initialDepth);

  const Buffers& buffers = pointCloud.getBuffers();
  const glm::vec3* pointPositions = buffers.getPositionBuffer();
  const glm::u8vec3* pointColours = buffers.getColourBuffer();

  for (unsigned int i = 0; i < buffers.getNumPoints(); i++) {
    root.insert(&pointPositions[i], &pointColours[i]);
  }

  return root;
}

void OctreeNode::insert(const glm::vec3* position, const glm::u8vec3* colour) {
  // spatial hash: project the point's 3D cell position into one integer.
  int gridCellHash =
      (std::floor(position->x / cellSize)) +
      (std::floor(position->y / cellSize)) * resolution +
      (std::floor(position->z / cellSize)) * resolution * resolution;

  if (grid.find(gridCellHash) == grid.end()) {
    grid[gridCellHash] = {*position, *colour};
  } else if (grid.size() + overflowPositions.size() < minPointsPerNode) {
    overflowPositions.push_back(*position);
    overflowColours.push_back(*colour);
  } else {
    unsigned int childNodeIdx = getChildNodeIndex(position);
    if (!isChildActive(childNodeIdx)) {
      createChildNode(childNodeIdx);
    }
    children[childNodeIdx]->insert(position, colour);

    if (!overflowPositions.empty()) {
      for (size_t i = 0; i < overflowPositions.size(); i++) {
        unsigned int childNodeIdx = getChildNodeIndex(&overflowPositions[i]);
        if (!isChildActive(childNodeIdx)) {
          createChildNode(childNodeIdx);
        }
        children[childNodeIdx]->insert(&overflowPositions[i], &overflowColours[i]);
      }
      overflowPositions.clear();
      overflowColours.clear();
    }
  }
}

unsigned int OctreeNode::getChildNodeIndex(const glm::vec3* position) const {
  glm::vec3 center = bbox.getCenter();

  // 3 bits encode the 8 octants: x -> bit 2, y -> bit 1, z -> bit 0.
  unsigned int idx = 0;
  if (position->x > center.x) idx |= 4;
  if (position->y > center.y) idx |= 2;
  if (position->z > center.z) idx |= 1;

  return idx;
}

void OctreeNode::createChildNode(unsigned int idx) {
  glm::vec3 min = bbox.getMin();
  glm::vec3 max = bbox.getMax();
  glm::vec3 center = bbox.getCenter();

  glm::vec3 childMin = min;
  glm::vec3 childMax = center;

  if (idx & 4) {
    childMin.x = center.x;
    childMax.x = max.x;
  }
  if (idx & 2) {
    childMin.y = center.y;
    childMax.y = max.y;
  }
  if (idx & 1) {
    childMin.z = center.z;
    childMax.z = max.z;
  }

  BoundingBox boundingBox(childMin, childMax, false);
  children[idx] = new OctreeNode(boundingBox, depth + 1);
  totalNodes++;
  activateChild(idx);
}

void OctreeNode::collect(const glm::mat4& modelViewMat) {
  // get the position of the node with the model-view matrix applied to sync
  // its CPU position with its GPU position
  glm::vec3 bboxViewPosition = modelViewMat * glm::vec4(bbox.getCenter(), 1);
  float distance = glm::length(bboxViewPosition);
  screenProjectedSize =
      view.getScreenProjectedSize(bbox.getBoundingSphereRadius(), distance);

  if (screenProjectedSize > minScreenSize && depth != 0) {
    collectedNodes.push_back(this);
  }

  for (int i = 0; i < 8; i++) {
    if (isChildActive(i) && children[i]->isBuffered) {
      children[i]->collect(modelViewMat);
    }
  }
}

void OctreeNode::draw(const glm::mat4& modelViewMat) {
  pointDrawCount = 0;
  collectedNodes.clear();

  // the root node (LOD 0) is always drawn
  glBindVertexArray(vao);
  glDrawArrays(GL_POINTS, 0, buffers.getNumPoints());
  isDrawn = true;
  pointDrawCount += buffers.getNumPoints();
  if (pointDrawCount >= frameBudget) return;

  collect(modelViewMat);

  std::sort(collectedNodes.begin(), collectedNodes.end(),
            &compareByScreenProjectedSize);

  for (OctreeNode* node : collectedNodes) {
    unsigned int nodePointCount = node->buffers.getNumPoints();
    if (pointDrawCount + nodePointCount > frameBudget) return;

    glBindVertexArray(node->vao);
    glDrawArrays(GL_POINTS, 0, nodePointCount);
    node->isDrawn = true;

    pointDrawCount += nodePointCount;
  }
}

void OctreeNode::drawLevel(unsigned int level) {
  std::queue<OctreeNode*> queue;
  queue.push(this);

  while (!queue.empty()) {
    OctreeNode* current = queue.front();
    queue.pop();

    if (current->depth == level) {
      glBindVertexArray(current->vao);
      glDrawArrays(GL_POINTS, 0, current->buffers.getNumPoints());
      current->isDrawn = true;
    } else if (current->depth > level) {
      return;
    }

    for (int i = 0; i < 8; i++) {
      if (current->isChildActive(i)) {
        queue.push(current->children[i]);
      }
    }
  }
}

void OctreeNode::drawDebug() {
  if (isDrawn || depth == 0) {
    bbox.draw();
    isDrawn = false;
  }

  for (int i = 0; i < 8; i++) {
    if (isChildActive(i)) {
      children[i]->drawDebug();
    }
  }
}

void OctreeNode::drawDebugAll() {
  bbox.draw();

  for (int i = 0; i < 8; i++) {
    if (isChildActive(i)) {
      children[i]->drawDebugAll();
    }
  }
}

void OctreeNode::buffer() {
  std::queue<OctreeNode*> queue;
  queue.push(this);

  while (!queue.empty()) {
    OctreeNode* current = queue.front();
    queue.pop();

    bufferNode(current);

    for (int i = 0; i < 8; i++) {
      if (current->isChildActive(i)) {
        queue.push(current->children[i]);
      }
    }
  }
}

void OctreeNode::bufferNode(OctreeNode* node) {
  unsigned int bufferSize = node->grid.size() + node->overflowPositions.size();
  glm::vec3* positions = new glm::vec3[bufferSize];
  glm::u8vec3* colours = new glm::u8vec3[bufferSize];

  unsigned int idx = 0;
  for (const auto& pair : node->grid) {
    positions[idx] = pair.second.first;
    colours[idx] = pair.second.second;
    idx++;
  }
  node->grid.clear();

  for (size_t i = 0; i < node->overflowPositions.size(); i++) {
    positions[idx] = node->overflowPositions[i];
    colours[idx] = node->overflowColours[i];
    idx++;
  }
  node->overflowPositions.clear();
  node->overflowColours.clear();

  node->buffers = Buffers(positions, colours, bufferSize);

  delete[] positions;
  delete[] colours;

  glGenVertexArrays(1, &node->vao);
  glBindVertexArray(node->vao);
  node->buffers.uploadToGPU();
  node->isBuffered = true;
}

void OctreeNode::bufferDebug() {
  bbox.buffer();

  for (int i = 0; i < 8; i++) {
    if (isChildActive(i)) {
      children[i]->bufferDebug();
    }
  }
}

/* static members & methods */

View OctreeNode::view;
unsigned int OctreeNode::totalNodes = 1;
unsigned int OctreeNode::maxDepth = 0;
unsigned int OctreeNode::pointDrawCount = 0;
unsigned int OctreeNode::frameBudget = 0;
unsigned int OctreeNode::minPointsPerNode = 0;
std::vector<OctreeNode*> OctreeNode::collectedNodes;

unsigned int OctreeNode::getTotalNodes() {
  return totalNodes;
}

unsigned int OctreeNode::getMaxDepth() {
  return maxDepth;
}

unsigned int OctreeNode::getPointDrawCount() {
  return pointDrawCount;
}
