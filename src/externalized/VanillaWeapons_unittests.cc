#include "Exceptions.h"
#ifdef WITH_UNITTESTS
#include "gtest/gtest.h"


#include "AmmoTypeModel.h"
#include "CalibreModel.h"
#include "DefaultContentManager.h"
#include "DefaultContentManagerUT.h"
#include "GameInstance.h"
#include "GamePolicy.h"
#include "Items.h"
#include "MagazineModel.h"
#include "Soldier.h"
#include "Weapons.h"
#include "WeaponModels.h"
#include <utility>

TEST(Items, weaponsLoading)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm != NULL);
	ASSERT_TRUE(cm->loadGameData());
	EXPECT_TRUE(cm->getWeaponByName("MP5K") != NULL);
	EXPECT_TRUE(cm->getWeapon(9 /* MP5K */) != NULL);
	EXPECT_EQ(cm->getWeaponByName("MP5K"), cm->getWeapon(9 /* MP5K */));
	delete cm;
}

TEST(Items, bug120_cawsAmmo)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm->loadGameData());

	// test SAP clip parameters
	const MagazineModel* sapClip = cm->getMagazineByName("CLIPCAWS_10_SAP");
	EXPECT_EQ(sapClip->calibre->internalName, ST::string("AMMOCAWS"));
	EXPECT_EQ(sapClip->ammoType->getInternalName(), ST::string("AMMO_SUPER_AP"));

	// test FLECH clip parameters
	const MagazineModel* flechClip = cm->getMagazineByName("CLIPCAWS_10_FLECH");
	EXPECT_EQ(flechClip->calibre->internalName, ST::string("AMMOCAWS"));
	EXPECT_EQ(flechClip->ammoType->getInternalName(), ST::string("AMMO_BUCKSHOT"));

	delete cm;
}

TEST(Items, bug120_12gAmmo)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm->loadGameData());

	// test SAP clip parameters
	const MagazineModel* clip = cm->getMagazineByName("CLIP12G_7");
	EXPECT_EQ(clip->calibre->internalName, ST::string("AMMO12G"));
	EXPECT_EQ(clip->ammoType->getInternalName(), ST::string("AMMO_REGULAR"));

	// test FLECH clip parameters
	const MagazineModel* clipBuckshot = cm->getMagazineByName("CLIP12G_7_BUCKSHOT");
	EXPECT_EQ(clipBuckshot->calibre->internalName, ST::string("AMMO12G"));
	EXPECT_EQ(clipBuckshot->ammoType->getInternalName(), ST::string("AMMO_BUCKSHOT"));

	delete cm;
}

TEST(Items, bug120_cawsDefaultMag)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm->loadGameData());

	GCM = cm;

	const WeaponModel *caws = cm->getWeaponByName("CAWS");
	ASSERT_TRUE(caws != NULL);

	const MagazineModel* flechClip = cm->getMagazineByName("CLIPCAWS_10_FLECH");
	const MagazineModel* sapClip = cm->getMagazineByName("CLIPCAWS_10_SAP");
	ASSERT_TRUE(flechClip != NULL);
	ASSERT_TRUE(sapClip != NULL);

	EXPECT_EQ(DefaultMagazine(caws->getItemIndex()), flechClip->getItemIndex());

	EXPECT_EQ(FindReplacementMagazine(sapClip->calibre, 10, AMMO_BUCKSHOT), flechClip->getItemIndex());
	EXPECT_EQ(FindReplacementMagazine(sapClip->calibre, 10, AMMO_SUPER_AP), sapClip->getItemIndex());

	delete cm;
}

TEST(Items, vanillaClip38ApCreation)
{
	std::unique_ptr<DefaultContentManager> cm(DefaultContentManagerUT::createDefaultCMForTesting());
	ASSERT_TRUE(cm->loadGameData());
	const auto oldGCM = std::exchange(GCM, cm.release());

	OBJECTTYPE object{};
	CreateItem(ITEMDEFINE::__ITEM_78, 100, &object);

	EXPECT_EQ(object.usItem, ITEMDEFINE::__ITEM_78);
	EXPECT_EQ(object.ubNumberOfObjects, 1);
	EXPECT_EQ(object.ubShotsLeft[0], 6);
	const auto* magazine = GCM->getMagazineByItemIndex(ITEMDEFINE::__ITEM_78);
	ASSERT_NE(magazine, nullptr);
	EXPECT_EQ(magazine->calibre->internalName, ST::string("AMMO38"));
	EXPECT_EQ(magazine->capacity, 6);
	EXPECT_EQ(magazine->ammoType->getInternalName(), ST::string("AMMO_AP"));

	delete GCM;
	GCM = oldGCM;
}

