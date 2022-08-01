package top.topsea.camera2ndk

import android.opengl.GLSurfaceView
import android.util.Log
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

/**
 * @ProjectName:    NDKOpenGL
 * @Package:        top.topsea.ndkopengl
 * @Description:
 * @Author:         TopSea
 * @AboutAuthor:    https://github.com/TopSea
 * @CreateDate:     2022/7/6 8:42
 **/
class JniRender: GLSurfaceView.Renderer {
    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
        Log.d(TAG, "onSurfaceCreated: ")
        JNINative.initOpengl()
    }

    override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
        Log.d(TAG, "onSurfaceChanged: ")
        JNINative.onViewPortChanged(width, height)
    }

    override fun onDrawFrame(gl: GL10?) {
//        Log.d(TAG, "onDrawFrame: ")
        JNINative.renderFrame()
    }

    fun setRenderFrame(format: Int, data: ByteArray, width: Int, height: Int) {
        Log.d(
            TAG,
            "setRenderFrame() called with: data = [$data], width = [$width], height = [$height]"
        )
        JNINative.jniUpdateFrame(format, data, width, height)
    }
}