/* OldGameVM modification notice
 * This file was changed for OldGameVM in July 2026.
 * It is not the original file. See NOTICE.md.
 */
#include "logo32.png.h"
#include "Logger.h"
#include "RustInterface.h"
#include "FileMan.h"
#include "Types.h"
#include "GameRes.h"
#include "Video.h"

#include "Launcher.h"
#include "EditionDetector.h"

#include "FL/Fl_Native_File_Chooser.H"
#include <FL/Fl_PNG_Image.H>
#include <FL/fl_ask.H>
#include <string_theory/string>

#include <filesystem>
#include <algorithm>
#include <vector>
#include <limits>
#include <cstring>
#include <fstream>   /* OGVM-CONTROLLER: doc/ghi controller.ini */
#include <SDL.h>

#define RESOLUTION_SEPARATOR "x"

const double checkGameRunningIntervalSeconds = 1.0;

const Fl_Text_Display::Style_Table_Entry styleTable[] = {
	{  FL_BLACK,		FL_COURIER_BOLD,	14 }, // A - Header
	{  FL_BLACK,		FL_COURIER,			14 }, // B - Text
	{  FL_DARK_RED,		FL_COURIER,			14 }, // B - Error Text
};

/* RECRES: default to the size the new layout targets. */
const char* defaultResolution = "1024x768";

const std::vector<GameVersion> predefinedVersions = {
	GameVersion::DUTCH,
	GameVersion::ENGLISH,
	GameVersion::FRENCH,
	GameVersion::GERMAN,
	GameVersion::ITALIAN,
	GameVersion::POLISH,
	GameVersion::RUSSIAN,
	GameVersion::RUSSIAN_GOLD,
	GameVersion::SIMPLIFIED_CHINESE
};
const std::vector< std::pair<int, int> > predefinedResolutions = {
    /* RESLIST-LOCK: only the modes that have actually been played through are
     * offered here. 1024x768 is Wildfire's native size -- its interface art is
     * exactly 1024px wide, so nothing has to be stretched or tiled. 1366x768 is
     * the layout that was tuned screen by screen. 640x480 was dropped from
     * this menu too: nobody plays at that size and tuning it costs real time.
     * Every 640x480 code path is still in place though, so it can still be
     * entered by hand in the two resolution fields when the original layout
     * needs to be compared against. Wider modes were removed on
     * purpose: most screens are still hard-coded to the 640x480 layout, and some
     * of them (the auto-resolve battle screen above all) become unusable on very
     * wide screens. Do not add modes back without playing through a battle, the
     * strategic map and the sector inventory at that size first. */
    std::make_pair(1024, 768),
    std::make_pair(1366, 768),
};
const std::vector<VideoScaleQuality> scalingModes = {
	VideoScaleQuality::LINEAR,
	VideoScaleQuality::NEAR_PERFECT,
	VideoScaleQuality::PERFECT,
};

void showError(const ST::string& error) {
	fl_message_title("Error");
	fl_alert("%s", error.c_str());
}

void showRustError() {
	RustPointer<char> err(getRustError());
	if (err) {
		SLOGE("{}", err.get());
		showError(err.get());
	} else {
		showError("showRustError called but no rust error is present");
	}
}

ST::string encodePath(const char* path) {
	if (path == nullptr) {
		return ST::string();
	}
	RustPointer<char> encodedPath(Path_encodeU8(reinterpret_cast<const uint8_t*>(path), strlen(path)));
	return ST::string(encodedPath.get());
}

ST::char_buffer decodePath(const char* path) {
	if (path == nullptr) {
		return ST::char_buffer{};
	}
	ST::char_buffer buf{ST::char_buffer::strlen(path), '\0'}; // the decoded size always fits in the original size
	size_t len = Path_decodeU8(path, reinterpret_cast<uint8_t*>(buf.data()), buf.size());
	if (len > buf.size()) {
		showRustError();
		return ST::char_buffer{};
	}
	return ST::char_buffer{buf.c_str(), len};
}


/* OGVM early forward — browse/import callbacks sit above OGVM block */
static void OgvmRefreshEditionLabel(Launcher* window);
Launcher::Launcher(int argc, char* argv[]) : StracciatellaLauncher() {
	this->argc = argc;
	this->argv = argv;
}

Launcher::~Launcher() {
	closeControllerPad();
}

void Launcher::loadJa2Json() {
	RustPointer<char> configFolderPath(EngineOptions_getStracciatellaHome());
	if (configFolderPath.get() == NULL) {
		auto rustError = getRustError();
		if (rustError != NULL) {
			SLOGE("Failed to find home directory: {}", rustError);
		}
	}

	this->engineOptions.reset(EngineOptions_create(configFolderPath.get(), argv, argc));
	this->modManager.reset(ModManager_createUnchecked(this->engineOptions.get()));

	if (this->engineOptions == NULL) {
		exit(EXIT_FAILURE);
	}
	if (EngineOptions_shouldShowHelp(this->engineOptions.get())) {
		exit(EXIT_SUCCESS);
	}
}


/* OGVM-FLUENT2-DARK ----------------------------------------------------------
 * WinUI3 / Fluent 2 dual-mode theme for FLTK.
 * Light: F3F3F3 bg, 0078D4 accent.
 * Dark : 202020 bg, 60CDFF accent (WinUI3 dark accent).
 */

/* Forward so toggle button callback can call it */
static void OgvmApplyFluent2Theme(Launcher* w, bool dark);
static bool gOgvmDarkMode = false;

/* macOS system dark mode detection (compile-time guard) */
#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_MAC
#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>
static bool OgvmSystemIsDark()
{
    /* NSApp.effectiveAppearance.name contains "Dark" when in dark mode. */
    id app = ((id(*)(Class,SEL))objc_msgSend)(objc_getClass("NSApplication"),
              sel_registerName("sharedApplication"));
    if (!app) return false;
    id ap = ((id(*)(id,SEL))objc_msgSend)(app,
              sel_registerName("effectiveAppearance"));
    if (!ap) return false;
    id name = ((id(*)(id,SEL))objc_msgSend)(ap, sel_registerName("name"));
    if (!name) return false;
    const char* cs = ((const char*(*)(id,SEL))objc_msgSend)(name,
                       sel_registerName("UTF8String"));
    if (!cs) return false;
    return strstr(cs, "Dark") != nullptr;
}
#else
static bool OgvmSystemIsDark() { return false; }
#endif
#else
static bool OgvmSystemIsDark() { return false; }
#endif

static const char* OgvmThemeIcon(bool dark) { return dark ? "\xe2\x98\x80" : "\xe2\x98\xbd"; }

static void OgvmThemeToggleCb(Fl_Widget* btn, void* userdata)
{
    Launcher* window = static_cast<Launcher*>(userdata);
    gOgvmDarkMode = !gOgvmDarkMode;
    OgvmApplyFluent2Theme(window, gOgvmDarkMode);
    btn->label(OgvmThemeIcon(gOgvmDarkMode));
    btn->redraw();
}

static void OgvmApplyFluent2Theme(Launcher* window, bool dark)
{
    if (!window || !window->stracciatellaLauncher) return;

    /* ---- Palettes -------------------------------------------------------- */
    Fl_Color bg, card, text, secondary, accent, accentDk, accentFg,
             green, stroke, tabBar;

    if (!dark) {
        /* Fluent 2 Light */
        bg        = fl_rgb_color(243, 243, 243);
        card      = fl_rgb_color(255, 255, 255);
        text      = fl_rgb_color( 32,  31,  30);
        secondary = fl_rgb_color( 96,  94,  92);
        accent    = fl_rgb_color(  0, 120, 212);
        accentDk  = fl_rgb_color(  0,  90, 158);
        accentFg  = FL_WHITE;
        green     = fl_rgb_color( 16, 124,  16);
        stroke    = fl_rgb_color(237, 235, 233);
        tabBar    = fl_rgb_color(243, 243, 243);
        Fl::background(243, 243, 243);
        Fl::background2(255, 255, 255);
        Fl::foreground( 32,  31,  30);
    } else {
        /* WinUI3 Dark */
        bg        = fl_rgb_color( 32,  32,  32);
        card      = fl_rgb_color( 43,  43,  43);
        text      = fl_rgb_color(255, 255, 255);
        secondary = fl_rgb_color(160, 160, 160);
        accent    = fl_rgb_color( 96, 205, 255);   /* 60CDFF */
        accentDk  = fl_rgb_color( 60, 180, 230);
        accentFg  = fl_rgb_color( 32,  32,  32);
        green     = fl_rgb_color( 92, 192,  92);
        stroke    = fl_rgb_color( 60,  60,  60);
        tabBar    = fl_rgb_color( 40,  40,  40);
        Fl::background( 32,  32,  32);
        Fl::background2( 43,  43,  43);
        Fl::foreground(255, 255, 255);
    }

    Fl::scheme("none");
    fl_message_font(FL_HELVETICA, 13);

    window->stracciatellaLauncher->color(bg);
    window->stracciatellaLauncher->labelcolor(text);

    if (window->tabs) {
        window->tabs->color(tabBar);
        window->tabs->selection_color(card);
        window->tabs->labelcolor(secondary);
        window->tabs->labelsize(14);
    }

    /* Play button */
    if (window->playButton) {
        window->playButton->box(FL_ROUNDED_BOX);
        window->playButton->color(accent);
        window->playButton->selection_color(accentDk);
        window->playButton->labelcolor(accentFg);
        window->playButton->labelfont(FL_HELVETICA_BOLD);
        window->playButton->labelsize(16);
    }
    if (window->editorButton) {
        window->editorButton->box(FL_ROUNDED_BOX);
        window->editorButton->color(card);
        window->editorButton->labelcolor(accent);
        window->editorButton->labelsize(13);
    }

    /* Inputs */
    Fl_Input* ins[] = { window->gameDirectoryInput, window->saveGameDirectoryInput };
    for (Fl_Input* w : ins) {
        if (!w) continue;
        w->box(FL_ROUNDED_BOX); w->color(card);
        w->textcolor(text); w->textsize(13); w->textfont(FL_HELVETICA);
        w->labelsize(12); w->labelcolor(secondary);
    }
    if (window->gameSettingsOutput) {
        window->gameSettingsOutput->box(FL_ROUNDED_BOX);
        window->gameSettingsOutput->color(card);
        window->gameSettingsOutput->textcolor(secondary);
        window->gameSettingsOutput->textsize(12); window->gameSettingsOutput->labelsize(12);
        window->gameSettingsOutput->labelcolor(secondary);
    }
    if (window->ja2JsonPathOutput) {
        window->ja2JsonPathOutput->box(FL_FLAT_BOX);
        window->ja2JsonPathOutput->color(bg);
        window->ja2JsonPathOutput->textcolor(secondary);
        window->ja2JsonPathOutput->textsize(11);
    }

    /* Dropdowns */
    Fl_Choice* chs[] = { window->gameVersionInput, window->scalingModeChoice };
    for (Fl_Choice* w : chs) {
        if (!w) continue;
        w->box(FL_ROUNDED_BOX); w->down_box(FL_BORDER_BOX);
        w->color(card); w->textcolor(text); w->textsize(13);
        w->textfont(FL_HELVETICA); w->labelcolor(secondary); w->labelsize(12);
    }

    /* Secondary buttons */
    Fl_Button* sec[] = {
        window->browseJa2DirectoryButton,
        window->browseSaveGameDirectoryButton,
        window->guessVersionButton,
        window->detectEditionButton,
        window->ja2JsonReloadBtn,
        window->ja2JsonSaveBtn,
        window->enableModsButton,
        window->disableModsButton,
        window->moveUpModsButton,
        window->moveDownModsButton,
    };
    for (Fl_Button* w : sec) {
        if (!w) continue;
        w->box(FL_ROUNDED_BOX); w->color(card);
        w->labelcolor(accent); w->labelsize(13);
    }
    if (window->predefinedResolutionMenuButton) {
        window->predefinedResolutionMenuButton->box(FL_ROUNDED_BOX);
        window->predefinedResolutionMenuButton->color(card);
        window->predefinedResolutionMenuButton->labelcolor(text);
        window->predefinedResolutionMenuButton->labelsize(13);
    }
    if (window->importGameDataButton) {
        window->importGameDataButton->box(FL_ROUNDED_BOX);
        window->importGameDataButton->color(accent);
        window->importGameDataButton->selection_color(accentDk);
        window->importGameDataButton->labelcolor(accentFg);
        window->importGameDataButton->labelfont(FL_HELVETICA_BOLD);
        window->importGameDataButton->labelsize(14);
    }

    /* Radios + checkboxes */
    for (Fl_Round_Button* w : {window->sourceInstallerRadio, window->sourceFolderRadio}) {
        if (!w) continue;
        w->labelsize(13); w->labelcolor(text);
        w->labelfont(FL_HELVETICA); w->selection_color(accent);
    }
    for (Fl_Check_Button* w : {window->fullscreenCheckbox, window->playSoundsCheckbox}) {
        if (!w) continue;
        w->labelcolor(text); w->selection_color(accent); w->labelsize(13);
    }

    /* OGVM-CONTROLLER widgets */
    if (window->controllerEnableToggle) {
        window->controllerEnableToggle->labelcolor(text);
        window->controllerEnableToggle->selection_color(accent);
        window->controllerEnableToggle->labelsize(13);
    }
    if (window->controllerLayoutChoice) {
        window->controllerLayoutChoice->color(card);
        window->controllerLayoutChoice->textcolor(text);
        window->controllerLayoutChoice->labelsize(12);
    }
    if (window->leftStickModeChoice) {
        window->leftStickModeChoice->color(card);
        window->leftStickModeChoice->textcolor(text);
        window->leftStickModeChoice->labelsize(11);
        window->leftStickModeChoice->textsize(11);
    }
    if (window->rightStickModeChoice) {
        window->rightStickModeChoice->color(card);
        window->rightStickModeChoice->textcolor(text);
        window->rightStickModeChoice->labelsize(11);
        window->rightStickModeChoice->textsize(11);
    }
    if (window->leftStickLabel) {
        window->leftStickLabel->labelcolor(text);
        window->leftStickLabel->labelsize(11);
    }
    if (window->rightStickLabel) {
        window->rightStickLabel->labelcolor(text);
        window->rightStickLabel->labelsize(11);
    }
    if (window->controllerStatusLabel) {
        window->controllerStatusLabel->labelfont(FL_HELVETICA);
        window->controllerStatusLabel->labelsize(12);
        window->controllerStatusLabel->labelcolor(secondary);
    }
    if (window->controllerView) {
        window->controllerView->color(card);
    }
    if (window->controllerHelp) {
        window->controllerHelp->color(card);
        window->controllerHelp->textsize(11);
        window->controllerHelp->textfont(FL_COURIER);
        window->controllerHelp->textcolor(text);
    }
    if (window->controllerBindScroll) {
        window->controllerBindScroll->color(card);
    }
    for (int i = 0; i < StracciatellaLauncher::kPadBindCount; ++i) {
        if (window->padLabel[i]) {
            window->padLabel[i]->labelcolor(text);
            window->padLabel[i]->labelsize(11);
        }
        if (window->padKindChoice[i]) {
            window->padKindChoice[i]->color(card);
            window->padKindChoice[i]->textcolor(text);
            window->padKindChoice[i]->labelsize(11);
            window->padKindChoice[i]->textsize(11);
        }
        if (window->padValueChoice[i]) {
            window->padValueChoice[i]->color(card);
            window->padValueChoice[i]->textcolor(text);
            window->padValueChoice[i]->labelsize(11);
            window->padValueChoice[i]->textsize(11);
        }
    }

    /* Status */
    if (window->sourceStatusLabel) {
        window->sourceStatusLabel->labelfont(FL_HELVETICA);
        window->sourceStatusLabel->labelsize(13);
        window->sourceStatusLabel->labelcolor(green);
    }
    if (window->importProgress) {
        window->importProgress->color(stroke);
        window->importProgress->selection_color(accent);
        window->importProgress->labelsize(12);
    }

    /* Value inputs */
    for (Fl_Value_Input* w : {window->resolutionXInput, window->resolutionYInput}) {
        if (!w) continue;
        w->box(FL_ROUNDED_BOX); w->color(card);
        w->textsize(13); w->labelsize(12); w->labelcolor(secondary);
    }

    /* Browsers / logs */
    if (window->enabledModsBrowser) {
        window->enabledModsBrowser->color(card);
        window->enabledModsBrowser->textsize(13); window->enabledModsBrowser->textfont(FL_HELVETICA);
        window->enabledModsBrowser->labelsize(12); window->enabledModsBrowser->labelcolor(secondary);
    }
    if (window->availableModsBrowser) {
        window->availableModsBrowser->color(card);
        window->availableModsBrowser->textsize(13); window->availableModsBrowser->textfont(FL_HELVETICA);
        window->availableModsBrowser->labelsize(12); window->availableModsBrowser->labelcolor(secondary);
    }
    if (window->logsDisplay) {
        window->logsDisplay->color(card);
        window->logsDisplay->textsize(12); window->logsDisplay->textfont(FL_COURIER);
        window->logsDisplay->labelcolor(secondary);
    }
    if (window->modDetails) { window->modDetails->color(card); }

    window->stracciatellaLauncher->redraw();
}