TEST(Items, vanillaP90KeepsApMagazine)
{
	std::unique_ptr<DefaultContentManager> cm(DefaultContentManagerUT::createDefaultCMForTesting());
	ASSERT_TRUE(cm->loadGameData());
	auto const oldGCM = std::exchange(GCM, cm.release());

	OBJECTTYPE p90{};
	CreateItem(ITEMDEFINE::__ITEM_15, 100, &p90);
	ASSERT_EQ(p90.usGunAmmoItem, ITEMDEFINE::__ITEM_105);
	ASSERT_EQ(p90.ubGunAmmoType, AMMO_AP);
	p90.ubGunShotsLeft = 7;

	OBJECTTYPE unloaded{};
	EXPECT_TRUE(EmptyWeaponMagazine(&p90, &unloaded));
	EXPECT_EQ(unloaded.usItem, ITEMDEFINE::__ITEM_105);
	EXPECT_EQ(unloaded.ubShotsLeft[0], 7);

	delete GCM;
	GCM = oldGCM;
}

TEST(Items, bug120_spas15DefaultMag)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm->loadGameData());

	GCM = cm;

	const WeaponModel *spas15 = cm->getWeaponByName("SPAS15");
	ASSERT_TRUE(spas15 != NULL);

	const MagazineModel* clipBuckshot = cm->getMagazineByName("CLIP12G_7_BUCKSHOT");
	const MagazineModel* clip = cm->getMagazineByName("CLIP12G_7");
	ASSERT_TRUE(clipBuckshot != NULL);
	ASSERT_TRUE(clip != NULL);

	EXPECT_EQ(DefaultMagazine(spas15->getItemIndex()), clipBuckshot->getItemIndex());

	EXPECT_EQ(FindReplacementMagazine(clip->calibre, 7, AMMO_BUCKSHOT), clipBuckshot->getItemIndex());
	EXPECT_EQ(FindReplacementMagazine(clip->calibre, 7, AMMO_REGULAR),  clip->getItemIndex());

	delete cm;
}

TEST(Items, ValidLaunchable)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm->loadGameData());

	GCM = cm;

	EXPECT_TRUE(ValidLaunchable(MORTAR_SHELL, MORTAR));
	EXPECT_FALSE(ValidLaunchable(MORTAR_SHELL, MORTAR_SHELL));
	EXPECT_FALSE(ValidLaunchable(MORTAR_SHELL, TANK_CANNON));
	EXPECT_FALSE(ValidLaunchable(MORTAR, MORTAR_SHELL));
	EXPECT_TRUE(ValidLaunchable(GL_HE_GRENADE, GLAUNCHER));
	EXPECT_TRUE(ValidLaunchable(GL_HE_GRENADE, UNDER_GLAUNCHER));

	// Check if the function handles some random garbage input
	EXPECT_FALSE(ValidLaunchable(BATTERIES, WINE));
	EXPECT_FALSE(ValidLaunchable(0xf123, 0x97b2));

	delete cm;
}

TEST(Items, GetLauncherFromLaunchable)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm->loadGameData());

	GCM = cm;

	EXPECT_EQ(GetLauncherFromLaunchable(GL_TEARGAS_GRENADE), GLAUNCHER);
	EXPECT_EQ(GetLauncherFromLaunchable(MORTAR_SHELL), MORTAR);
	EXPECT_EQ(GetLauncherFromLaunchable(TANK_SHELL), TANK_CANNON);
	EXPECT_EQ(GetLauncherFromLaunchable(TANK_CANNON), NOTHING);

	// Check if the function handles some random garbage input
	EXPECT_EQ(GetLauncherFromLaunchable(G11), NOTHING);
	EXPECT_EQ(GetLauncherFromLaunchable(0xe941), NOTHING);

	delete cm;
}

