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
