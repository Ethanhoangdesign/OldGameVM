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
	// ID 77 is tested separately below: metadata and Wildfire artwork corrected.
	static constexpr std::array<Expected, 31> expected = {{
		{  71, "AMMO9",    15, "AMMO_AP",      "bigitems/p1item33.sti", 33 },
		{  72, "AMMO9",    30, "AMMO_AP",      "bigitems/p1item36.sti", 36 },
		{  73, "AMMO9",    50, "AMMO_AP",      "bigitems/p1item36.sti", 36 },
		{  74, "AMMO9",    15, "AMMO_HP",      "bigitems/p1item34.sti", 34 },
		{  75, "AMMO9",    30, "AMMO_HP",      "bigitems/p1item37.sti", 37 },
		{  76, "AMMO9",    50, "AMMO_HP",      "bigitems/p1item37.sti", 37 },
		{  78, "AMMO762W", 10, "AMMO_AP",      "bigitems/p1item22.sti", 22 },
		{  79, "AMMO762W", 10, "AMMO_HP",      "bigitems/p1item23.sti", 23 },
		{  86, "AMMO357",   6, "AMMO_AP",      "bigitems/p1item12.sti", 12 },
		{  87, "AMMO357",   9, "AMMO_AP",      "bigitems/p1item18.sti", 18 },
		{  88, "AMMO357",   6, "AMMO_HP",      "bigitems/p1item13.sti", 13 },
		{  89, "AMMO357",   9, "AMMO_HP",      "bigitems/p1item19.sti", 19 },
		{  90, "AMMO545",  30, "AMMO_AP",      "bigitems/p1item09.sti",  9 },
		{  91, "AMMO545",  30, "AMMO_HP",      "bigitems/p1item10.sti", 10 },
		{  92, "AMMO556",  30, "AMMO_AP",      "bigitems/p1item22.sti", 22 },
		{  93, "AMMO556", 100, "AMMO_AP",      "bigitems/p1item22.sti", 22 },
		{  94, "AMMO556",  30, "AMMO_HP",      "bigitems/p1item23.sti", 23 },
		{  95, "AMMO556", 100, "AMMO_HP",      "bigitems/p1item23.sti", 23 },
		{  96, "AMMO762W", 30, "AMMO_AP",      "bigitems/p1item22.sti", 22 },
		{  97, "AMMO762W", 75, "AMMO_AP",      "bigitems/p1item22.sti", 22 },
		{  98, "AMMO762W", 30, "AMMO_HP",      "bigitems/p1item23.sti", 23 },
		{  99, "AMMO762W", 75, "AMMO_HP",      "bigitems/p1item23.sti", 23 },
		{ 100, "AMMO762N", 20, "AMMO_AP",      "bigitems/p1item22.sti", 22 },
		{ 101, "AMMO762N",100, "AMMO_AP",      "bigitems/p1item22.sti", 22 },
		{ 102, "AMMO762N", 20, "AMMO_HP",      "bigitems/p1item23.sti", 23 },
		{ 103, "AMMO762N",100, "AMMO_HP",      "bigitems/p1item23.sti", 23 },
		{ 104, "AMMO762W", 10, "AMMO_AP",      "bigitems/p1item22.sti", 22 },
		{ 105, "AMMO762W", 20, "AMMO_AP",      "bigitems/p1item22.sti", 22 },
		{ 111, "AMMO762N",  5, "AMMO_AP",      "bigitems/p1item110.sti",110 },
		{ 112, "AMMO762N",  5, "AMMO_HE",      "bigitems/p1item115.sti",115 },
		{ 113, "AMMO762N",  5, "AMMO_HEAT",    "bigitems/p1item114.sti",114 },
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

	// ID 77: metadata and evidence-backed Wildfire artwork must be corrected.
	{
		JsonObject obj77;
		obj77.set("itemIndex", 77);
		obj77.set("calibre",   "AMMO38");
		obj77.set("capacity",  6);
		obj77.set("ammoType",  "AMMO_AP");
		JsonObject small77;
		small77.set("path",          "interface/mdp1items.sti");
		small77.set("subImageIndex", 0);
		JsonObject big77;
		big77.set("path", "bigitems/p1item00.sti");
		JsonObject graphics77;
		graphics77.set("small", small77.toValue());
		graphics77.set("big",   big77.toValue());
		obj77.set("inventoryGraphics", graphics77.toValue());

		ASSERT_TRUE(DefaultContentManagerUT::applyWildfireMagazineFixup(obj77));
		EXPECT_STREQ(obj77.GetString("calibre").c_str(),  "AMMO46");
		EXPECT_EQ   (obj77.GetUInt("capacity"),           20u);
		EXPECT_STREQ(obj77.GetString("ammoType").c_str(), "AMMO_REGULAR");

		auto g77 = obj77["inventoryGraphics"].toObject();
		EXPECT_STREQ(g77["small"].toObject().GetString("path").c_str(), "interface/mdguns.sti");
		EXPECT_EQ(g77["small"].toObject().GetUInt("subImageIndex"), 73u);
		EXPECT_STREQ(g77["big"].toObject().GetString("path").c_str(), "bigitems/gun73.sti");
	}

	JsonObject unchanged;
	unchanged.set("itemIndex", 80);
	EXPECT_FALSE(DefaultContentManagerUT::applyWildfireMagazineFixup(unchanged));
}

TEST(WildfireWeaponFixups, SeparateMp7FromP90)
{
	JsonObject mp7;
	mp7.set("itemIndex", 56);
	mp7.set("calibre", "AMMO57");
	mp7.set("ubMagSize", 50);
	ASSERT_TRUE(DefaultContentManagerUT::applyWildfireWeaponFixup(mp7));
	EXPECT_STREQ(mp7.GetString("calibre").c_str(), "AMMO46");
	EXPECT_EQ(mp7.GetUInt("ubMagSize"), 20u);

	JsonObject p90;
	p90.set("itemIndex", 15);
	p90.set("calibre", "AMMO57");
	p90.set("ubMagSize", 50);
	EXPECT_FALSE(DefaultContentManagerUT::applyWildfireWeaponFixup(p90));
	EXPECT_STREQ(p90.GetString("calibre").c_str(), "AMMO57");
	EXPECT_EQ(p90.GetUInt("ubMagSize"), 50u);
}

// WildfireWeaponArtFixups.SeparateMacheteFromVSS: ID 54 uses the
// evidence-backed Wildfire gun47 artwork.

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