TEST(Items, ValidAttachment)
{
	std::unique_ptr<DefaultContentManager> cm(DefaultContentManagerUT::createDefaultCMForTesting());
	ASSERT_TRUE(cm->loadGameData());
	auto const oldGCM = std::exchange(GCM, cm.release());

	bool& extra_attachments = const_cast<GamePolicy *>(GCM->getGamePolicy())->extra_attachments;

	extra_attachments = false;
	EXPECT_TRUE(ValidAttachment(ITEMDEFINE::DETONATOR, ITEMDEFINE::HMX));
	EXPECT_TRUE(ValidAttachment(ITEMDEFINE::DETONATOR, ITEMDEFINE::TNT));
	EXPECT_TRUE(ValidAttachment(ITEMDEFINE::CHEWING_GUM, ITEMDEFINE::FUMBLE_PAK));
	EXPECT_FALSE(ValidAttachment(ITEMDEFINE::UVGOGGLES, ITEMDEFINE::SPECTRA_HELMET));;
	EXPECT_FALSE(ValidAttachment(ITEMDEFINE::SUNGOGGLES, ITEMDEFINE::SPECTRA_HELMET));;
	EXPECT_FALSE(ValidAttachment(ITEMDEFINE::ADRENALINE_BOOSTER, ITEMDEFINE::KEVLAR_LEGGINGS_Y));
	EXPECT_FALSE(ValidAttachment(ITEMDEFINE::AUTO_ROCKET_RIFLE, ITEMDEFINE::BRASS_KNUCKLES));
	EXPECT_FALSE(ValidAttachment(0xf083, 0x8c12)); // Random junk crashes the old version of ValidAttachment

	// Next test relies on a certain order of the vests
	static_assert(ITEMDEFINE::SPECTRA_VEST_Y - ITEMDEFINE::FLAK_JACKET == 8);
	int count = 0;
	for (int i = ITEMDEFINE::FLAK_JACKET; i <= ITEMDEFINE::SPECTRA_VEST_Y; ++i)
	{
		if (ValidAttachment(ITEMDEFINE::CERAMIC_PLATES, i)) ++count;
	}
	EXPECT_EQ(count, 9);

	extra_attachments = true;
	EXPECT_TRUE(ValidAttachment(ITEMDEFINE::DETONATOR, ITEMDEFINE::HMX));
	EXPECT_TRUE(ValidAttachment(ITEMDEFINE::DETONATOR, ITEMDEFINE::TNT));
	EXPECT_TRUE(ValidAttachment(ITEMDEFINE::CHEWING_GUM, ITEMDEFINE::FUMBLE_PAK));
	EXPECT_TRUE(ValidAttachment(ITEMDEFINE::UVGOGGLES, ITEMDEFINE::SPECTRA_HELMET));;
	EXPECT_TRUE(ValidAttachment(ITEMDEFINE::UVGOGGLES, ITEMDEFINE::KEVLAR_HELMET_18));;
	EXPECT_TRUE(ValidAttachment(ITEMDEFINE::SUNGOGGLES, ITEMDEFINE::SPECTRA_HELMET));;
	EXPECT_TRUE(ValidAttachment(ITEMDEFINE::ADRENALINE_BOOSTER, ITEMDEFINE::KEVLAR_LEGGINGS_Y));
	EXPECT_FALSE(ValidAttachment(ITEMDEFINE::SUNGOGGLES, ITEMDEFINE::SPECTRA_VEST));;
	EXPECT_FALSE(ValidAttachment(ITEMDEFINE::AUTO_ROCKET_RIFLE, ITEMDEFINE::BRASS_KNUCKLES));
	EXPECT_FALSE(ValidAttachment(0xf083, 0x8c12));

	delete GCM;
	GCM = oldGCM;
}