void Launcher::show() {
	editorButton->callback( (Fl_Callback*)startEditor, (void*)(this) );
	/* OGVM-AUTODETECT: khai bao truoc; dinh nghia nam gan detectEditionCb */
	void OgvmAutodetectTimeoutCb(void* userdata);
	void OgvmSourceRadioCb(Fl_Widget* w, void* userdata);   /* OGVM-RADIO */
	playButton->callback( (Fl_Callback*)startGame, (void*)(this) );
	gameDirectoryInput->callback( (Fl_Callback*)widgetChanged, (void*)(this) );
	saveGameDirectoryInput->callback( (Fl_Callback*)widgetChanged, (void*)(this) );
	browseJa2DirectoryButton->callback((Fl_Callback *) openGameDirectorySelector, (void *) (this));
	browseSaveGameDirectoryButton->callback((Fl_Callback *) openSaveGameDirectorySelector, (void *) (this));
	gameVersionInput->callback( (Fl_Callback*)selectGameVersion, (void*)(this) );
	guessVersionButton->callback( (Fl_Callback*)guessVersion, (void*)(this) );
    detectEditionButton->callback( (Fl_Callback*)detectEditionCb, (void*)(this) );
    importGameDataButton->callback( (Fl_Callback*)importGameDataCb, (void*)(this) );
    sourceInstallerRadio->callback(OgvmSourceRadioCb, (void*)(this));   /* OGVM-RADIO */
    sourceFolderRadio->callback(OgvmSourceRadioCb, (void*)(this));
    /* OGVM-AUTODETECT: do phien ban ngay khi mo launcher. Cho 0,5 giay de
     * o duong dan kip nap gia tri tu ~/.ja2/ja2.json truoc khi do. */
    Fl::add_timeout(0.5, OgvmAutodetectTimeoutCb, (void*)(this));
	scalingModeChoice->callback( (Fl_Callback*)widgetChanged, (void*)(this) );
	resolutionXInput->callback( (Fl_Callback*)widgetChanged, (void*)(this) );
	resolutionYInput->callback( (Fl_Callback*)widgetChanged, (void*)(this) );
	RustPointer<char> game_json_path(findPathFromAssetsDir("externalized/game.json", true, true));
	if (game_json_path) {
		gameSettingsOutput->value(game_json_path.get());
	} else {
		gameSettingsOutput->value("failed to find path to game.json");
	}
	fullscreenCheckbox->callback( (Fl_Callback*)widgetChanged, (void*)(this) );
	playSoundsCheckbox->callback( (Fl_Callback*)widgetChanged, (void*)(this) );
	RustPointer<char> ja2_json_path(findPathFromStracciatellaHome(this->engineOptions.get(), "ja2.json", false, true));
	if (ja2_json_path) {
		ja2JsonPathOutput->value(ja2_json_path.get());
	} else {
		ja2JsonPathOutput->value("failed to find path to ja2.json");
	}
	ja2JsonReloadBtn->callback( (Fl_Callback*)reloadJa2Json, (void*)(this) );
	ja2JsonSaveBtn->callback( (Fl_Callback*)saveJa2Json, (void*)(this) );

	auto nmods = ModManager_getAvailableModsLength(this->modManager.get());
	for (size_t i = 0; i < nmods; ++i) {
		RustPointer<Mod> mod(ModManager_getAvailableModByIndex(this->modManager.get(), i));
		RustPointer<char> modId(Mod_getId(mod.get()));
	}
	availableModsBrowser->callback( (Fl_Callback*)selectAvailableMods, (void*)(this) );
	enabledModsBrowser->callback( (Fl_Callback*)selectEnabledMods, (void*)(this) );
	enableModsButton->callback( (Fl_Callback*)enableMods, (void*)(this) );
	disableModsButton->callback( (Fl_Callback*)disableMods, (void*)(this) );
	moveDownModsButton->callback( (Fl_Callback*)moveDownMods, (void*)(this) );
	moveUpModsButton->callback( (Fl_Callback*)moveUpMods, (void*)(this) );

	populateChoices();
	initializeInputsFromDefaults();

	/* OGVM-CONTROLLER: register callbacks + load art + config */
	controllerEnableToggle->callback((Fl_Callback*)controllerToggleCb, (void*)(this));
	if (controllerLayoutChoice)
		controllerLayoutChoice->callback((Fl_Callback*)controllerLayoutCb, (void*)(this));
	if (leftStickModeChoice)
		leftStickModeChoice->callback((Fl_Callback*)stickModeCb, (void*)(this));
	if (rightStickModeChoice)
		rightStickModeChoice->callback((Fl_Callback*)stickModeCb, (void*)(this));
	if (touchpadSensSlider)
		touchpadSensSlider->callback((Fl_Callback*)touchpadSensCb, (void*)(this));
	if (touchpadModeChoice)
		touchpadModeChoice->callback((Fl_Callback*)touchpadModeCb, (void*)(this));
	if (touchpadKindChoice)
		touchpadKindChoice->callback((Fl_Callback*)touchpadKindCb, (void*)(this));
	if (touchpadValueChoice)
		touchpadValueChoice->callback((Fl_Callback*)touchpadValueCb, (void*)(this));
	for (int i = 0; i < StracciatellaLauncher::kPadBindCount; ++i) {
		if (padKindChoice[i])
			padKindChoice[i]->callback((Fl_Callback*)padKindCb, (void*)(this));
		if (padValueChoice[i])
			padValueChoice[i]->callback((Fl_Callback*)padValueCb, (void*)(this));
	}
	if (controllerView) {
		RustPointer<char> xboxPng(findPathFromAssetsDir("externalized/controller_xbox.png", true, true));
		RustPointer<char> ps5Png(findPathFromAssetsDir("externalized/controller_ps5.png", true, true));
		if (xboxPng) controllerView->setPadImage(0, xboxPng.get());
		if (ps5Png)  controllerView->setPadImage(1, ps5Png.get());
	}
	loadControllerConfig();
	openControllerPad();
	refreshControllerStatus();

	if (logsDisplay && logsDisplay->buffer() != &logsBuffer) {
		logsDisplay->buffer(&logsBuffer);
	}

	playButton->take_focus();

	const Fl_PNG_Image icon("logo32.png", logo32_png, 1374);
	stracciatellaLauncher->icon(&icon);

    fl_message_font(FL_HELVETICA, 13); /* OGVM-FLUENT2-DARK popup */
    gOgvmDarkMode = OgvmSystemIsDark();
    OgvmApplyFluent2Theme(this, gOgvmDarkMode);
    {
        Fl_Button* togBtn = new Fl_Button(542, 526, 34, 34, OgvmThemeIcon(gOgvmDarkMode));
        togBtn->box(FL_ROUNDED_BOX);
        togBtn->tooltip("Toggle light / dark mode");
        togBtn->labelsize(16);
        togBtn->callback(OgvmThemeToggleCb, (void*)(this));
        stracciatellaLauncher->add(togBtn);
    }

	stracciatellaLauncher->show();
}

/* OGVM-CONTROLLER: two-column map (kind | value), AntiMicroX-style.
 * controller.ini: a=mouse:left, b=key:escape, leftshoulder=wheel:up, ...
 */
