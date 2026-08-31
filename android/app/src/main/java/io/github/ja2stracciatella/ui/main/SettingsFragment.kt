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

class SettingsFragment : Fragment() {
    private var _binding: FragmentLauncherSettingsBinding? = null
    private val binding get() = _binding!!
    private lateinit var configurationModel: ConfigurationModel
    private lateinit var scalingQualities: Array<ScalingQuality>
    private var suppressPresetCallback = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        configurationModel = ViewModelProvider(requireActivity())[ConfigurationModel::class.java]
        scalingQualities = ScalingQuality.values()
    }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View {
        _binding = FragmentLauncherSettingsBinding.inflate(inflater, container, false)
        setupScalingSpinner()
        setupResolutionFields()
        setupResolutionPresets()
        setupDebug()
        return binding.root
    }

    private fun <T> spinnerAdapter(items: List<T>): ArrayAdapter<String> {
        val adapter = ArrayAdapter(requireContext(), R.layout.launcher_spinner_item, items.map { it.toString() })
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        return adapter
    }

    private fun setupScalingSpinner() {
        binding.scalingQualitySpinner.adapter = spinnerAdapter(scalingQualities.map { it.getLabel() })
        configurationModel.scalingQuality.observe(viewLifecycleOwner) { quality ->
            scalingQualities.indexOf(quality).takeIf { it >= 0 }?.let { binding.scalingQualitySpinner.setSelection(it) }
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

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
