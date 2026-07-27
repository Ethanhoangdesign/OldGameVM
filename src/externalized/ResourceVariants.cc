/* OldGameVM modification notice
 * This file was changed for OldGameVM in July 2026.
 * It is not the original file. See NOTICE.md.
 */
#include "ResourceVariants.h"

#include "ContentManager.h"

ST::string ResolveResourceVariant(const ContentManager* cm, const std::vector<ST::string>& candidates)
{
	if (cm == NULL)
	{
		return ST::string();
	}

	for (const ST::string& candidate : candidates)
	{
		if (cm->doesGameResExists(candidate))
		{
			return candidate;
		}
	}

	return ST::string();
}
