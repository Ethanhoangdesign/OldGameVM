/* OldGameVM modification notice
 * This file was changed for OldGameVM in July 2026.
 * It is not the original file. See NOTICE.md.
 */
#pragma once

#include <string_theory/string>

#include <vector>

class ContentManager;

/** Resolve a resource that different JA2 editions ship under different
 * filenames.
 *
 * The candidates are probed in order and the first one present in the
 * currently loaded game data is returned. An empty string is returned when
 * none of them exists, so callers can fall back gracefully instead of
 * failing at boot.
 */
ST::string ResolveResourceVariant(const ContentManager* cm, const std::vector<ST::string>& candidates);
