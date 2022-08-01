#include <jni.h>
#include <string>
#include "android/asset_manager_jni.h"
#include "my_log.h"
#include "scene.h"

AAssetManager *aAssetManager = nullptr;

unsigned char *LoadFileContent(const char *path, int &filesSize) {
    unsigned char *fileContent = nullptr;
    filesSize = 0;
    AAsset *asset = AAssetManager_open(aAssetManager, path, AASSET_MODE_UNKNOWN);
    if (asset == nullptr) {
        LOGE("LoadFileContent asset is null, load shader error ");
        return nullptr;
    }
    filesSize = AAsset_getLength(asset);
    // 加一防止数组越界
    fileContent = new unsigned char[filesSize + 1];
    AAsset_read(asset, fileContent, filesSize);
    fileContent[filesSize] = '\0';
    AAsset_close(asset);

    LOGI("LoadFileContent success ...%s", path);
    return fileContent;
}

extern "C"
JNIEXPORT void JNICALL
Java_top_topsea_camera2ndk_JNINative_initAssetManger(JNIEnv *env, jobject thiz, jobject am) {
    aAssetManager = AAssetManager_fromJava(env, am);
}
extern "C"
JNIEXPORT void JNICALL
Java_top_topsea_camera2ndk_JNINative_initOpengl(JNIEnv *env, jobject thiz) {
    Init();
    init();
}

extern "C"
JNIEXPORT void JNICALL
Java_top_topsea_camera2ndk_JNINative_onViewPortChanged(JNIEnv *env, jobject thiz, jint width,
                                                      jint height) {
    SetViewPortSize(width, height);
}

extern "C"
JNIEXPORT void JNICALL
Java_top_topsea_camera2ndk_JNINative_renderFrame(JNIEnv *env, jobject thiz) {
    Draw();
}

float GetFrameTime() {
    static unsigned long long lastTime = 0, currentTime = 0;
    timeval current;
    gettimeofday(&current, nullptr);
    // 将时间转化为毫秒
    currentTime = current.tv_sec * 1000 + current.tv_usec / 1000;
    unsigned long long frameTime = lastTime == 0 ? 0 : currentTime - lastTime;
    lastTime = currentTime;
    return float(frameTime) / 1000.0f;
}

extern "C"
JNIEXPORT void JNICALL
Java_top_topsea_camera2ndk_JNINative_onFrameReady(JNIEnv *env, jobject thiz, jobject image) {

}