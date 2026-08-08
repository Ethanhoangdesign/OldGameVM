package io.github.ja2stracciatella

import android.util.Log
import java.io.File
import java.io.IOException

/** Full Android controller.ini reader/writer shared with native GameController.cc. */
object ControllerIni {
    private const val TAG = "ControllerIni"
    private const val FILENAME = "controller.ini"

    val PAD_TOKENS = arrayOf(
        "a", "b", "x", "y", "leftshoulder", "rightshoulder",
        "lefttrigger", "righttrigger", "dpup", "dpdown", "dpleft", "dpright",
        "start", "back"
    )

    val PAD_LABELS = arrayOf(
        "A / Cross", "B / Circle", "X / Square", "Y / Triangle", "Left shoulder", "Right shoulder",
        "Left trigger", "Right trigger", "D-pad up", "D-pad down", "D-pad left", "D-pad right",
        "Start / Options", "Back / Create"
    )

    val STICK_MODES = arrayOf("none", "cursor", "wasd", "arrow")
    val LAYOUTS = arrayOf("xbox", "ps5")
    val TOUCHPAD_MODES = arrayOf("cursor", "button", "none")

    /** Values accepted by native ParseOutSpec, grouped for Android dropdowns. */
    val OUTPUTS = listOf(
        Output("None", "none", "none"),
        Output("Mouse left", "mouse", "mouse:left"),
        Output("Mouse right", "mouse", "mouse:right"),
        Output("Mouse middle", "mouse", "mouse:middle"),
        Output("Wheel up", "wheel", "wheel:up"),
        Output("Wheel down", "wheel", "wheel:down"),
        Output("Nudge up", "motion", "nudge:up"),
        Output("Nudge down", "motion", "nudge:down"),
        Output("Nudge left", "motion", "nudge:left"),
        Output("Nudge right", "motion", "nudge:right"),
        Output("Key Up", "key", "key:up"),
        Output("Key Down", "key", "key:down"),
        Output("Key Left", "key", "key:left"),
        Output("Key Right", "key", "key:right"),
        Output("Enter", "key", "key:return"),
        Output("Escape", "key", "key:escape"),
        Output("Space", "key", "key:space"),
        Output("Tab", "key", "key:tab"),
        Output("Backspace", "key", "key:backspace"),
        Output("Delete", "key", "key:delete"),
        Output("F1", "key", "key:f1"), Output("F2", "key", "key:f2"),
        Output("F3", "key", "key:f3"), Output("F4", "key", "key:f4"),
        Output("F5", "key", "key:f5"), Output("F6", "key", "key:f6"),
        Output("F7", "key", "key:f7"), Output("F8", "key", "key:f8"),
        Output("F9", "key", "key:f9"), Output("F10", "key", "key:f10"),
        Output("F11", "key", "key:f11"), Output("F12", "key", "key:f12")
    ) + ('A'..'Z').map { Output("Key $it", "key", "key:${it.lowercase()}") } +
        ('0'..'9').map { Output("Key $it", "key", "key:$it") }

    data class Output(val label: String, val kind: String, val spec: String)

    data class Config(
        val enabled: Boolean = false,
        val layout: String = "xbox",
        val leftStick: String = "cursor",
        val rightStick: String = "none",
        val touchpad: String = "cursor",
        val touchpadSens: Int = 1100,
        val touchpadOut: String = "none",
        val bindings: Map<String, String> = ControllerIni.defaultBindings()
    ) {
        fun binding(token: String): String = bindings[token] ?: defaultBindings()[token] ?: "none"
        fun withBinding(token: String, spec: String): Config =
            copy(bindings = bindings.toMutableMap().also { it[token] = sanitizeOutput(spec) })
    }

    data class Parsed(val config: Config)

