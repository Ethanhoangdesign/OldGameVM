package io.github.ja2stracciatella

import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import kotlinx.serialization.KSerializer
import kotlinx.serialization.Serializable
import kotlinx.serialization.SerializationException
import kotlinx.serialization.descriptors.PrimitiveKind
import kotlinx.serialization.descriptors.PrimitiveSerialDescriptor
import kotlinx.serialization.descriptors.SerialDescriptor
import kotlinx.serialization.encoding.Decoder
import kotlinx.serialization.encoding.Encoder

enum class VanillaVersion(val value: String) {
    DUTCH("DUTCH"),
    ENGLISH("ENGLISH"),
    FRENCH("FRENCH"),
    GERMAN("GERMAN"),
    ITALIAN("ITALIAN"),
    POLISH("POLISH"),
    RUSSIAN("RUSSIAN"),
    RUSSIAN_GOLD("RUSSIAN_GOLD"),
    SIMPLIFIED_CHINESE("SIMPLIFIED_CHINESE");

    fun getLabel(): String {
        return when (this) {
            DUTCH -> "Dutch"
            ENGLISH -> "English"
            FRENCH -> "French"
            GERMAN -> "German"
            ITALIAN -> "Italian"
            POLISH -> "Polish"
            RUSSIAN -> "Russian"
            RUSSIAN_GOLD -> "Russian (Gold)"
            SIMPLIFIED_CHINESE -> "Simplified Chinese"
        }
    }

    companion object {
        val DEFAULT = ENGLISH
    }
}

@Serializable(with = ResolutionSerializer::class)
class Resolution(
    val width: UInt,
    val height: UInt
) {
    fun label(): String = "${width}x${height}"

    companion object {
        /** Base for Detect math (menu/map layout reference). */
        val VANILLA = Resolution(640u, 480u)
        // OGVM desktop default (Launcher.cc defaultResolution)
        val DEFAULT = Resolution(1024u, 768u)

        /**
         * Mobile presets. Desktop RESLIST-LOCK stays 1024+1366 only.
         * 1664x768: ultra-wide mobile — engine UILayout expands sides
         * (STD_SCREEN_X center, map wood grow, bottom pin-right). Height 768.
         * Freeform WxH still allowed via edit fields.
         */
        val PRESETS = listOf(
            Resolution(934u, 480u),  // widescreen at minimum height, narrower than 1280
            Resolution(1366u, 768u), // mobile-recommended 16:9 HD (Big Map)
            Resolution(1280u, 768u), // mobile-recommended 5:3 HD (Big Map)
            Resolution(1024u, 768u),
            Resolution(1280u, 600u), // widescreen at 600 height — same zoom as 800x600
            Resolution(1280u, 480u), // widescreen at minimum height
            Resolution(1024u, 600u), // medium-wide at 600 height — same zoom as 800x600
            Resolution(800u, 600u),
            Resolution(640u, 480u),  // original 4:3
            Resolution(1664u, 768u)  // mobile-only ultra-wide
        )
    }
}


object ResolutionSerializer : KSerializer<Resolution> {
    override val descriptor: SerialDescriptor =
        PrimitiveSerialDescriptor("Resolution", PrimitiveKind.STRING)

    override fun serialize(encoder: Encoder, value: Resolution) {
        val width = value.width.toString()
        val height = value.height.toString()
        encoder.encodeString("${width}x${height}")
    }

    override fun deserialize(decoder: Decoder): Resolution {
        val parts = decoder.decodeString().split("x")
        if (parts.size != 2) {
            throw SerializationException("must be in format 640x480")
        }
        val width: UInt
        val height: UInt
        try {
            width = parts[0].toUInt()
            height = parts[1].toUInt()
        } catch (e: NumberFormatException) {
            throw SerializationException("must be in format 640x480")
        }
        return Resolution(width, height)
    }
}

enum class ScalingQuality(val value: String) {
    LINEAR("LINEAR"),
    NEAR_PERFECT("NEAR_PERFECT"),
    PERFECT("PERFECT");

    fun getLabel(): String {
        return when (this) {
            LINEAR -> "Linear Interpolation"
            NEAR_PERFECT -> "Near perfect with oversampling"
            PERFECT -> "Pixel perfect centered"
        }
    }

    companion object {
        val DEFAULT = LINEAR
    }
}

class ConfigurationModel : ViewModel() {

    val vanillaGameDir = MutableLiveData<String?>()
    val vanillaGameVersion = MutableLiveData(VanillaVersion.DEFAULT)
    val saveGameDir = MutableLiveData<String?>()
    val resolution = MutableLiveData(Resolution.DEFAULT)
    val scalingQuality = MutableLiveData(ScalingQuality.DEFAULT)
    val debug = MutableLiveData(false)
    // OGVM-CONTROLLER — stored in controller.ini, not ja2.json
    val controllerConfig = MutableLiveData(ControllerIni.Config())
    val controllerEnabled = MutableLiveData(false)
    val leftStickMode = MutableLiveData("cursor")
    val rightStickMode = MutableLiveData("none")

    fun setVanillaGameDir(vanillaGameDirSet: String?) {
        vanillaGameDir.value = vanillaGameDirSet
    }

    fun setVanillaGameVersion(version: VanillaVersion) {
        vanillaGameVersion.value = version
    }

    fun setSaveGameDir(saveGameDirSet: String?) {
        saveGameDir.value = saveGameDirSet
    }

    fun setResolution(res: Resolution) {
        resolution.value = res
    }

    fun setScalingQuality(quality: ScalingQuality) {
        scalingQuality.value = quality
    }

    fun setDebug(enabled: Boolean) {
        debug.value = enabled
    }

    fun setControllerConfig(config: ControllerIni.Config) {
        controllerConfig.value = config
        controllerEnabled.value = config.enabled
        leftStickMode.value = config.leftStick
        rightStickMode.value = config.rightStick
    }

    fun updateController(config: ControllerIni.Config) {
        setControllerConfig(config)
    }

    fun setControllerEnabled(enabled: Boolean) {
        controllerEnabled.value = enabled
        controllerConfig.value = (controllerConfig.value ?: ControllerIni.Config()).copy(enabled = enabled)
    }

    fun setLeftStickMode(mode: String) {
        leftStickMode.value = mode
        controllerConfig.value = (controllerConfig.value ?: ControllerIni.Config()).copy(leftStick = ControllerIni.sanitizeStick(mode))
    }

    fun setRightStickMode(mode: String) {
        rightStickMode.value = mode
        controllerConfig.value = (controllerConfig.value ?: ControllerIni.Config()).copy(rightStick = ControllerIni.sanitizeStick(mode))
    }
}
