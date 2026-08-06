package io.github.ja2stracciatella.ui.main

import android.os.Bundle
import android.text.Editable
import android.text.TextWatcher
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.AdapterView
import android.widget.ArrayAdapter
import androidx.fragment.app.Fragment
import androidx.lifecycle.ViewModelProvider
import io.github.ja2stracciatella.*
import io.github.ja2stracciatella.databinding.FragmentLauncherSettingsBinding


/**
 * Settings: resolution presets (desktop OGVM list + 1360), scaling, debug, controller.
 */
class SettingsFragment : Fragment() {
    private var _binding: FragmentLauncherSettingsBinding? = null
    private val binding get() = _binding!!

    private lateinit var configurationModel: ConfigurationModel
    private lateinit var scalingQualities: Array<ScalingQuality>
    private val stickModes = ControllerIni.STICK_MODES
    private var suppressPresetCallback = false
    private var suppressStickCallback = false

    override fun onCreate(savedInstanceState: Bundle?) {
        configurationModel = ViewModelProvider(requireActivity())[ConfigurationModel::class.java]
        scalingQualities = ScalingQuality.values()
        super.onCreate(savedInstanceState)
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentLauncherSettingsBinding.inflate(inflater, container, false)

        setupScalingSpinner()
        setupResolutionFields()
        setupResolutionPresets()
        setupDebug()
        setupController()

        return binding.root
    }

    private fun setupScalingSpinner() {
        val spinnerLabels = scalingQualities.map { it.getLabel() }
        val adapter: ArrayAdapter<String> =
            ArrayAdapter(this.requireContext(), R.layout.launcher_spinner_item, spinnerLabels)
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        binding.scalingQualitySpinner.adapter = adapter

        configurationModel.scalingQuality.observe(viewLifecycleOwner) { scalingQuality ->
            val index = scalingQualities.indexOf(scalingQuality)
            if (index >= 0) binding.scalingQualitySpinner.setSelection(index)
        }
        binding.scalingQualitySpinner.onItemSelectedListener =
            object : AdapterView.OnItemSelectedListener {
                override fun onItemSelected(
                    parent: AdapterView<*>?,
                    view: View?,
                    position: Int,
                    id: Long
                ) {
                    if (position >= 0 && position < scalingQualities.size) {
                        configurationModel.setScalingQuality(scalingQualities[position])
                    }
                }

                override fun onNothingSelected(parent: AdapterView<*>?) {}
            }
    }

    private fun setupResolutionFields() {
        configurationModel.resolution.observe(viewLifecycleOwner) { resolution ->
            if (binding.resolutionWidthEdit.text.toString() != resolution.width.toString()) {
                binding.resolutionWidthEdit.setText(resolution.width.toString())
            }
            if (binding.resolutionHeightEdit.text.toString() != resolution.height.toString()) {
                binding.resolutionHeightEdit.setText(resolution.height.toString())
            }
            syncPresetSpinner(resolution)
        }
        binding.resolutionWidthEdit.addTextChangedListener(object : TextWatcher {
            override fun afterTextChanged(s: Editable) {}
            override fun beforeTextChanged(s: CharSequence, start: Int, count: Int, after: Int) {}
            override fun onTextChanged(s: CharSequence, start: Int, before: Int, count: Int) {
                if (s.isNotEmpty()) {
                    try {
                        val width = s.toString().toUInt()
                        val current = configurationModel.resolution.value ?: Resolution.DEFAULT
                        if (width != current.width) {
                            configurationModel.setResolution(Resolution(width, current.height))
                        }
                    } catch (_: NumberFormatException) {
                    } catch (_: NullPointerException) {
                    }
                }
            }
        })
        binding.resolutionHeightEdit.addTextChangedListener(object : TextWatcher {
            override fun afterTextChanged(s: Editable) {}
            override fun beforeTextChanged(s: CharSequence, start: Int, count: Int, after: Int) {}
            override fun onTextChanged(s: CharSequence, start: Int, before: Int, count: Int) {
                if (s.isNotEmpty()) {
                    try {
                        val height = s.toString().toUInt()
                        val current = configurationModel.resolution.value ?: Resolution.DEFAULT
                        if (height != current.height) {
                            configurationModel.setResolution(Resolution(current.width, height))
                        }
                    } catch (_: NumberFormatException) {
                    } catch (_: NullPointerException) {
                    }
                }
            }
        })

        binding.resolutionAutoButton.setOnClickListener {
            val launcherActivity = requireActivity()
            if (launcherActivity is LauncherActivity) {
                configurationModel.setResolution(launcherActivity.getRecommendedResolution())
            }
        }
    }

