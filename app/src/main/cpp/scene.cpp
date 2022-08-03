//
// Created by Gao on 2022/7/6.
//

#include "scene.h"
#include <EGL/egl.h>
#include "GLES3/gl31.h"
#include "util.h"
#include "native-lib.h"
#include "my_log.h"
#include <ctime>
#include <jni.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/types_c.h>
#include "random"

GLuint program;
GLint uni_texture;

static const float vertices[] = {
     1.f,  1.f, 0.0f, 0.0f, 0.0f,
     1.f, -1.f, 0.0f, 1.0f, 0.0f,
    -1.f, -1.f, 0.0f, 1.0f, 1.0f,
    -1.f,  1.f, 0.0f, 0.0f, 1.0f
};
unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
};
unsigned int VBO, VAO, EBO;

void Init() {
    int fileSize = 0;
    // 从assets目录加载顶点着色器代码
    unsigned char *shaderCode = LoadFileContent("vertexSrc.glsl", fileSize);
    GLuint vsShader = LoadShader(GL_VERTEX_SHADER, (char *) shaderCode);
    delete shaderCode;
    // 从assets目录加载片段着色器代码
    shaderCode = LoadFileContent("fragmentSrc.glsl", fileSize);
    GLuint fsShader = LoadShader(GL_FRAGMENT_SHADER, (char *) shaderCode);
    program = CreateProgram(vsShader, fsShader);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    uni_texture = glGetUniformLocation(program, "uni_texture");
    glUniform1i(uni_texture, 0);
}

void SetViewPortSize(int width, int height) {
    glViewport(0, 0, width, height);
}

GLuint m_texture;
cv::Mat img;
int mWidth, mHeight;

extern "C"
JNIEXPORT void JNICALL
Java_top_topsea_camera2ndk_JNINative_jniUpdateFrame(JNIEnv *env, jobject thiz, jint format,
                                                    jbyteArray data, jint width, jint height) {

    int len = env->GetArrayLength (data);
    auto* buf = new unsigned char[len];
    env->GetByteArrayRegion(data, 0, len, reinterpret_cast<jbyte*>(buf));

    mWidth = width;
    mHeight = height;

    img = cv::Mat((int)(height * 1.5), width, CV_8U);
    img.data = buf;

    cv::cvtColor(img, img, cv::COLOR_YUV2RGB_I420);

    delete[] buf;
}

bool CreateTextures() {
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mWidth, mHeight, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 nullptr);
    glGenerateMipmap(GL_TEXTURE_2D);

    if (!m_texture) {
//        GLUtils::CheckGLError("GLByteFlowRender::CreateTextures Create Y texture");
        return false;
    }

    LOGI("CreateTextures done");
    return true;
}

bool UpdateTextures() {
    if (!m_texture && !CreateTextures()) {
        return false;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.cols,
                 img.rows, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, img.data);

    return true;
}

void Draw() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_CULL_FACE);

    glUseProgram(program);
    UpdateTextures();

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glUseProgram(0);
}