enum PadKind {
	KIND_EMPTY = 0,
	KIND_MOUSE_BTN,
	KIND_MOUSE_WHEEL,
	KIND_MOUSE_MOTION,
	KIND_KEYBOARD,
	KIND_COUNT
};

static const char* kKindLabels[KIND_COUNT] = {
	"<empty>",
	"Mouse Button",
	"Mouse Wheel",
	"Mouse Motion",
	"Keyboard key"
};

/* Value menus per kind (labels + matching out specs). */
static const char* kMouseBtnLabels[] = { "0 (main click)", "1 (secondary click)", "2 (middle click)", nullptr };
static const char* kMouseBtnSpecs[]  = { "mouse:left", "mouse:right", "mouse:middle" };

static const char* kWheelLabels[] = { "Up", "Down", nullptr };
static const char* kWheelSpecs[]  = { "wheel:up", "wheel:down" };

static const char* kMotionLabels[] = { "Up", "Down", "Left", "Right", nullptr };
static const char* kMotionSpecs[]  = { "nudge:up", "nudge:down", "nudge:left", "nudge:right" };

/* Keyboard: A-Z, 0-9, arrows, common, F1-F12 */
static const char* kKeyLabels[] = {
	"A","B","C","D","E","F","G","H","I","J","K","L","M",
	"N","O","P","Q","R","S","T","U","V","W","X","Y","Z",
	"0","1","2","3","4","5","6","7","8","9",
	"Up","Down","Left","Right",
	"Space","Return","Escape","Tab","Backspace","Delete",
	"Insert","Home","End","PageUp","PageDown",
	"Shift","Ctrl","Alt",
	"F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12",
	nullptr
};
static const char* kKeySpecs[] = {
	"key:a","key:b","key:c","key:d","key:e","key:f","key:g","key:h","key:i","key:j","key:k","key:l","key:m",
	"key:n","key:o","key:p","key:q","key:r","key:s","key:t","key:u","key:v","key:w","key:x","key:y","key:z",
	"key:0","key:1","key:2","key:3","key:4","key:5","key:6","key:7","key:8","key:9",
	"key:up","key:down","key:left","key:right",
	"key:space","key:return","key:escape","key:tab","key:backspace","key:delete",
	"key:insert","key:home","key:end","key:pageup","key:pagedown",
	"key:lshift","key:lctrl","key:lalt",
	"key:f1","key:f2","key:f3","key:f4","key:f5","key:f6","key:f7","key:f8","key:f9","key:f10","key:f11","key:f12"
};

const char* Launcher::padTokens[14] = {
	"a", "b", "x", "y",
	"leftshoulder", "rightshoulder", "lefttrigger", "righttrigger",
	"dpup", "dpdown", "dpleft", "dpright",
	"start", "back"
};

static const char* kPadDefaultOut[14] = {
	"mouse:left", "mouse:right", "none", "none",
	"wheel:up", "wheel:down", "none", "none",
	"key:up", "key:down", "key:left", "key:right",
	"key:return", "key:escape"
};

static std::string ogvmControllerConfigPath(EngineOptions* eo) {
	RustPointer<char> home(findPathFromStracciatellaHome(eo, nullptr, false, false));
	if (!home) return std::string();
	std::filesystem::path p(home.get());
	p /= "controller.ini";
	return p.string();
}

static const char* padLabelFor(const char* token, int layout) {
	const bool ps = (layout == 1);
	if (std::strcmp(token, "a") == 0) return ps ? "Cross" : "A";
	if (std::strcmp(token, "b") == 0) return ps ? "Circle" : "B";
	if (std::strcmp(token, "x") == 0) return ps ? "Square" : "X";
	if (std::strcmp(token, "y") == 0) return ps ? "Triangle" : "Y";
	if (std::strcmp(token, "leftshoulder") == 0) return ps ? "L1" : "LB";
	if (std::strcmp(token, "rightshoulder") == 0) return ps ? "R1" : "RB";
	if (std::strcmp(token, "lefttrigger") == 0) return ps ? "L2" : "LT";
	if (std::strcmp(token, "righttrigger") == 0) return ps ? "R2" : "RT";
	if (std::strcmp(token, "back") == 0) return ps ? "Create" : "View";
	if (std::strcmp(token, "start") == 0) return ps ? "Options" : "Menu";
	if (std::strcmp(token, "dpup") == 0) return "D-Up";
	if (std::strcmp(token, "dpdown") == 0) return "D-Down";
	if (std::strcmp(token, "dpleft") == 0) return "D-Left";
	if (std::strcmp(token, "dpright") == 0) return "D-Right";
	return token;
}

static int kindFromSpec(const char* spec) {
	if (!spec || !*spec || std::strcmp(spec, "none") == 0) return KIND_EMPTY;
	if (std::strncmp(spec, "mouse:", 6) == 0) return KIND_MOUSE_BTN;
	if (std::strncmp(spec, "wheel:", 6) == 0) return KIND_MOUSE_WHEEL;
	if (std::strncmp(spec, "nudge:", 6) == 0 || std::strncmp(spec, "motion:", 7) == 0)
		return KIND_MOUSE_MOTION;
	if (std::strncmp(spec, "key:", 4) == 0) return KIND_KEYBOARD;
	return KIND_EMPTY;
}

static int valueIndexFromSpec(int kind, const char* spec) {
	if (!spec) return 0;
	const char** specs = nullptr;
	int n = 0;
	switch (kind) {
		case KIND_MOUSE_BTN:   specs = kMouseBtnSpecs; n = 3; break;
		case KIND_MOUSE_WHEEL: specs = kWheelSpecs;    n = 2; break;
		case KIND_MOUSE_MOTION:specs = kMotionSpecs;   n = 4; break;
		case KIND_KEYBOARD:    specs = kKeySpecs;      n = 66; break;
		default: return 0;
	}
	for (int i = 0; i < n; ++i)
		if (std::strcmp(specs[i], spec) == 0) return i;
	return 0;
}

static const char* specFromKindValue(int kind, int valueIdx) {
	switch (kind) {
		case KIND_MOUSE_BTN:
			if (valueIdx >= 0 && valueIdx < 3) return kMouseBtnSpecs[valueIdx];
			return kMouseBtnSpecs[0];
		case KIND_MOUSE_WHEEL:
			if (valueIdx >= 0 && valueIdx < 2) return kWheelSpecs[valueIdx];
			return kWheelSpecs[0];
		case KIND_MOUSE_MOTION:
			if (valueIdx >= 0 && valueIdx < 4) return kMotionSpecs[valueIdx];
			return kMotionSpecs[0];
		case KIND_KEYBOARD:
			if (valueIdx >= 0 && valueIdx < 66) return kKeySpecs[valueIdx];
			return kKeySpecs[0];
		default:
			return "none";
	}
}

static bool knownSpec(const char* spec) {
	if (!spec || !*spec) return false;
	if (std::strcmp(spec, "none") == 0) return true;
	int k = kindFromSpec(spec);
	if (k == KIND_EMPTY) return false;
	return true;
}

void Launcher::openControllerPad() {
	if (padHandle) return;
	if (!SDL_WasInit(SDL_INIT_GAMECONTROLLER)) {
		if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0) {
			SLOGW("OGVM-CONTROLLER: SDL_INIT_GAMECONTROLLER failed: {}", SDL_GetError());
			return;
		}
	}
	for (int i = 0; i < SDL_NumJoysticks(); ++i) {
		if (SDL_IsGameController(i)) {
			SDL_GameController* pad = SDL_GameControllerOpen(i);
			if (pad) {
				padHandle = pad;
				SLOGI("OGVM-CONTROLLER: launcher opened '{}'", SDL_GameControllerName(pad));
				return;
			}
		}
	}
}

void Launcher::closeControllerPad() {
	if (padHandle) {
		SDL_GameControllerClose((SDL_GameController*)padHandle);
		padHandle = nullptr;
	}
}

void Launcher::fillPadValueMenu(int padIndex, int kind) {
	if (padIndex < 0 || padIndex >= StracciatellaLauncher::kPadBindCount) return;
	Fl_Choice* v = padValueChoice[padIndex];
	if (!v) return;
	v->clear();
	const char** labels = nullptr;
	switch (kind) {
		case KIND_MOUSE_BTN:    labels = kMouseBtnLabels; break;
		case KIND_MOUSE_WHEEL:  labels = kWheelLabels; break;
		case KIND_MOUSE_MOTION: labels = kMotionLabels; break;
		case KIND_KEYBOARD:     labels = kKeyLabels; break;
		default:
			v->add("-");
			v->value(0);
			v->deactivate();
			return;
	}
	for (int i = 0; labels[i]; ++i) v->add(labels[i]);
	v->activate();
	v->value(0);
}

void Launcher::ensurePadMenusFilled() {
	if (padMenusFilled) return;
	for (int p = 0; p < StracciatellaLauncher::kPadBindCount; ++p) {
		if (!padKindChoice[p]) continue;
		padKindChoice[p]->clear();
		for (int k = 0; k < KIND_COUNT; ++k)
			padKindChoice[p]->add(kKindLabels[k]);
		padKindChoice[p]->value(0);
		fillPadValueMenu(p, KIND_EMPTY);
	}
	padMenusFilled = true;
}

void Launcher::refreshBindLabels() {
	ensurePadMenusFilled();
	for (int p = 0; p < StracciatellaLauncher::kPadBindCount; ++p) {
		if (!padLabel[p] || !padKindChoice[p] || !padValueChoice[p]) continue;
		padLabel[p]->copy_label(padLabelFor(padTokens[p], layoutIndex));
		padLabel[p]->redraw();

		const char* spec = padOutSpec[p].empty() ? "none" : padOutSpec[p].c_str();
		int kind = kindFromSpec(spec);
		padKindChoice[p]->value(kind);
		fillPadValueMenu(p, kind);
		if (kind != KIND_EMPTY)
			padValueChoice[p]->value(valueIndexFromSpec(kind, spec));
		padKindChoice[p]->redraw();
		padValueChoice[p]->redraw();
	}
	if (controllerView) {
		controllerView->layout = layoutIndex;
		controllerView->redraw();
	}
}

void Launcher::applyLayoutChoice() {
	layoutIndex = controllerLayoutChoice ? controllerLayoutChoice->value() : 0;
	if (controllerView) {
		controllerView->layout = layoutIndex;
		controllerView->redraw();
	}
	// PS5 touchpad row: mode + sens/button widgets.
	refreshTouchpadRow();
	refreshBindLabels();
	refreshControllerStatus();
}

void Launcher::fillTouchpadValueMenu(int kind) {
	if (!touchpadValueChoice) return;
	touchpadValueChoice->clear();
	const char** labels = nullptr;
	switch (kind) {
		case KIND_MOUSE_BTN:    labels = kMouseBtnLabels; break;
		case KIND_MOUSE_WHEEL:  labels = kWheelLabels; break;
		case KIND_MOUSE_MOTION: labels = kMotionLabels; break;
		case KIND_KEYBOARD:     labels = kKeyLabels; break;
		default:
			touchpadValueChoice->add("-");
			touchpadValueChoice->value(0);
			touchpadValueChoice->deactivate();
			return;
	}
	for (int i = 0; labels[i]; ++i) touchpadValueChoice->add(labels[i]);
	touchpadValueChoice->activate();
	touchpadValueChoice->value(0);
}

void Launcher::refreshTouchpadRow() {
	const bool ps = (layoutIndex == 1);
	if (touchpadSensLabel) {
		if (ps) touchpadSensLabel->show(); else touchpadSensLabel->hide();
	}
	if (touchpadModeChoice) {
		if (ps) {
			touchpadModeChoice->show();
			touchpadModeChoice->value(touchpadMode == 1 ? 1 : 0);
		} else {
			touchpadModeChoice->hide();
		}
	}
	const bool cursorMode = ps && touchpadMode != 1;
	const bool buttonMode = ps && touchpadMode == 1;
	if (touchpadSensSlider) {
		if (cursorMode) {
			touchpadSensSlider->show();
			touchpadSensSlider->value(touchpadSens);
		} else {
			touchpadSensSlider->hide();
		}
	}
	if (touchpadKindChoice) {
		if (buttonMode) {
			if (touchpadKindChoice->size() <= 1) {
				touchpadKindChoice->clear();
				for (int k = 0; k < KIND_COUNT; ++k)
					touchpadKindChoice->add(kKindLabels[k]);
			}
			const char* spec = touchpadOutSpec.empty() ? "none" : touchpadOutSpec.c_str();
			int kind = kindFromSpec(spec);
			touchpadKindChoice->value(kind);
			fillTouchpadValueMenu(kind);
			if (kind != KIND_EMPTY && touchpadValueChoice)
				touchpadValueChoice->value(valueIndexFromSpec(kind, spec));
			touchpadKindChoice->show();
			if (touchpadValueChoice) touchpadValueChoice->show();
		} else {
			touchpadKindChoice->hide();
			if (touchpadValueChoice) touchpadValueChoice->hide();
		}
	} else if (touchpadValueChoice) {
		touchpadValueChoice->hide();
	}
}

