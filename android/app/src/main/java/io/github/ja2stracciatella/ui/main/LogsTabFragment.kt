package io.github.ja2stracciatella.ui.main

import android.content.ClipData
import android.content.Context
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.fragment.app.Fragment
import io.github.ja2stracciatella.NativeExceptionContainer
import io.github.ja2stracciatella.R
import io.github.ja2stracciatella.databinding.FragmentLauncherLogsTabBinding
import java.io.File


class LogsTabFragment : Fragment() {
    private var _binding: FragmentLauncherLogsTabBinding? = null
    private val binding get() = _binding!!

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentLauncherLogsTabBinding.inflate(inflater, container, false)

        binding.logsCopyToClipboardButton.setOnClickListener {
            val text = binding.logsText.text
            val clipboard =
                requireContext().getSystemService(Context.CLIPBOARD_SERVICE) as android.content.ClipboardManager
            val clip = ClipData.newPlainText(resources.getText(R.string.logs_copied_to_clipboard_name), text)
            clipboard.setPrimaryClip(clip)
            Toast.makeText(requireContext(), resources.getText(R.string.logs_copied_to_clipboard_toast), Toast.LENGTH_SHORT).show()
        }

        return binding.root
    }

    override fun onResume() {
        super.onResume()

        val context = activity ?: return
        val home = NativeExceptionContainer.reportDirectory(context)
        val sections = listOf(
            "CRASH REPORT" to NativeExceptionContainer.readReport(context),
            "NATIVE SIGNAL" to NativeExceptionContainer.readSignal(context),
            "CURRENT LOG" to File(home, "ja2.log").takeIf { it.isFile }?.readText(),
            "PREVIOUS LOG" to File(home, "ja2.log.last").takeIf { it.isFile }?.readText()
        )
        binding.logsText.text = sections
            .filter { it.second != null }
            .joinToString("\n\n") { "===== ${it.first} =====\n${it.second}" }
            .ifEmpty { "No logs available." }
    }
}
