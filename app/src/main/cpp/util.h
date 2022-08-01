//
// Created by Gao on 2022/7/6.
//

#ifndef NDKOPENGL_UTIL_H
#define NDKOPENGL_UTIL_H

#endif //NDKOPENGL_UTIL_H

#include <EGL/egl.h>
#include "GLES3/gl31.h"
#include <malloc.h>
#include "my_log.h"
#include "native-lib.h"

GLuint LoadShader(GLenum type, const char* shaderSrc);
GLuint CreateProgram(GLuint vShader, GLuint fShader);
GLuint CreateTexture2D(unsigned char *pixelData, int width, int height);
unsigned char *DecodeBMP(unsigned char *bmpFileData, int &width, int &height);
GLuint CreateTextureFromBMP(const char *bmpPath);
