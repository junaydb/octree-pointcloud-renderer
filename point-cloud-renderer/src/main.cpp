#define MAJOR_VERSION 4
#define MINOR_VERSION 1

#define DEFAULT_WIN_WIDTH 1280
#define DEFAULT_WIN_HEIGHT 720

#define FPS_LIMIT 240
#define FPS_LIMIT_MS 1000.f / FPS_LIMIT

#define VERTEX_SHADER_PATH "./shaders/vertex.glsl"
#define POINTCLOUD_FRAGMENT_SHADER_PATH "./shaders/pc-frag.glsl"
#define BBOX_FRAGMENT_SHADER_PATH "./shaders/bbox-frag.glsl"

#define MEM_ENV_VAR "MAXMEM_MB"

#include <iostream>
#include <sys/resource.h>
#include <sstream>
#include <iomanip>
#include <SDL2/SDL.h>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <shader-compiler/shader-compiler.h>
#include <point-cloud/point-cloud.h>
#include <camera/camera.h>
#include <timer/timer.h>
#include <octree/octree-node.h>
#include <view/view.h>

int main(int argc, char** argv) {
  /*---------------------------------------------------------------------------
    INITIALISATION
    --------------------------------------------------------------------------*/
  // ensure required args were passed
  if (argc < 5) {
    std::cerr << "ERROR: Min points per node required" << std::endl;
    if (argc < 4) std::cerr << "ERROR: Point buffer budget required" << std::endl;
    if (argc < 3) std::cerr << "ERROR: Point frame budget required" << std::endl;
    if (argc < 2) std::cerr << "ERROR: Path to point cloud file required" << std::endl;
    std::cerr << "Usage: PointCloudRenderer [FILE] [POINTS PER FRAME BUDGET] [POINT BUFFER BUDGET] [MIN POINTS PER NODE]" << std::endl;
    exit(EXIT_FAILURE);
  }

  // get args
  // WARN: input validation is not implemented yet, incorrect input types are
  // undefined behaviour
  const std::string filepath = argv[1];
  const unsigned int frameBudget = atoi(argv[2]);
  const unsigned int bufferBudget = atoi(argv[3]);
  const unsigned int minPointsPerNode = atoi(argv[4]);

  bool octreeEnabled = true;
  if (argc == 6) {
    octreeEnabled = atoi(argv[5]);
  }

  // initialise SDL video subsystem
  if (SDL_Init(SDL_INIT_VIDEO)) {
    std::cerr << SDL_GetError() << std::endl;
    exit(EXIT_FAILURE);
  }

  // set relevant OpenGL attributes
  if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, MAJOR_VERSION) ||
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, MINOR_VERSION) ||
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE) ||
      SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) ||
      SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1) ||
      SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16)) {
    std::cerr << SDL_GetError() << std::endl;
    exit(EXIT_FAILURE);
  }

  // create SDL window, set title to loading to indicate loading state
  SDL_Window* window = SDL_CreateWindow("Loading...",
                                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT,
                                        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  if (!window) {
    std::cerr << SDL_GetError() << std::endl;
    exit(EXIT_FAILURE);
  }

  // create OpenGL context and link it to window
  SDL_GLContext glContext = SDL_GL_CreateContext(window);
  if (!glContext) {
    std::cerr << SDL_GetError() << std::endl;
    exit(EXIT_FAILURE);
  }

  // enable immediate updates / disable vertical resync
  if (SDL_GL_SetSwapInterval(0)) {
    std::cerr << SDL_GetError() << std::endl;
    exit(EXIT_FAILURE);
  }

  // load OpenGL functions using Glad
  int version = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
  if (!version) {
    std::cerr << "ERROR: Glad failed to initialise" << std::endl;
    exit(EXIT_FAILURE);
  }

  // print OpenGL info
  std::cout << "Vendor          : " << glGetString(GL_VENDOR) << std::endl;
  std::cout << "Renderer        : " << glGetString(GL_RENDERER) << std::endl;
  std::cout << "OpenGL Version  : " << glGetString(GL_VERSION) << std::endl;
  std::cout << "Shading Language: " << glGetString(GL_SHADING_LANGUAGE_VERSION)
            << std::endl;

  // enable relevant OpenGL state variables
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_MULTISAMPLE);
  glClearColor(0.1f, 0.1f, 0.1f, 1.f);
  glLineWidth(1.0f);

  // initialise view settings
  View view;
  view.width = DEFAULT_WIN_WIDTH;
  view.height = DEFAULT_WIN_HEIGHT;
  view.FOV = 70.f;
  view.Z_NEAR_PLANE = 0.1f;
  view.Z_FAR_PLANE = 1000.f;

  // variables used throughout total program duration
  int liveDebug = 0;
  bool mouseDown = false;
  Timer timer;
  Camera camera;
  OctreeNode octree;

  // load point cloud
  PointCloud pointCloud = PointCloud::build(filepath);

  if (octreeEnabled) {
    // build LOD structure and measure build time
    timer.start();
    octree = OctreeNode::buildOctree(pointCloud, frameBudget, bufferBudget, minPointsPerNode, view);
    timer.end();

    // print LOD structure build time
    float octreeBuildTime = timer.getMS();
    long defaultPrecision = std::cout.precision();
    std::cout.precision(2);
    std::cout << "OCTREE BUILD TIME: " << octreeBuildTime / 1000.f << "s" << std::endl;
    std::cout << "TOTAL NODES: " << octree.getTotalNodes() << std::endl;
    std::cout << "MAX DEPTH: " << octree.getMaxDepth() << std::endl;
    std::cout.precision(defaultPrecision);

    // send points to GPU memory and free from CPU-side memory
    octree.buffer();
    octree.bufferDebug();
    pointCloud.buffers.free();
  } else {
    std::cout << "WARN: OCTREE DISABLED" << std::endl;
    pointCloud.buffer();
    pointCloud.bbox.buffer();
  }

  // update title to indicate loading is complete
  std::string standardTitle = "Point Cloud Renderer";
  SDL_SetWindowTitle(window, standardTitle.c_str());

  /*---------------------------------------------------------------------------
    MODEL-VIEW-PROJECTION
    --------------------------------------------------------------------------*/

  // WARN: glm functions expect radians!

  // get projection matrix
  glm::mat4 projectionMatrix = glm::perspective(glm::radians(view.FOV), ((float)view.width / view.height), view.Z_NEAR_PLANE, view.Z_FAR_PLANE);
  // model to world -> world to view -> view projection
  glm::mat4 mvp = projectionMatrix * camera.getViewMatrix() * pointCloud.getModelMatrix();

  /*---------------------------------------------------------------------------
    SHADERS
    --------------------------------------------------------------------------*/

  // create points shader program
  unsigned int pointsShaderProg = shader::createProgram(VERTEX_SHADER_PATH, POINTCLOUD_FRAGMENT_SHADER_PATH);
  // create bounding box shader program
  unsigned int bboxShaderProg = shader::createProgram(VERTEX_SHADER_PATH, BBOX_FRAGMENT_SHADER_PATH);

  /*---------------------------------------------------------------------------
    GAME LOOP INITIALISATION
    --------------------------------------------------------------------------*/

  // set initial uniform values for point cloud shader
  glUseProgram(pointsShaderProg);
  unsigned int mvpUniLoc = glGetUniformLocation(pointsShaderProg, "MVP");
  unsigned int pointSizeUniLoc = glGetUniformLocation(pointsShaderProg, "pointSize");
  glUniformMatrix4fv(mvpUniLoc, 1, GL_FALSE, glm::value_ptr(mvp));
  glUniform1f(pointSizeUniLoc, pointCloud.getPointSize());

  // set initial uniform values for bbox shader
  glUseProgram(bboxShaderProg);
  glUniformMatrix4fv(mvpUniLoc, 1, GL_FALSE, glm::value_ptr(mvp));

  // set other initial values
  glViewport(0, 0, view.width, view.height);

  /*---------------------------------------------------------------------------
    GAME LOOP
    --------------------------------------------------------------------------*/

  while (true) {
    // force GPU operations to complete for higher time measurement accuracy
    glFinish();

    timer.start();

    /*-------------------------------------------------------------------------
      USER INPUT HANDLING
      ------------------------------------------------------------------------*/

    const Uint8* keyStates = SDL_GetKeyboardState(NULL);
    if (keyStates[SDL_SCANCODE_W]) {
      camera.moveForward();
    }
    if (keyStates[SDL_SCANCODE_A]) {
      camera.strafeLeft();
    }
    if (keyStates[SDL_SCANCODE_S]) {
      camera.moveBackward();
    }
    if (keyStates[SDL_SCANCODE_D]) {
      camera.strafeRight();
    }
    if (keyStates[SDL_SCANCODE_Q]) {
      camera.moveDown();
    }
    if (keyStates[SDL_SCANCODE_E]) {
      camera.moveUp();
    }
    if (keyStates[SDL_SCANCODE_F]) {
      camera.reset();
    }

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          SDL_DestroyWindow(window);
          SDL_Quit();
          return EXIT_SUCCESS;

        case SDL_MOUSEMOTION:
          // if the left mouse button is being held down whilst the mouse is
          // being moved, rotate the point cloud, otherwise move the camera
          if (mouseDown) {
            pointCloud.rotate(event.motion.xrel, event.motion.yrel);
          } else {
            camera.rotate(event.motion.xrel, event.motion.yrel);
          }
          break;

        case SDL_KEYDOWN:
          switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
              // release mouse from the window when escape key is pressed
              camera.mouse.unlock();
              break;
            case SDLK_EQUALS:
              camera.setSpeed(camera.getSpeed() + 0.1f);
              break;
            case SDLK_MINUS:
              camera.setSpeed(camera.getSpeed() - 0.1f);
              break;
            case SDLK_RIGHTBRACKET:
              camera.mouse.setSens(camera.mouse.getSens() + 0.025f);
              break;
            case SDLK_LEFTBRACKET:
              camera.mouse.setSens(camera.mouse.getSens() - 0.025f);
              break;
            case SDLK_i:
              camera.mouse.invert();
              pointCloud.mouse.invert();
              break;
            case SDLK_TAB:
              if (octreeEnabled) {
                liveDebug++;
                liveDebug = liveDebug % 4;
              } else {
                liveDebug = !liveDebug;
              }
              break;
          }
          break;

        case SDL_MOUSEBUTTONDOWN:
          if (event.button.button == SDL_BUTTON_LEFT) {
            // trap mouse in window when the window is left-clicked
            if (!camera.mouse.isLocked()) camera.mouse.lock();
            mouseDown = true;
          }
          break;

        case SDL_MOUSEBUTTONUP:
          if (event.button.button == SDL_BUTTON_LEFT) {
            mouseDown = false;
          }
          break;

        case SDL_MOUSEWHEEL:
          // update point size uniform to update visible point size
          if (event.wheel.y > 0) {
            pointCloud.incrementPointSize();
            glUniform1f(pointSizeUniLoc, pointCloud.getPointSize());
          }
          if (event.wheel.y < 0) {
            pointCloud.decrementPointSize();
            glUniform1f(pointSizeUniLoc, pointCloud.getPointSize());
          }
          break;

        case SDL_WINDOWEVENT:
          // update projection matrix and viewport to account for new window size
          if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
            SDL_GetWindowSize(window, &view.width, &view.height);
            projectionMatrix = glm::perspective(glm::radians(view.FOV), ((float)view.width / (float)view.height), view.Z_NEAR_PLANE, view.Z_FAR_PLANE);
            glViewport(0, 0, view.width, view.height);
          }
          break;
      }
    }

    /*-------------------------------------------------------------------------
      DRAWING
      ------------------------------------------------------------------------*/

    mvp = projectionMatrix * camera.getViewMatrix() * pointCloud.getModelMatrix();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* bounding box drawing */
    if (liveDebug) {
      // enable bounding box shader program
      glUseProgram(bboxShaderProg);
      // update mvp uniform (in bounding box shader program)
      glUniformMatrix4fv(mvpUniLoc, 1, GL_FALSE, glm::value_ptr(mvp));
      if (octreeEnabled) {
        // debug mode 2: draw bounding boxes of nodes being drawn
        if (liveDebug == 2) octree.drawDebug();
        // debug mode 3: draw bounding boxes of all nodes
        if (liveDebug == 3) octree.drawDebugAll();
      } else {
        pointCloud.bbox.draw();
      }
    }

    /* pointcloud drawing */
    // enable point cloud shader program
    glUseProgram(pointsShaderProg);
    // update mvp uniform (in point cloud shader program)
    glUniformMatrix4fv(mvpUniLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    if (octreeEnabled) {
      // draw point cloud
      octree.draw(
          // for checking how far a node is from the camera
          camera.getPosition(),
          // model-view matrix - for syncing node position with GPU
          camera.getViewMatrix() * pointCloud.getModelMatrix());
    } else {
      pointCloud.draw();
    }

    // swap front and back buffer, i.e., show rendered result on screen
    SDL_GL_SwapWindow(window);

    /*-------------------------------------------------------------------------
      FRAMERATE CAP, INPUT SYNC, & PERFORMANCE MEASURING
      ------------------------------------------------------------------------*/

    // force GPU operations to complete for higher time measurement accuracy
    glFinish();

    timer.updateAverages();
    const int avgFPS = timer.getAvgFPS();
    const float avgMS = timer.getAvgMS();

    // get uncapped time measurements
    timer.end();
    const float elapsedMS = timer.getMS();
    const int fps = timer.getFPS();

    // limit FPS using uncapped ms time measurement
    if (elapsedMS < FPS_LIMIT_MS) {
      SDL_Delay(FPS_LIMIT_MS - elapsedMS);
    }

    // get capped time measurement
    timer.end();
    const float elapsedMSCapped = timer.getMS();

    // use capped time measurement to sync input sensitivities
    camera.tickSync(elapsedMSCapped);
    pointCloud.tickSync(elapsedMSCapped);

    // construct debug title
    std::ostringstream os;
    if (octreeEnabled) {
      os << "Points: " << octree.getPointDrawCount()
         << " | Uncapped: " << std::setprecision(2) << fps << "FPS " << elapsedMS
         << "MS | Average: " << avgFPS << "FPS " << avgMS << "MS";
    } else {
      os << "Uncapped: " << std::setprecision(2) << fps << "FPS " << elapsedMS
         << "MS | Average: " << avgFPS << "FPS " << avgMS << "MS";
    }
    std::string debugTitle = os.str();

    // use relevant window title
    if (liveDebug) {
      SDL_SetWindowTitle(window, debugTitle.c_str());
    } else {
      SDL_SetWindowTitle(window, standardTitle.c_str());
    }
  }
}
