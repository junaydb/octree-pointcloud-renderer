#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <shader-compiler/shader-compiler.h>

namespace {

  bool checkCompileStatus(unsigned int shaderID) {
    int compileStatus;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compileStatus);
    return compileStatus != 0;
  }

  std::string getShaderLog(unsigned int shaderID) {
    int logLength;
    glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logLength);

    std::string log(logLength, '\0');
    glGetShaderInfoLog(shaderID, logLength, nullptr, log.data());
    return log;
  }

  std::string readShaderSrc(const char* shaderSrcPath) {
    std::ifstream file(shaderSrcPath);

    if (!file.is_open()) {
      throw std::runtime_error(
          std::string("Failed to open shader file: ") + shaderSrcPath);
    }

    std::ostringstream stream;
    stream << file.rdbuf();
    return stream.str();
  }

  const char* shaderTypeName(GLenum type) {
    switch (type) {
      case GL_VERTEX_SHADER:
        return "Vertex";
      case GL_FRAGMENT_SHADER:
        return "Fragment";
      default:
        return "Unknown";
    }
  }

  unsigned int compile(GLenum type, const char* shaderSrcPath) {
    if (type != GL_VERTEX_SHADER && type != GL_FRAGMENT_SHADER) {
      throw std::invalid_argument("Invalid shader type passed to compile()");
    }

    unsigned int shaderID = glCreateShader(type);
    std::string src = readShaderSrc(shaderSrcPath);
    const char* srcPtr = src.c_str();

    glShaderSource(shaderID, 1, &srcPtr, nullptr);
    glCompileShader(shaderID);

    if (!checkCompileStatus(shaderID)) {
      std::string log = getShaderLog(shaderID);
      glDeleteShader(shaderID);
      throw std::runtime_error(
          std::string(shaderTypeName(type)) + " shader compilation failed:\n" + log);
    }

    std::cout << shaderTypeName(type) << " Shader: compiled successfully"
              << std::endl;
    return shaderID;
  }

}  // namespace

namespace shader {

  unsigned int createProgram(const char* vertexShaderSrcPath,
                             const char* fragShaderSrcPath) {
    unsigned int vs = compile(GL_VERTEX_SHADER, vertexShaderSrcPath);
    unsigned int fs = compile(GL_FRAGMENT_SHADER, fragShaderSrcPath);

    unsigned int programID = glCreateProgram();
    glAttachShader(programID, vs);
    glAttachShader(programID, fs);
    glLinkProgram(programID);

    glDeleteShader(vs);
    glDeleteShader(fs);

    int linkStatus;
    glGetProgramiv(programID, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE) {
      glDeleteProgram(programID);
      throw std::runtime_error("Shader program linking failed");
    }

    return programID;
  }

}  // namespace shader
