//
// Created by Gao on 2022/7/6.
//
#include "util.h"

#include <EGL/egl.h>
#include "GLES3/gl31.h"
#include <malloc.h>
#include "my_log.h"
#include "native-lib.h"
#include "opencv2/opencv.hpp"
#include "iostream"

using namespace std;
using namespace cv;

GLuint LoadShader(GLenum type, const char* shaderSrc) {
    GLuint shader;
    GLint compiled;

    shader = glCreateShader(type);

    if (shader == 0) {
        return 0;
    }

    // 加载Shader
    glShaderSource(shader, 1, &shaderSrc, nullptr);
    // 编译Shader
    glCompileShader(shader);
    // 检查编译结果
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char* infoLog = (char *) malloc (sizeof(char) * infoLen);
            glGetShaderInfoLog(shader, infoLen, nullptr, infoLog);
            LOGE("Error compiling shader: \n%s\n", infoLog);
            free(infoLog);
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint CreateProgram(GLuint vShader, GLuint fShader) {
    GLuint program = glCreateProgram();

    glAttachShader(program, vShader);
    glAttachShader(program, fShader);
    glLinkProgram(program);
    glDetachShader(program, vShader);
    glDetachShader(program, fShader);

    GLint result;
    glGetProgramiv(program, GL_LINK_STATUS, &result);
    if (result == GL_FALSE) {
        char* infoLog = (char *) malloc (sizeof(char) * result);
        glGetProgramInfoLog(result, result, nullptr, infoLog);
        LOGE("Create program error : \n%s\n", infoLog);
        glDeleteProgram(program);
        free(infoLog);
    }
    LOGI("Create program success.");
    glDeleteShader(vShader);
    glDeleteShader(fShader);
    return program;
}
