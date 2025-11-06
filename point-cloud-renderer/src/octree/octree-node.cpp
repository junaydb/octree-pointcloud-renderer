#include <queue>
#include <algorithm>
#include <octree/octree-node.h>
#include <octree/states/states.h>
#include <view/view.h>

// implementation for this portable `floor` function is taken directly from
// stackoverflow:
// https://stackoverflow.com/a/41871699
// credit to 'chqrlie' on stackoverflow:
// https://stackoverflow.com/users/4593267/chqrlie
double floor_portable(double num) {
  if (num >= LLONG_MAX || num <= LLONG_MIN || num != num) {
    /* handle large values, infinities and nan */
    return num;
  }
  long long n = (long long)num;
  double d = (double)n;
  if (d == num || num >= 0)
    return d;
  else
    return d - 1;
}

// sort by screen-projected-size in descending order
bool compareByScreenProjectedSize(OctreeNode* node1, OctreeNode* node2) {
  return node1->screenProjectedSize > node2->screenProjectedSize;
}

OctreeNode::OctreeNode()
    : children{nullptr},
      activeChildren(0),
      depth(0),
      cellSize(0),
      isBuffered(false),
      isDrawn(false) {
  /* void */
}

OctreeNode::OctreeNode(BoundingBox bbox, unsigned int depth)
    : bbox(bbox),
      activeChildren(0),
      depth(depth),
      cellSize(bbox.getScale() / resolution),
      isBuffered(false),
      isDrawn(false) {
  if (depth > maxDepth) {
    maxDepth = depth;
  }
}

OctreeNode OctreeNode::buildOctree(const PointCloud& pointCloud,
                                   unsigned int pointBudget,
                                   unsigned int bufferBudget,
                                   unsigned int minPointsPerNode,
                                   const View& view) {
  OctreeNode::frameBudget = pointBudget;
  OctreeNode::bufferBudget = bufferBudget;
  OctreeNode::minPointsPerNode = minPointsPerNode;
  OctreeNode::view = view;

  OctreeNode root(pointCloud.bbox, INITIAL_DEPTH);

  // get all point positions and colours
  const glm::vec3* pointPositions = pointCloud.buffers.getPositionBuffer();
  const glm::u8vec3* pointColours = pointCloud.buffers.getColourBuffer();

  // insert all point data into octree
  for (unsigned int i = 0; i < pointCloud.buffers.getNumPoints(); i++) {
    root.insert(&pointPositions[i], &pointColours[i]);
  }

  return root;
}

void OctreeNode::insert(const glm::vec3* position, const glm::u8vec3* colour) {
  // point data is grouped for ease of insertion into grid
  Point point{position, colour};

  // get the 1D projected grid cell index of the point's position in 3D space
  int gridCellHash =
      (floor_portable(position->x / cellSize)) +
      (floor_portable(position->y / cellSize)) * resolution +
      (floor_portable(position->z / cellSize)) * resolution * resolution;

  // if the point falls into an unoccupied grid cell, place it there,
  // else if the number of points in this node is below the min thresh, store
  // this point in the overflow vector,
  // else if both above conditions are true, i.e., all grid cells are occupied
  // and the min tresh is passed, insert the point into the child node it falls
  // into - if the child node does not exist yet, create it.
  if (grid.find(gridCellHash) == grid.end()) {
    grid[gridCellHash] = point;
  } else if (grid.size() + overflowPoints.size() < minPointsPerNode) {
    overflowPoints.push_back(point);
  } else {
    unsigned int childNodeIdx = getChildNodeIndex(point.position);
    if (!states::isActive(&activeChildren, childNodeIdx)) {
      createChildNode(childNodeIdx);
    }
    children[childNodeIdx]->insert(point.position, point.colour);
  }

  // always check if the number of points in in this node has surpassed the min
  // threshold, if it has, insert all points in the overflow vector into child
  // nodes and empty the overflow vector.
  if (grid.size() + overflowPoints.size() > minPointsPerNode) {
    for (Point& point : overflowPoints) {
      unsigned int childNodeIdx = getChildNodeIndex(point.position);
      if (!states::isActive(&activeChildren, childNodeIdx)) {
        createChildNode(childNodeIdx);
      }
      children[childNodeIdx]->insert(point.position, point.colour);
    }
    overflowPoints.clear();
  }
}