    private fun defaultBindings(): Map<String, String> = linkedMapOf(
        "a" to "mouse:left", "b" to "mouse:right", "x" to "none", "y" to "none",
        "leftshoulder" to "wheel:up", "rightshoulder" to "wheel:down",
        "lefttrigger" to "none", "righttrigger" to "none",
        "dpup" to "key:up", "dpdown" to "key:down", "dpleft" to "key:left", "dpright" to "key:right",
        "start" to "key:return", "back" to "key:escape"
    )

    fun path(filesDir: File): File = File(filesDir, ".ja2/$FILENAME")

    fun load(filesDir: File): Config {
        val values = linkedMapOf<String, String>()
        val f = path(filesDir)
        if (!f.exists()) return Config()
        try {
            f.forEachLine { line ->
                val eq = line.indexOf('=')
                if (eq > 0) values[line.substring(0, eq).trim()] = line.substring(eq + 1).trim()
            }
        } catch (e: IOException) {
            Log.w(TAG, "read ${f.path}: ${e.message}")
            return Config()
        }
        val bindings = defaultBindings().toMutableMap()
        PAD_TOKENS.forEach { token ->
            values[token]?.let { bindings[token] = sanitizeOutput(it) }
        }
        return Config(
            enabled = values["enabled"] == "1",
            layout = if (values["layout"] == "ps5" || values["layout"] == "ps") "ps5" else "xbox",
            leftStick = sanitizeStick(values["left_stick"] ?: "cursor"),
            rightStick = sanitizeStick(values["right_stick"] ?: "none"),
            touchpad = when (values["touchpad"]) {
                "button", "btn", "2" -> "button"
                "none", "off", "0", "empty" -> "none"
                else -> "cursor"
            },
            touchpadSens = values["touchpad_sens"]?.toIntOrNull()?.coerceIn(200, 4000) ?: 1100,
            touchpadOut = sanitizeOutput(values["touchpad_out"] ?: "none"),
            bindings = bindings
        )
    }

    fun save(filesDir: File, config: Config) {
        val f = path(filesDir)
        f.parentFile?.mkdirs()
        val body = buildString {
            append("enabled=").append(if (config.enabled) "1" else "0").append('\n')
            append("layout=").append(if (config.layout == "ps5") "ps5" else "xbox").append('\n')
            append("left_stick=").append(sanitizeStick(config.leftStick)).append('\n')
            append("right_stick=").append(sanitizeStick(config.rightStick)).append('\n')
            append("touchpad=").append(if (config.touchpad in TOUCHPAD_MODES) config.touchpad else "cursor").append('\n')
            append("touchpad_sens=").append(config.touchpadSens.coerceIn(200, 4000)).append('\n')
            append("touchpad_out=").append(sanitizeOutput(config.touchpadOut)).append('\n')
            PAD_TOKENS.forEach { token -> append(token).append('=').append(sanitizeOutput(config.binding(token))).append('\n') }
        }
        try {
            f.writeText(body)
        } catch (e: IOException) {
            Log.e(TAG, "write ${f.path}: ${e.message}")
            throw e
        }
    }

    fun sanitizeStick(value: String): String = when (value.lowercase()) {
        "cursor" -> "cursor"
        "wasd" -> "wasd"
        "arrow", "arrows" -> "arrow"
        else -> "none"
    }

    fun sanitizeOutput(value: String): String {
        val normalized = value.trim().lowercase()
        return if (OUTPUTS.any { it.spec == normalized }) normalized else "none"
    }

    // Compatibility helpers for callers that only need old fields.
    fun loadEnabled(filesDir: File): Boolean = load(filesDir).enabled
    fun loadLeftStick(filesDir: File): String = load(filesDir).leftStick
    fun loadRightStick(filesDir: File): String = load(filesDir).rightStick
    fun save(filesDir: File, enabled: Boolean, leftStick: String = "cursor", rightStick: String = "none") {
        save(filesDir, Config(enabled = enabled, leftStick = leftStick, rightStick = rightStick))
    }
}
