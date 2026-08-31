package io.github.ja2stracciatella

import android.content.pm.ActivityInfo
import android.os.Bundle
import android.util.Log
import android.view.View
import org.libsdl.app.SDLActivity
import java.io.File

class EZeusActivity : SDLActivity() {
    override fun getLibraries(): Array<String?>? = arrayOf("SDL2", "ezeus")

    override fun getArguments(): Array<String> {
        val externalRoot = File(getExternalFilesDir(null) ?: filesDir, "ezeus")
        val commercialRoot = File(externalRoot, "commercial")
        val supportRoot = File(externalRoot, "support")
        val writableRoot = File(filesDir, "ezeus")
        listOf(commercialRoot, supportRoot, writableRoot).forEach { directory ->
            check(directory.isDirectory || directory.mkdirs()) {
                "Could not create eZeus directory: ${directory.absolutePath}"
            }
        }
        return arrayOf(
            commercialRoot.absolutePath,
            supportRoot.absolutePath,
            writableRoot.absolutePath
        )
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE
        super.onCreate(savedInstanceState)
    }

    override fun setOrientationBis(w: Int, h: Int, resizable: Boolean, hint: String?) {
        Log.v("SDL", "eZeus locked landscape w=$w h=$h resizable=$resizable hint=$hint")
        requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE
    }

    @Suppress("DEPRECATION")
    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus) {
            window.decorView.systemUiVisibility = (View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                or View.SYSTEM_UI_FLAG_FULLSCREEN
                or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                or View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION)
        }
    }
}
