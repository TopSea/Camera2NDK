//
// Created by Gao on 2022/7/6.
//

#include "scene.h"
#include <EGL/egl.h>
#include "GLES3/gl31.h"
#include "util.h"
#include "native-lib.h"
#include "my_log.h"
#include "ImageDef.h"
#include <ctime>
#include <jni.h>

// 将数据从 cpu 放到 gpu
GLuint program;

GLint uni_y;
GLint uni_u;
GLint uni_v;

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

    uni_y = glGetUniformLocation(program, "s_textureY");
    uni_u = glGetUniformLocation(program, "s_textureU");
    uni_v = glGetUniformLocation(program, "s_textureV");
    glUniform1i(uni_y, 0);
    glUniform1i(uni_u, 1);
    glUniform1i(uni_v, 2);
}

void SetViewPortSize(int width, int height) {
    glViewport(0, 0, width, height);
}

GLuint m_YTextureId;
GLuint m_UTextureId;
GLuint m_VTextureId;
NativeImage m_RenderFrame;

void init() {
    memset(&m_RenderFrame, 0, sizeof(NativeImage));
}

extern "C"
JNIEXPORT void JNICALL
Java_top_topsea_camera2ndk_JNINative_jniUpdateFrame(JNIEnv *env, jobject thiz, jint format,
                                                    jbyteArray data, jint width, jint height) {

    int len = env->GetArrayLength (data);
    auto* buf = new unsigned char[len];
    env->GetByteArrayRegion(data, 0, len, reinterpret_cast<jbyte*>(buf));

//    LOGI("%d", *buf);

//    LOGI("update_frame format=%d, width=%d, height=%d, pData=%p",
//         format, width, height, buf);
//    NativeImage nativeImage;
    m_RenderFrame.format = format;
    m_RenderFrame.width = width;
    m_RenderFrame.height = height;
    m_RenderFrame.ppPlane[0] = buf;

    switch (format)
    {
        case IMAGE_FORMAT_NV12:
        case IMAGE_FORMAT_NV21:
            m_RenderFrame.ppPlane[1] = m_RenderFrame.ppPlane[0] + width * height;
            break;
        case IMAGE_FORMAT_I420:
            m_RenderFrame.ppPlane[1] = m_RenderFrame.ppPlane[0] + width * height;
            m_RenderFrame.ppPlane[2] = m_RenderFrame.ppPlane[1] + width * height / 4;
            break;
        default:
            break;
    }
//    NativeImageUtil::CopyNativeImage(&nativeImage, &m_RenderFrame);
    delete[] buf;
}

bool CreateTextures() {
    LOGI("CreateTextures start");
    GLsizei yWidth = static_cast<GLsizei>(m_RenderFrame.width);
    GLsizei yHeight = static_cast<GLsizei>(m_RenderFrame.height);

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &m_YTextureId);
    glBindTexture(GL_TEXTURE_2D, m_YTextureId);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, yWidth, yHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                 nullptr);
//    glGenerateMipmap(GL_TEXTURE_2D);

    if (!m_YTextureId) {
//        GLUtils::CheckGLError("GLByteFlowRender::CreateTextures Create Y texture");
        return false;
    }

    GLsizei uWidth = static_cast<GLsizei>(m_RenderFrame.width / 2);
    GLsizei uHeight = yHeight / 2;

    glActiveTexture(GL_TEXTURE1);
    glGenTextures(1, &m_UTextureId);
    glBindTexture(GL_TEXTURE_2D, m_UTextureId);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, uWidth, uHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                 nullptr);
//    glGenerateMipmap(GL_TEXTURE_2D);

    if (!m_UTextureId) {
//        GLUtils::CheckGLError("GLByteFlowRender::CreateTextures Create U texture");
        return false;
    }

    auto vWidth = static_cast<GLsizei>(m_RenderFrame.width / 2);
    GLsizei vHeight = (GLsizei) yHeight / 2;

    glActiveTexture(GL_TEXTURE2);
    glGenTextures(1, &m_VTextureId);
    glBindTexture(GL_TEXTURE_2D, m_VTextureId);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, vWidth, vHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                 nullptr);
//    glGenerateMipmap(GL_TEXTURE_2D);

    if (!m_VTextureId) {
//        GLUtils::CheckGLError("GLByteFlowRender::CreateTextures Create V texture");
        return false;
    }

    LOGI("CreateTextures done");
    return true;
}

bool UpdateTextures() {
    if (m_RenderFrame.ppPlane[0] == nullptr) {
        LOGI("::UpdateTextures null");
        return false;
    }

    if (!m_YTextureId && !m_UTextureId && !m_VTextureId && !CreateTextures()) {
        return false;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_YTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, (GLsizei) m_RenderFrame.width,
                 (GLsizei) m_RenderFrame.height, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, m_RenderFrame.ppPlane[0]);
//    glGenerateMipmap(GL_TEXTURE_2D);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_UTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, (GLsizei) m_RenderFrame.width >> 1,
                 (GLsizei) m_RenderFrame.height >> 1, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, m_RenderFrame.ppPlane[1]);
//    glGenerateMipmap(GL_TEXTURE_2D);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_VTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, (GLsizei) m_RenderFrame.width >> 1,
                 (GLsizei) m_RenderFrame.height >> 1, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, m_RenderFrame.ppPlane[2]);
//    glGenerateMipmap(GL_TEXTURE_2D);

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