unsigned int OctreeNode::getChildNodeIndex(const glm::vec3* position) const {
  glm::vec3 center = bbox.getCenter();

  // 3 bits = 2^3 = 8 combinations for 8 octants:
  // 000 -> bottom left back octant
  // 001 -> bottom left front octant
  // 100 -> bottom right back octant
  // 101 -> bottom right front octant
  // 010 -> upper left back octant
  // 011 -> upper left front octant
  // 110 -> upper right back octant
  // 111 -> upper right front octant
  unsigned int idx = 0;
  if (position->x > center.x) idx |= 4;  // set third bit :  100
  if (position->y > center.y) idx |= 2;  // set second bit:  010
  if (position->z > center.z) idx |= 1;  // set first bit :  001

  return idx;
}

void OctreeNode::createChildNode(unsigned int idx) {
  glm::vec3 min = bbox.getMin();
  glm::vec3 max = bbox.getMax();
  glm::vec3 center = bbox.getCenter();

  glm::vec3 childMin = min;
  glm::vec3 childMax = center;

  // if index is in a right octant
  if (idx & 4) {
    childMin.x = center.x;
    childMax.x = max.x;
  }

  // if index is in an upper octant
  if (idx & 2) {
    childMin.y = center.y;
    childMax.y = max.y;
  }

  // if index is in a front octant
  if (idx & 1) {
    childMin.z = center.z;
    childMax.z = max.z;
  }

  BoundingBox boundingBox(childMin, childMax, false);

  children[idx] = new OctreeNode(boundingBox, depth + 1);

  totalNodes++;

  states::activateState(&activeChildren, idx);
}

// visit every node. during a visit, calculate the node's distance to the camera
// and screen-projected size. if the screen-projected size is above the defined
// minimum screen size, add it to the `collectedNodes` vector, meaning it may
// be drawn this frame.
void OctreeNode::collect(const glm::vec3& cameraPos, const glm::mat4& modelViewMat) {
  // get the position of the node with the model-view matrix applied to sync
  // its CPU position with its GPU position
  glm::vec3 bboxViewPosition = modelViewMat * glm::vec4(bbox.getCenter(), 1);

  distance = glm::distance(cameraPos, bboxViewPosition);
  screenProjectedSize = view.getScreenProjectedSize(bbox.getBoundingSphereRadius(), distance);

  if (screenProjectedSize > MIN_SCREEN_SIZE && depth != 0) {
    collectedNodes.push_back(this);
  }

  // recurse on the node's children
  for (int i = 0; i < 8; i++) {
    if (states::isActive(&activeChildren, i) && children[i]->isBuffered) {
      children[i]->collect(cameraPos, modelViewMat);
    }
  }
}

// PERF: the current drawing strategy uses too many draw calls, needs to be
// optimised using some form of batch-rendering
void OctreeNode::draw(const glm::vec3& cameraPos, const glm::mat4& modelViewMat) {
  // reset these as this is a new frame
  pointDrawCount = 0;
  collectedNodes.clear();

  // the root node (LOD 0) is always drawn
  glBindVertexArray(vao);
  glDrawArrays(GL_POINTS, 0, buffers.getNumPoints());
  isDrawn = true;

  collect(cameraPos, modelViewMat);

  // sort nodes in descending screen-projected-size order (largest nodes on
  // screen to smallest nodes on screen)
  std::sort(collectedNodes.begin(), collectedNodes.end(),
            &compareByScreenProjectedSize);

  // draw collected nodes
  for (OctreeNode* node : collectedNodes) {
    unsigned int nodePointCount = node->buffers.getNumPoints();
    // stop drawing if the point budget will be exceeded
    if (pointDrawCount + nodePointCount > frameBudget) return;

    glBindVertexArray(node->vao);
    glDrawArrays(GL_POINTS, 0, nodePointCount);
    node->isDrawn = true;

    pointDrawCount += nodePointCount;
  }
}

