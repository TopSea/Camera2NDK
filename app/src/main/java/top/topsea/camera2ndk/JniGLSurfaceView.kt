package top.topsea.camera2ndk

import android.content.Context
import android.opengl.GLSurfaceView
import java.util.jar.Attributes

/**
 * @ProjectName:    NDKOpenGL
 * @Package:        top.topsea.ndkopengl
 * @Description:
 * @Author:         TopSea
 * @AboutAuthor:    https://github.com/TopSea
 * @CreateDate:     2022/7/6 8:46
 **/
class JniGLSurfaceView(context: Context): GLSurfaceView(context) {

    init {
        initView()
    }

    constructor(context: Context, attributes: Attributes): this(context) {
        initView()
    }

    private fun initView() {
        JNINative.initAssetManger(context.assets)
        setEGLContextClientVersion(3)
    }
}