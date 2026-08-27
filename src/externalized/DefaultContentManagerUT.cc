#include "DefaultContentManagerUT.h"

#include "BinaryProfileData.h"
#include "DefaultContentManager.h"
#include "FileMan.h"
#include "TestUtils.h"
#include "TranslatableString.h"
#include <utility>


namespace
{
RustPointer<EngineOptions> makeTestEngineOptions()
{
	RustPointer<EngineOptions> engineOptions(EngineOptions_default());
	ST::string extraDataDir = GetExtraDataDir();
	ST::string gameResRootPath = FileMan::joinPaths(extraDataDir, "unittests");
	EngineOptions_setVanillaGameDir(engineOptions.get(), gameResRootPath.c_str());
	return engineOptions;
}
}

DefaultContentManagerUT* DefaultContentManagerUT::createDefaultCMForTesting()
{
	return new DefaultContentManagerUT(makeTestEngineOptions());
}

DefaultContentManagerUT* DefaultContentManagerUT::createWildfireCMForTesting()
{
	return new DefaultContentManagerUT(makeTestEngineOptions(), true);
}

bool DefaultContentManagerUT::doesGameResExists(const ST::string& filename) const
{
	if (filename == "interface/b_map.sti") return m_wildfire;
	if (filename == "interface/b_map.pcx") return !m_wildfire;
	return DefaultContentManager::doesGameResExists(filename);
}

bool DefaultContentManagerUT::loadGameData()
{
	auto loader = TranslatableString::Unittests::TestLoader();
	return DefaultContentManager::loadGameData(loader, BinaryProfileData());
}