void Launcher::applyTouchpadKind() {
	if (!touchpadKindChoice) return;
	int kind = touchpadKindChoice->value();
	fillTouchpadValueMenu(kind);
	if (kind == KIND_EMPTY)
		touchpadOutSpec = "none";
	else
		touchpadOutSpec = specFromKindValue(kind, 0);
	saveControllerConfig();
}

void Launcher::applyTouchpadValue() {
	if (!touchpadKindChoice || !touchpadValueChoice) return;
	int kind = touchpadKindChoice->value();
	int val = touchpadValueChoice->value();
	if (kind == KIND_EMPTY)
		touchpadOutSpec = "none";
	else
		touchpadOutSpec = specFromKindValue(kind, val);
	saveControllerConfig();
}

void Launcher::applyPadKind(int padIndex) {
	if (padIndex < 0 || padIndex >= StracciatellaLauncher::kPadBindCount) return;
	int kind = padKindChoice[padIndex] ? padKindChoice[padIndex]->value() : KIND_EMPTY;
	fillPadValueMenu(padIndex, kind);
	if (kind == KIND_EMPTY)
		padOutSpec[padIndex] = "none";
	else
		padOutSpec[padIndex] = specFromKindValue(kind, 0);
	if (controllerView) {
		controllerView->setHighlight(padTokens[padIndex]);
		controllerView->redraw();
	}
	saveControllerConfig();
}

void Launcher::applyPadValue(int padIndex) {
	if (padIndex < 0 || padIndex >= StracciatellaLauncher::kPadBindCount) return;
	int kind = padKindChoice[padIndex] ? padKindChoice[padIndex]->value() : KIND_EMPTY;
	int val  = padValueChoice[padIndex] ? padValueChoice[padIndex]->value() : 0;
	if (kind == KIND_EMPTY)
		padOutSpec[padIndex] = "none";
	else
		padOutSpec[padIndex] = specFromKindValue(kind, val);
	if (controllerView) {
		controllerView->setHighlight(padTokens[padIndex]);
		controllerView->redraw();
	}
	saveControllerConfig();
}

static int stickModeFromString(const std::string& val) {
	if (val == "cursor" || val == "1") return 1;
	if (val == "wasd" || val == "2") return 2;
	if (val == "arrow" || val == "arrows" || val == "3") return 3;
	return 0; // empty / none / off
}

static const char* stickModeToString(int mode) {
	switch (mode) {
		case 1: return "cursor";
		case 2: return "wasd";
		case 3: return "arrow";
		default: return "none";
	}
}

void Launcher::loadControllerConfig() {
	for (int i = 0; i < 14; ++i) padOutSpec[i] = kPadDefaultOut[i];
	layoutIndex = 0;
	leftStickMode = 1;  // Cursor default
	rightStickMode = 0;
	touchpadSens = 1100.f;
	touchpadMode = 0;
	touchpadOutSpec = "none";
	bool enabled = false;
	bool anyNew = false;
	bool sawStickMode = false;

	// legacy action=pad maps
	static const char* legacyAction[10] = {
		"mouse_left", "mouse_right", "scroll_up", "scroll_down",
		"arrow_up", "arrow_down", "arrow_left", "arrow_right",
		"confirm", "cancel"
	};
	static const char* legacyOut[10] = {
		"mouse:left", "mouse:right", "wheel:up", "wheel:down",
		"key:up", "key:down", "key:left", "key:right",
		"key:return", "key:escape"
	};
	std::string legacyPad[10];
	bool legacySeen = false;
	int legacyCursorStick = -1;

	std::string cfg = ogvmControllerConfigPath(this->engineOptions.get());
	if (!cfg.empty()) {
		std::ifstream in(cfg);
		std::string line;
		while (std::getline(in, line)) {
			auto eq = line.find('=');
			if (eq == std::string::npos) continue;
			std::string key = line.substr(0, eq);
			std::string val = line.substr(eq + 1);
			if (!val.empty() && val.back() == '\r') val.pop_back();
			if (key == "enabled") { enabled = (val == "1"); continue; }
			if (key == "layout") {
				layoutIndex = (val == "ps5" || val == "ps" || val == "1") ? 1 : 0;
				continue;
			}
			if (key == "touchpad") {
				if (val == "button" || val == "btn" || val == "2") touchpadMode = 1;
				else touchpadMode = 0;
				continue;
			}
			if (key == "touchpad_out") {
				touchpadOutSpec = val.empty() ? "none" : val;
				continue;
			}
			if (key == "touchpad_sens") {
				float s = 1100.f;
				try { s = std::stof(val); } catch (...) { s = 1100.f; }
				if (s < 200.f) s = 200.f;
				if (s > 4000.f) s = 4000.f;
				touchpadSens = s;
				continue;
			}
			if (key == "left_stick") {
				leftStickMode = stickModeFromString(val);
				sawStickMode = true;
				continue;
			}
			if (key == "right_stick") {
				rightStickMode = stickModeFromString(val);
				sawStickMode = true;
				continue;
			}
			// legacy single cursor_stick=left|right
			if (key == "cursor_stick") {
				legacyCursorStick = (val == "right" || val == "1") ? 1 : 0;
				continue;
			}
			// new format: pad_token=out_spec
			for (int p = 0; p < 14; ++p) {
				if (key == padTokens[p]) {
					if (val.empty() || val == "none" || knownSpec(val.c_str()) || val.find(':') != std::string::npos) {
						padOutSpec[p] = val.empty() ? "none" : val;
						anyNew = true;
					}
					break;
				}
			}
			// legacy format: action=pad_token
			for (int a = 0; a < 10; ++a) {
				if (key == legacyAction[a]) {
					legacyPad[a] = val;
					legacySeen = true;
					break;
				}
			}
		}
	}

	if (!anyNew && legacySeen) {
		for (int i = 0; i < 14; ++i) padOutSpec[i] = "none";
		for (int a = 0; a < 10; ++a) {
			if (legacyPad[a].empty() || legacyPad[a] == "none") continue;
			for (int p = 0; p < 14; ++p) {
				if (legacyPad[a] == padTokens[p]) {
					padOutSpec[p] = legacyOut[a];
					break;
				}
			}
		}
	}
	// migrate old cursor_stick if new keys absent in file
	if (!sawStickMode && legacyCursorStick >= 0) {
		// only apply when left/right stick keys were not written (still defaults
		// unless user saved new format — detect via absence of left_stick in file
		// by checking: if both still at initial after parse of left/right keys
		// we already set from left_stick/right_stick if present; if only legacy:
		if (legacyCursorStick == 0) { leftStickMode = 1; rightStickMode = 0; }
		else { leftStickMode = 0; rightStickMode = 1; }
	}

	controllerEnableToggle->value(enabled ? 1 : 0);
	if (controllerLayoutChoice) controllerLayoutChoice->value(layoutIndex);
	if (leftStickModeChoice) leftStickModeChoice->value(leftStickMode);
	if (rightStickModeChoice) rightStickModeChoice->value(rightStickMode);
	if (touchpadSensSlider) touchpadSensSlider->value(touchpadSens);
	if (controllerView) {
		controllerView->layout = layoutIndex;
		controllerView->setHighlight(nullptr);
	}
	applyLayoutChoice(); // show/hide PS5 touchpad sens + labels + status
}

void Launcher::saveControllerConfig() {
	std::string cfg = ogvmControllerConfigPath(this->engineOptions.get());
	if (cfg.empty()) return;
	std::ofstream out(cfg, std::ios::trunc);
	if (!out) {
		SLOGW("OGVM-CONTROLLER: cannot write {}", cfg.c_str());
		return;
	}
	out << "enabled=" << (controllerEnableToggle->value() ? "1" : "0") << "\n";
	out << "layout=" << (layoutIndex == 1 ? "ps5" : "xbox") << "\n";
	out << "left_stick=" << stickModeToString(leftStickMode) << "\n";
	out << "right_stick=" << stickModeToString(rightStickMode) << "\n";
	if (touchpadMode == 1)
		out << "touchpad=button\n";
	else
		out << "touchpad=cursor\n";
	out << "touchpad_sens=" << (int)touchpadSens << "\n";
	out << "touchpad_out=" << (touchpadOutSpec.empty() ? "none" : touchpadOutSpec) << "\n";
	for (int p = 0; p < 14; ++p) {
		const char* spec = padOutSpec[p].empty() ? "none" : padOutSpec[p].c_str();
		out << padTokens[p] << "=" << spec << "\n";
	}
}

void Launcher::setControllerFieldsEnabled(bool on) {
	if (controllerLayoutChoice) {
		if (on) controllerLayoutChoice->activate(); else controllerLayoutChoice->deactivate();
	}
	if (leftStickModeChoice) {
		if (on) leftStickModeChoice->activate(); else leftStickModeChoice->deactivate();
	}
	if (rightStickModeChoice) {
		if (on) rightStickModeChoice->activate(); else rightStickModeChoice->deactivate();
	}
	if (controllerBindScroll) {
		if (on) controllerBindScroll->activate(); else controllerBindScroll->deactivate();
	}
	for (int i = 0; i < StracciatellaLauncher::kPadBindCount; ++i) {
		if (padKindChoice[i]) {
			if (on) padKindChoice[i]->activate(); else padKindChoice[i]->deactivate();
		}
		if (padValueChoice[i]) {
			// keep value greyed when kind empty even if enabled
			if (!on) {
				padValueChoice[i]->deactivate();
			} else {
				const int kind = padKindChoice[i] ? padKindChoice[i]->value() : 0;
				if (kind == 0) padValueChoice[i]->deactivate();
				else padValueChoice[i]->activate();
			}
		}
	}
	if (controllerView) controllerView->redraw();
	if (leftStickLabel) leftStickLabel->redraw();
	if (rightStickLabel) rightStickLabel->redraw();
	if (touchpadModeChoice) {
		if (on && layoutIndex == 1) touchpadModeChoice->activate();
		else touchpadModeChoice->deactivate();
	}
	if (touchpadSensSlider) {
		if (on && layoutIndex == 1 && touchpadMode != 1) touchpadSensSlider->activate();
		else touchpadSensSlider->deactivate();
	}
	if (touchpadKindChoice) {
		if (on && layoutIndex == 1 && touchpadMode == 1) touchpadKindChoice->activate();
		else touchpadKindChoice->deactivate();
	}
	if (touchpadValueChoice) {
		if (on && layoutIndex == 1 && touchpadMode == 1) {
			const int kind = touchpadKindChoice ? touchpadKindChoice->value() : 0;
			if (kind == 0) touchpadValueChoice->deactivate();
			else touchpadValueChoice->activate();
		} else {
			touchpadValueChoice->deactivate();
		}
	}
}

void Launcher::refreshControllerStatus() {
	static std::string helpText;
	static const char* modeName[] = { "off", "cursor", "WASD", "arrow" };
	const char* lm = modeName[leftStickMode >= 0 && leftStickMode <= 3 ? leftStickMode : 0];
	const char* rm = modeName[rightStickMode >= 0 && rightStickMode <= 3 ? rightStickMode : 0];
	helpText = std::string("L stick: ") + lm + "  R stick: " + rm + "\n"
		"Pad sides: kind + value.  "
		"Mouse/Keyboard/Wheel/Motion. "
		"<empty> = unbound.";
	if (controllerHelp->buffer() == nullptr) {
		controllerHelp->buffer(new Fl_Text_Buffer());
	}
	controllerHelp->buffer()->text(helpText.c_str());

	const bool on = controllerEnableToggle && controllerEnableToggle->value();
	setControllerFieldsEnabled(on);
	if (on) {
		controllerStatusLabel->copy_label("On. Map pad button to key/mouse.");
	} else {
		controllerStatusLabel->copy_label("Off. Tick box above to enable.");
	}
	controllerStatusLabel->redraw();
}