TEST(Items, WildfireMp7MagazineCompatibility)
{
	std::unique_ptr<DefaultContentManager> cm(DefaultContentManagerUT::createWildfireCMForTesting());
	ASSERT_TRUE(cm->loadGameData());
	auto const oldGCM = std::exchange(GCM, cm.release());

	const auto* mp7 = GCM->getWeapon(ITEMDEFINE::AUTOMAG_III);
	const auto* p90 = GCM->getWeapon(ITEMDEFINE::__ITEM_15);
	const auto* magazine46 = GCM->getMagazineByItemIndex(ITEMDEFINE::CLIP38_6);
	const auto* magazine57 = GCM->getMagazineByItemIndex(ITEMDEFINE::__ITEM_106);
	ASSERT_NE(mp7, nullptr);
	ASSERT_NE(p90, nullptr);
	ASSERT_NE(magazine46, nullptr);
	ASSERT_NE(magazine57, nullptr);

	EXPECT_EQ(mp7->calibre->internalName, ST::string("AMMO46"));
	EXPECT_EQ(mp7->ubMagSize, 20);
	EXPECT_EQ(magazine46->calibre->internalName, ST::string("AMMO46"));
	EXPECT_EQ(magazine46->capacity, 20);
	EXPECT_EQ(p90->calibre->internalName, ST::string("AMMO57"));
	EXPECT_EQ(p90->ubMagSize, 50);
	EXPECT_EQ(DefaultMagazine(ITEMDEFINE::AUTOMAG_III), ITEMDEFINE::CLIP38_6);
	EXPECT_TRUE(ValidAmmoType(ITEMDEFINE::AUTOMAG_III, ITEMDEFINE::CLIP38_6));
	EXPECT_FALSE(ValidAmmoType(ITEMDEFINE::__ITEM_15, ITEMDEFINE::CLIP38_6));
	EXPECT_TRUE(ValidAmmoType(ITEMDEFINE::__ITEM_15, ITEMDEFINE::__ITEM_106));

	OBJECTTYPE p90Object{};
	CreateItem(ITEMDEFINE::__ITEM_15, 100, &p90Object);
	EXPECT_EQ(p90Object.usGunAmmoItem, ITEMDEFINE::__ITEM_106);
	EXPECT_EQ(GCM->getMagazineByItemIndex(p90Object.usGunAmmoItem)->calibre->internalName, ST::string("AMMO57"));
	EXPECT_EQ(GCM->getMagazineByItemIndex(p90Object.usGunAmmoItem)->capacity, 50);
	EXPECT_EQ(FindReplacementMagazine(p90->calibre, 50, AMMO_AP), ITEMDEFINE::NOTHING);

	SOLDIERTYPE soldier{};
	soldier.bVisible = -1;
	soldier.bTeam = ENEMY_TEAM;

	OBJECTTYPE magazine57Object{};
	CreateItem(ITEMDEFINE::__ITEM_106, 100, &magazine57Object);
	p90Object.usGunAmmoItem = ITEMDEFINE::__ITEM_105;
	p90Object.ubGunAmmoType = AMMO_AP;
	p90Object.ubGunShotsLeft = 18;
	EXPECT_TRUE(ReloadGun(&soldier, &p90Object, &magazine57Object));
	EXPECT_EQ(p90Object.usGunAmmoItem, ITEMDEFINE::__ITEM_106);
	EXPECT_EQ(p90Object.ubGunAmmoType, AMMO_HP);
	EXPECT_EQ(p90Object.ubGunShotsLeft, 50);
	EXPECT_EQ(magazine57Object.usItem, ITEMDEFINE::__ITEM_106);
	EXPECT_EQ(magazine57Object.ubShotsLeft[0], 18);

	OBJECTTYPE unloadedP90{};
	EXPECT_TRUE(EmptyWeaponMagazine(&p90Object, &unloadedP90));
	EXPECT_EQ(unloadedP90.usItem, ITEMDEFINE::__ITEM_106);
	EXPECT_EQ(unloadedP90.ubShotsLeft[0], 50);
	EXPECT_NE(unloadedP90.usItem, ITEMDEFINE::__ITEM_105);

	CreateItem(ITEMDEFINE::__ITEM_15, 100, &p90Object);
	p90Object.usGunAmmoItem = ITEMDEFINE::__ITEM_78;
	p90Object.ubGunAmmoType = AMMO_AP;
	p90Object.ubGunShotsLeft = 10;
	EXPECT_TRUE(EmptyWeaponMagazine(&p90Object, &unloadedP90));
	EXPECT_EQ(unloadedP90.usItem, ITEMDEFINE::__ITEM_106);
	EXPECT_EQ(unloadedP90.ubShotsLeft[0], 10);
	EXPECT_NE(unloadedP90.usItem, ITEMDEFINE::__ITEM_78);

	CreateItem(ITEMDEFINE::__ITEM_15, 100, &p90Object);
	p90Object.ubGunShotsLeft = 7;
	EXPECT_TRUE(EmptyWeaponMagazine(&p90Object, &unloadedP90));
	EXPECT_EQ(unloadedP90.usItem, ITEMDEFINE::__ITEM_106);
	EXPECT_EQ(unloadedP90.ubShotsLeft[0], 7);

	OBJECTTYPE mp7Object{};
	OBJECTTYPE magazine46Object{};
	CreateItem(ITEMDEFINE::AUTOMAG_III, 100, &mp7Object);
	EXPECT_EQ(mp7Object.usGunAmmoItem, ITEMDEFINE::CLIP38_6);
	EXPECT_EQ(mp7Object.ubGunShotsLeft, 20);

	OBJECTTYPE unloaded{};
	EXPECT_TRUE(EmptyWeaponMagazine(&mp7Object, &unloaded));
	EXPECT_EQ(unloaded.usItem, ITEMDEFINE::CLIP38_6);
	EXPECT_EQ(unloaded.ubShotsLeft[0], 20);

	CreateItem(ITEMDEFINE::CLIP38_6, 100, &magazine46Object);
	EXPECT_EQ(magazine46Object.usItem, ITEMDEFINE::CLIP38_6);
	EXPECT_EQ(magazine46Object.ubShotsLeft[0], 20);
	EXPECT_NE(mp7Object.usGunAmmoItem, ITEMDEFINE::__ITEM_78);

	EXPECT_TRUE(ReloadGun(&soldier, &mp7Object, &magazine46Object));
	EXPECT_EQ(mp7Object.usGunAmmoItem, ITEMDEFINE::CLIP38_6);
	EXPECT_EQ(mp7Object.ubGunShotsLeft, 20);
	EXPECT_TRUE(EmptyWeaponMagazine(&mp7Object, &unloaded));
	EXPECT_EQ(unloaded.usItem, ITEMDEFINE::CLIP38_6);
	EXPECT_NE(unloaded.usItem, ITEMDEFINE::__ITEM_78);

	delete GCM;
	GCM = oldGCM;
}

