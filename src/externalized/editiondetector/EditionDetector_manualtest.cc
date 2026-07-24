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

	// Fixture 1: looks like JA2 Gold (Data + Ja2set.dat.xml + JA2.exe)
	const fs::path gold = base / "gold";
	makeFile(gold / "Data" / "Ja2set.dat.xml");
	makeFile(gold / "JA2.exe");
	{
		DetectionResult r = detectEdition(gold.string());
		check(r.edition == EditionId::Gold, "Gold edition detected");
		check(r.confidence == DetectionConfidence::Exact, "Gold confidence Exact");
	}

	// Fixture 2: empty folder -> Unknown
	const fs::path empty = base / "empty";
	fs::create_directories(empty);
	{
		DetectionResult r = detectEdition(empty.string());
		check(r.edition == EditionId::Unknown, "Empty folder -> Unknown");
	}

	// Fixture 3: non-existent path -> Unknown with reason
	{
		DetectionResult r = detectEdition((base / "nope").string());
		check(r.edition == EditionId::Unknown, "Missing path -> Unknown");
		check(!r.reasons.empty(), "Missing path has a reason");
	}

	fs::remove_all(base);

	std::cout << "\n" << (g_failures == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED")
	          << " (failures: " << g_failures << ")\n";
	return g_failures == 0 ? 0 : 1;
}