void Launcher::controllerToggleCb(Fl_Widget* /*w*/, void* userdata) {
	Launcher* window = static_cast<Launcher*>(userdata);
	window->saveControllerConfig();
	window->refreshControllerStatus();
}

void Launcher::controllerLayoutCb(Fl_Widget* /*w*/, void* userdata) {
	Launcher* window = static_cast<Launcher*>(userdata);
	window->applyLayoutChoice();
	window->saveControllerConfig();
}

void Launcher::stickModeCb(Fl_Widget* /*w*/, void* userdata) {
	Launcher* window = static_cast<Launcher*>(userdata);
	window->leftStickMode = window->leftStickModeChoice
		? window->leftStickModeChoice->value() : 0;
	window->rightStickMode = window->rightStickModeChoice
		? window->rightStickModeChoice->value() : 0;
	// only one stick may own Cursor
	if (window->leftStickMode == 1 && window->rightStickMode == 1) {
		if (window->leftStickModeChoice && window->leftStickModeChoice->changed()) {
			window->rightStickMode = 0;
			if (window->rightStickModeChoice) window->rightStickModeChoice->value(0);
		} else {
			window->leftStickMode = 0;
			if (window->leftStickModeChoice) window->leftStickModeChoice->value(0);
		}
	}
	window->saveControllerConfig();
	window->refreshControllerStatus();
}

void Launcher::padKindCb(Fl_Widget* w, void* userdata) {
	Launcher* window = static_cast<Launcher*>(userdata);
	for (int i = 0; i < StracciatellaLauncher::kPadBindCount; ++i) {
		if (window->padKindChoice[i] == w) {
			window->applyPadKind(i);
			return;
		}
	}
}

void Launcher::padValueCb(Fl_Widget* w, void* userdata) {
	Launcher* window = static_cast<Launcher*>(userdata);
	for (int i = 0; i < StracciatellaLauncher::kPadBindCount; ++i) {
		if (window->padValueChoice[i] == w) {
			window->applyPadValue(i);
			return;
		}
	}
}

void Launcher::touchpadSensCb(Fl_Widget* /*w*/, void* userdata) {
	Launcher* window = static_cast<Launcher*>(userdata);
	if (window->touchpadSensSlider) {
		window->touchpadSens = (float)window->touchpadSensSlider->value();
		if (window->touchpadSens < 200.f) window->touchpadSens = 200.f;
		if (window->touchpadSens > 4000.f) window->touchpadSens = 4000.f;
	}
	window->saveControllerConfig();
}

void Launcher::touchpadModeCb(Fl_Widget* /*w*/, void* userdata) {
	Launcher* window = static_cast<Launcher*>(userdata);
	window->touchpadMode = window->touchpadModeChoice
		? window->touchpadModeChoice->value() : 0;
	window->refreshTouchpadRow();
	window->saveControllerConfig();
	window->setControllerFieldsEnabled(
		window->controllerEnableToggle && window->controllerEnableToggle->value());
}

void Launcher::touchpadKindCb(Fl_Widget* /*w*/, void* userdata) {
	Launcher* window = static_cast<Launcher*>(userdata);
	window->applyTouchpadKind();
	window->setControllerFieldsEnabled(
		window->controllerEnableToggle && window->controllerEnableToggle->value());
}

void Launcher::touchpadValueCb(Fl_Widget* /*w*/, void* userdata) {
	Launcher* window = static_cast<Launcher*>(userdata);
	window->applyTouchpadValue();
}


void Launcher::initializeInputsFromDefaults() {
	RustPointer<char> rustResRootPath(EngineOptions_getVanillaGameDir(this->engineOptions.get()));
	gameDirectoryInput->value(rustResRootPath.get());
	RustPointer<char> rustSaveGamePath(EngineOptions_getSaveGameDir(this->engineOptions.get()));
	saveGameDirectoryInput->value(rustSaveGamePath.get());

	uint32_t n = EngineOptions_getModsLength(this->engineOptions.get());
	enabledModsBrowser->clear();
	for (uint32_t i = 0; i < n; ++i) {
		RustPointer<char> modId(EngineOptions_getMod(this->engineOptions.get(), i));
		RustPointer<Mod> mod(ModManager_getAvailableModById(this->modManager.get(), modId.get()));
		if (mod.get() != NULL) {
			RustPointer<char> modName(Mod_getName(mod.get()));

			enabledModsBrowser->add(modName.get());
			enabledModsBrowser->data(enabledModsBrowser->size(), modId.release());
		} else {
			// @C72 is dark red, should be highlighted because it is not available
			enabledModsBrowser->add(ST::format("@C72{}", modId.get()).c_str());
			enabledModsBrowser->data(enabledModsBrowser->size(), modId.release());
		}
	}

	availableModsBrowser->clear();
	auto nmods = ModManager_getAvailableModsLength(this->modManager.get());
	for (uintptr_t i = 0; i < nmods; i++) {
		RustPointer<Mod> mod(ModManager_getAvailableModByIndex(this->modManager.get(), i));
		RustPointer<char> modId(Mod_getId(mod.get()));
		RustPointer<char> modName(Mod_getName(mod.get()));
		availableModsBrowser->add(modName.get());
		if (EngineOptions_isModEnabled(this->engineOptions.get(), modId.get())) {
			availableModsBrowser->hide(availableModsBrowser->size());
		}
		availableModsBrowser->data(availableModsBrowser->size(), modId.release());
	}

	GameVersion rustResVersion = EngineOptions_getResourceVersion(this->engineOptions.get());
	int resourceVersionIndex = 0;
	for (GameVersion version : predefinedVersions) {
		if (version == rustResVersion) {
			break;
		}
		resourceVersionIndex += 1;
	}
	gameVersionInput->value(resourceVersionIndex);

	int x = EngineOptions_getResolutionX(this->engineOptions.get());
	int y = EngineOptions_getResolutionY(this->engineOptions.get());

	resolutionXInput->value(x);
	resolutionYInput->value(y);

	VideoScaleQuality quality = EngineOptions_getScalingQuality(this->engineOptions.get());
	int scalingModeIndex = 0;
	for (VideoScaleQuality scalingMode : scalingModes) {
		if (scalingMode == quality) {
			break;
		}
		scalingModeIndex += 1;
	}
	this->scalingModeChoice->value(scalingModeIndex);

	fullscreenCheckbox->value(EngineOptions_shouldStartInFullscreen(this->engineOptions.get()) ? 1 : 0);
	playSoundsCheckbox->value(EngineOptions_shouldStartWithoutSound(this->engineOptions.get()) ? 0 : 1);
	update(false);
}

int Launcher::writeJsonFile() {
	EngineOptions_setStartInFullscreen(this->engineOptions.get(), fullscreenCheckbox->value());
	EngineOptions_setStartWithoutSound(this->engineOptions.get(), !playSoundsCheckbox->value());

	EngineOptions_setVanillaGameDir(this->engineOptions.get(), gameDirectoryInput->value());
	EngineOptions_setSaveGameDir(this->engineOptions.get(), saveGameDirectoryInput->value());

	EngineOptions_clearMods(this->engineOptions.get());
	int nitems = enabledModsBrowser->size();
	for (int item = 1; item <= nitems; ++item) {
		char* modId = static_cast<char*>(enabledModsBrowser->data(item));
		EngineOptions_pushMod(this->engineOptions.get(), modId);
	}

	int x = (int)resolutionXInput->value();
	int y = (int)resolutionYInput->value();
	EngineOptions_setResolution(this->engineOptions.get(), x, y);
	EngineOptions_setBrightness(this->engineOptions.get(), -1.0f);

	int currentResourceVersionIndex = gameVersionInput->value();
	GameVersion currentResourceVersion = predefinedVersions.at(currentResourceVersionIndex);
	EngineOptions_setResourceVersion(this->engineOptions.get(), currentResourceVersion);

	VideoScaleQuality currentScalingMode = scalingModes[this->scalingModeChoice->value()];
	EngineOptions_setScalingQuality(this->engineOptions.get(), currentScalingMode);

	bool success = EngineOptions_write(this->engineOptions.get());

	if (success) {
		update(false);
		SLOGD("Succeeded writing config file");
		return 0;
	}
	SLOGD("Failed writing config file");
	return 1;
}

void Launcher::populateChoices() {
	for(GameVersion version : predefinedVersions) {
		RustPointer<char> resourceVersionString(VanillaVersion_toString(version));
		gameVersionInput->add(resourceVersionString.get());
	}
	for (std::pair<int,int> res : predefinedResolutions) {
		ST::string resolutionString = ST::format("{d}x{d}", res.first, res.second);
		if (res.first == 1366 && res.second == 768) {
		    /* RECRES: sscanf below reads only the leading numbers,
		     * so trailing text in the label is harmless. */
		    resolutionString = ST::format("{} (recommended)", resolutionString);
		}
		predefinedResolutionMenuButton->insert(-1, resolutionString.c_str(), 0, setPredefinedResolution, this, 0);
	}

	for (VideoScaleQuality scalingMode : scalingModes) {
		RustPointer<char> scalingModeString(ScalingQuality_toString(scalingMode));
		this->scalingModeChoice->add(scalingModeString.get());
	}
}

void Launcher::openGameDirectorySelector(Fl_Widget *btn, void *userdata) {
	Launcher* window = static_cast< Launcher* >( userdata );
	Fl_Native_File_Chooser fnfc;
	fnfc.title("Select the original Jagged Alliance 2 installation directory");
	fnfc.type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
	ST::char_buffer decoded = decodePath(window->gameDirectoryInput->value());
	fnfc.directory(decoded.empty() ? nullptr : decoded.c_str());

	switch ( fnfc.show() ) {
		case -1:
			break; // ERROR
		case  1:
			break; // CANCEL
		default:
		{
			const auto dir = encodePath(fnfc.filename());
			if (!checkGameDirectoryForCommonMistakes(dir)) return;
			window->gameDirectoryInput->value(dir.c_str());
			window->update(true);
			OgvmRefreshEditionLabel(window);

			break; // FILE CHOSEN
		}
	}
}

bool Launcher::checkGameDirectoryForCommonMistakes(const ST::string& dir) {
	auto fileToCheck = FileMan::resolveExistingComponents(FileMan::joinPaths(dir, "data/Ja2Set.dat.xml"));
	try {
		if (!checkIfRelativePathExists(dir.c_str(), "Data", true)) {
			fl_message_title("Incorrect game directory detected");
			int choice = fl_choice(
				"The Data directory was not found within game directory.\nThis means the chosen directory does not contain a Jagged Alliance 2 installation.",
				"Continue",
				"Cancel",
				0
			);
			if (choice == 1) {
				return false;
			}
		}
		if (checkIfRelativePathExists(dir.c_str(), "Data/Ja2Set.dat.xml", true)) {
			fl_message_title("Modified game directory detected");
			auto choice = fl_choice(
				"We detected that the game directory contains the 1.13 patch.\nJA2 Stracciatella will not work properly with the modified files.\nPlease use a clean installation of Jagged Alliance 2.",
				"Continue",
				"Cancel",
				0
			);

			if (choice == 1) {
				return false;
			}
		}
	} catch (const std::runtime_error &ex) {
		SLOGE("failed to read game dir: {}", ex.what());
	}
	return true;
}

void Launcher::openSaveGameDirectorySelector(Fl_Widget *btn, void *userdata) {
	Launcher* window = static_cast< Launcher* >( userdata );
	Fl_Native_File_Chooser fnfc;
	fnfc.title("Select your save game directory");
	fnfc.type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
	ST::char_buffer decoded = decodePath(window->saveGameDirectoryInput->value());
	fnfc.directory(decoded.empty() ? nullptr : decoded.c_str());

	switch ( fnfc.show() ) {
		case -1:
			break; // ERROR
		case  1:
			break; // CANCEL
		default:
		{
			ST::string encoded = encodePath(fnfc.filename());
			window->saveGameDirectoryInput->value(encoded.c_str());
			window->update(true);
			break; // FILE CHOSEN
		}
	}
}