    private fun setupResolutionPresets() {
        val labels = Resolution.PRESETS.map { it.label() } + "Custom…"
        val adapter: ArrayAdapter<String> =
            ArrayAdapter(requireContext(), R.layout.launcher_spinner_item, labels)
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        binding.resolutionPresetSpinner.adapter = adapter

        binding.resolutionPresetSpinner.onItemSelectedListener =
            object : AdapterView.OnItemSelectedListener {
                override fun onItemSelected(
                    parent: AdapterView<*>?,
                    view: View?,
                    position: Int,
                    id: Long
                ) {
                    if (suppressPresetCallback) return
                    if (position in Resolution.PRESETS.indices) {
                        configurationModel.setResolution(Resolution.PRESETS[position])
                    }
                    // last index = Custom — leave edit fields alone
                }

                override fun onNothingSelected(parent: AdapterView<*>?) {}
            }
        configurationModel.resolution.value?.let { syncPresetSpinner(it) }
    }

    private fun syncPresetSpinner(resolution: Resolution) {
        val idx = Resolution.PRESETS.indexOfFirst {
            it.width == resolution.width && it.height == resolution.height
        }
        val target = if (idx >= 0) idx else Resolution.PRESETS.size // Custom
        if (binding.resolutionPresetSpinner.selectedItemPosition != target) {
            suppressPresetCallback = true
            binding.resolutionPresetSpinner.setSelection(target)
            suppressPresetCallback = false
        }
    }

    private fun setupDebug() {
        configurationModel.debug.observe(viewLifecycleOwner) { enabled ->
            if (binding.debugModeChip.isChecked != enabled) {
                binding.debugModeChip.isChecked = enabled
            }
        }
        binding.debugModeChip.setOnCheckedChangeListener { _, isChecked ->
            configurationModel.setDebug(isChecked)
        }
    }

    private fun setupController() {
        configurationModel.controllerEnabled.observe(viewLifecycleOwner) { enabled ->
            if (binding.controllerEnableChip.isChecked != enabled) {
                binding.controllerEnableChip.isChecked = enabled
            }
            binding.leftStickSpinner.isEnabled = enabled
            binding.rightStickSpinner.isEnabled = enabled
        }
        binding.controllerEnableChip.setOnCheckedChangeListener { _, isChecked ->
            configurationModel.setControllerEnabled(isChecked)
        }

        val stickLabels = stickModes.map { mode ->
            when (mode) {
                "none" -> "None"
                "cursor" -> "Cursor"
                "wasd" -> "WASD"
                "arrow" -> "Arrows"
                else -> mode
            }
        }
        val stickAdapter: ArrayAdapter<String> =
            ArrayAdapter(requireContext(), R.layout.launcher_spinner_item, stickLabels)
        stickAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        binding.leftStickSpinner.adapter = stickAdapter
        binding.rightStickSpinner.adapter = stickAdapter

        configurationModel.leftStickMode.observe(viewLifecycleOwner) { mode ->
            val i = stickModes.indexOf(mode).coerceAtLeast(0)
            if (binding.leftStickSpinner.selectedItemPosition != i) {
                suppressStickCallback = true
                binding.leftStickSpinner.setSelection(i)
                suppressStickCallback = false
            }
        }
        configurationModel.rightStickMode.observe(viewLifecycleOwner) { mode ->
            val i = stickModes.indexOf(mode).coerceAtLeast(0)
            if (binding.rightStickSpinner.selectedItemPosition != i) {
                suppressStickCallback = true
                binding.rightStickSpinner.setSelection(i)
                suppressStickCallback = false
            }
        }

        binding.leftStickSpinner.onItemSelectedListener =
            object : AdapterView.OnItemSelectedListener {
                override fun onItemSelected(
                    parent: AdapterView<*>?,
                    view: View?,
                    position: Int,
                    id: Long
                ) {
                    if (suppressStickCallback) return
                    if (position in stickModes.indices) {
                        configurationModel.setLeftStickMode(stickModes[position])
                    }
                }

                override fun onNothingSelected(parent: AdapterView<*>?) {}
            }
        binding.rightStickSpinner.onItemSelectedListener =
            object : AdapterView.OnItemSelectedListener {
                override fun onItemSelected(
                    parent: AdapterView<*>?,
                    view: View?,
                    position: Int,
                    id: Long
                ) {
                    if (suppressStickCallback) return
                    if (position in stickModes.indices) {
                        configurationModel.setRightStickMode(stickModes[position])
                    }
                }

                override fun onNothingSelected(parent: AdapterView<*>?) {}
            }
    }

    companion object {
        private const val ARG_SECTION_NUMBER = "section_number"

        @JvmStatic
        fun newInstance(sectionNumber: Int): SettingsFragment {
            return SettingsFragment().apply {
                arguments = Bundle().apply {
                    putInt(ARG_SECTION_NUMBER, sectionNumber)
                }
            }
        }
    }
}
