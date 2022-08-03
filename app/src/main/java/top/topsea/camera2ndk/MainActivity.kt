package top.topsea.camera2ndk

import android.annotation.SuppressLint
import android.content.pm.PackageManager
import android.graphics.ImageFormat
import android.graphics.SurfaceTexture
import android.hardware.camera2.*
import android.media.Image
import android.media.ImageReader
import android.opengl.GLSurfaceView
import android.os.*
import android.util.Log
import android.util.Size
import android.view.Surface
import android.view.TextureView
import androidx.annotation.MainThread
import androidx.annotation.RequiresApi
import androidx.annotation.WorkerThread
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import top.topsea.camera2ndk.PreviewUtil.isHardwareLevelSupported

const val TAG: String = "TopSea:::"
@RequiresApi(Build.VERSION_CODES.M)
class MainActivity : AppCompatActivity(), Handler.Callback {

    companion object {
        private const val REQUEST_PERMISSION_CODE: Int = 1
        private val REQUIRED_PERMISSIONS: Array<String> = arrayOf(
                android.Manifest.permission.CAMERA,
                android.Manifest.permission.WRITE_EXTERNAL_STORAGE
        )
        private const val REQUIRED_SUPPORTED_HARDWARE_LEVEL: Int = CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL_FULL
        private const val MSG_OPEN_CAMERA: Int = 1
        private const val MSG_CREATE_SESSION: Int = 3
        private const val MSG_SET_PREVIEW_SIZE: Int = 5
        private const val MSG_START_PREVIEW: Int = 6
    }

    private val mainHandler: Handler = Handler(Looper.getMainLooper())
    private val cameraManager: CameraManager by lazy { getSystemService(CameraManager::class.java) }
    private var cameraThread: HandlerThread? = null
    private var cameraHandler: Handler? = null
    private var frontCameraId: String? = null
    private var frontCameraCharacteristics: CameraCharacteristics? = null
    private var backCameraId: String? = null
    private var backCameraCharacteristics: CameraCharacteristics? = null
    private var cameraDevice: SettableFuture<CameraDevice>? = null
    private var cameraCharacteristics: SettableFuture<CameraCharacteristics>? = null
    private var captureSession: SettableFuture<CameraCaptureSession>? = null

    private var previewDataImageReader: ImageReader? = null
    private var previewDataSurface: Surface? = null

