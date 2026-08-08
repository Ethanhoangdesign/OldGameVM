package io.github.ja2stracciatella.ui.main

import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.text.Editable
import android.text.TextWatcher
import android.view.Gravity
import android.view.LayoutInflater
import android.view.View
import android.view.InputDevice
import android.view.ViewGroup
import android.widget.AdapterView
import android.widget.Toast
import android.widget.ArrayAdapter
import android.widget.LinearLayout
import android.widget.SeekBar
import android.widget.Spinner
import android.widget.TextView
import androidx.fragment.app.Fragment
import androidx.lifecycle.ViewModelProvider
import io.github.ja2stracciatella.*
import io.github.ja2stracciatella.databinding.FragmentLauncherSettingsBinding

class SettingsFragment : Fragment() {
    private var _binding: FragmentLauncherSettingsBinding? = null
    private val binding get() = _binding!!
    private lateinit var configurationModel: ConfigurationModel
    private lateinit var scalingQualities: Array<ScalingQuality>
    private val stickModes = ControllerIni.STICK_MODES
    private val layouts = ControllerIni.LAYOUTS
    private var suppressPresetCallback = false
    private var suppressStickCallback = false
    private var suppressControllerCallback = false
    private val bindingRows = mutableListOf<BindingRow>()
    private val deviceHandler = Handler(Looper.getMainLooper())
    private var detectedDeviceId: Int? = null
    private var detectedDeviceName: String? = null
    private val devicePoll = object : Runnable {
        override fun run() {
            refreshControllerStatus(true)
            deviceHandler.postDelayed(this, 500L)
        }
    }

    private data class BindingRow(val token: String, val kind: Spinner, val value: Spinner)

    override fun onCreate(savedInstanceState: Bundle?) {
        configurationModel = ViewModelProvider(requireActivity())[ConfigurationModel::class.java]
        scalingQualities = ScalingQuality.values()
        super.onCreate(savedInstanceState)
    }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View {
        _binding = FragmentLauncherSettingsBinding.inflate(inflater, container, false)
        setupScalingSpinner()
        setupResolutionFields()
        setupResolutionPresets()
        setupDebug()
        setupController()
        refreshControllerStatus(false)
        deviceHandler.post(devicePoll)
        return binding.root
    }

    private fun refreshControllerStatus(showToast: Boolean) {
        val device = InputDevice.getDeviceIds().asSequence()
            .mapNotNull { id -> InputDevice.getDevice(id)?.let { id to it } }
            .firstOrNull { (_, input) ->
                val sources = input.sources
                (sources and InputDevice.SOURCE_CLASS_JOYSTICK) != 0 ||
                    (sources and InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD ||
                    (sources and InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
            }
        val newId = device?.first
        val newName = device?.second?.name ?: ""
        if (newId == detectedDeviceId && newName == detectedDeviceName) return
        val wasConnected = detectedDeviceId != null
        val inserted = !wasConnected && newId != null
        detectedDeviceId = newId
        detectedDeviceName = if (newId == null) null else newName
        when {
            newId != null -> {
                binding.controllerStatusText.text = getString(R.string.controller_status_detected, newName)
                if (inserted && showToast) Toast.makeText(
                    requireContext(), getString(R.string.controller_detected_toast, newName), Toast.LENGTH_SHORT
                ).show()
            }
            wasConnected -> binding.controllerStatusText.setText(R.string.controller_status_disconnected)
            else -> binding.controllerStatusText.setText(R.string.controller_status_none)
        }
    }

    private fun <T> spinnerAdapter(items: List<T>): ArrayAdapter<String> {
        val adapter = ArrayAdapter(requireContext(), R.layout.launcher_spinner_item, items.map { it.toString() })
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        return adapter
    }

    private fun setupScalingSpinner() {
        binding.scalingQualitySpinner.adapter = spinnerAdapter(scalingQualities.map { it.getLabel() })
        configurationModel.scalingQuality.observe(viewLifecycleOwner) { quality ->
            val index = scalingQualities.indexOf(quality)
            if (index >= 0) binding.scalingQualitySpinner.setSelection(index)
        }
        binding.scalingQualitySpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                if (position in scalingQualities.indices) configurationModel.setScalingQuality(scalingQualities[position])
            }
            override fun onNothingSelected(parent: AdapterView<*>?) {}
        }
    }

    private fun setupResolutionFields() {
        configurationModel.resolution.observe(viewLifecycleOwner) { resolution ->
            if (binding.resolutionWidthEdit.text.toString() != resolution.width.toString()) binding.resolutionWidthEdit.setText(resolution.width.toString())
            if (binding.resolutionHeightEdit.text.toString() != resolution.height.toString()) binding.resolutionHeightEdit.setText(resolution.height.toString())
            syncPresetSpinner(resolution)
        }
        binding.resolutionWidthEdit.addTextChangedListener(resolutionWatcher { width, current -> Resolution(width, current.height) })
        binding.resolutionHeightEdit.addTextChangedListener(resolutionWatcher { height, current -> Resolution(current.width, height) })
        binding.resolutionAutoButton.setOnClickListener {
            (requireActivity() as? LauncherActivity)?.let { configurationModel.setResolution(it.getRecommendedResolution()) }
        }
    }

