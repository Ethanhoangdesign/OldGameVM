/* OldGameVM modification notice
 * This file was changed for OldGameVM in July 2026.
 * It is not the original file. See NOTICE.md.
 */
#pragma once

#include <string>
#include <vector>

/** Multi-Edition Detector (Sprint 1).
 *
 * Standalone module. Not yet connected to the launcher or engine.
 * Inspects a user-selected game directory and guesses which JA2-based
 * edition it is, based on required files. It never runs game executables
 * and never modifies game files.
 */

/** Known editions this detector can recognize. */
enum class EditionId
{
	Vanilla,
	Gold,
	Wildfire5,
	Wildfire6,
	Unknown
};

/** How confident the detector is about the result. */
enum class DetectionConfidence
{
	Exact,
	Probable,
	Unknown
};

/** Signature describing how to recognize one edition. */
struct EditionSignature
{
	EditionId edition;
	std::string displayName;
	/** Files that must exist (relative paths, case-insensitive). */
	std::vector<std::string> requiredFiles;
	/** Files that help confirm but are not mandatory. */
	std::vector<std::string> optionalFiles;
};

/** Result of a detection run. */
struct DetectionResult
{
	EditionId edition = EditionId::Unknown;
	DetectionConfidence confidence = DetectionConfidence::Unknown;
	std::vector<std::string> missingFiles;
	std::vector<std::string> foundFiles;
	std::vector<std::string> reasons;
};

/** Human-readable name for an edition id. */
std::string editionIdToString(EditionId edition);

/** Human-readable name for a confidence level. */
std::string detectionConfidenceToString(DetectionConfidence confidence);

/** Built-in signatures used by the detector. */
std::vector<EditionSignature> defaultEditionSignatures();

/** Detect the edition of a game directory using the given signatures. */
DetectionResult detectEdition(const std::string& gameDir,
                              const std::vector<EditionSignature>& signatures);

/** Detect the edition of a game directory using the default signatures. */
DetectionResult detectEdition(const std::string& gameDir);