    @SuppressLint("MissingPermission")
    override fun handleMessage(msg: Message): Boolean {
        when (msg.what) {
            MSG_OPEN_CAMERA -> {
                val cameraId = msg.obj as String
                val cameraStateCallback = CameraStateCallback()
                cameraManager.openCamera(cameraId, cameraStateCallback, mainHandler)
                Log.d(TAG, "Handle message: MSG_OPEN_CAMERA")
            }
            MSG_CREATE_SESSION -> {
                val sessionStateCallback = SessionStateCallback()
                val outputs = mutableListOf<Surface>()
                previewDataSurface?.let {
                    Log.d(TAG, "Handle message: MSG_CREATE_SESSION  5")
                    outputs.add(it)
                }

                cameraDevice?.get()?.createCaptureSession(outputs, sessionStateCallback, mainHandler)
                Log.d(TAG, "Handle message: MSG_CREATE_SESSION")
            }
            MSG_SET_PREVIEW_SIZE -> {
                val cameraCharacteristics = cameraCharacteristics?.get()
                if (cameraCharacteristics != null) {
                    // Get optimal preview size according to the specified max width and max height.
                    val maxWidth = 1440
                    val maxHeight = 1080
                    val previewSize = getOptimalSize(cameraCharacteristics, SurfaceTexture::class.java, maxWidth, maxHeight)!!

                    val imageFormat = ImageFormat.YUV_420_888
                    val streamConfigurationMap = cameraCharacteristics[CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP]
                    if (streamConfigurationMap?.isOutputSupportedFor(imageFormat) == true) {
                        previewDataImageReader = ImageReader.newInstance(previewSize.width, previewSize.height, imageFormat, 3)
                        previewDataImageReader?.setOnImageAvailableListener(OnPreviewDataAvailableListener(), cameraHandler)
                        previewDataSurface = previewDataImageReader?.surface
                        Log.d(TAG, "previewDataImageReader: ${previewDataImageReader == null}")
                    }
                }
                Log.d(TAG, "Handle message: MSG_SET_PREVIEW_SIZE")
            }
            MSG_START_PREVIEW -> {
                val cameraDevice = cameraDevice?.get()
                val captureSession = captureSession?.get()
                if (cameraDevice != null && captureSession != null) {
                    val requestBuilder = cameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW)

                    previewDataSurface?.let {
                        requestBuilder.addTarget(it)
                    }
                    val request = requestBuilder.build()
                    captureSession.setRepeatingRequest(request, RepeatingCaptureStateCallback(), mainHandler)
                }
                Log.d(TAG, "Handle message: MSG_START_PREVIEW")
            }
        }
        return false
    }

    val IMAGE_FORMAT_I420 = 0x04
    val IMAGE_FORMAT_RGBA = 0x08
    private lateinit var jniGLSurfaceView: JniGLSurfaceView
    private lateinit var jniRender: JniRender

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        jniGLSurfaceView = JniGLSurfaceView(this)
        jniRender = JniRender()
        jniGLSurfaceView.setRenderer(jniRender)
        jniGLSurfaceView.renderMode = GLSurfaceView.RENDERMODE_WHEN_DIRTY
        setContentView(jniGLSurfaceView)
        val params = jniGLSurfaceView.layoutParams
        params.height = 1440
        params.width = 1080

        startCameraThread()

        val cameraIdList = cameraManager.cameraIdList
        cameraIdList.forEach { cameraId ->
            val cameraCharacteristics = cameraManager.getCameraCharacteristics(cameraId)
            if (cameraCharacteristics.isHardwareLevelSupported(REQUIRED_SUPPORTED_HARDWARE_LEVEL)) {
                if (cameraCharacteristics[CameraCharacteristics.LENS_FACING] == CameraCharacteristics.LENS_FACING_FRONT) {
                    frontCameraId = cameraId
                    frontCameraCharacteristics = cameraCharacteristics
                } else if (cameraCharacteristics[CameraCharacteristics.LENS_FACING] == CameraCharacteristics.LENS_FACING_BACK) {
                    backCameraId = cameraId
                    backCameraCharacteristics = cameraCharacteristics
                }
            }
        }
    }

    override fun onResume() {
        super.onResume()
        if (checkRequiredPermissions()) {
            openCamera()
            setPreviewSize()
            createSession()
            startPreview()
        }
    }

    override fun onPause() {
        super.onPause()
        stopPreview()
        closeSession()
        closeCamera()
        previewDataImageReader?.close()
    }

    override fun onDestroy() {
        super.onDestroy()
        stopCameraThread()
    }

    private fun startCameraThread() {
        cameraThread = HandlerThread("CameraThread")
        cameraThread!!.start()
        cameraHandler = Handler(cameraThread!!.looper, this)
    }

    private fun stopCameraThread() {
        cameraThread?.quitSafely()
        cameraThread = null
        cameraHandler = null
    }

    @MainThread
    private fun openCamera() {
        // Only open back or front camera.
        val cameraId = backCameraId ?: frontCameraId
        if (cameraId != null) {
            cameraDevice = SettableFuture()
            cameraCharacteristics = SettableFuture()
            cameraHandler?.obtainMessage(MSG_OPEN_CAMERA, cameraId)?.sendToTarget()
        } else {
            throw RuntimeException("Camera id must not be null.")
        }
    }

    @MainThread
    private fun closeCamera() {
        cameraDevice?.get()?.close()
        cameraDevice = null
        cameraCharacteristics = null
        Log.d(TAG, "Handle message: MSG_CLOSE_CAMERA")
    }

    @MainThread
    private fun setPreviewSize() {
        cameraHandler?.obtainMessage(MSG_SET_PREVIEW_SIZE)?.sendToTarget()
    }

    @MainThread
    private fun createSession() {
        captureSession = SettableFuture()
        cameraHandler?.sendEmptyMessage(MSG_CREATE_SESSION)
    }

    @MainThread
    private fun closeSession() {
        captureSession?.get()?.close()
        Log.d(TAG, "Handle message: MSG_CLOSE_SESSION")
    }

    @MainThread
    private fun startPreview() {
        cameraHandler?.sendEmptyMessage(MSG_START_PREVIEW)
    }

    @MainThread
    private fun stopPreview() {
        captureSession?.get()?.stopRepeating()
    }

    @WorkerThread
    private fun getOptimalSize(cameraCharacteristics: CameraCharacteristics, clazz: Class<*>, maxWidth: Int, maxHeight: Int): Size? {
        val aspectRatio = maxWidth.toFloat() / maxHeight
        val streamConfigurationMap = cameraCharacteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
        val supportedSizes = streamConfigurationMap?.getOutputSizes(clazz)
        if (supportedSizes != null) {
            for (size in supportedSizes) {
                if (size.width.toFloat() / size.height == aspectRatio && size.height <= maxHeight && size.width <= maxWidth) {
                    return size
                }
            }
        }
        return null
    }

    private fun getDisplayRotation(cameraCharacteristics: CameraCharacteristics): Int {
        val rotation = windowManager.defaultDisplay.rotation
        val degrees = when (rotation) {
            Surface.ROTATION_0 -> 0
            Surface.ROTATION_90 -> 90
            Surface.ROTATION_180 -> 180
            Surface.ROTATION_270 -> 270
            else -> 0
        }
        val sensorOrientation = cameraCharacteristics[CameraCharacteristics.SENSOR_ORIENTATION]!!
        return if (cameraCharacteristics[CameraCharacteristics.LENS_FACING] == CameraCharacteristics.LENS_FACING_FRONT) {
            (360 - (sensorOrientation + degrees) % 360) % 360
        } else {
            (sensorOrientation - degrees + 360) % 360
        }
    }

    private fun checkRequiredPermissions(): Boolean {
        val deniedPermissions = mutableListOf<String>()
        for (permission in REQUIRED_PERMISSIONS) {
            if (ContextCompat.checkSelfPermission(this, permission) == PackageManager.PERMISSION_DENIED) {
                deniedPermissions.add(permission)
            }
        }
        if (deniedPermissions.isEmpty().not()) {
            requestPermissions(deniedPermissions.toTypedArray(), REQUEST_PERMISSION_CODE)
        }
        return deniedPermissions.isEmpty()
    }

    private inner class CameraStateCallback : CameraDevice.StateCallback() {
        @MainThread
        override fun onOpened(camera: CameraDevice) {
            cameraDevice?.set(camera)
            cameraCharacteristics?.set(when (camera.id) {
                frontCameraId -> frontCameraCharacteristics
                backCameraId -> backCameraCharacteristics
                else -> null
            })
        }

        @MainThread
        override fun onClosed(camera: CameraDevice) {
            cameraDevice = null
            cameraCharacteristics = null
        }

        @MainThread
        override fun onDisconnected(camera: CameraDevice) {
            cameraDevice?.set(camera)
            closeCamera()
        }

        @MainThread
        override fun onError(camera: CameraDevice, error: Int) {
            cameraDevice?.set(camera)
            closeCamera()
        }
    }

    private inner class SessionStateCallback : CameraCaptureSession.StateCallback() {
        @MainThread
        override fun onConfigureFailed(session: CameraCaptureSession) {

        }

        @MainThread
        override fun onConfigured(session: CameraCaptureSession) {
            captureSession?.set(session)
        }

        @MainThread
        override fun onClosed(session: CameraCaptureSession) {
            captureSession = null
        }
    }

    private inner class RepeatingCaptureStateCallback : CameraCaptureSession.CaptureCallback() {
        @MainThread
        override fun onCaptureStarted(session: CameraCaptureSession, request: CaptureRequest, timestamp: Long, frameNumber: Long) {
            super.onCaptureStarted(session, request, timestamp, frameNumber)
        }

        @MainThread
        override fun onCaptureCompleted(session: CameraCaptureSession, request: CaptureRequest, result: TotalCaptureResult) {
            super.onCaptureCompleted(session, request, result)
        }
    }

    private inner class OnPreviewDataAvailableListener : ImageReader.OnImageAvailableListener {

        /**
         * Called every time the preview frame data is available.
         */
        override fun onImageAvailable(imageReader: ImageReader) {
            val image = imageReader.acquireNextImage()
            onPreviewReady(
                PreviewUtil.YUV_420_888_data(image),
                image.width,
                image.height
            )
            image.close()
        }
    }

    fun onPreviewReady(data: ByteArray, width: Int, height: Int) {
//        Log.d(TAG, "onPreviewReady: $width  $height")
        jniRender.setRenderFrame(IMAGE_FORMAT_I420, data, width, height)
        jniGLSurfaceView.requestRender()
    }
}