void Launcher::startExecutable(bool asEditor) {
	if (gameIsRunning()) {
		return;
	}
	// check minimal resolution:
	if (resolutionIsInvalid()) {
		fl_message_title("Invalid resolution");
		fl_alert("Invalid custom resolution %dx%d.\nJA2 Stracciatella needs a resolution of at least 640x480.",
			(int) resolutionXInput->value(),
			(int) resolutionYInput->value());
		return;
	}

	auto nenabled = this->enabledModsBrowser->size();
	std::vector<ST::string> invalidMods;
	for (auto i = 1; i <= nenabled; i++) {
		ST::string modId = static_cast<char*>(this->enabledModsBrowser->data(i));
		if (ModManager_getAvailableModById(this->modManager.get(), modId.c_str()) == NULL) {
			invalidMods.push_back(modId);
		}
	}
	if (invalidMods.size() > 0) {
		ST::string message = "The following mods are enabled, but dont exist on the filesystem: ";
		for (auto i = invalidMods.begin(); i < invalidMods.end(); i++) {
			if (i != invalidMods.begin()) {
				message += ", ";
			}
			message += *i;
		}

		fl_message_title("Invalid mods");
		fl_alert("%s", message.c_str());
		return;
	}

	RustPointer<char> exePath(Env_currentExe());
	if (!exePath) {
		showRustError();
		return;
	}
	ST::string filename = FileMan::getFileName(exePath.get());
	if (filename.size() == 0) {
		fl_message_title("No filename");
		fl_alert("%s", exePath.get());
		return;
	}
	ST::string target("-launcher");
	ST::string newFilename(filename);
	auto pos = newFilename.find_last(target);
	if (pos == -1) {
		fl_message_title("Not launcher");
		fl_alert("%s", exePath.get());
		return;
	}
	newFilename = newFilename.replace(target, "");
	exePath.reset(Path_setFilename(exePath.get(), newFilename.c_str()));
	if (!FileMan::exists(exePath.get())) {
		fl_message_title("Not found");
		fl_alert("%s", exePath.get());
		return;
	}
	RustPointer<VecCString> args(VecCString_create());
	if (asEditor) {
		VecCString_push(args.get(), "-editor");
	}
	subProcess = std::make_optional(RustPointer<SubProcess>(Subprocess_new(exePath.get(), args.get())));
	update(false);
	Launcher::maintainSubProcessState(this);
}

// Writes logs into log tab
void Launcher::updateLogs() {
	if (!logsDisplay) return;
	// Member buffer must be attached once; null buffer → SIGSEGV on quit-to-launcher.
	if (logsDisplay->buffer() != &logsBuffer) {
		logsDisplay->buffer(&logsBuffer);
	}
	RustPointer<char> logPath(Logger_getFilePath("ja2.log"));
	if (!logPath) return;
	try {
		AutoSGPFile logsFd(FileMan::openForReading(logPath.get()));
		auto logs = logsFd->readStringToEnd();
		logsBuffer.text(logs.c_str());
		const int nlines = logsBuffer.count_lines(0, logsBuffer.length());
		if (nlines > 0) logsDisplay->scroll(nlines - 1, 0);
	} catch (const std::runtime_error &ex) {
		SLOGW("Error reading logs: {}", ex.what());
	}
}

void Launcher::maintainSubProcessState(void* userdata) {
	Launcher* window = static_cast< Launcher* >( userdata );
	if (window->subProcess) {
		Subprocess_process(window->subProcess.value().get());
		if (!Subprocess_isDone(window->subProcess.value().get())) {
			Fl::add_timeout(checkGameRunningIntervalSeconds, Launcher::maintainSubProcessState, window);
		} else {
			auto exitCode = Subprocess_getExitCode(window->subProcess.value().get());
			if (exitCode != 0) {
				ST::string error = "JA2 Stracciatella crashed with an unknown error.";
				if (exitCode == std::numeric_limits<std::int32_t>::min()) {
					RustPointer<char> err(getRustError());
					if (err.get() != NULL) {
						error = ST::format("JA2 Stracciatella crashed with error: {}", err.get());
					}
				} else {
					error = ST::format("JA2 Stracciatella crashed with exit code: {}", exitCode);
				}

				SLOGE("{}", error);
				error = ST::format("{}\n\nYou will be taken to the logs tab, where you can investigate the error.", error);

				showError(error);

				window->tabs->value(window->logsTab);
			}

			window->subProcess = std::nullopt;
			window->updateLogs();
			window->update(false);
		}
	}
}

bool Launcher::resolutionIsInvalid() {
	return resolutionXInput->value() < 640 || resolutionYInput->value() < 480;
}

bool Launcher::gameIsRunning() {
	return subProcess ? !Subprocess_isDone(subProcess.value().get()) : false;
}

void Launcher::update(bool changed) {
	// invalid resolution warning
	if (resolutionIsInvalid()) {
		invalidResolutionLabel->show();
	} else {
		invalidResolutionLabel->hide();
	}

	// something changed indicator
	if (changed && ja2JsonPathOutput->value()[0] != '*') {
		ST::string tmp("*"); // add '*'
		tmp += ja2JsonPathOutput->value();
		ja2JsonPathOutput->value(tmp.c_str());
	} else if (!changed && ja2JsonPathOutput->value()[0] == '*') {
		ST::string tmp(ja2JsonPathOutput->value() + 1); // remove '*'
		ja2JsonPathOutput->value(tmp.c_str());
	}

	if (gameIsRunning()) {
		tabs->deactivate();
		playButton->deactivate();
		editorButton->deactivate();
		ja2JsonReloadBtn->deactivate();
		ja2JsonSaveBtn->deactivate();
	} else {
		tabs->activate();
		playButton->activate();
		editorButton->activate();
		ja2JsonReloadBtn->activate();
		ja2JsonSaveBtn->activate();
	}
}

void Launcher::startGame(Fl_Widget* btn, void* userdata) {
	Launcher* window = static_cast< Launcher* >( userdata );

	window->writeJsonFile();

	if (!checkGameDirectoryForCommonMistakes(window->gameDirectoryInput->value())) return;

	window->startExecutable(false);
}

void Launcher::startEditor(Fl_Widget* btn, void* userdata) {
	Launcher* window = static_cast< Launcher* >( userdata );

	window->writeJsonFile();
	bool has_editor_slf = checkIfRelativePathExists(window->gameDirectoryInput->value(), "Data/Editor.slf", true);
	if (!has_editor_slf) {
		RustPointer<char> assets_dir(findPathFromAssetsDir(nullptr, false, false));
		if (assets_dir) {
			// free editor.slf
			has_editor_slf = checkIfRelativePathExists(assets_dir.get(), "externalized/editor.slf", true);
		}
	}
	if (!has_editor_slf) {
		fl_message_title(window->editorButton->label());
		int choice = fl_choice("Editor.slf not found.\nAre you sure you want to continue?", "Stop", "Continue", 0);
		if (choice != 1) {
			return;
		}
	}
	window->startExecutable(true);
}

void Launcher::guessVersion(Fl_Widget* btn, void* userdata) {
	Launcher* window = static_cast< Launcher* >( userdata );
	ST::string gamedir = window->gameDirectoryInput->value();

	if (!checkGameDirectoryForCommonMistakes(gamedir)) return;

	fl_message_title("Guess Game Version");
	int choice = fl_choice("Comparing resources packs can take a long time.\nAre you sure you want to continue?", "Stop", "Continue", 0);
	if (choice != 1) {
		return;
	}

	int guessedVersion = guessResourceVersion(gamedir.c_str());
	if (guessedVersion != -1) {
		int resourceVersionIndex = 0;
		for (GameVersion version : predefinedVersions) {
			if (static_cast<int>(version) == guessedVersion) {
				break;
			}
			resourceVersionIndex += 1;
		}
		window->gameVersionInput->value(resourceVersionIndex);
		window->update(true);
		fl_message_title(window->guessVersionButton->label());
		fl_message("Success!");
	} else {
		fl_message_title(window->guessVersionButton->label());
		fl_alert("Failure!");
	}
}

/* OGVM-AUTODETECT ------------------------------------------------------------
 * Hien phien ban game da nhan dien ngay tren nhan nut.
 * LUU Y FLTK: label()/tooltip() chi giu CON TRO — chuoi phai song lau.
 */
#define OGVM_IDLE_LABEL "Detect Edition"

static std::string gOgvmEditionLabel;

/* OGVM-SRC2: unpack dirs vs installed files (khong dung Manual.pdf/mss32.dll). */
enum class OgvmDataSource { Unknown, InstallerUnpack, ExistingFolder };

static OgvmDataSource OgvmDetectSource(const std::string& gamedir)
{
    if (gamedir.empty()) {
        return OgvmDataSource::Unknown;
    }
    try {
        ST::char_buffer decoded = decodePath(gamedir.c_str());
        if (decoded.empty()) {
            return OgvmDataSource::Unknown;
        }
        const std::filesystem::path root(decoded.c_str());
        std::error_code ec;
        if (!std::filesystem::is_directory(root, ec)) {
            return OgvmDataSource::Unknown;
        }

        static const char* const unpackDirs[] = {
            "app", "tmp", "commonappdata", "__redist", "__support",
        };
        static const char* const installedFiles[] = {
            "Uninstall.exe", "Uninstall.log", "WF6.set",
            "unins000.exe", "unins000.dat",
        };

        int unpackHits = 0;
        for (const char* name : unpackDirs) {
            std::error_code e2;
            if (std::filesystem::is_directory(root / name, e2)) {
                unpackHits++;
            }
        }
        int installedHits = 0;
        for (const char* name : installedFiles) {
            std::error_code e2;
            if (std::filesystem::is_regular_file(root / name, e2)) {
                installedHits++;
            }
        }

        if (unpackHits >= 2) {
            return OgvmDataSource::InstallerUnpack;
        }
        return OgvmDataSource::ExistingFolder;
    } catch (...) {
        return OgvmDataSource::Unknown;
    }
}

static std::string OgvmSourceToString(OgvmDataSource src)
{
    switch (src) {
        case OgvmDataSource::InstallerUnpack:
            return "a GOG installer that has been unpacked";
        case OgvmDataSource::ExistingFolder:
            return "an existing game folder (Steam, or an installed copy)";
        default:
            return "not known yet - choose a game directory first";
    }
}

static std::string gOgvmSourceTip;
static std::string gOgvmStatusText;

/* Forwards — phai nam truoc moi ham dung chung */
static void OgvmRefreshEditionLabel(Launcher* window);
static void OgvmApplySourceLayout(Launcher* window);
static bool gOgvmRadioUserClick = true;

static void OgvmApplySourceLayout(Launcher* window)
{
    if (!window) {
        return;
    }
    const bool installer = window->sourceInstallerRadio
        && window->sourceInstallerRadio->value();
    if (window->ogvmInstallerPanel) {
        if (installer) {
            window->ogvmInstallerPanel->show();
        } else {
            window->ogvmInstallerPanel->hide();
        }
        window->ogvmInstallerPanel->redraw();
    }
    if (window->ogvmFolderPanel) {
        if (installer) {
            window->ogvmFolderPanel->hide();
        } else {
            window->ogvmFolderPanel->show();
        }
        window->ogvmFolderPanel->redraw();
    }
    if (window->tabs) {
        window->tabs->redraw();
    }
}

#define OGVM_IMPORT_MB_GUESS 1024.0
#define OGVM_IMPORT_TICK 0.5

static bool gOgvmImportRunning = false;
static std::string gOgvmImportDir;
static std::string gOgvmImportText;
static int gOgvmImportTicks = 0;
static Fl_Progress* gOgvmProgress = nullptr;
static Fl_Box* gOgvmStatusBox = nullptr;
static Fl_Button* gOgvmImportBtn = nullptr;

static double OgvmDirMegabytes(const std::string& dir)
{
    std::error_code ec;
    const std::filesystem::path root(dir);
    if (dir.empty() || !std::filesystem::is_directory(root, ec)) {
        return 0.0;
    }
    uintmax_t total = 0;
    std::error_code walkEc;
    std::filesystem::recursive_directory_iterator it(
        root, std::filesystem::directory_options::skip_permission_denied, walkEc);
    std::filesystem::recursive_directory_iterator end;
    for (; it != end; it.increment(walkEc)) {
        std::error_code fileEc;
        if (it->is_regular_file(fileEc)) {
            std::error_code sizeEc;
            total += it->file_size(sizeEc);
        }
    }
    return (double) total / (1024.0 * 1024.0);
}

