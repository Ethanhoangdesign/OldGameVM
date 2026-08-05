/* OldGameVM modification notice
 * This file was changed for OldGameVM in July 2026.
 * It is not the original file. See NOTICE.md.
 */
#ifndef JA2_LAUNCHER_H_H
#define JA2_LAUNCHER_H_H

#include "StracciatellaLauncher.h"
#include "RustInterface.h"

#include <string_theory/string>

#include <iostream>
#include <iterator>
#include <set>
#include <optional>
#include <string>

struct sortMods {
    bool operator() (ST::string a, ST::string b) const {
        return a.compare(b) < 0;
    }
};

#define SIMPLIFIED_CHINESE_MOD_NAME "simplified-chinese-localization"

class Launcher : public StracciatellaLauncher {
public:
	Launcher(int argc, char* argv[]);
	~Launcher();

	void loadJa2Json();
	void show();
	void initializeInputsFromDefaults();
	int writeJsonFile();
private:
	int argc;
	char** argv;
	RustPointer<EngineOptions> engineOptions;
	RustPointer<ModManager> modManager;
	std::optional<RustPointer<SubProcess>> subProcess;
	std::optional<RustPointer<SubProcess>> importProcess;
	ST::string importDestination;
	Fl_Text_Buffer logsBuffer;

	void populateChoices();
	void startExecutable(bool asEditor);
	bool resolutionIsInvalid();
	bool gameIsRunning();
	void update(bool changed);
	void updateLogs();
	void showModDetails(const ST::string& modName);
	void hideModDetails();
	static bool checkGameDirectoryForCommonMistakes(const ST::string& gameDir);
	static void openGameDirectorySelector(Fl_Widget *btn, void *userdata);
	static void openSaveGameDirectorySelector(Fl_Widget *btn, void *userdata);
	static void startGame(Fl_Widget* btn, void* userdata);
	static void startEditor(Fl_Widget* btn, void* userdata);
	static void guessVersion(Fl_Widget* btn, void* userdata);
    static void detectEditionCb(Fl_Widget* btn, void* userdata);
    static void importGameDataCb(Fl_Widget* btn, void* userdata);
    static void maintainImportState(void*);
	static void setPredefinedResolution(Fl_Widget* btn, void* userdata);
	static void widgetChanged(Fl_Widget* widget, void* userdata);
	static void reloadJa2Json(Fl_Widget* widget, void* userdata);
	static void saveJa2Json(Fl_Widget* widget, void* userdata);
	static void selectEnabledMods(Fl_Widget* widget, void* userdata);
	static void selectAvailableMods(Fl_Widget* widget, void* userdata);
	static void enableMods(Fl_Widget* widget, void* userdata);
	static void disableMods(Fl_Widget* widget, void* userdata);
	static void moveUpMods(Fl_Widget* widget, void* userdata);
	static void moveDownMods(Fl_Widget* widget, void* userdata);
	static void selectGameVersion(Fl_Widget* widget, void* userdata);
	static void maintainSubProcessState(void*);
	/* OGVM-CONTROLLER: pad -> kind + value (two columns) */
	void loadControllerConfig();
	void saveControllerConfig();
	void refreshControllerStatus();
	void setControllerFieldsEnabled(bool on);
	void openControllerPad();
	void closeControllerPad();
	void refreshBindLabels();
	void applyLayoutChoice();
	void fillPadValueMenu(int padIndex, int kind);
	void applyPadKind(int padIndex);
	void applyPadValue(int padIndex);
	void ensurePadMenusFilled();
	void refreshTouchpadRow();
	void fillTouchpadValueMenu(int kind);
	void applyTouchpadKind();
	void applyTouchpadValue();
	static void controllerToggleCb(Fl_Widget* widget, void* userdata);
	static void controllerLayoutCb(Fl_Widget* widget, void* userdata);
	static void stickModeCb(Fl_Widget* widget, void* userdata);
	static void padKindCb(Fl_Widget* widget, void* userdata);
	static void padValueCb(Fl_Widget* widget, void* userdata);
	static void touchpadSensCb(Fl_Widget* widget, void* userdata);
	static void touchpadModeCb(Fl_Widget* widget, void* userdata);
	static void touchpadKindCb(Fl_Widget* widget, void* userdata);
	static void touchpadValueCb(Fl_Widget* widget, void* userdata);

	// ponytail: first pad only; multi-pad later
	void* padHandle = nullptr; // SDL_GameController*
	int layoutIndex = 0; // 0 Xbox, 1 PS5
	// stick mode: 0 empty, 1 cursor, 2 wasd, 3 arrow
	int leftStickMode = 1;
	int rightStickMode = 0;
	float touchpadSens = 1100.f; // PS5 full-swipe logic px
		int touchpadMode = 0; // 0 cursor, 1 button
		std::string touchpadOutSpec = "none"; // button mode output
	// pad token index 0..13 -> output spec ("mouse:left", "key:a", "none")
	std::string padOutSpec[14];
	bool padMenusFilled = false;
	static const char* padTokens[14];
};

#endif //JA2_LAUNCHER_H_H
