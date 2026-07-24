#include "EditionDetector.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace
{
	std::string toLower(std::string value)
	{
		std::transform(value.begin(), value.end(), value.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return value;
	}

	bool relativeFileExists(const std::filesystem::path& gameDir,
	                        const std::string& relativeFile)
	{
		std::error_code ec;
		const std::filesystem::path direct = gameDir / relativeFile;
		if (std::filesystem::exists(direct, ec))
		{
			return true;
		}

		const std::filesystem::path rel(relativeFile);
		const std::filesystem::path parent = (gameDir / rel).parent_path();
		if (!std::filesystem::exists(parent, ec))
		{
			return false;
		}

		const std::string wanted = toLower(rel.filename().string());
		for (const auto& entry : std::filesystem::directory_iterator(parent, ec))
		{
			if (toLower(entry.path().filename().string()) == wanted)
			{
				return true;
			}
		}
		return false;
	}
}

std::string editionIdToString(EditionId edition)
{
	switch (edition)
	{
		case EditionId::Vanilla:   return "Vanilla";
		case EditionId::Gold:      return "Gold";
		case EditionId::Wildfire5: return "Wildfire 5";
		case EditionId::Wildfire6: return "Wildfire 6";
		case EditionId::Unknown:   return "Unknown";
	}
	return "Unknown";
}

std::string detectionConfidenceToString(DetectionConfidence confidence)
{
	switch (confidence)
	{
		case DetectionConfidence::Exact:    return "Exact";
		case DetectionConfidence::Probable: return "Probable";
		case DetectionConfidence::Unknown:  return "Unknown";
	}
	return "Unknown";
}

std::vector<EditionSignature> defaultEditionSignatures()
{
	// Signatures verified against legally-owned Steam installs.
	// Only relative file NAMES are stored here; no commercial asset
	// content is embedded. The detector never opens or copies these files.
	return {
		EditionSignature{
			EditionId::Wildfire6,
			"JA2 Wildfire 6",
			{ "WF6.exe", "WF6.ini", "Data/Data.slf" },
			{ "WF6.set", "Data/BinaryData.slf", "Data/NPCData.slf" }
		},
		EditionSignature{
			EditionId::Wildfire5,
			"JA2 Wildfire 5",
			{ "WF5.exe", "Data/Data.slf" },
			{ "WF5.ini", "Data/BinaryData.slf" }
		},
		EditionSignature{
			EditionId::Gold,
			"JA2 Gold",
			{ "JA2.exe", "Data/Data.slf" },
			{ "Data/BinaryData.slf", "Data/NPCData.slf" }
		},
		EditionSignature{
			EditionId::Vanilla,
			"JA2 Vanilla",
			{ "Data/Data.slf" },
			{ "JA2.exe" }
		},
	};
}

DetectionResult detectEdition(const std::string& gameDir,
                              const std::vector<EditionSignature>& signatures)
{
	DetectionResult best;

	std::error_code ec;
	const std::filesystem::path root(gameDir);
	if (!std::filesystem::exists(root, ec) ||
	    !std::filesystem::is_directory(root, ec))
	{
		best.reasons.push_back("Game directory does not exist: " + gameDir);
		return best;
	}

	size_t bestScore = 0;
	for (const auto& sig : signatures)
	{
		std::vector<std::string> found;
		std::vector<std::string> missing;

		for (const auto& file : sig.requiredFiles)
		{
			if (relativeFileExists(root, file))
			{
				found.push_back(file);
			}
			else
			{
				missing.push_back(file);
			}
		}

		size_t optionalFound = 0;
		for (const auto& file : sig.optionalFiles)
		{
			if (relativeFileExists(root, file))
			{
				++optionalFound;
				found.push_back(file);
			}
		}

		const bool allRequired = missing.empty() && !sig.requiredFiles.empty();
		const size_t score = sig.requiredFiles.size() * 10 + optionalFound;

		if (allRequired && score > bestScore)
		{
			bestScore = score;
			best.edition = sig.edition;
			best.foundFiles = found;
			best.missingFiles = missing;
			best.confidence = (optionalFound > 0)
				? DetectionConfidence::Exact
				: DetectionConfidence::Probable;
			best.reasons.clear();
			best.reasons.push_back("Matched signature: " + sig.displayName);
		}
	}

	if (best.edition == EditionId::Unknown)
	{
		best.reasons.push_back("No signature matched all required files.");
	}

	return best;
}

DetectionResult detectEdition(const std::string& gameDir)
{
	return detectEdition(gameDir, defaultEditionSignatures());
}
