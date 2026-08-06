/* OldGameVM modification notice
 * This file was changed for OldGameVM in July 2026.
 * It is not the original file. See NOTICE.md.
 */
// Standalone manual test for the Multi-Edition Detector.
// Not part of the engine build. Compile directly with clang++ (C++17).
// Uses dummy fixtures created at runtime under a temp folder.
// No commercial game assets are used.

#include "EditionDetector.h"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

namespace
{
	int g_failures = 0;

	void makeFile(const fs::path& path)
	{
		fs::create_directories(path.parent_path());
		std::ofstream(path) << "dummy";
	}

	void check(bool condition, const std::string& name)
	{
		if (condition)
		{
			std::cout << "[PASS] " << name << "\n";
		}
		else
		{
			std::cout << "[FAIL] " << name << "\n";
			++g_failures;
		}
	}
}

int main()
{
	const fs::path base = fs::temp_directory_path() / "ja2_detector_test";
	fs::remove_all(base);

	// Fixture 1: JA2 Gold (JA2.exe + Data/Data.slf + optional BinaryData)
	const fs::path gold = base / "gold";
	makeFile(gold / "Data" / "Data.slf");
	makeFile(gold / "JA2.exe");
	makeFile(gold / "Data" / "BinaryData.slf");
	{
		DetectionResult r = detectEdition(gold.string());
		check(r.edition == EditionId::Gold, "Gold edition detected");
		check(r.confidence == DetectionConfidence::Exact, "Gold confidence Exact");
		check(!r.reasons.empty() && r.reasons.front().size() < 80,
		      "OGVM-SHORTERR: Gold reason short");
	}

	// Fixture 2: empty folder -> Unknown, short reason
	const fs::path empty = base / "empty";
	fs::create_directories(empty);
	{
		DetectionResult r = detectEdition(empty.string());
		check(r.edition == EditionId::Unknown, "Empty folder -> Unknown");
		check(!r.reasons.empty() && r.reasons.front().size() < 120,
		      "OGVM-SHORTERR: empty reason short");
	}

	// Fixture 3: non-existent path -> Unknown with short reason
	{
		DetectionResult r = detectEdition((base / "nope").string());
		check(r.edition == EditionId::Unknown, "Missing path -> Unknown");
		check(!r.reasons.empty(), "Missing path has a reason");
		check(r.reasons.front().find('/') == std::string::npos,
		      "OGVM-SHORTERR: no path dump in reason");
	}

	// Fixture 4: near-Gold (JA2.exe only — Data.slf missing). Data.slf alone
	// would match Vanilla; JA2.exe alone is a true near-miss.
	const fs::path near = base / "near";
	makeFile(near / "JA2.exe");
	{
		DetectionResult r = detectEdition(near.string());
		check(r.edition == EditionId::Unknown, "Near-Gold still Unknown");
		check(!r.reasons.empty() && r.reasons.front().find("missing") != std::string::npos,
		      "OGVM-SHORTERR: near miss names missing files");
	}

	fs::remove_all(base);

	std::cout << "\n" << (g_failures == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED")
	          << " (failures: " << g_failures << ")\n";
	return g_failures == 0 ? 0 : 1;
}