static void OgvmImportFinishLook()
{
    if (gOgvmProgress) {
        gOgvmProgress->hide();
    }
    if (gOgvmStatusBox) {
        gOgvmStatusBox->show();
        gOgvmStatusBox->redraw();
    }
    if (gOgvmImportBtn) {
        gOgvmImportBtn->activate();
        gOgvmImportBtn->redraw();
    }
}

void OgvmImportTickCb(void* userdata)
{
    if (!gOgvmImportRunning) {
        OgvmImportFinishLook();
        return;
    }
    gOgvmImportTicks++;
    const double mb = OgvmDirMegabytes(gOgvmImportDir);
    const int secs = (int) (gOgvmImportTicks * OGVM_IMPORT_TICK);
    double pct = mb / OGVM_IMPORT_MB_GUESS * 100.0;
    if (pct > 99.0) pct = 99.0;
    if (pct < 0.0) pct = 0.0;
    gOgvmImportText = "Importing... " + std::to_string((int) (mb + 0.5))
        + " MB unpacked, " + std::to_string(secs) + "s";
    if (gOgvmProgress) {
        gOgvmProgress->value((float) pct);
        gOgvmProgress->label(gOgvmImportText.c_str());
        gOgvmProgress->redraw();
    }
    Fl::add_timeout(OGVM_IMPORT_TICK, OgvmImportTickCb, userdata);
}

void OgvmSourceRadioCb(Fl_Widget* w, void* userdata)
{
    Launcher* window = static_cast< Launcher* >(userdata);
    OgvmApplySourceLayout(window);
    if (!gOgvmRadioUserClick) {
        return;
    }
    if (w == window->sourceFolderRadio) {
        window->browseJa2DirectoryButton->do_callback();
    } else if (w == window->sourceInstallerRadio) {
        window->importGameDataButton->do_callback();
    }
    OgvmRefreshEditionLabel(window);
}

static void OgvmRefreshEditionLabel(Launcher* window)
{
    const char* raw = window->gameDirectoryInput->value();
    std::string gamedir = raw ? raw : "";

    DetectionResult result;
    result.edition = EditionId::Unknown;
    if (gamedir.empty()) {
        gOgvmEditionLabel = OGVM_IDLE_LABEL;
    } else {
        result = detectEdition(gamedir);
        if (result.edition == EditionId::Unknown) {
            gOgvmEditionLabel = "Unknown edition - click for details";
        } else {
            gOgvmEditionLabel = editionIdToString(result.edition)
                + " (" + detectionConfidenceToString(result.confidence) + ")";
        }
    }

    window->detectEditionButton->label(gOgvmEditionLabel.c_str());
    gOgvmSourceTip = "Data source: " + OgvmSourceToString(OgvmDetectSource(gamedir));
    window->detectEditionButton->tooltip(gOgvmSourceTip.c_str());

    {
        OgvmDataSource ogvmSrc = OgvmDetectSource(gamedir);
        gOgvmRadioUserClick = false;
        if (ogvmSrc == OgvmDataSource::InstallerUnpack) {
            window->sourceInstallerRadio->value(1);
            window->sourceFolderRadio->value(0);
        } else if (ogvmSrc == OgvmDataSource::ExistingFolder) {
            window->sourceInstallerRadio->value(0);
            window->sourceFolderRadio->value(1);
        } else if (!window->sourceInstallerRadio->value()
                   && !window->sourceFolderRadio->value()) {
            window->sourceFolderRadio->value(1);
        }
        OgvmApplySourceLayout(window);
        gOgvmRadioUserClick = true;

        if (gamedir.empty()) {
            gOgvmStatusText = "Choose how you have the game data";
        } else if (result.edition == EditionId::Unknown) {
            gOgvmStatusText = "Folder set — edition unknown (click Detect for details)";
        } else {
            gOgvmStatusText = "Ready — "
                + editionIdToString(result.edition)
                + " (" + detectionConfidenceToString(result.confidence) + ") — to play";
        }
        window->sourceStatusLabel->label(gOgvmStatusText.c_str());
        /* OGVM-READYSTYLE: xanh la + 12pt; an nut Detect khi da biet edition */
        window->sourceStatusLabel->labelfont(FL_HELVETICA);
        window->sourceStatusLabel->labelsize(12);
        if (!gamedir.empty() && result.edition != EditionId::Unknown) {
            window->sourceStatusLabel->labelcolor(fl_rgb_color(16,124,16)); /* OGVM-FLUENT2-DARK */
            window->detectEditionButton->hide();
        } else {
            window->sourceStatusLabel->labelcolor(
                gamedir.empty() ? fl_rgb_color(32,31,30)
                               : fl_rgb_color(196,43,28) /* OGVM-FLUENT2-DARK */);
            window->detectEditionButton->show();
            window->detectEditionButton->redraw();
        }

        window->sourceStatusLabel->redraw();
        window->sourceInstallerRadio->redraw();
        window->sourceFolderRadio->redraw();
    }
    window->detectEditionButton->redraw();
}

void OgvmAutodetectTimeoutCb(void* userdata)
{
    OgvmRefreshEditionLabel(static_cast< Launcher* >(userdata));
}
/* --------------------------------------------------------------------- */

void Launcher::detectEditionCb(Fl_Widget* btn, void* userdata) {
    Launcher* window = static_cast<Launcher*>(userdata);
    const char* raw = window->gameDirectoryInput->value();
    std::string gamedir = raw ? raw : "";

    fl_message_title("Detect Edition");

    if (gamedir.empty()) {
        fl_alert("Please choose the JA2 game directory first.");
        return;
    }

    DetectionResult result = detectEdition(gamedir);

    /* OGVM-SHORTERR: short popup — no full file dump */
    std::string message;
    if (result.edition == EditionId::Unknown) {
        message = "Could not detect a known JA2 edition.";
        if (!result.reasons.empty()) {
            message += "\n";
            message += result.reasons.front();
        }
    } else {
        message = editionIdToString(result.edition)
            + " (" + detectionConfidenceToString(result.confidence) + ")\n"
            + "Source: " + OgvmSourceToString(OgvmDetectSource(gamedir));
    }

    /* OGVM-SYNCORDER */
    OgvmRefreshEditionLabel(window);
    fl_message("%s", message.c_str());
}

void Launcher::importGameDataCb(Fl_Widget* btn, void* userdata) {
    Launcher* window = static_cast< Launcher* >( userdata );
    fl_message_title(window->importGameDataButton->label());

    if (window->importProcess) {
        fl_alert("An import is already running. Please wait until it has finished.");
        return;
    }

    // OGVM-INNOPATH: search innoextract next to the launcher exe first
    std::string ogvmInnoDir;
    {
        RustPointer<char> _exeP(Env_currentExe());
        if (_exeP) {
            std::string _ep(_exeP.get());
            auto _sl = _ep.find_last_of("/\\");
            if (_sl != std::string::npos) {
                std::string _dir = _ep.substr(0, _sl + 1);
#ifdef _WIN32
                std::string _cand = _dir + "innoextract.exe";
#else
                std::string _cand = _dir + "innoextract";
#endif
                if (FileMan::exists(_cand.c_str())) ogvmInnoDir = _cand;
            }
        }
    }
    // innoextract performs the actual unpacking. It is an external tool, so
    // locate it first and tell the user how to install it when it is missing.
    const char* toolCandidates[] = {
        "/opt/homebrew/bin/innoextract",
        "/usr/local/bin/innoextract",
        "/usr/bin/innoextract",
    };
    ST::string tool;
    if (!ogvmInnoDir.empty()) tool = ogvmInnoDir; // OGVM-INNOPATH set
    for (const char* candidate : toolCandidates) {
        if (FileMan::exists(candidate)) {
            tool = candidate;
            break;
        }
    }
    if (tool.empty()) {
        fl_alert("Could not find innoextract, which is required to unpack an installer.\n\n"
#ifdef _WIN32
            "Place innoextract.exe in the same folder as ja2-launcher.exe\n"
            "Download from: https://constexpr.org/innoextract/"
#else
            "On macOS install it with:\n    brew install innoextract"
#endif
        );
        return;
    }


    Fl_Native_File_Chooser installerChooser;
    installerChooser.title("Select the Jagged Alliance 2 installer (setup_*.exe)");
    installerChooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
    installerChooser.filter("Windows installer\t*.exe");
    if (installerChooser.show() != 0) return;

    std::error_code ec;
    const std::filesystem::path installer(installerChooser.filename());
    if (!std::filesystem::is_regular_file(installer, ec)) {
        fl_alert("That is not a file.\n\nPlease choose the installer executable itself, for example setup_jagged_alliance_2.exe.");
        return;
    }

    /* OGVM-IMPHOME: always unpack under ~/.ja2/imported/<installer-stem>.
     * User only picks the .exe (keep the companion .bin next to it).
     * Avoids Documents/Desktop TCC "Operation not permitted" on macOS
     * and skips the destination-folder chooser entirely. */
    RustPointer<char> homeP(findPathFromStracciatellaHome(window->engineOptions.get(), nullptr, false, false));
    if (!homeP) {
        fl_alert("Could not find the OldGameVM home folder (~/.ja2).");
        return;
    }
    const std::filesystem::path parent = std::filesystem::path(homeP.get()) / "imported";
    const std::filesystem::path destination = parent / installer.stem();
    std::filesystem::create_directories(destination, ec);
    if (!std::filesystem::is_directory(destination, ec)) {
        fl_alert("Could not create the destination folder:\n%s", destination.string().c_str());
        return;
    }

    const std::string installerPath = installer.string();
    const std::string destinationPath = destination.string();

    // SLICE-CHECK: GOG installers keep most of the payload in companion
    // slice files named "<installer>-1.bin". A renamed or missing slice
    // makes the unpacking produce an empty Data folder.
    {
    	const std::filesystem::path installerFile(installerPath.c_str());
    	const std::filesystem::path sliceDir = installerFile.parent_path();
    	const std::string stem = installerFile.stem().string();
    	std::error_code sliceEc;
    	const bool numbered = std::filesystem::is_regular_file(sliceDir / (stem + "-1.bin"), sliceEc);
    	const bool plain = std::filesystem::is_regular_file(sliceDir / (stem + ".bin"), sliceEc);
    	if (!numbered && plain) {
    		fl_message_title("Import Game Data");
    		fl_alert("This installer needs its companion data file to be named\n\n%s-1.bin\n\nbut the folder only contains\n\n%s.bin\n\nRename that file so it ends with \"-1.bin\", then try again.", stem.c_str(), stem.c_str());
    		return;
    	}
    }

    if (fl_choice("Import game data from:\n%s\n\ninto:\n%s\n\nKeep the companion .bin file next to the .exe (same folder, original name).\nThis takes a few minutes and needs roughly 1 GB of free space.",
            "Cancel", "Import", nullptr, installerPath.c_str(), destinationPath.c_str()) != 1) {
        return;
    }

    RustPointer<VecCString> args(VecCString_create());
    VecCString_push(args.get(), "--extract");
    VecCString_push(args.get(), "--output-dir");
    VecCString_push(args.get(), destinationPath.c_str());
    VecCString_push(args.get(), installerPath.c_str());

    window->importDestination = encodePath(destinationPath.c_str());
    window->importProcess = std::make_optional(RustPointer<SubProcess>(Subprocess_new(tool.c_str(), args.get())));
    /* OGVM-PROGRESS: bat dau do tien trinh. Duong dan phai qua decodePath
     * dung nhu cach ma hien co lam, khong lay chuoi tho. */
    ST::char_buffer ogvmDestDecoded = decodePath(window->importDestination.c_str());
    gOgvmImportDir = std::string(ogvmDestDecoded.c_str());
    gOgvmImportRunning = true;
    gOgvmImportTicks = 0;
    gOgvmProgress = window->importProgress;
    gOgvmStatusBox = window->sourceStatusLabel;
    gOgvmImportBtn = window->importGameDataButton;
    gOgvmImportText = "Importing... starting";
    if (gOgvmImportBtn)
    {
    	gOgvmImportBtn->deactivate();
    }
    if (gOgvmStatusBox)
    {
    	gOgvmStatusBox->hide();
    }
    if (gOgvmProgress)
    {
    	gOgvmProgress->value(0.0f);
    	gOgvmProgress->label(gOgvmImportText.c_str());
    	gOgvmProgress->show();
    }
    Fl::add_timeout(OGVM_IMPORT_TICK, OgvmImportTickCb, (void*)(window));
    Launcher::maintainImportState(window);
}

