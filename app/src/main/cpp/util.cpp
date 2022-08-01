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
#include "ImageDef.h"

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

GLuint CreateTexture2D(unsigned char *pixelData, int width, int height) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);// 表示图像放大时候，使用线性过滤
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);// 表示图像缩小时候，使用线性过滤
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixelData);//GL_RGBA
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

unsigned char *DecodeBMP(unsigned char *bmpFileData, int &width, int &height) {
    if (0x4D42 == *((unsigned short *) bmpFileData)) { // 数据头是否为0x4D42 判断是否是 24 位的位图,
        // 读格式头
        int pixelDataOffset = *((int *) (bmpFileData + 10));// 取出像素数据在内存块的偏移地址
        width = *((int *) (bmpFileData + 18));
        height = *((int *) (bmpFileData + 22));
        unsigned char *pixelData = bmpFileData + pixelDataOffset;
        LOGI("DecodeBMP success ");
        return pixelData;
    } else {
        // 读格式头
        int pixelDataOffset = *((int *) (bmpFileData + 6));// 取出像素数据在内存块的偏移地址
        width = *((int *) (bmpFileData + 18));
        height = *((int *) (bmpFileData + 22));
        unsigned char *pixelData = bmpFileData + pixelDataOffset;
        LOGI("DecodeBMP success ");
        return pixelData;
    }
}

GLuint CreateTextureFromBMP(const char *bmpPath) {
    int nFileSize = 0;
    unsigned char *bmpFileContent = LoadFileContent(bmpPath, nFileSize);
    if (bmpFileContent == nullptr) {
        return 0;
    }
    int bmpWidth = 0, bmpHeight = 0;
    unsigned char *pixelData = DecodeBMP(bmpFileContent, bmpWidth, bmpHeight);
    if (pixelData == nullptr) {
        delete[] bmpFileContent;
        LOGE("CreateTextureFromBMP error ");
        return 0;
    }

    Mat src(128,128, CV_8UC3);
    src.data = pixelData;
    cvtColor(src, src, COLOR_BGR2RGB);
    for (int i = 0; i < 100; ++i) {
        LOGE("LoadFileContent success ...src.data[i]: %d", (int)src.data[i]);
    }

    GLuint texture = CreateTexture2D(src.data, bmpWidth, bmpHeight);
    delete[] bmpFileContent;
    LOGI("CreateTextureFromBMP success ");
    return texture;
}