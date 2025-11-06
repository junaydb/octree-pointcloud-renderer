#include <iostream>
#include <fstream>
#include <shader-compiler/shader-compiler.h>

namespace {
  // returns a bool indicating whether the
  // shader compiled successfully
  bool checkStatus(unsigned int shaderID) {
    int compileStatus;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compileStatus);

    if (!compileStatus) {
      return false;
    }
    return true;
  }

  // outputs the error message from the shader object
  void logShaderError(unsigned int shaderID) {
    int infoLogLength;
    glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);

    char* buffer = new char[infoLogLength];
    int bufferSize;
    glGetShaderInfoLog(shaderID, infoLogLength, &bufferSize, buffer);

    std::cout << buffer << std::endl;

    delete[] buffer;
  }

  // read shader source code from file
  std::string readShaderSrc(const char* shaderSrcPath) {
    std::ifstream file(shaderSrcPath);

    if (!file.is_open()) {
      std::cerr << "Error: Failed to open \"" << shaderSrcPath
                << "\" (incorrect path?)" << std::endl;
      exit(EXIT_FAILURE);
    }

    std::string src;
    std::string line;
    while (std::getline(file, line)) {
      src.append(line + "\n");
    }
    file.close();

    return src;
  }

  // helper function for compiling shaders,
  // returns the handle of the compiled shader object.
  unsigned int compile(GLenum type, const char* shaderSrcPath) {
    unsigned int shaderID;
    if (type == GL_VERTEX_SHADER) {
      shaderID = glCreateShader(GL_VERTEX_SHADER);
    } else if (type == GL_FRAGMENT_SHADER) {
      shaderID = glCreateShader(GL_FRAGMENT_SHADER);
    } else {
      std::cerr << "Error: Invalid GLenum passed to compilerShader()"
                << std::endl;
      return -1;
    }

    std::string src = readShaderSrc(shaderSrcPath);

    // glShaderSource expects array of pointers
    const char* srcs[1]{src.c_str()};

    glShaderSource(shaderID, 1, srcs, nullptr);
    glCompileShader(shaderID);

    if (!checkStatus(shaderID)) {
      if (type == GL_VERTEX_SHADER) {
        std::cerr << "Error: VERTEX SHADER COMPILATION FAILED" << std::endl;
      } else if (type == GL_FRAGMENT_SHADER) {
        std::cerr << "Error: FRAGMENT SHADER COMPILATION FAILED" << std::endl;
      }

      logShaderError(shaderID);
      glDeleteShader(shaderID);

    } else {
      if (type == GL_VERTEX_SHADER) {
        std::cout << "Vertex Shader  : compiled successfully" << std::endl;
      } else if (type == GL_FRAGMENT_SHADER) {
        std::cout << "Fragment Shader: compiled successfully" << std::endl;
      }
      // std::cout << src << std::endl;
    }

    return shaderID;
  }
}  // namespace

namespace shader {
  unsigned int createProgram(const char* vertexShaderSrcPath, const char* fragShaderSrcPath) {
    // compile shaders
    unsigned int vs = compile(GL_VERTEX_SHADER, vertexShaderSrcPath);
    unsigned int fs = compile(GL_FRAGMENT_SHADER, fragShaderSrcPath);

    // create and link program
    unsigned int programObjID = glCreateProgram();
    glAttachShader(programObjID, vs);
    glAttachShader(programObjID, fs);
    glLinkProgram(programObjID);

    // check for link errors, return 0 on error
    int linkStatus;
    glGetProgramiv(programObjID, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE) {
      std::cerr << "Error: PROGRAM LINKING FAILED" << std::endl;
      return 0;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return programObjID;
  }
}  // namespace shader