// debug method for visualising individual level
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
      if (states::isActive(&current->activeChildren, i)) {
        queue.push(current->children[i]);
      }
    }
  }
}

// draw a node's bounding box only if its points are drawn
void OctreeNode::drawDebug() {
  if (isDrawn || depth == 0) {
    bbox.draw();
    isDrawn = false;
  }

  for (int i = 0; i < 8; i++) {
    if (states::isActive(&activeChildren, i)) {
      children[i]->drawDebug();
    }
  }
}

// draw all node's bounding boxes
void OctreeNode::drawDebugAll() {
  bbox.draw();

  for (int i = 0; i < 8; i++) {
    if (states::isActive(&activeChildren, i)) {
      children[i]->drawDebugAll();
    }
  }
}

// traverse the octree in breadth-first order and buffer each node that is
// visited, until the buffer budget is hit
void OctreeNode::buffer() {
  std::queue<OctreeNode*> queue;
  queue.push(this);

  while (!queue.empty()) {
    OctreeNode* current = queue.front();
    queue.pop();

    unsigned int nodePointCount = current->buffers.getNumPoints();
    if (pointBufferCount + nodePointCount > bufferBudget) return;
    bufferNode(current);
    pointBufferCount += nodePointCount;

    for (int i = 0; i < 8; i++) {
      if (states::isActive(&current->activeChildren, i)) {
        queue.push(current->children[i]);
      }
    }
  }
}

// allocate GPU buffer for passed in node's points
void OctreeNode::bufferNode(OctreeNode* node) {
  unsigned int bufferSize = node->grid.size() + node->overflowPoints.size();
  glm::vec3* positions = new glm::vec3[bufferSize];
  glm::u8vec3* colours = new glm::u8vec3[bufferSize];

  // construct the buffers for this node's points
  unsigned int idx = 0;
  for (const auto& pair : node->grid) {
    positions[idx] = *(pair.second.position);
    colours[idx] = *(pair.second.colour);
    idx++;
  }
  node->grid.clear();
  if (!node->overflowPoints.empty()) {
    for (const auto& point : node->overflowPoints) {
      positions[idx] = *(point.position);
      colours[idx] = *(point.colour);
      idx++;
    }
    node->overflowPoints.clear();
  }
  node->buffers = Buffers(positions, colours, bufferSize);

  delete[] positions;
  delete[] colours;

  // get and bind the VAO handle for this node so this node can be drawn later
  // by simply rebinding this VAO
  glGenVertexArrays(1, &node->vao);
  glBindVertexArray(node->vao);

  // create buffers for this node's points on the GPU
  // i.e., send this node's points to the GPU
  node->buffers.buffer();

  // mark this node as buffered
  node->isBuffered = true;
}

// send every nodes's bounding box geo data to GPU
void OctreeNode::bufferDebug() {
  bbox.buffer();

  for (int i = 0; i < 8; i++) {
    if (states::isActive(&activeChildren, i)) {
      children[i]->bufferDebug();
    }
  }
}

/* static members & methods */

View OctreeNode::view;
unsigned int OctreeNode::totalNodes = 0;
unsigned int OctreeNode::maxDepth = 0;
unsigned int OctreeNode::pointDrawCount = 0;
unsigned int OctreeNode::frameBudget = 0;
unsigned int OctreeNode::pointBufferCount = 0;
unsigned int OctreeNode::bufferBudget = 0;
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