    private fun resolutionWatcher(make: (UInt, Resolution) -> Resolution) = object : TextWatcher {
        override fun afterTextChanged(s: Editable?) {}
        override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) {}
        override fun onTextChanged(s: CharSequence?, start: Int, before: Int, count: Int) {
            if (!s.isNullOrEmpty()) s.toString().toUIntOrNull()?.let { value ->
                val current = configurationModel.resolution.value ?: Resolution.DEFAULT
                val next = make(value, current)
                if (next != current) configurationModel.setResolution(next)
            }
        }
    }

    private fun setupResolutionPresets() {
        binding.resolutionPresetSpinner.adapter = spinnerAdapter(Resolution.PRESETS.map { it.label() } + "Custom…")
        binding.resolutionPresetSpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                if (!suppressPresetCallback && position in Resolution.PRESETS.indices) configurationModel.setResolution(Resolution.PRESETS[position])
            }
            override fun onNothingSelected(parent: AdapterView<*>?) {}
        }
    }

    private fun syncPresetSpinner(resolution: Resolution) {
        val index = Resolution.PRESETS.indexOfFirst { it.width == resolution.width && it.height == resolution.height }
        val target = if (index >= 0) index else Resolution.PRESETS.size
        if (binding.resolutionPresetSpinner.selectedItemPosition != target) {
            suppressPresetCallback = true
            binding.resolutionPresetSpinner.setSelection(target)
            suppressPresetCallback = false
        }
    }

    private fun setupDebug() {
        configurationModel.debug.observe(viewLifecycleOwner) { enabled ->
            if (binding.debugModeChip.isChecked != enabled) binding.debugModeChip.isChecked = enabled
        }
        binding.debugModeChip.setOnCheckedChangeListener { _, checked -> configurationModel.setDebug(checked) }
    }

    private fun setupController() {
        binding.controllerLayoutSpinner.adapter = spinnerAdapter(layouts.map { if (it == "ps5") "PS5" else "Xbox" })
        binding.leftStickSpinner.adapter = spinnerAdapter(stickModes.map(::stickLabel))
        binding.rightStickSpinner.adapter = spinnerAdapter(stickModes.map(::stickLabel))
        binding.touchpadModeSpinner.adapter = spinnerAdapter(ControllerIni.TOUCHPAD_MODES.map(::touchpadLabel))
        binding.touchpadOutputSpinner.adapter = spinnerAdapter(ControllerIni.OUTPUTS.map { it.label })
        buildBindingRows()

        configurationModel.controllerConfig.observe(viewLifecycleOwner) { config -> syncControllerUi(config) }
        binding.controllerEnableChip.setOnCheckedChangeListener { _, checked ->
            if (!suppressControllerCallback) updateConfig { it.copy(enabled = checked) }
        }
        binding.controllerLayoutSpinner.onItemSelectedListener = selectionListener { position ->
            if (!suppressControllerCallback && position in layouts.indices) updateConfig { it.copy(layout = layouts[position]) }
        }
        binding.leftStickSpinner.onItemSelectedListener = selectionListener { position ->
            if (!suppressControllerCallback && position in stickModes.indices) updateConfig { it.copy(leftStick = stickModes[position]) }
        }
        binding.rightStickSpinner.onItemSelectedListener = selectionListener { position ->
            if (!suppressControllerCallback && position in stickModes.indices) updateConfig { it.copy(rightStick = stickModes[position]) }
        }
        binding.touchpadModeSpinner.onItemSelectedListener = selectionListener { position ->
            if (!suppressControllerCallback && position in ControllerIni.TOUCHPAD_MODES.indices) updateConfig { it.copy(touchpad = ControllerIni.TOUCHPAD_MODES[position]) }
        }
        binding.touchpadOutputSpinner.onItemSelectedListener = selectionListener { position ->
            if (!suppressControllerCallback && position in ControllerIni.OUTPUTS.indices) updateConfig { it.copy(touchpadOut = ControllerIni.OUTPUTS[position].spec) }
        }
        binding.touchpadSensitivityBar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                if (fromUser) updateConfig { it.copy(touchpadSens = progress + 200) }
            }
            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })
    }

    private fun buildBindingRows() {
        val kinds = listOf("None", "Mouse", "Wheel", "Motion", "Keyboard")
        ControllerIni.PAD_TOKENS.forEachIndexed { index, token ->
            val row = LinearLayout(requireContext()).apply {
                orientation = LinearLayout.HORIZONTAL
                gravity = Gravity.CENTER_VERTICAL
            }
            val label = TextView(requireContext()).apply { text = ControllerIni.PAD_LABELS[index] }
            val kind = Spinner(requireContext())
            val value = Spinner(requireContext())
            kind.adapter = spinnerAdapter(kinds)
            row.addView(label, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1.1f))
            row.addView(kind, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 0.8f))
            row.addView(value, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1.5f))
            binding.controllerBindingsContainer.addView(row)
            val bindingRow = BindingRow(token, kind, value)
            bindingRows += bindingRow
            kind.onItemSelectedListener = selectionListener { kindIndex ->
                if (!suppressControllerCallback) {
                    val outputs = outputsForKind(kindIndex)
                    value.adapter = spinnerAdapter(outputs.map { it.label })
                    value.isEnabled = kindIndex != 0
                    updateConfig { it.withBinding(token, outputs.firstOrNull()?.spec ?: "none") }
                }
            }
            value.onItemSelectedListener = selectionListener { valueIndex ->
                if (!suppressControllerCallback) {
                    val outputs = outputsForKind(kind.selectedItemPosition)
                    outputs.getOrNull(valueIndex)?.let { output -> updateConfig { it.withBinding(token, output.spec) } }
                }
            }
        }
    }

    private fun syncControllerUi(config: ControllerIni.Config) {
        suppressControllerCallback = true
        binding.controllerEnableChip.isChecked = config.enabled
        binding.controllerLayoutSpinner.setSelection(layouts.indexOf(config.layout).coerceAtLeast(0))
        binding.leftStickSpinner.setSelection(stickModes.indexOf(config.leftStick).coerceAtLeast(0))
        binding.rightStickSpinner.setSelection(stickModes.indexOf(config.rightStick).coerceAtLeast(0))
        binding.touchpadModeSpinner.setSelection(ControllerIni.TOUCHPAD_MODES.indexOf(config.touchpad).coerceAtLeast(0))
        binding.touchpadOutputSpinner.setSelection(ControllerIni.OUTPUTS.indexOfFirst { it.spec == config.touchpadOut }.coerceAtLeast(0))
        binding.touchpadSensitivityBar.progress = config.touchpadSens.coerceIn(200, 4000) - 200
        binding.touchpadSensitivityLabel.text = "Touchpad sensitivity: ${config.touchpadSens}"
        binding.touchpadSettingsGroup.visibility = if (config.layout == "ps5") View.VISIBLE else View.GONE
        binding.touchpadOutputSpinner.visibility = if (config.touchpad == "button") View.VISIBLE else View.GONE
        binding.touchpadSensitivityBar.visibility = if (config.touchpad == "cursor") View.VISIBLE else View.GONE
        binding.touchpadSensitivityLabel.visibility = binding.touchpadSensitivityBar.visibility
        binding.controllerBindingsContainer.alpha = if (config.enabled) 1f else 0.5f
        bindingRows.forEach { row ->
            val spec = config.binding(row.token)
            val output = ControllerIni.OUTPUTS.firstOrNull { it.spec == spec } ?: ControllerIni.OUTPUTS.first()
            val kind = kindIndex(output.kind)
            row.kind.setSelection(kind)
            val options = outputsForKind(kind)
            row.value.adapter = spinnerAdapter(options.map { it.label })
            row.value.setSelection(options.indexOfFirst { it.spec == output.spec }.coerceAtLeast(0))
            row.value.isEnabled = config.enabled && kind != 0
            row.kind.isEnabled = config.enabled
        }
        binding.controllerLayoutSpinner.isEnabled = config.enabled
        binding.leftStickSpinner.isEnabled = config.enabled
        binding.rightStickSpinner.isEnabled = config.enabled
        binding.touchpadModeSpinner.isEnabled = config.enabled
        binding.touchpadSensitivityBar.isEnabled = config.enabled && config.touchpad == "cursor"
        binding.touchpadOutputSpinner.isEnabled = config.enabled && config.touchpad == "button"
        suppressControllerCallback = false
    }

    private fun updateConfig(change: (ControllerIni.Config) -> ControllerIni.Config) {
        val current = configurationModel.controllerConfig.value ?: ControllerIni.Config()
        configurationModel.setControllerConfig(change(current))
    }

    private fun selectionListener(action: (Int) -> Unit) = object : AdapterView.OnItemSelectedListener {
        override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) = action(position)
        override fun onNothingSelected(parent: AdapterView<*>?) {}
    }

    private fun outputsForKind(kind: Int): List<ControllerIni.Output> {
        val group = listOf("none", "mouse", "wheel", "motion", "key").getOrElse(kind) { "none" }
        return ControllerIni.OUTPUTS.filter { it.kind == group }
    }

    private fun kindIndex(kind: String): Int = listOf("none", "mouse", "wheel", "motion", "key").indexOf(kind).coerceAtLeast(0)
    private fun stickLabel(mode: String) = when (mode) { "cursor" -> "Cursor"; "wasd" -> "WASD"; "arrow" -> "Arrows"; else -> "None" }
    private fun touchpadLabel(mode: String) = when (mode) { "cursor" -> "Cursor"; "button" -> "Button"; else -> "Disabled" }

    override fun onDestroyView() {
        deviceHandler.removeCallbacks(devicePoll)
        detectedDeviceId = null
        detectedDeviceName = null
        super.onDestroyView()
        _binding = null
    }

    companion object {
        private const val ARG_SECTION_NUMBER = "section_number"
        @JvmStatic
        fun newInstance(sectionNumber: Int) = SettingsFragment().apply {
            arguments = Bundle().apply { putInt(ARG_SECTION_NUMBER, sectionNumber) }
        }
    }
}
