package io.github.ja2stracciatella

import android.Manifest
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.content.Intent
import android.content.pm.ActivityInfo
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import androidx.core.content.ContextCompat
import androidx.lifecycle.ViewModelProvider
import com.google.android.material.tabs.TabLayoutMediator
import io.github.ja2stracciatella.databinding.ActivityLauncherBinding
import io.github.ja2stracciatella.ui.main.SectionsPagerAdapter
import kotlinx.serialization.SerializationException
import kotlinx.serialization.decodeFromString
import kotlinx.serialization.encodeToString
import kotlinx.serialization.json.Json
import java.io.File
import java.io.IOException


class LauncherActivity : AppCompatActivity() {
    private lateinit var binding: ActivityLauncherBinding

    private val activityLogTag = "LauncherActivity"
    private val requestPermissionsCode = 1000
    private val jsonFormat = Json {
        prettyPrint = true
    }
    private val ja2JsonFilename = ".ja2/ja2.json"
    private val launcherPreferencesName = "launcher"
    private val selectedGamePreference = "selected_game"
    private val crashChannelId = "game-crashes"
    private val crashNotificationId = 27
    private val notifiedCrashId = "notified_crash_id"
    private lateinit var configurationModel: ConfigurationModel
    private var selectedGame = GameId.JA
    private var tabLayoutMediator: TabLayoutMediator? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        // Portrait + landscape theo cam bien / user (khong khoa).
        // Game activity ep landscape; ve day phai tra free rotate.
        requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_FULL_USER
        super.onCreate(savedInstanceState)

        configurationModel = ViewModelProvider(this)[ConfigurationModel::class.java]
        binding = ActivityLauncherBinding.inflate(layoutInflater)
        val view = binding.root

        loadJA2Json()

        setContentView(view)
        selectedGame = loadSelectedGame()
        setupGameSelector()
        showSelectedGame()
        reportPreviousCrash()

