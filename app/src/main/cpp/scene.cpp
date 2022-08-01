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
GLuint position;
GLuint texcoord;
static const float VERTICES_COORS[] =
        {
                -1.f, 1.f,
                -1.f, -1.f,
                1.f, 1.f,
                1.f, -1.f
        };

static const float TEXTURE_COORS[] =
        {
                0, 0,
                0, 1,
                1, 0,
                1, 1
        };

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

    uni_y = glGetUniformLocation(program, "s_textureY");
    uni_u = glGetUniformLocation(program, "s_textureU");
    uni_v = glGetUniformLocation(program, "s_textureV");
    position = (GLuint) glGetAttribLocation(program, "position");
    texcoord = (GLuint) glGetAttribLocation(program, "texcoord");
//    m_MVPHandle = glGetUniformLocation(program, "MVP");
}

void SetViewPortSize(int width, int height) {
    glViewport(0, 0, width, height);
}

void Draw() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_CULL_FACE);

    glUseProgram(program);

    glVertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE, 2 * 4, VERTICES_COORS);
    glEnableVertexAttribArray(position);
    glUniform1i(uni_y, 0);
    glUniform1i(uni_u, 1);
    glUniform1i(uni_v, 2);
    glVertexAttribPointer(texcoord, 2, GL_FLOAT, GL_FALSE, 2 * 4, TEXTURE_COORS);
    glEnableVertexAttribArray(texcoord);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
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

    LOGI("ByteFlowRenderContext::update_frame format=%d, width=%d, height=%d, pData=%p",
         format, width, height, buf);
    NativeImage nativeImage;
    nativeImage.format = format;
    nativeImage.width = width;
    nativeImage.height = height;
    nativeImage.ppPlane[0] = buf;

    switch (format)
    {
        case IMAGE_FORMAT_NV12:
        case IMAGE_FORMAT_NV21:
            nativeImage.ppPlane[1] = nativeImage.ppPlane[0] + width * height;
            break;
        case IMAGE_FORMAT_I420:
            nativeImage.ppPlane[1] = nativeImage.ppPlane[0] + width * height;
            nativeImage.ppPlane[2] = nativeImage.ppPlane[1] + width * height / 4;
            break;
        default:
            break;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_YTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, (GLsizei) m_RenderFrame.width,
                 (GLsizei) m_RenderFrame.height, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, m_RenderFrame.ppPlane[0]);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_UTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, (GLsizei) m_RenderFrame.width >> 1,
                 (GLsizei) m_RenderFrame.height >> 1, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, m_RenderFrame.ppPlane[1]);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_VTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, (GLsizei) m_RenderFrame.width >> 1,
                 (GLsizei) m_RenderFrame.height >> 1, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, m_RenderFrame.ppPlane[2]);
}