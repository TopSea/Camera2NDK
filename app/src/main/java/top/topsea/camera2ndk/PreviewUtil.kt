package top.topsea.camera2ndk

import android.graphics.ImageFormat
import android.hardware.camera2.CameraCharacteristics
import android.media.Image
import android.os.Build
import android.util.Log
import androidx.annotation.RequiresApi
import java.lang.Integer.min
import java.nio.Buffer
import java.nio.ByteBuffer
import java.util.*

object PreviewUtil {
    /**
     * 判断相机的 Hardware Level 是否大于等于指定的 Level。
     */
    fun CameraCharacteristics.isHardwareLevelSupported(requiredLevel: Int): Boolean {
        val sortedLevels = intArrayOf(
            CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY,
            CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED,
            CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL_FULL,
            CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL_3
        )
        val deviceLevel = this[CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL]
        if (requiredLevel == deviceLevel) {
            return true
        }
        for (sortedLevel in sortedLevels) {
            if (requiredLevel == sortedLevel) {
                return true
            } else if (deviceLevel == sortedLevel) {
                return false
            }
        }
        return false
    }

    fun YUV_420_888_data(image: Image): ByteArray {
        val imageWidth = image.width
        val imageHeight = image.height
        val planes = image.planes
        val data = ByteArray(imageWidth * imageHeight *
                    ImageFormat.getBitsPerPixel(ImageFormat.YUV_420_888) / 8)
        var offset = 0
        for (plane in planes.indices) {
            val buffer = planes[plane].buffer
            val rowStride = planes[plane].rowStride
            // Experimentally, U and V planes have |pixelStride| = 2, which
            // essentially means they are packed.
            val pixelStride = planes[plane].pixelStride
            val planeWidth = if ((plane == 0)) imageWidth else imageWidth / 2
            val planeHeight = if ((plane == 0)) imageHeight else imageHeight / 2
            if (pixelStride == 1 && rowStride == planeWidth) {
                // Copy whole plane from buffer into |data| at once.
                buffer[data, offset, planeWidth * planeHeight]
                offset += planeWidth * planeHeight
            } else {
                // Copy pixels one by one respecting pixelStride and rowStride.
                val rowData = ByteArray(rowStride)
                for (row in 0 until planeHeight - 1) {
                    buffer[rowData, 0, rowStride]
                    for (col in 0 until planeWidth) {
                        data[offset++] = rowData[col * pixelStride]
                    }
                }
                // Last row is special in some devices and may not contain the full
                // |rowStride| bytes of data.
                // See http://developer.android.com/reference/android/media/Image.Plane.html#getBuffer()
                buffer[rowData, 0, min(rowStride, buffer.remaining())]
                for (col in 0 until planeWidth) {
                    data[offset++] = rowData[col * pixelStride]
                }
            }
        }

//        Log.d(TAG, "YUV_420_888_data: ${data.contentToString()}")
        return data
    }
}