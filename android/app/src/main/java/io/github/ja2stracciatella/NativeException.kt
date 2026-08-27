package io.github.ja2stracciatella

// This class is used from c++ code to communicate native exceptions using std::set_terminate
object NativeExceptionContainer {
    private var exception: String? = null

    @Synchronized
    fun getException(): String? {
        return this.exception
    }

    fun reportDirectory(context: android.content.Context): java.io.File =
        java.io.File(context.filesDir, ".ja2")

    fun reportFile(context: android.content.Context): java.io.File =
        java.io.File(reportDirectory(context), "crash-report")

    fun signalFile(context: android.content.Context): java.io.File =
        java.io.File(reportDirectory(context), "crash-signal")

    fun runningFile(context: android.content.Context): java.io.File =
        java.io.File(reportDirectory(context), "game-running")

    fun readReport(context: android.content.Context): String? =
        reportFile(context).takeIf { it.isFile }?.runCatching { readText() }?.getOrNull()

    fun readSignal(context: android.content.Context): String? =
        signalFile(context).takeIf { it.isFile }?.runCatching { readText() }?.getOrNull()

    fun hasUncleanRun(context: android.content.Context): Boolean = runningFile(context).isFile

    fun markGameRunning(context: android.content.Context) {
        val dir = reportDirectory(context)
        dir.mkdirs()
        java.io.File(dir, "game-running.tmp").writeText("running\n")
            .renameTo(runningFile(context))
    }

    fun clearGameRunning(context: android.content.Context) {
        runningFile(context).delete()
    }

    @Synchronized
    fun setException(exception: String) {
        this.exception = exception
    }

    @Synchronized
    fun resetException() {
        this.exception = null
    }
}