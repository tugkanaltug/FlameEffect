package flame.effect

import android.content.Context
import android.opengl.GLSurfaceView
import android.util.AttributeSet
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

/**
 * OpenGL ES kullanarak Flame efektini çizen GLSurfaceView.
 */
class FlameEffect @JvmOverloads constructor(
    context: Context, attrs: AttributeSet? = null
) : GLSurfaceView(context, attrs) {

    // Native C++ fonksiyonlarını yükle
    init {
        try {
            System.loadLibrary("flame_renderer")
        } catch (e: Exception) {
            e.printStackTrace()
        }

        setEGLContextClientVersion(3) // OpenGL ES 3.0 kullan
        setRenderer(FlameGLRenderer())
        renderMode = RENDERMODE_CONTINUOUSLY
    }

    inner class FlameGLRenderer : Renderer {
        override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
            // C++ başlatma fonksiyonunu çağır
            NativeRenderer.nativeInit()
        }

        override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
            // C++ boyutlandırma fonksiyonunu çağır
            NativeRenderer.nativeResize(width, height)
        }

        override fun onDrawFrame(gl: GL10?) {
            // C++ çizim fonksiyonunu çağır (GPU hesaplamasını tetikler)
            NativeRenderer.nativeRender()
        }
    }
}

// JNI çağrılarını içeren object
object NativeRenderer {
    external fun nativeInit()
    external fun nativeResize(width: Int, height: Int)
    external fun nativeRender()

    external fun nativeShutdown()
}