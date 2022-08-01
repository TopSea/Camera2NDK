package top.topsea.camera2ndk

import android.content.res.AssetManager
import android.media.Image
import android.util.Log

/**
 * @ProjectName:    NDKOpenGL
 * @Package:        top.topsea.ndkopengl
 * @Description:
 * @Author:         TopSea
 * @AboutAuthor:    https://github.com/TopSea
 * @CreateDate:     2022/7/6 8:18
 **/
object JNINative {
    init {
        System.loadLibrary("camera2ndk")
        Log.d(TAG, "worinima: ")
    }

    external fun initAssetManger(am: AssetManager)

    external fun initOpengl()

    external fun onViewPortChanged(width: Int, height: Int)

    external fun renderFrame()

    external fun jniUpdateFrame(format: Int, data: ByteArray, width: Int, height: Int)

    external fun onFrameReady(image: Image)
}