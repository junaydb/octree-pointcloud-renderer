#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

#include <SDL2/SDL.h>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <camera/camera.h>
#include <mouse/mouse.h>
#include <octree/octree-node.h>
#include <point-cloud/point-cloud.h>
#include <shader-compiler/shader-compiler.h>
#include <timer/timer.h>
#include <view/view.h>

static constexpr int glMajorVersion = 4;
static constexpr int glMinorVersion = 1;
static constexpr int defaultWinWidth = 1280;
static constexpr int defaultWinHeight = 720;
static constexpr unsigned int defaultMinPointsPerNode = 10000;
static constexpr int fpsLimit = 240;
static constexpr float fpsLimitMS = 1000.f / fpsLimit;
static constexpr const char* vertexShaderPath = "./shaders/vertex.glsl";
static constexpr const char* pcFragShaderPath = "./shaders/pc-frag.glsl";
static constexpr const char* bboxFragShaderPath = "./shaders/bbox-frag.glsl";

int main(int argc, char** argv) {
  // --- initialisation ---
  if (argc < 3 || argc > 5) {
    std::cerr
        << "Usage:\n"
        << "  PointCloudRenderer <FILE> <POINTS PER FRAME BUDGET> "
           "[POINT BUFFER BUDGET] [MIN POINTS PER NODE]\n\n"

        << "Arguments:\n"
        << "  FILE\n"
        << "      Path to the input point cloud file.\n\n"

        << "  POINTS PER FRAME BUDGET\n"
        << "      Maximum number of points to render per frame.\n\n"

        << "  POINT BUFFER BUDGET (optional)\n"
        << "      Maximum number of points that can be loaded into memory.\n"
        << "      If not specified, no limit is applied.\n\n"

        << "  MIN POINTS PER NODE (optional)\n"
        << "      Minimum number of points per octree node.\n"
        << "      Defaults to " << defaultMinPointsPerNode << ".\n"
        << std::endl;
    return EXIT_FAILURE;
  }

  const std::string filepath = argv[1];
  const unsigned int frameBudget = std::stoul(argv[2]);
  const std::optional<unsigned int> bufferBudget =
      argc >= 4 ? std::optional<unsigned int>(std::stoul(argv[3])) : std::nullopt;
  const unsigned int minPointsPerNode =
      argc == 5 ? std::stoul(argv[4]) : defaultMinPointsPerNode;

  if (SDL_Init(SDL_INIT_VIDEO)) {
    std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
    return EXIT_FAILURE;
  }

  if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, glMajorVersion) ||
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, glMinorVersion) ||
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE) ||
      SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) ||
      SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1) ||
      SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16)) {
    std::cerr << "SDL_GL_SetAttribute failed: " << SDL_GetError() << std::endl;
    SDL_Quit();
    return EXIT_FAILURE;
  }

  SDL_Window* window = SDL_CreateWindow("Loading...",
                                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        defaultWinWidth, defaultWinHeight,
                                        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  if (!window) {
    std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
    SDL_Quit();
    return EXIT_FAILURE;
  }

  SDL_GLContext glContext = SDL_GL_CreateContext(window);
  if (!glContext) {
    std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << std::endl;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_FAILURE;
  }

  // disable vsync. This is non-fatal because some platforms/drivers reject it.
  if (SDL_GL_SetSwapInterval(0)) {
    std::cerr << "Warning: could not disable vsync: " << SDL_GetError() << std::endl;
  }

  int version = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
  if (!version) {
    std::cerr << "Failed to initialise OpenGL via Glad" << std::endl;
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_FAILURE;
  }

  std::cout << "Vendor          : " << glGetString(GL_VENDOR) << '\n'
            << "Renderer        : " << glGetString(GL_RENDERER) << '\n'
            << "OpenGL Version  : " << glGetString(GL_VERSION) << '\n'
            << "Shading Language: " << glGetString(GL_SHADING_LANGUAGE_VERSION)
            << std::endl;

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_MULTISAMPLE);
  glClearColor(0.1f, 0.1f, 0.1f, 1.f);
  glLineWidth(1.0f);

  View view;
  view.width = defaultWinWidth;
  view.height = defaultWinHeight;

  int liveDebug = 0;
  bool mouseDown = false;
  float deltaTime = 0.f;
  Timer timer;
  Camera camera;
  Mouse pointCloudMouse(0.02f, true);

  PointCloud pointCloud = PointCloud::build(filepath, bufferBudget);

  timer.start();
  OctreeNode octree = OctreeNode::buildOctree(
      pointCloud, frameBudget, minPointsPerNode, view);
  timer.end();

  const float octreeBuildTime = timer.getMS();
  const auto defaultPrecision = std::cout.precision();
  std::cout.precision(2);
  std::cout << "OCTREE BUILD TIME: " << octreeBuildTime / 1000.f << "s\n"
            << "TOTAL NODES: " << octree.getTotalNodes() << '\n'
            << "MAX DEPTH: " << octree.getMaxDepth() << std::endl;
  std::cout.precision(defaultPrecision);

  octree.buffer();
  octree.bufferDebug();

  const std::string standardTitle = "Point Cloud Renderer";
  SDL_SetWindowTitle(window, standardTitle.c_str());

  // get projection matrix
  glm::mat4 projectionMatrix = glm::perspective(
      glm::radians(view.fov),
      static_cast<float>(view.width) / static_cast<float>(view.height),
      view.zNearPlane, view.zFarPlane);
  // model to world -> world to view -> view projection
  glm::mat4 mvp = projectionMatrix * camera.getViewMatrix() * pointCloud.getModelMatrix();

  unsigned int pointsShaderProg = shader::createProgram(vertexShaderPath, pcFragShaderPath);
  unsigned int bboxShaderProg = shader::createProgram(vertexShaderPath, bboxFragShaderPath);

  glUseProgram(pointsShaderProg);
  unsigned int pcMvpLoc = glGetUniformLocation(pointsShaderProg, "MVP");
  unsigned int pointSizeLoc = glGetUniformLocation(pointsShaderProg, "pointSize");
  glUniformMatrix4fv(pcMvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
  glUniform1f(pointSizeLoc, pointCloud.getPointSize());

  glUseProgram(bboxShaderProg);
  unsigned int bboxMvpLoc = glGetUniformLocation(bboxShaderProg, "MVP");
  glUniformMatrix4fv(bboxMvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

  glViewport(0, 0, view.width, view.height);

  while (true) {
    timer.start();

    // --- input handling ---

    const Uint8* keyStates = SDL_GetKeyboardState(nullptr);
    if (keyStates[SDL_SCANCODE_W]) camera.moveForward();
    if (keyStates[SDL_SCANCODE_A]) camera.strafeLeft();
    if (keyStates[SDL_SCANCODE_S]) camera.moveBackward();
    if (keyStates[SDL_SCANCODE_D]) camera.strafeRight();
    if (keyStates[SDL_SCANCODE_Q]) camera.moveDown();
    if (keyStates[SDL_SCANCODE_E]) camera.moveUp();
    if (keyStates[SDL_SCANCODE_F]) camera.reset();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          SDL_GL_DeleteContext(glContext);
          SDL_DestroyWindow(window);
          SDL_Quit();
          return EXIT_SUCCESS;

        case SDL_MOUSEMOTION:
          // if the left mouse button is being held down whilst the mouse is
          // being moved, rotate the point cloud, otherwise move the camera
          if (mouseDown) {
            pointCloud.rotate(event.motion.xrel, event.motion.yrel,
                              deltaTime, pointCloudMouse.getSens(),
                              pointCloudMouse.isInverted());
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
              camera.mouse.toggleInvert();
              pointCloudMouse.toggleInvert();
              break;
            case SDLK_TAB:
              liveDebug = (liveDebug + 1) % 4;
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
          } else if (event.wheel.y < 0) {
            pointCloud.decrementPointSize();
          }
          glUseProgram(pointsShaderProg);
          glUniform1f(pointSizeLoc, pointCloud.getPointSize());
          break;

        case SDL_WINDOWEVENT:
          // update projection matrix and viewport to account for new window size
          if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
            SDL_GetWindowSize(window, &view.width, &view.height);
            projectionMatrix = glm::perspective(
                glm::radians(view.fov),
                static_cast<float>(view.width) / static_cast<float>(view.height),
                view.zNearPlane, view.zFarPlane);
            glViewport(0, 0, view.width, view.height);
          }
          break;
      }
    }

    // --- drawing ---

    mvp = projectionMatrix * camera.getViewMatrix() * pointCloud.getModelMatrix();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (liveDebug) {
      glUseProgram(bboxShaderProg);
      glUniformMatrix4fv(bboxMvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
      // debug mode 2: draw bounding boxes of nodes being drawn
      if (liveDebug == 2) octree.drawDebug();
      // debug mode 3: draw bounding boxes of all nodes
      if (liveDebug == 3) octree.drawDebugAll();
    }

    glUseProgram(pointsShaderProg);
    glUniformMatrix4fv(pcMvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    octree.draw(
        // model-view matrix - for syncing node position with GPU
        camera.getViewMatrix() * pointCloud.getModelMatrix());

    SDL_GL_SwapWindow(window);

    // --- framerate cap & performance ---

    // force GPU operations to complete for higher time measurement accuracy
    glFinish();
    timer.updateAverages();
    const int avgFPS = timer.getAvgFPS();
    const float avgMS = timer.getAvgMS();

    timer.end();
    const float elapsedMS = timer.getMS();
    const int fps = timer.getFPS();

    if (elapsedMS < fpsLimitMS) {
      SDL_Delay(static_cast<Uint32>(fpsLimitMS - elapsedMS));
    }

    timer.end();
    const float elapsedMSCapped = timer.getMS();
    camera.setDeltaTime(elapsedMSCapped);
    deltaTime = elapsedMSCapped;

    if (liveDebug) {
      std::ostringstream os;
      os << "Points: " << octree.getPointDrawCount()
         << " | Uncapped: " << std::setprecision(2) << fps << "FPS " << elapsedMS
         << "MS | Average: " << avgFPS << "FPS " << avgMS << "MS";
      SDL_SetWindowTitle(window, os.str().c_str());
    } else {
      SDL_SetWindowTitle(window, standardTitle.c_str());
    }
  }
}