void Launcher::maintainImportState(void* userdata) {
    Launcher* window = static_cast< Launcher* >( userdata );
    if (!window->importProcess) {
        return;
    }

    Subprocess_process(window->importProcess.value().get());
    if (!Subprocess_isDone(window->importProcess.value().get())) {
        Fl::add_timeout(checkGameRunningIntervalSeconds, Launcher::maintainImportState, window);
        return;
    }

    auto exitCode = Subprocess_getExitCode(window->importProcess.value().get());
    window->importProcess = std::nullopt;
    gOgvmImportRunning = false;   /* OGVM-PROGRESS */
    OgvmImportFinishLook();

    // Installers frequently contain redistributable payloads that cannot be
    // unpacked, which makes innoextract report a non-zero exit code even
    // though all game data was written. Judge success by the data itself.
    ST::char_buffer decoded = decodePath(window->importDestination.c_str());
    const std::filesystem::path destination(decoded.c_str());
    std::error_code ec;
    int slfCount = 0;
    const std::filesystem::path dataDir = destination / "Data";
    if (std::filesystem::is_directory(dataDir, ec)) {
    	std::error_code walkEc;
    	for (const auto& entry : std::filesystem::directory_iterator(dataDir, walkEc)) {
    		std::string ext = entry.path().extension().string();
    		std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
    		if (ext == ".slf") slfCount++;
    	}
    }
    const bool hasData = slfCount > 0;

    fl_message_title("Import Game Data");
    if (!hasData) {
        fl_alert("Unpacking finished with exit code %d, but no Data folder was found in:\n%s\n\nMake sure the chosen file is a Jagged Alliance 2 installer, and that any companion .bin file sits next to it under its original name.",
                (int) exitCode, destination.string().c_str());
        return;
    }

    window->gameDirectoryInput->value(window->importDestination.c_str());
    window->update(true);
    OgvmRefreshEditionLabel(window);


    DetectionResult result = detectEdition(destination.string());
    std::string message = "The game data was imported successfully.\n\n";
    if (result.edition == EditionId::Unknown) {
        message += "The edition could not be identified, so you may need to pick the game version by hand.";
    } else {
        message += "Detected edition: " + editionIdToString(result.edition) + "\n";
        message += "The game directory has been filled in for you.";
    }
    if (exitCode != 0) {
        message += "\n\n(innoextract reported warnings, exit code ";
        message += std::to_string((int) exitCode);
        message += ". Game data looks complete.)";
    }
    fl_message("%s", message.c_str());
}

void Launcher::setPredefinedResolution(Fl_Widget* btn, void* userdata) {
	Fl_Menu_Button* menuBtn = static_cast< Fl_Menu_Button* >( btn );
	Launcher* window = static_cast< Launcher* >( userdata );
	ST::string res = menuBtn->mvalue()->label();
	int x = 0;
	int y = 0;
	(void)sscanf(res.c_str(), "%d" RESOLUTION_SEPARATOR "%d", &x, &y);
	window->resolutionXInput->value(x);
	window->resolutionYInput->value(y);
	window->update(true);
}

void Launcher::widgetChanged(Fl_Widget* widget, void* userdata) {
	Launcher* window = static_cast< Launcher* >( userdata );
	window->update(true);
}

void Launcher::reloadJa2Json(Fl_Widget* widget, void* userdata) {
	Launcher* window = static_cast< Launcher* >( userdata );
	window->loadJa2Json();
	window->initializeInputsFromDefaults();
}

void Launcher::saveJa2Json(Fl_Widget* widget, void* userdata) {
	Launcher* window = static_cast< Launcher* >( userdata );
	window->writeJsonFile();
}

void Launcher::showModDetails(const ST::string& modId) {
	int styleTableSize = sizeof(styleTable)/sizeof(styleTable[0]);
	Fl_Text_Buffer *textBuffer = new Fl_Text_Buffer();
    Fl_Text_Buffer *styleBuffer = new Fl_Text_Buffer();
	RustPointer<Mod> mod(ModManager_getAvailableModById(this->modManager.get(), modId.c_str()));
	if (mod.get() != NULL) {
		ST::string modName(RustPointer<char>(Mod_getName(mod.get())).get());
		ST::string modVersion(RustPointer<char>(Mod_getVersionString(mod.get())).get());
		ST::string modDescription(RustPointer<char>(Mod_getDescription(mod.get())).get());

		auto nameLine = ST::format("{}\n", modName);
		auto versionLine = ST::format("Version: {}\n\n", modVersion);
		auto description = ST::string("");
		if (!modDescription.empty()) {
			description = ST::format("{}\n\n", modDescription);
		}
		auto idLine = ST::format("Mod Id: {}", modId);

		auto modDetails = nameLine + versionLine + description + idLine;
		ST::string modDetailsStyle;
		for (size_t i = 0; i < nameLine.size(); i++) {
			modDetailsStyle += "A";
		}
		for (size_t i = 0; i < (versionLine.size() + description.size() + idLine.size()); i++) {
			modDetailsStyle += "B";
		}

		textBuffer->text(modDetails.c_str());
		styleBuffer->text(modDetailsStyle.c_str());
	} else {
		auto error = ST::format("Error: Could not find mod '{}'", modId);

		ST::string errorStyle;
		for (size_t i = 0; i < error.size(); i++) {
			errorStyle += "C";
		}

		textBuffer->text(error.c_str());
		styleBuffer->text(errorStyle.c_str());
	}

	this->modDetails->buffer(textBuffer);
	this->modDetails->highlight_data(styleBuffer, styleTable, styleTableSize, 'A', 0, 0);
	this->modDetails->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);
	this->modDetails->show();
}

void Launcher::hideModDetails() {
	this->modDetails->hide();
}

void Launcher::selectAvailableMods(Fl_Widget* widget, void* userdata) {
	Launcher* window = static_cast< Launcher* >( userdata );

	window->enableModsButton->activate();
	window->disableModsButton->deactivate();
	window->moveUpModsButton->deactivate();
	window->moveDownModsButton->deactivate();
	auto nenabled = window->enabledModsBrowser->size();
	for (auto i = 1; i <= nenabled; i++) {
		window->enabledModsBrowser->select(i, 0);
	}
	std::vector<ST::string> selectedMods;
	auto navailable = window->availableModsBrowser->size();
	for (auto i = 1; i <= navailable; i++) {
		if (window->availableModsBrowser->visible(i) && window->availableModsBrowser->selected(i)) {
			selectedMods.push_back(ST::string(static_cast<char*>(window->availableModsBrowser->data(i))));
		}
	}
	if (selectedMods.size() == 1) {
		window->showModDetails(selectedMods[0]);
	} else {
		window->hideModDetails();
	}
}

void Launcher::selectEnabledMods(Fl_Widget* widget, void* userdata) {
	Launcher* window = static_cast< Launcher* >( userdata );

	window->enableModsButton->deactivate();
	window->disableModsButton->activate();
	window->moveUpModsButton->activate();
	window->moveDownModsButton->activate();
	auto navailable = window->availableModsBrowser->size();
	for (auto i = 1; i <= navailable; i++) {
		window->availableModsBrowser->select(i, 0);
	}
	std::vector<ST::string> selectedMods;
	auto nenabled = window->enabledModsBrowser->size();
	for (auto i = 1; i <= nenabled; i++) {
		if (window->enabledModsBrowser->visible(i) && window->enabledModsBrowser->selected(i)) {
			selectedMods.push_back(ST::string(static_cast<char*>(window->enabledModsBrowser->data(i))));
		}
	}
	if (selectedMods.size() == 1) {
		window->showModDetails(selectedMods[0]);
	} else {
		window->hideModDetails();
	}
}

void Launcher::enableMods(Fl_Widget* widget, void* userdata) {
	Launcher* window = static_cast< Launcher* >( userdata );

	bool updated = false;
	for (auto i = window->availableModsBrowser->size(); i > 0; i--) {
		if (window->availableModsBrowser->selected(i) && window->availableModsBrowser->visible(i)) {
			updated = true;
			window->enabledModsBrowser->add(window->availableModsBrowser->text(i), window->availableModsBrowser->data(i));
			window->enabledModsBrowser->select(window->enabledModsBrowser->size());
			window->enabledModsBrowser->bottomline(window->enabledModsBrowser->size());
			window->availableModsBrowser->hide(i);
		}
	}

	if (updated) {
		window->selectEnabledMods(widget, userdata);
		window->enabledModsBrowser->redraw();
		window->availableModsBrowser->redraw();
		window->update(true);
	}
}

void Launcher::disableMods(Fl_Widget* widget, void* userdata) {
	Launcher* window = static_cast< Launcher* >( userdata );

	bool updated = false;
	for (auto i = window->enabledModsBrowser->size(); i > 0; i--) {
		if (window->enabledModsBrowser->selected(i)) {
			updated = true;

			auto id = ST::string(static_cast<char*>(window->enabledModsBrowser->data(i)));
			window->enabledModsBrowser->remove(i);
			for (auto j = window->availableModsBrowser->size(); j > 0; j--) {
				auto otherId = ST::string(static_cast<char*>(window->availableModsBrowser->data(j)));
				if (id == otherId) {
					window->availableModsBrowser->show(j);
					window->availableModsBrowser->select(j, 1);
				}
			}
		}
	}

	if (updated) {
		window->selectAvailableMods(widget, userdata);
		window->enabledModsBrowser->redraw();
		window->availableModsBrowser->redraw();
		window->update(true);
	}
}

void Launcher::moveUpMods(Fl_Widget* widget, void* userdata) {
	Launcher* window = static_cast< Launcher* >( userdata );
	int nitems = window->enabledModsBrowser->size();

	if (nitems <= 1) {
		return;
	}

	// Fltk line indexing is 1 based
	for (auto i = 2; i <= nitems; i++) {
		if (window->enabledModsBrowser->selected(i) && !window->enabledModsBrowser->selected(i-1)) {
			window->enabledModsBrowser->swap(i, i-1);
		}
	}

	window->enabledModsBrowser->redraw();
	window->update(true);
}

void Launcher::moveDownMods(Fl_Widget* widget, void* userdata) {
	Launcher* window = static_cast< Launcher* >( userdata );
	int nitems = window->enabledModsBrowser->size();

	if (nitems <= 1) {
		return;
	}

	// Fltk line indexing is 1 based
	for (auto i = nitems - 1; i > 0; i--) {
		if (window->enabledModsBrowser->selected(i) && !window->enabledModsBrowser->selected(i+1)) {
			window->enabledModsBrowser->swap(i, i+1);
		}
	}

	window->enabledModsBrowser->redraw();
	window->update(true);
}

void Launcher::selectGameVersion(Fl_Widget* widget, void* userdata)
{
	Launcher* window = static_cast<Launcher*>(userdata);
	int currentResourceVersionIndex = window->gameVersionInput->value();
	GameVersion currentResourceVersion = predefinedVersions.at(currentResourceVersionIndex);
	if (currentResourceVersion == GameVersion::SIMPLIFIED_CHINESE)
	{
		//force enable Simplified Chinese Mod
		for (auto i = window->availableModsBrowser->size(); i > 0; i--)
		{
			char* modId = static_cast<char*>(window->availableModsBrowser->data(i));
			window->availableModsBrowser->select(i, strcmp(modId, SIMPLIFIED_CHINESE_MOD_NAME) == 0 ? 1 : 0);
		}
		enableMods(window->enableModsButton, userdata);
	}
	else
	{
		//force diable Simplified Chinese Mod
		for (auto i = window->enabledModsBrowser->size(); i > 0; i--)
		{
			char* modId = static_cast<char*>(window->enabledModsBrowser->data(i));
			window->enabledModsBrowser->select(i, strcmp(modId, SIMPLIFIED_CHINESE_MOD_NAME) == 0 ? 1 : 0);
		}
		disableMods(window->disableModsButton, userdata);
	}

	window->update(true);
}
