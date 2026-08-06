package io.github.ja2stracciatella

import android.content.pm.ActivityInfo
import android.os.Bundle
import android.util.Log
import android.view.View
import org.libsdl.app.SDLActivity

open class StracciatellaActivity : SDLActivity() {
    override fun getLibraries(): Array<String?>? {
        return arrayOf(
            "SDL2",
            "ja2"
        )
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        // Default landscape for game (Wildfire 1366x768 / desktop layout)
        requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE
        super.onCreate(savedInstanceState)
    }

    // SDL_SetWindowResizable + empty orientation hint calls setOrientationBis with
    // FULL_SENSOR and flips phone portrait. Keep landscape for wide WF layout.
    override fun setOrientationBis(w: Int, h: Int, resizable: Boolean, hint: String?) {
        Log.v("SDL", "setOrientationBis locked landscape w=$w h=$h resizable=$resizable hint=$hint")
        requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE
    }

    // We suppress deprecation warnings here as our Android SDK minimum version
    // does not have replacements for those APIs yet.
    @Suppress("DEPRECATION")
    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)

        // Set app to fullscreen mode
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
