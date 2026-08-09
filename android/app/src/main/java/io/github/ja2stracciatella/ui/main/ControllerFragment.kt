package io.github.ja2stracciatella.ui.main

import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.Gravity
import android.view.InputDevice
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.LinearLayout
import android.widget.SeekBar
import android.widget.Spinner
import android.widget.TextView
import android.widget.Toast
import androidx.core.content.ContextCompat
import androidx.fragment.app.Fragment
import androidx.lifecycle.ViewModelProvider
import com.google.android.material.textfield.TextInputLayout
import io.github.ja2stracciatella.*
import io.github.ja2stracciatella.databinding.FragmentLauncherControllerBinding

class ControllerFragment : Fragment() {
    private var _binding: FragmentLauncherControllerBinding? = null
    private val binding get() = _binding!!
    private lateinit var configurationModel: ConfigurationModel
    private val stickModes = ControllerIni.STICK_MODES
    private val layouts = ControllerIni.LAYOUTS
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

    private data class BindingRow(
        val token: String,
        val kind: Spinner,
        val value: Spinner,
        val kindField: TextInputLayout,
        val valueField: TextInputLayout
    )

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        configurationModel = ViewModelProvider(requireActivity())[ConfigurationModel::class.java]
    }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View {
        _binding = FragmentLauncherControllerBinding.inflate(inflater, container, false)
        setupController()
        refreshControllerStatus(false)
        deviceHandler.post(devicePoll)
        return binding.root
    }

    private fun refreshControllerStatus(showToast: Boolean) {
        val devices = InputDevice.getDeviceIds().asSequence()
            .mapNotNull { id -> InputDevice.getDevice(id)?.let { id to it } }
            .filter { (_, input) ->
                val sources = input.sources
                (sources and InputDevice.SOURCE_CLASS_JOYSTICK) != 0 ||
                    (sources and InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD ||
                    (sources and InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
            }.toList()
        val device = devices.firstOrNull { (_, input) -> !input.isVirtual } ?: devices.firstOrNull()
        val input = device?.second
        val isPs5 = input?.vendorId == 0x054c && input.productId == 0x0ce6
        val newId = device?.first
        val newName = input?.name?.let { if (isPs5) "PS5 DualSense ($it)" else it } ?: ""
        if (newId == detectedDeviceId && newName == detectedDeviceName) return
        val wasConnected = detectedDeviceId != null
        val inserted = !wasConnected && newId != null
        detectedDeviceId = newId
        detectedDeviceName = if (newId == null) null else newName
        if (inserted && isPs5) updateConfig { it.copy(layout = "ps5") }
        when {
            newId != null -> {
                binding.controllerStatusText.text = getString(R.string.controller_status_detected, newName)
                if (inserted && showToast) Toast.makeText(requireContext(), getString(R.string.controller_detected_toast, newName), Toast.LENGTH_SHORT).show()
            }
            wasConnected -> binding.controllerStatusText.setText(R.string.controller_status_disconnected)
            else -> binding.controllerStatusText.setText(R.string.controller_status_none)
        }
    }

    private fun <T> spinnerAdapter(items: List<T>): ArrayAdapter<String> {
        val adapter = object : ArrayAdapter<String>(requireContext(), R.layout.launcher_spinner_item, items.map { it.toString() }) {
            override fun getView(position: Int, convertView: View?, parent: ViewGroup): View {
                return super.getView(position, convertView, parent).apply {
                    isEnabled = parent.isEnabled
                    (this as? TextView)?.setTextColor(
                        ContextCompat.getColor(
                            requireContext(),
                            if (parent.isEnabled) R.color.launcherFieldEnabled else R.color.launcherFieldDisabled
                        )
                    )
                }
            }
        }
        adapter.setDropDownViewResource(R.layout.launcher_spinner_dropdown_item)
        return adapter
    }

    private fun setupController() {
        binding.controllerLayoutSpinner.adapter = spinnerAdapter(layouts.map { if (it == "ps5") "PS5" else "Xbox" })
        binding.leftStickSpinner.adapter = spinnerAdapter(stickModes.map(::stickLabel))
        binding.rightStickSpinner.adapter = spinnerAdapter(stickModes.map(::stickLabel))
        binding.touchpadModeSpinner.adapter = spinnerAdapter(ControllerIni.TOUCHPAD_MODES.map(::touchpadLabel))
        binding.touchpadOutputSpinner.adapter = spinnerAdapter(ControllerIni.OUTPUTS.map { it.label })
        listOf(
            binding.controllerLayoutSpinner,
            binding.leftStickSpinner,
            binding.rightStickSpinner,
            binding.touchpadModeSpinner,
            binding.touchpadOutputSpinner
        ).forEach(::guardSpinner)

        buildBindingRows()
        configurationModel.controllerConfig.observe(viewLifecycleOwner) { config -> syncControllerUi(config) }
        binding.controllerEnableChip.setOnCheckedChangeListener { _, checked -> if (!suppressControllerCallback) updateConfig { it.copy(enabled = checked) } }
        binding.controllerLayoutSpinner.onItemSelectedListener = selectionListener { position -> if (!suppressControllerCallback && position in layouts.indices) updateConfig { it.copy(layout = layouts[position]) } }
        binding.leftStickSpinner.onItemSelectedListener = selectionListener { position -> if (!suppressControllerCallback && position in stickModes.indices) updateConfig { it.copy(leftStick = stickModes[position]) } }
        binding.rightStickSpinner.onItemSelectedListener = selectionListener { position -> if (!suppressControllerCallback && position in stickModes.indices) updateConfig { it.copy(rightStick = stickModes[position]) } }
        binding.touchpadModeSpinner.onItemSelectedListener = selectionListener { position -> if (!suppressControllerCallback && position in ControllerIni.TOUCHPAD_MODES.indices) updateConfig { it.copy(touchpad = ControllerIni.TOUCHPAD_MODES[position]) } }
        binding.touchpadOutputSpinner.onItemSelectedListener = selectionListener { position -> if (!suppressControllerCallback && position in ControllerIni.OUTPUTS.indices) updateConfig { it.copy(touchpadOut = ControllerIni.OUTPUTS[position].spec) } }
        binding.touchpadSensitivityBar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) { if (fromUser) updateConfig { it.copy(touchpadSens = progress + 200) } }
            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })
    }

    private fun buildBindingRows() {
        val kinds = listOf("<empty>", "Mouse Button", "Mouse Wheel", "Mouse Motion", "Keyboard key")
        ControllerIni.PAD_TOKENS.forEachIndexed { index, token ->
            val row = LinearLayout(requireContext()).apply {
                orientation = LinearLayout.HORIZONTAL
                gravity = Gravity.CENTER_VERTICAL
                setPadding(0, dp(6), 0, dp(6))
            }
            val label = TextView(requireContext()).apply { text = ControllerIni.PAD_LABELS[index] }
            val kind = Spinner(requireContext())
            val value = Spinner(requireContext())
            kind.adapter = spinnerAdapter(kinds)
            value.adapter = spinnerAdapter(listOf("None"))
            kind.tag = 0
            value.tag = 0
            val kindField = outlinedSpinner(kind, R.string.controller_binding_kind_field_label)
            val valueField = outlinedSpinner(value, R.string.controller_binding_value_field_label)
            val rowParams = LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT).apply { bottomMargin = dp(6) }
            row.addView(label, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 0.8f))
            row.addView(kindField, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1.2f))
            row.addView(valueField, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1.7f))
            kindField.layoutParams = kindField.layoutParams.apply { (this as LinearLayout.LayoutParams).marginEnd = dp(4) }
            valueField.layoutParams = valueField.layoutParams.apply { (this as LinearLayout.LayoutParams).marginStart = dp(4) }
            binding.controllerBindingsContainer.addView(row, rowParams)
            bindingRows += BindingRow(token, kind, value, kindField, valueField)
            kind.onItemSelectedListener = selectionListener { kindIndex ->
                if (suppressControllerCallback) return@selectionListener
                val outputs = outputsForKind(kindIndex)
                suppressControllerCallback = true
                kind.tag = kindIndex
                value.adapter = spinnerAdapter(outputs.map { it.label })
                value.setSelection(0, false)
                setSpinnerEnabled(value, (configurationModel.controllerConfig.value?.enabled == true) && kindIndex != 0)
                updateConfig { it.withBinding(token, outputs.firstOrNull()?.spec ?: "none") }
                suppressControllerCallback = false
            }
            value.onItemSelectedListener = selectionListener { valueIndex ->
                if (suppressControllerCallback) return@selectionListener
                val kindIndex = (kind.tag as? Int) ?: kind.selectedItemPosition
                outputsForKind(kindIndex).getOrNull(valueIndex)?.let { output ->
                    suppressControllerCallback = true
                    updateConfig { it.withBinding(token, output.spec) }
                    suppressControllerCallback = false
                }
            }
        }
    }

    private fun syncControllerUi(config: ControllerIni.Config) {
        // User callback already updated visible row; nested LiveData dispatch must not reset it.
        if (suppressControllerCallback) return
        suppressControllerCallback = true
        binding.controllerEnableChip.isChecked = config.enabled
        binding.controllerLayoutSpinner.setSelection(layouts.indexOf(config.layout).coerceAtLeast(0), false)
        binding.leftStickSpinner.setSelection(stickModes.indexOf(config.leftStick).coerceAtLeast(0), false)
        binding.rightStickSpinner.setSelection(stickModes.indexOf(config.rightStick).coerceAtLeast(0), false)
        binding.touchpadModeSpinner.setSelection(ControllerIni.TOUCHPAD_MODES.indexOf(config.touchpad).coerceAtLeast(0), false)
        binding.touchpadOutputSpinner.setSelection(ControllerIni.OUTPUTS.indexOfFirst { it.spec == config.touchpadOut }.coerceAtLeast(0), false)
        binding.touchpadSensitivityBar.progress = config.touchpadSens.coerceIn(200, 4000) - 200
        binding.touchpadSensitivityLabel.text = "Touchpad sensitivity: ${config.touchpadSens}"
        binding.touchpadSettingsGroup.visibility = if (config.layout == "ps5") View.VISIBLE else View.GONE
        binding.touchpadOutputSpinner.visibility = if (config.touchpad == "button") View.VISIBLE else View.GONE
        binding.touchpadSensitivityBar.visibility = if (config.touchpad == "cursor") View.VISIBLE else View.GONE
        binding.touchpadSensitivityLabel.visibility = binding.touchpadSensitivityBar.visibility
        binding.controllerBindingsContainer.alpha = 1f
        bindingRows.forEach { row ->
            val output = ControllerIni.OUTPUTS.firstOrNull { it.spec == config.binding(row.token) } ?: ControllerIni.OUTPUTS.first()
            val kind = kindIndex(output.kind)
            val options = outputsForKind(kind)
            row.kind.tag = kind
            row.kind.setSelection(kind, false)
            row.value.adapter = spinnerAdapter(options.map { it.label })
            row.value.setSelection(options.indexOfFirst { it.spec == output.spec }.coerceAtLeast(0), false)
            setSpinnerEnabled(row.kind, config.enabled)
            setSpinnerEnabled(row.value, config.enabled && kind != 0)
            guardSpinner(row.kind)
            guardSpinner(row.value)
        }
        setSpinnerEnabled(binding.controllerLayoutSpinner, config.enabled)
        setSpinnerEnabled(binding.leftStickSpinner, config.enabled)
        setSpinnerEnabled(binding.rightStickSpinner, config.enabled)
        setSpinnerEnabled(binding.touchpadModeSpinner, config.enabled)
        binding.touchpadSensitivityBar.isEnabled = config.enabled && config.touchpad == "cursor"
        setSpinnerEnabled(binding.touchpadOutputSpinner, config.enabled && config.touchpad == "button" )
        suppressControllerCallback = false
    }

    private fun updateConfig(change: (ControllerIni.Config) -> ControllerIni.Config) {
        val current = configurationModel.controllerConfig.value ?: ControllerIni.Config()
        configurationModel.setControllerConfig(change(current))
    }

    private fun setSpinnerEnabled(spinner: Spinner, enabled: Boolean) {
        spinner.isEnabled = enabled
        val field = spinner.parent as? TextInputLayout
        field?.isEnabled = enabled
        field?.alpha = 1f
        field?.setBoxStrokeColorStateList(requireNotNull(ContextCompat.getColorStateList(requireContext(), R.color.launcher_field_stroke)))
        field?.setHintTextColor(requireNotNull(ContextCompat.getColorStateList(requireContext(), R.color.launcher_field_text)))
        (spinner.selectedView as? TextView)?.background = ContextCompat.getDrawable(
            requireContext(),
            if (enabled) R.drawable.launcher_spinner_enabled_border else R.drawable.launcher_spinner_disabled_border
        )
        fun refreshSelectedView() {
            (spinner.selectedView as? TextView)?.apply {
                isEnabled = enabled
                setTextColor(ContextCompat.getColor(requireContext(), if (enabled) R.color.launcherFieldEnabled else R.color.launcherFieldDisabled))
                background = ContextCompat.getDrawable(
                    requireContext(),
                    if (enabled) R.drawable.launcher_spinner_enabled_border else R.drawable.launcher_spinner_disabled_border
                )
                refreshDrawableState()
            }
        }
        refreshSelectedView()
        spinner.post(::refreshSelectedView)
    }

    private fun guardSpinner(spinner: Spinner) {
        val field = spinner.parent as? TextInputLayout
        spinner.setOnTouchListener { _, _ -> !spinner.isEnabled || field?.isEnabled == false }
        field?.setOnTouchListener { _, _ -> !field.isEnabled || !spinner.isEnabled }
    }

    private fun selectionListener(action: (Int) -> Unit) = object : AdapterView.OnItemSelectedListener {
        override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) = action(position)
        override fun onNothingSelected(parent: AdapterView<*>?) {}
    }

    private fun outlinedSpinner(spinner: Spinner, label: Int): TextInputLayout =
        TextInputLayout(requireContext(), null, com.google.android.material.R.attr.textInputStyle).apply {
            setHint(label)
            boxBackgroundMode = TextInputLayout.BOX_BACKGROUND_OUTLINE
            setBoxStrokeColorStateList(requireNotNull(ContextCompat.getColorStateList(requireContext(), R.color.launcher_field_stroke)))
            boxStrokeWidth = dp(1)
            boxStrokeWidthFocused = dp(1)
            addView(spinner, ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))
        }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).toInt()

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
}
