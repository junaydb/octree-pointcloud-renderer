#pragma once
#include <glad/gl.h>

namespace shader {
  unsigned int createProgram(const char* vertexShaderSrcPath, const char* fragShaderSrcPath);
}