TEST(Items, P90SilencerCompatibility)
{
	std::unique_ptr<DefaultContentManager> cm(DefaultContentManagerUT::createDefaultCMForTesting());
	ASSERT_TRUE(cm->loadGameData());
	auto const oldGCM = std::exchange(GCM, cm.release());

	auto const* p90 = dynamic_cast<const WeaponModel *>(GCM->getItem(ITEMDEFINE::__ITEM_15));
	auto const* silencer = GCM->getItem(ITEMDEFINE::SILENCER);
	ASSERT_TRUE(p90 != nullptr);
	ASSERT_TRUE(silencer != nullptr);
	ASSERT_EQ(silencer->getItemIndex(), ITEMDEFINE::SILENCER);

	EXPECT_TRUE(p90->canBeAttached(GCM->getGamePolicy(), silencer));
	EXPECT_TRUE(ValidAttachment(ITEMDEFINE::SILENCER, ITEMDEFINE::__ITEM_15));
	EXPECT_FALSE(ValidAttachment(ITEMDEFINE::SILENCER, ITEMDEFINE::HMX));

	OBJECTTYPE p90Object{};
	OBJECTTYPE silencerObject{};
	CreateItem(ITEMDEFINE::__ITEM_15, 100, &p90Object);
	CreateItem(ITEMDEFINE::SILENCER, 100, &silencerObject);
	EXPECT_TRUE(AttachObject(nullptr, &p90Object, &silencerObject));
	EXPECT_EQ(silencerObject.usItem, ITEMDEFINE::NOTHING);
	EXPECT_NE(FindAttachment(&p90Object, ITEMDEFINE::SILENCER), ITEM_NOT_FOUND);

	delete GCM;
	GCM = oldGCM;
}

TEST(Items, CompatibleFaceItem)
{
	EXPECT_TRUE(CompatibleFaceItem(ITEMDEFINE::NIGHTGOGGLES, ITEMDEFINE::EXTENDEDEAR));
	EXPECT_TRUE(CompatibleFaceItem(ITEMDEFINE::EXTENDEDEAR, ITEMDEFINE::NIGHTGOGGLES));
	EXPECT_FALSE(CompatibleFaceItem(ITEMDEFINE::EXTENDEDEAR, ITEMDEFINE::EXTENDEDEAR));
	EXPECT_TRUE(CompatibleFaceItem(ITEMDEFINE::WALKMAN, ITEMDEFINE::GASMASK));
	EXPECT_FALSE(CompatibleFaceItem(ITEMDEFINE::UVGOGGLES, ITEMDEFINE::RDX));
	EXPECT_TRUE(CompatibleFaceItem(0xda83, NOTHING)); // item2 == NOTHING is a special case
	EXPECT_FALSE(CompatibleFaceItem(0x75e4, 0xcafe));
	for (int i = 0; i <= 0xffff; ++i)
	{
		EXPECT_FALSE(CompatibleFaceItem(i, ITEMDEFINE::STEEL_HELMET));
	}
}

TEST(Items, Invalid_ItemIndex_Exception)
{
	std::unique_ptr<DefaultContentManager> cm(DefaultContentManagerUT::createDefaultCMForTesting());
	ASSERT_TRUE(cm->loadGameData());
	auto const oldGCM = std::exchange(GCM, cm.release());

	for (uint16_t i = UINT16_MAX; i >= MAXITEMS; --i)
	{
		EXPECT_THROW(GCM->getItem(i), NotFoundError);
	}

	EXPECT_NO_THROW(GCM->getItem(57000, ItemSystem::nothrow));
	EXPECT_EQ(GCM->getItem(38000, ItemSystem::nothrow), nullptr);

	delete GCM;
	GCM = oldGCM;
}

#endif
