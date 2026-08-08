package io.github.ja2stracciatella

import java.nio.file.Files
import org.junit.Assert.assertEquals
import org.junit.Test

class ControllerIniTest {
    @Test
    fun roundTripSanitizesAndClampsConfig() {
        val dir = Files.createTempDirectory("controller-ini").toFile()
        val config = ControllerIni.Config(
            enabled = true,
            layout = "ps5",
            leftStick = "wasd",
            touchpadSens = 9999,
            bindings = mapOf("a" to "mouse:right")
        ).withBinding("back", "not-valid")
        ControllerIni.save(dir, config)

        val loaded = ControllerIni.load(dir)
        assertEquals(true, loaded.enabled)
        assertEquals("ps5", loaded.layout)
        assertEquals("wasd", loaded.leftStick)
        assertEquals(4000, loaded.touchpadSens)
        assertEquals("mouse:right", loaded.binding("a"))
        assertEquals("none", loaded.binding("back"))
        assertEquals("key:return", loaded.binding("start"))
    }

    @Test
    fun missingFileUsesEngineDefaults() {
        val dir = Files.createTempDirectory("controller-ini-empty").toFile()
        val loaded = ControllerIni.load(dir)
        assertEquals(false, loaded.enabled)
        assertEquals("mouse:left", loaded.binding("a"))
        assertEquals("key:escape", loaded.binding("back"))
    }
}
