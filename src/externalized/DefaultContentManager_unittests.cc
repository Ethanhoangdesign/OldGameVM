#ifdef WITH_UNITTESTS

#include "DefaultContentManagerUT.h"
#include "FileMan.h"
#include "TestUtils.h"

#include "gtest/gtest.h"

#include <array>

TEST(TempFiles, createFile)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();

	{
		AutoSGPFile file(cm->tempFiles()->openForWriting("foo.txt", true));
	}

	std::vector<ST::string> results = cm->tempFiles()->findAllFilesInDir(ST::string(""), false, false, true);
	ASSERT_EQ(results.size(), 1u);
	EXPECT_STREQ(results[0].c_str(), "foo.txt");

	delete cm;
}

TEST(TempFiles, writeToFile)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();

	{
		AutoSGPFile file(cm->tempFiles()->openForWriting("foo.txt", true));
		file->write("hello", 5);
	}

	// open for writing, but don't truncate
	{
		AutoSGPFile file(cm->tempFiles()->openForWriting("foo.txt", false));
		ASSERT_EQ(file->size(), 5u);
	}

	// open with truncate and check that it is empty
	{
		AutoSGPFile file(cm->tempFiles()->openForWriting("foo.txt", true));
		ASSERT_EQ(file->size(), 0u);
	}

	delete cm;
}

TEST(TempFiles, writeAndRead)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();

	{
		AutoSGPFile file(cm->tempFiles()->openForWriting("foo.txt", true));
		file->write("hello", 5);
	}

	{
		char buf[10];
		AutoSGPFile file(cm->tempFiles()->openForReading("foo.txt"));
		file->read(buf, 5);
		buf[5] = 0;
		ASSERT_STREQ(buf, "hello");
	}

	delete cm;
}

TEST(TempFiles, append)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();

	{
		AutoSGPFile file(cm->tempFiles()->openForWriting("foo.txt", true));
		file->write("hello", 5);
	}

	{
		AutoSGPFile file(cm->tempFiles()->openForAppend("foo.txt"));
		file->write("hello", 5);
	}

	{
		AutoSGPFile file(cm->tempFiles()->openForReading("foo.txt"));
		ASSERT_EQ(file->size(), 10u);
	}

	delete cm;
}

TEST(TempFiles, deleteFile)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();

	{
		AutoSGPFile file(cm->tempFiles()->openForWriting("foo.txt", true));
	}

	std::vector<ST::string> results = cm->tempFiles()->findAllFilesInDir("", false, false, true);
	ASSERT_EQ(results.size(), 1u);

	cm->tempFiles()->deleteFile("foo.txt");

	results = cm->tempFiles()->findAllFilesInDir("", false, false, true);
	ASSERT_EQ(results.size(), 0u);

	delete cm;
}

TEST(WildfireMagazineFixups, CorrectAffectedDefinitions)
{
	struct Expected
	{
		uint16_t itemIndex;
		const char* calibre;
		uint16_t capacity;
		const char* ammoType;
		const char* bigPath;
		uint16_t smallSubImage;
	};
	static constexpr std::array<Expected, 8> expected = {{
		{ 77, "AMMO57",  20, "AMMO_REGULAR", "bigitems/p1item20.sti", 20 },
		{ 86, "AMMO357",  6, "AMMO_AP",      "bigitems/p1item12.sti", 12 },
		{ 87, "AMMO357",  9, "AMMO_AP",      "bigitems/p1item18.sti", 18 },
		{ 88, "AMMO357",  6, "AMMO_HP",      "bigitems/p1item13.sti", 13 },
		{ 89, "AMMO357",  9, "AMMO_HP",      "bigitems/p1item19.sti", 19 },
		{ 90, "AMMO545", 30, "AMMO_AP",      "bigitems/p1item09.sti",  9 },
		{ 91, "AMMO545", 30, "AMMO_HP",      "bigitems/p1item10.sti", 10 },
		{ 105, "AMMO762W", 20, "AMMO_AP",    "bigitems/p1item22.sti", 22 },
	}};

	for (const Expected& e : expected)
	{
		JsonObject small;
		small.set("path", "interface/mdp1items.sti");
		small.set("subImageIndex", 0);
		JsonObject big;
		big.set("path", "bigitems/p1item00.sti");
		JsonObject graphics;
		graphics.set("small", small.toValue());
		graphics.set("big", big.toValue());
		JsonObject obj;
		obj.set("itemIndex", e.itemIndex);
		obj.set("calibre", "AMMO38");
		obj.set("capacity", 1);
		obj.set("ammoType", "AMMO_REGULAR");
		obj.set("inventoryGraphics", graphics.toValue());

		ASSERT_TRUE(DefaultContentManagerUT::applyWildfireMagazineFixup(obj));
		EXPECT_STREQ(obj.GetString("calibre").c_str(), e.calibre);
		EXPECT_EQ(obj.GetUInt("capacity"), e.capacity);
		EXPECT_STREQ(obj.GetString("ammoType").c_str(), e.ammoType);
		auto fixedGraphics = obj["inventoryGraphics"].toObject();
		EXPECT_STREQ(fixedGraphics["big"].toObject().GetString("path").c_str(), e.bigPath);
		EXPECT_EQ(fixedGraphics["small"].toObject().GetUInt("subImageIndex"), e.smallSubImage);
	}

	JsonObject unchanged;
	unchanged.set("itemIndex", 80);
	EXPECT_FALSE(DefaultContentManagerUT::applyWildfireMagazineFixup(unchanged));
}

TEST(ExternalizedData, readAllData)
{
	DefaultContentManager* cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm->loadGameData());
	delete cm;
}

TEST(ExternalizedData, readEveryFile)
{
	// Not all files (e.g. translations) are covered by the previous test
	DefaultContentManagerUT* cm = DefaultContentManagerUT::createDefaultCMForTesting();

	ST::string dataPath = ST::format("{}/externalized", GetExtraDataDir());
	std::vector<ST::string> results = FileMan::findFilesInDir(dataPath, "json", true, true, false, true);
	for (ST::string f : results)
	{
		ST::string relativePath = f.substr(dataPath.size() + 1);
		auto json = cm->readJsonDataFile(relativePath);
		ASSERT_FALSE(json.get() == NULL);
	}

	delete cm;
}
#endif
