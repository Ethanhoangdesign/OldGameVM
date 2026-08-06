package io.github.ja2stracciatella

import android.util.Log
import java.io.File
import java.io.IOException

/**
 * OGVM-CONTROLLER Android: engine ([GameController.cc]) reads
 * `<stracciatella home>/controller.ini` — same path as desktop.
 * Home on Android = `filesDir/.ja2/`.
 *
 * Full pad remap UI stays desktop (FLTK). Mobile: enable + defaults + stick modes.
 */
object ControllerIni {
    private const val TAG = "ControllerIni"
    private const val FILENAME = "controller.ini"

    /** Default binds — match desktop [Launcher.cc] kPadDefaultOut. */
    private val DEFAULT_BODY = """
        |enabled=1
        |layout=xbox
        |left_stick=cursor
        |right_stick=none
        |touchpad=cursor
        |touchpad_sens=1100
        |touchpad_out=none
        |a=mouse:left
        |b=mouse:right
        |x=none
        |y=none
        |leftshoulder=wheel:up
        |rightshoulder=wheel:down
        |lefttrigger=none
        |righttrigger=none
        |dpup=key:up
        |dpdown=key:down
        |dpleft=key:left
        |dpright=key:right
        |start=key:return
        |back=key:escape
        |""".trimMargin()

    fun path(filesDir: File): File = File(filesDir, ".ja2/$FILENAME")

    fun loadEnabled(filesDir: File): Boolean {
        val f = path(filesDir)
        if (!f.exists()) return false
        return try {
            f.readLines().any { line ->
                val t = line.trim()
                t == "enabled=1" || t.startsWith("enabled=1")
            }
        } catch (e: IOException) {
            Log.w(TAG, "read ${f.path}: ${e.message}")
            false
        }
    }

    fun loadLeftStick(filesDir: File): String {
        return loadKey(filesDir, "left_stick") ?: "cursor"
    }

    fun loadRightStick(filesDir: File): String {
        return loadKey(filesDir, "right_stick") ?: "none"
    }

    private fun loadKey(filesDir: File, key: String): String? {
        val f = path(filesDir)
        if (!f.exists()) return null
        return try {
            f.readLines().firstNotNullOfOrNull { line ->
                val eq = line.indexOf('=')
                if (eq <= 0) return@firstNotNullOfOrNull null
                val k = line.substring(0, eq).trim()
                if (k != key) return@firstNotNullOfOrNull null
                line.substring(eq + 1).trim().removeSuffix("\r")
            }
        } catch (e: IOException) {
            Log.w(TAG, "read ${f.path}: ${e.message}")
            null
        }
    }

    /**
     * Write full ini. When [enabled] false, keep binds but set enabled=0
     * so engine stays off; re-enable restores same binds.
     */
    fun save(
        filesDir: File,
        enabled: Boolean,
        leftStick: String = "cursor",
        rightStick: String = "none"
    ) {
        val f = path(filesDir)
        f.parentFile?.mkdirs()
        val body = buildString {
            append("enabled=").append(if (enabled) "1" else "0").append('\n')
            append("layout=xbox\n")
            append("left_stick=").append(sanitizeStick(leftStick)).append('\n')
            append("right_stick=").append(sanitizeStick(rightStick)).append('\n')
            // rest of defaults without the header keys
            DEFAULT_BODY.lineSequence().forEach { line ->
                when {
                    line.startsWith("enabled=") -> {}
                    line.startsWith("layout=") -> {}
                    line.startsWith("left_stick=") -> {}
                    line.startsWith("right_stick=") -> {}
                    line.isBlank() -> {}
                    else -> append(line).append('\n')
                }
            }
        }
        try {
            f.writeText(body)
            Log.i(TAG, "wrote ${f.path} enabled=$enabled")
        } catch (e: IOException) {
            Log.e(TAG, "write ${f.path}: ${e.message}")
            throw e
        }
    }

    private fun sanitizeStick(v: String): String {
        return when (v.lowercase()) {
            "cursor", "wasd", "arrow", "arrows", "none" ->
                if (v == "arrows") "arrow" else v.lowercase()
            else -> "none"
        }
    }

    val STICK_MODES = arrayOf("none", "cursor", "wasd", "arrow")
}