        binding.fab.setOnClickListener {
            startSelectedGame()
        }
    }

    private fun setupGameSelector() {
        val games = GameId.values()
        val labels = listOf(
            getString(R.string.game_selector_ja),
            getString(R.string.game_selector_zeus)
        )
        binding.gameSelector.adapter = ArrayAdapter(
            this,
            R.layout.launcher_spinner_item,
            labels
        ).apply {
            setDropDownViewResource(R.layout.launcher_spinner_dropdown_item)
        }
        binding.gameSelector.setSelection(games.indexOf(selectedGame))
        binding.gameSelector.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                val game = games[position]
                if (game != selectedGame) {
                    selectedGame = game
                    getSharedPreferences(launcherPreferencesName, MODE_PRIVATE)
                        .edit()
                        .putString(selectedGamePreference, game.name)
                        .apply()
                    showSelectedGame()
                }
            }

            override fun onNothingSelected(parent: AdapterView<*>?) = Unit
        }
    }

    private fun loadSelectedGame(): GameId {
        val name = getSharedPreferences(launcherPreferencesName, MODE_PRIVATE)
            .getString(selectedGamePreference, null)
        return GameId.values().firstOrNull { it.name == name } ?: GameId.JA
    }

    private fun showSelectedGame() {
        tabLayoutMediator?.detach()
        binding.viewPager.adapter = SectionsPagerAdapter(this, selectedGame)
        tabLayoutMediator = TabLayoutMediator(binding.tabs, binding.viewPager) { tab, position ->
            tab.text = getString(SectionsPagerAdapter.getTabTitle(selectedGame, position))
        }.also { it.attach() }
        binding.fab.contentDescription = getString(
            when (selectedGame) {
                GameId.JA -> R.string.start_ja_description
                GameId.ZEUS -> R.string.start_zeus_description
            }
        )
    }

    override fun onResume() {
        super.onResume()
        // Sau StracciatellaActivity (sensorLandscape) — mo khoa xoay lai.
        requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_FULL_USER
        reportPreviousCrash()
    }

    private fun reportPreviousCrash() {
        val report = NativeExceptionContainer.readReport(this)
        val signal = NativeExceptionContainer.readSignal(this)
        if (report == null && signal == null && !NativeExceptionContainer.hasUncleanRun(this)) return

        val details = report ?: signal?.let { "kind=native_signal\nsignal=${it.trim()}" }
            ?: "kind=process_death\nmessage=The previous game run ended unexpectedly."
        val id = details.hashCode().toString()
        val prefs = getSharedPreferences(launcherPreferencesName, MODE_PRIVATE)
        if (prefs.getString(notifiedCrashId, null) == id) return

        createCrashChannel()
        val summary = details.lineSequence().firstOrNull { it.startsWith("message=") }
            ?.removePrefix("message=") ?: "Previous game run ended unexpectedly."
        if (Build.VERSION.SDK_INT < 33 ||
            ContextCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS) == PackageManager.PERMISSION_GRANTED) {
            val intent = Intent(this, LauncherActivity::class.java).apply {
                putExtra("open_logs", true)
            }
            val pendingIntent = PendingIntent.getActivity(
                this, crashNotificationId, intent,
                PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
            )
            NotificationManagerCompat.from(this).notify(
                crashNotificationId,
                NotificationCompat.Builder(this, crashChannelId)
                    .setSmallIcon(android.R.drawable.ic_dialog_alert)
                    .setContentTitle("JA2 crash detected")
                    .setContentText(summary.take(100))
                    .setStyle(NotificationCompat.BigTextStyle().bigText("$summary\nOpen Logs to copy the report."))
                    .setContentIntent(pendingIntent)
                    .setAutoCancel(true)
                    .build()
            )
        }
        prefs.edit().putString(notifiedCrashId, id).apply()
        NativeExceptionContainer.clearGameRunning(this)
        Toast.makeText(this, "Previous game crash detected. Check Logs.", Toast.LENGTH_LONG).show()
    }

    private fun createCrashChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            getSystemService(NotificationManager::class.java).createNotificationChannel(
                NotificationChannel(crashChannelId, "Game crashes", NotificationManager.IMPORTANCE_HIGH)
            )
        }
    }

    private fun getPermissionsIfNecessaryForAction(action: () -> Unit) {
        val permissions = arrayOf(
            android.Manifest.permission.READ_EXTERNAL_STORAGE,
            android.Manifest.permission.WRITE_EXTERNAL_STORAGE
        )
        val hasAllPermissions = permissions.all {
            ContextCompat.checkSelfPermission(
                applicationContext,
                android.Manifest.permission.READ_EXTERNAL_STORAGE
            ) == PackageManager.PERMISSION_GRANTED
        }
        if (hasAllPermissions) {
            action()
        } else {
            requestPermissions(permissions, requestPermissionsCode)
        }
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        if (requestCode == requestPermissionsCode) {
            if (grantResults.all { r -> r == PackageManager.PERMISSION_GRANTED }) {
                if (selectedGame == GameId.JA) startJA()
            } else {
                Toast.makeText(
                    this,
                    "Cannot start the game without proper permissions",
                    Toast.LENGTH_SHORT
                ).show()
            }
        }
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
    }

    fun getRecommendedResolution(): Resolution {
        // Detect scales from vanilla 640x480 base (not OGVM default 1024).
        val base = Resolution.VANILLA
        val screenWidth =
            Integer.max(resources.displayMetrics.widthPixels, resources.displayMetrics.heightPixels)
        val screenHeight =
            Integer.min(resources.displayMetrics.widthPixels, resources.displayMetrics.heightPixels)
        val scalingX = screenWidth.toDouble() / base.width.toDouble()
        val scalingY = screenHeight.toDouble() / base.height.toDouble()
        val scaling = java.lang.Double.min(scalingX, scalingY)

        if (configurationModel.scalingQuality.value == ScalingQuality.PERFECT) {
            val scalingInt = scaling.toInt().coerceAtLeast(1)
            val width =
                base.width + ((screenWidth - base.width.toInt() * scalingInt) / scalingInt).toUInt()
            val height =
                base.height + ((screenHeight - base.height.toInt() * scalingInt) / scalingInt).toUInt()
            return Resolution(width - (width % 2u), height - (height % 2u))
        }
        val width =
            base.width + ((screenWidth - base.width.toInt() * scaling) / scaling).toUInt()
        return Resolution(width - (width % 2u), base.height)
    }

    private fun startSelectedGame() {
        when (selectedGame) {
            GameId.JA -> startJA()
            GameId.ZEUS -> startZeus()
        }
    }

    private fun startJA() {
        try {
            getPermissionsIfNecessaryForAction {
                GameDir.checkGameDirectoryForCommonMistakes(
                    this,
                    configurationModel.vanillaGameDir.value
                ) {
                    saveJA2Json()
                    NativeExceptionContainer.resetException()
                    NativeExceptionContainer.markGameRunning(this)
                    startActivity(Intent(this@LauncherActivity, StracciatellaActivity::class.java))
                }
            }
        } catch (e: IOException) {
            val message = "Could not write ${ja2JsonPath}: ${e.message}"
            Log.e(activityLogTag, message)
            Toast.makeText(this, message, Toast.LENGTH_SHORT).show()
        }
    }

    private fun startZeus() {
        val library = File(applicationInfo.nativeLibraryDir, "libezeus.so")
        if (!library.isFile) {
            Toast.makeText(this, R.string.ezeus_library_unavailable, Toast.LENGTH_LONG).show()
            return
        }
        startActivity(Intent(this, EZeusActivity::class.java))
    }

    private val ja2JsonPath: String
        get() {
            return "${applicationContext.filesDir.absolutePath}/$ja2JsonFilename"
        }

    private fun loadJA2Json() {
        try {
            val text = File(ja2JsonPath).readText()
            val json: Ja2Json = jsonFormat.decodeFromString(text)

            configurationModel.setVanillaGameDir(json.vanillaGameDir)
            configurationModel.setSaveGameDir(json.saveGameDir)

            if (json.vanillaGameVersion != null) {
                configurationModel.setVanillaGameVersion(json.vanillaGameVersion)
            } else {
                configurationModel.setVanillaGameVersion(VanillaVersion.DEFAULT)
            }
            if (json.scalingQuality != null) {
                configurationModel.setScalingQuality(json.scalingQuality)
            } else {
                configurationModel.setScalingQuality(ScalingQuality.DEFAULT)
            }
            if (json.resolution != null) {
                configurationModel.setResolution(json.resolution)
            } else {
                // OGVM desktop default when no config
                configurationModel.setResolution(Resolution.DEFAULT)
            }
            if (json.debug != null) {
                configurationModel.setDebug(json.debug)
            } else {
                configurationModel.setDebug(false)
            }
        } catch (e: SerializationException) {
            Log.w(activityLogTag, "Could not decode ja2.json: ${e.message}")
            configurationModel.setVanillaGameVersion(VanillaVersion.ENGLISH)
            configurationModel.setScalingQuality(ScalingQuality.DEFAULT)
            configurationModel.setResolution(Resolution.DEFAULT)
        } catch (e: IOException) {
            Log.w(activityLogTag, "Could not read $ja2JsonPath: ${e.message}")
            configurationModel.setVanillaGameVersion(VanillaVersion.ENGLISH)
            configurationModel.setScalingQuality(ScalingQuality.DEFAULT)
            configurationModel.setResolution(Resolution.DEFAULT)
        }
        loadControllerIni()
    }

    private fun loadControllerIni() {
        configurationModel.setControllerConfig(ControllerIni.load(applicationContext.filesDir))
    }

    private fun saveJA2Json() {
        val json = Ja2Json(
            configurationModel.vanillaGameDir.value,
            configurationModel.vanillaGameVersion.value,
            configurationModel.saveGameDir.value,
            configurationModel.resolution.value,
            configurationModel.scalingQuality.value,
            configurationModel.debug.value
        )
        val parentDir = File(ja2JsonPath).parentFile
        if (parentDir?.exists() != true) {
            parentDir?.mkdirs()
        }
        File(ja2JsonPath).writeText(jsonFormat.encodeToString(json))
        // Same home as ja2.json: filesDir/.ja2/controller.ini
        ControllerIni.save(
            applicationContext.filesDir,
            configurationModel.controllerConfig.value ?: ControllerIni.Config(
                enabled = configurationModel.controllerEnabled.value == true,
                leftStick = configurationModel.leftStickMode.value ?: "cursor",
                rightStick = configurationModel.rightStickMode.value ?: "none"
            )
        )
    }
}
