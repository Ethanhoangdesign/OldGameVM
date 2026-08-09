/* OldGameVM addition
 * OGVM-CONTROLLER: native gamepad support (SDL_GameController).
 * Pad button/trigger -> mouse/key output (AntiMicroX-style).
 */
#include "GameController.h"

#include "Input.h"
#include "Timer.h"
#include "Logger.h"
#include "UILayout.h"
#include "RustInterface.h"
#include "MouseSystem.h"

#include <fstream>
#include <string>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cassert>

namespace {

SDL_GameController* g_pad     = nullptr;
bool                g_enabled = false;
UINT32              g_lastUpdate = 0;
// stick mode: 0 empty, 1 cursor, 2 wasd, 3 arrow
int                 g_leftStickMode  = 1;
int                 g_rightStickMode = 0;

// PS4/PS5 touchpad: cursor (swipe) or button (click → key/mouse)
bool                g_touchpadCursor = true; // true only when touchpad=cursor
bool                g_tpFingerDown    = false;
float               g_tpLastX        = 0.f;
float               g_tpLastY        = 0.f;

constexpr int   AXIS_DEADZONE  = 8000;
constexpr float MAX_SPEED_PX   = 600.0f;
constexpr int   TRIGGER_THRESH = 12000;
constexpr int   NUDGE_PX       = 40;
constexpr int   STICK_KEY_THRESH = 16000;
// full swipe across pad ≈ this many logic pixels (default; overridable via ini)
constexpr float TP_SENS_DEFAULT = 1100.0f;
float               g_tpSensPx     = TP_SENS_DEFAULT;

enum OutType {
	OUT_NONE = 0,
	OUT_KEY,
	OUT_MOUSE_BTN,
	OUT_WHEEL,
	OUT_NUDGE
};

struct PadOut {
	OutType type = OUT_NONE;
	int     param = 0; // SDL keycode / mouse button / dir
};

// dir: 0=up 1=down 2=left 3=right
static PadOut g_buttonOut[SDL_CONTROLLER_BUTTON_MAX];
static PadOut g_axisOut[SDL_CONTROLLER_AXIS_MAX];
static bool   g_axisDown[SDL_CONTROLLER_AXIS_MAX];

struct DefaultBind {
	const char* pad; // SDL token
	const char* out; // output spec
};

static const DefaultBind kDefaults[] = {
	{ "a",             "mouse:left" },
	{ "b",             "mouse:right" },
	{ "leftshoulder",  "wheel:up" },
	{ "rightshoulder", "wheel:down" },
	{ "dpup",          "key:up" },
	{ "dpdown",        "key:down" },
	{ "dpleft",        "key:left" },
	{ "dpright",       "key:right" },
	{ "start",         "key:return" },
	{ "back",          "key:escape" },
};

void ClearMaps()
{
	for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) g_buttonOut[i] = {};
	for (int i = 0; i < SDL_CONTROLLER_AXIS_MAX; ++i) {
		g_axisOut[i] = {};
		g_axisDown[i] = false;
	}
}

static SDL_Keycode ParseKeyName(const char* name)
{
	if (!name || !*name) return SDLK_UNKNOWN;
	// single letter/digit
	if (name[1] == '\0') {
		char c = (char)std::tolower((unsigned char)name[0]);
		if (c >= 'a' && c <= 'z') return (SDL_Keycode)c;
		if (c >= '0' && c <= '9') return (SDL_Keycode)c;
	}
	if (std::strcmp(name, "return") == 0 || std::strcmp(name, "enter") == 0) return SDLK_RETURN;
	if (std::strcmp(name, "escape") == 0 || std::strcmp(name, "esc") == 0) return SDLK_ESCAPE;
	if (std::strcmp(name, "space") == 0) return SDLK_SPACE;
	if (std::strcmp(name, "tab") == 0) return SDLK_TAB;
	if (std::strcmp(name, "backspace") == 0) return SDLK_BACKSPACE;
	if (std::strcmp(name, "delete") == 0) return SDLK_DELETE;
	if (std::strcmp(name, "insert") == 0) return SDLK_INSERT;
	if (std::strcmp(name, "home") == 0) return SDLK_HOME;
	if (std::strcmp(name, "end") == 0) return SDLK_END;
	if (std::strcmp(name, "pageup") == 0) return SDLK_PAGEUP;
	if (std::strcmp(name, "pagedown") == 0) return SDLK_PAGEDOWN;
	if (std::strcmp(name, "up") == 0) return SDLK_UP;
	if (std::strcmp(name, "down") == 0) return SDLK_DOWN;
	if (std::strcmp(name, "left") == 0) return SDLK_LEFT;
	if (std::strcmp(name, "right") == 0) return SDLK_RIGHT;
	if (std::strcmp(name, "lshift") == 0 || std::strcmp(name, "shift") == 0) return SDLK_LSHIFT;
	if (std::strcmp(name, "rshift") == 0) return SDLK_RSHIFT;
	if (std::strcmp(name, "lctrl") == 0 || std::strcmp(name, "ctrl") == 0) return SDLK_LCTRL;
	if (std::strcmp(name, "rctrl") == 0) return SDLK_RCTRL;
	if (std::strcmp(name, "lalt") == 0 || std::strcmp(name, "alt") == 0) return SDLK_LALT;
	if (std::strcmp(name, "ralt") == 0) return SDLK_RALT;
	if (name[0] == 'f' || name[0] == 'F') {
		int n = std::atoi(name + 1);
		if (n >= 1 && n <= 12) return (SDL_Keycode)(SDLK_F1 + (n - 1));
	}
	// punctuation via SDL name
	SDL_Keycode k = SDL_GetKeyFromName(name);
	return k;
}

bool ParseOutSpec(const char* spec, PadOut& out)
{
	out = {};
	if (!spec || !*spec || std::strcmp(spec, "none") == 0 || std::strcmp(spec, "off") == 0)
		return true;
	const char* colon = std::strchr(spec, ':');
	if (!colon || colon == spec) return false;
	std::string kind(spec, colon - spec);
	const char* val = colon + 1;
	if (kind == "key") {
		SDL_Keycode k = ParseKeyName(val);
		if (k == SDLK_UNKNOWN) return false;
		out.type = OUT_KEY;
		out.param = (int)k;
		return true;
	}
	if (kind == "mouse") {
		out.type = OUT_MOUSE_BTN;
		if (std::strcmp(val, "left") == 0 || std::strcmp(val, "0") == 0) out.param = SDL_BUTTON_LEFT;
		else if (std::strcmp(val, "right") == 0 || std::strcmp(val, "1") == 0) out.param = SDL_BUTTON_RIGHT;
		else if (std::strcmp(val, "middle") == 0 || std::strcmp(val, "2") == 0) out.param = SDL_BUTTON_MIDDLE;
		else return false;
		return true;
	}
	if (kind == "wheel") {
		out.type = OUT_WHEEL;
		if (std::strcmp(val, "up") == 0) out.param = 1;
		else if (std::strcmp(val, "down") == 0) out.param = 0;
		else return false;
		return true;
	}
	if (kind == "nudge" || kind == "motion") {
		out.type = OUT_NUDGE;
		if (std::strcmp(val, "up") == 0) out.param = 0;
		else if (std::strcmp(val, "down") == 0) out.param = 1;
		else if (std::strcmp(val, "left") == 0) out.param = 2;
		else if (std::strcmp(val, "right") == 0) out.param = 3;
		else return false;
		return true;
	}
	return false;
}

bool BindPadToken(const char* padTok, const PadOut& out)
{
	if (!padTok || !*padTok) return false;
	SDL_GameControllerButton btn = SDL_GameControllerGetButtonFromString(padTok);
	if (btn != SDL_CONTROLLER_BUTTON_INVALID && btn < SDL_CONTROLLER_BUTTON_MAX) {
		g_buttonOut[btn] = out;
		return true;
	}
	SDL_GameControllerAxis axis = SDL_GameControllerGetAxisFromString(padTok);
	if (axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT || axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
		g_axisOut[axis] = out;
		return true;
	}
	return false;
}

void ApplyDefaults()
{
	ClearMaps();
	for (const auto& d : kDefaults) {
		PadOut o;
		if (ParseOutSpec(d.out, o))
			BindPadToken(d.pad, o);
	}
}

/* New format: a=mouse:left  Old format: mouse_left=a (migrated). */
bool ReadConfig()
{
	ApplyDefaults();

	RustPointer<char> home(EngineOptions_getStracciatellaHome());
	if (!home) return false;
	std::string path = std::string(home.get()) + "/controller.ini";
	std::ifstream in(path);
	if (!in) return false;

	bool enabled = false;
	g_leftStickMode = 1;
	g_rightStickMode = 0;
	g_touchpadCursor = true;
	g_tpSensPx = TP_SENS_DEFAULT;
	bool touchpadButtonMode = false;
	std::string touchpadOut;
	bool anyNew = false;
	bool sawStickMode = false;
	int legacyCursorStick = -1;

	auto parseStickMode = [](const std::string& val) -> int {
		if (val == "cursor" || val == "1") return 1;
		if (val == "wasd" || val == "2") return 2;
		if (val == "arrow" || val == "arrows" || val == "3") return 3;
		return 0;
	};

	// legacy action -> pad token
	struct Legacy { const char* action; const char* out; };
	static const Legacy kLegacy[] = {
		{ "mouse_left",  "mouse:left" },
		{ "mouse_right", "mouse:right" },
		{ "scroll_up",   "wheel:up" },
		{ "scroll_down", "wheel:down" },
		{ "arrow_up",    "key:up" },
		{ "arrow_down",  "key:down" },
		{ "arrow_left",  "key:left" },
		{ "arrow_right", "key:right" },
		{ "confirm",     "key:return" },
		{ "cancel",      "key:escape" },
	};
	std::string legacyPad[10];
	bool legacySeen = false;

	std::string line;
	while (std::getline(in, line))
	{
		auto eq = line.find('=');
		if (eq == std::string::npos) continue;
		std::string key = line.substr(0, eq);
		std::string val = line.substr(eq + 1);
		if (!val.empty() && val.back() == '\r') val.pop_back();

		if (key == "enabled") { enabled = (val == "1"); continue; }
		if (key == "left_stick") { g_leftStickMode = parseStickMode(val); sawStickMode = true; continue; }
		if (key == "right_stick") { g_rightStickMode = parseStickMode(val); sawStickMode = true; continue; }
		if (key == "cursor_stick") {
			legacyCursorStick = (val == "right" || val == "1") ? 1 : 0;
			continue;
		}
		if (key == "touchpad") {
				// cursor|on|1 → swipe mouse
				// button|btn|2 → click via touchpad_out / "touchpad" bind
				// off|none|0|empty → disabled
				if (val == "button" || val == "btn" || val == "2") {
					g_touchpadCursor = false;
					touchpadButtonMode = true;
				} else if (val == "off" || val == "none" || val == "0" || val == "empty") {
					g_touchpadCursor = false;
					touchpadButtonMode = false;
				} else {
					g_touchpadCursor = true;
					touchpadButtonMode = false;
				}
				continue;
			}
			if (key == "touchpad_out") {
				touchpadOut = val;
				continue;
			}
			if (key == "touchpad_sens") {
				float s = (float)std::atof(val.c_str());
				if (s < 200.f) s = 200.f;
				if (s > 4000.f) s = 4000.f;
				g_tpSensPx = s;
				continue;
			}
			if (key == "layout") continue;

		// new: pad_token=out_spec
		PadOut o;
		if (ParseOutSpec(val.c_str(), o)) {
			if (BindPadToken(key.c_str(), o)) {
				anyNew = true;
				continue;
			}
		}

		// legacy: action=pad_token
		for (int i = 0; i < 10; ++i) {
			if (key == kLegacy[i].action) {
				legacyPad[i] = val;
				legacySeen = true;
				break;
			}
		}
	}

	if (!anyNew && legacySeen) {
		ClearMaps();
		for (int i = 0; i < 10; ++i) {
			if (legacyPad[i].empty() || legacyPad[i] == "none") continue;
			PadOut o;
			if (!ParseOutSpec(kLegacy[i].out, o)) continue;
			BindPadToken(legacyPad[i].c_str(), o);
		}
	}
	if (!sawStickMode && legacyCursorStick >= 0) {
		if (legacyCursorStick == 0) { g_leftStickMode = 1; g_rightStickMode = 0; }
		else { g_leftStickMode = 0; g_rightStickMode = 1; }
	}
	// button mode only: bind physical touchpad click (SDL token "touchpad")
	if (touchpadButtonMode) {
		PadOut o;
		if (ParseOutSpec(touchpadOut.empty() ? "none" : touchpadOut.c_str(), o))
			BindPadToken("touchpad", o);
	}

	return enabled;
}

void LoadMappingDb()
{
	RustPointer<char> dbPath(findPathFromAssetsDir("externalized/gamecontrollerdb.txt", true, true));
	if (dbPath) {
		int n = SDL_GameControllerAddMappingsFromFile(dbPath.get());
		if (n >= 0) SLOGD("OGVM-CONTROLLER: loaded {} extra controller mappings", n);
	}
}

void OpenFirstController()
{
	if (g_pad) return;
	const int joystickCount = SDL_NumJoysticks();
	SLOGI("OGVM-CONTROLLER: enumerating {} joystick(s)", joystickCount);
	for (int i = 0; i < joystickCount; ++i) {
		const char* name = SDL_JoystickNameForIndex(i);
		const bool isController = SDL_IsGameController(i);
		SLOGI("OGVM-CONTROLLER: joystick {} name='{}' game_controller={}",
		      i, name ? name : "<unknown>", isController ? "yes" : "no");
		if (isController) {
			g_pad = SDL_GameControllerOpen(i);
			if (g_pad) {
				const int pads = SDL_GameControllerGetNumTouchpads(g_pad);
				SLOGI("OGVM-CONTROLLER: opened '{}' touchpads={}",
				      SDL_GameControllerName(g_pad), pads);
				g_tpFingerDown = false;
				return;
			}
			SLOGW("OGVM-CONTROLLER: open joystick {} failed: {}", i, SDL_GetError());
		}
	}
}


/* Relative move from PS touchpad finger (normalized 0..1). Mac trackpad style. */
void TouchpadRelMove(float x, float y, bool down)
{
	if (!g_touchpadCursor) return;
	SetUsingTouch(false);
	if (!down) {
		g_tpFingerDown = false;
		return;
	}
	if (!g_tpFingerDown) {
		g_tpFingerDown = true;
		g_tpLastX = x;
		g_tpLastY = y;
		return;
	}
	const float dxN = x - g_tpLastX;
	const float dyN = y - g_tpLastY;
	g_tpLastX = x;
	g_tpLastY = y;
	const int dx = (int)std::lround(dxN * g_tpSensPx);
	const int dy = (int)std::lround(dyN * g_tpSensPx);
	if (dx == 0 && dy == 0) return;
	const int nx = (int)gusMouseXPos + dx;
	const int ny = (int)gusMouseYPos + dy;
	SetSafeMousePosition(nx, ny);
	SimulateMouseMovement(gusMouseXPos, gusMouseYPos);
	RefreshMouseRegions();
}

/* Poll path — works even if host doesn't forward ctouchpad events. */
void UpdateTouchpadCursor()
{
	if (!g_pad || !g_touchpadCursor) return;
	if (SDL_GameControllerGetNumTouchpads(g_pad) <= 0) return;
	Uint8 state = 0;
	float x = 0.f, y = 0.f, pressure = 0.f;
	if (SDL_GameControllerGetTouchpadFinger(g_pad, 0, 0, &state, &x, &y, &pressure) != 0) {
		g_tpFingerDown = false;
		return;
	}
	TouchpadRelMove(x, y, state != 0);
}

void SendKey(SDL_Keycode key, bool down)
{
	SDL_Keysym ks{};
	ks.sym = key;
	ks.scancode = SDL_GetScancodeFromKey(key);
	if (down) KeyDown(&ks); else KeyUp(&ks);
}

void FireOut(const PadOut& o, bool down)
{
	switch (o.type) {
		case OUT_KEY:
			SendKey((SDL_Keycode)o.param, down);
			break;
		case OUT_MOUSE_BTN:
			PadInjectMouseButton((UINT8)o.param, down);
			break;
		case OUT_WHEEL:
			if (down) PadInjectWheel(o.param != 0);
			break;
		case OUT_NUDGE:
			if (down) {
				SetUsingTouch(false);
				int dx = 0, dy = 0;
				if (o.param == 0) dy = -NUDGE_PX;
				else if (o.param == 1) dy = NUDGE_PX;
				else if (o.param == 2) dx = -NUDGE_PX;
				else if (o.param == 3) dx = NUDGE_PX;
				int nx = (int)gusMouseXPos + dx;
				int ny = (int)gusMouseYPos + dy;
				SetSafeMousePosition(nx, ny);
				SimulateMouseMovement(gusMouseXPos, gusMouseYPos);
				RefreshMouseRegions();
			}
			break;
		default: break;
	}
}

void HandleButton(SDL_GameControllerButton btn, bool down)
{
	if (btn < 0 || btn >= SDL_CONTROLLER_BUTTON_MAX) return;
	FireOut(g_buttonOut[btn], down);
}

void UpdateTriggerActions()
{
	if (!g_pad) return;
	static const SDL_GameControllerAxis kTriggers[] = {
		SDL_CONTROLLER_AXIS_TRIGGERLEFT,
		SDL_CONTROLLER_AXIS_TRIGGERRIGHT
	};
	for (SDL_GameControllerAxis axis : kTriggers) {
		if (g_axisOut[axis].type == OUT_NONE) continue;
		int v = SDL_GameControllerGetAxis(g_pad, axis);
		bool down = v >= TRIGGER_THRESH;
		if (down != g_axisDown[axis]) {
			g_axisDown[axis] = down;
			FireOut(g_axisOut[axis], down);
		}
	}
}

} // namespace

void GameController_Init(void)
{
	g_enabled = ReadConfig();
	if (!g_enabled) {
		SLOGD("OGVM-CONTROLLER: disabled (controller.ini enabled!=1)");
		return;
	}
	if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0) {
		SLOGW("OGVM-CONTROLLER: SDL_INIT_GAMECONTROLLER failed: {}", SDL_GetError());
		g_enabled = false;
		return;
	}
	LoadMappingDb();
	OpenFirstController();
	g_lastUpdate = GetClock();
}

void GameController_Shutdown(void)
{
	if (g_pad) { SDL_GameControllerClose(g_pad); g_pad = nullptr; }
	if (SDL_WasInit(SDL_INIT_GAMECONTROLLER))
		SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}

void GameController_HandleEvent(const SDL_Event* event)
{
	if (!g_enabled) return;
	switch (event->type) {
		case SDL_CONTROLLERDEVICEADDED:
			OpenFirstController();
			break;
		case SDL_CONTROLLERDEVICEREMOVED:
			if (g_pad &&
			    event->cdevice.which == SDL_JoystickInstanceID(
			        SDL_GameControllerGetJoystick(g_pad))) {
				SDL_GameControllerClose(g_pad);
				g_pad = nullptr;
				OpenFirstController();
			}
			break;
		case SDL_CONTROLLERBUTTONDOWN:
			HandleButton((SDL_GameControllerButton)event->cbutton.button, true);
			break;
		case SDL_CONTROLLERBUTTONUP:
			HandleButton((SDL_GameControllerButton)event->cbutton.button, false);
			break;
		case SDL_CONTROLLERTOUCHPADDOWN:
		case SDL_CONTROLLERTOUCHPADMOTION:
			TouchpadRelMove(event->ctouchpad.x, event->ctouchpad.y, true);
			break;
		case SDL_CONTROLLERTOUCHPADUP:
			TouchpadRelMove(event->ctouchpad.x, event->ctouchpad.y, false);
			break;
		default: break;
	}
}

void GameController_Update(void)
{
	UINT32 now = GetClock();
	UINT32 dtMs = now - g_lastUpdate;
	g_lastUpdate = now;
	if (!g_enabled || !g_pad) return;
	if (dtMs == 0 || dtMs > 200) return;

	UpdateTriggerActions();
	UpdateTouchpadCursor();

	auto applyDead = [](int v) -> float {
		if (v > -AXIS_DEADZONE && v < AXIS_DEADZONE) return 0.0f;
		float sign = v < 0 ? -1.0f : 1.0f;
		/* avoid std::abs(int) ambiguity on MSVC when only <cmath> is seen */
		int av = v < 0 ? -v : v;
		float mag  = (av - AXIS_DEADZONE) / (32767.0f - AXIS_DEADZONE);
		if (mag > 1.0f) mag = 1.0f;
		return sign * mag;
	};

	auto moveCursor = [&](int ax, int ay) {
		float nx = applyDead(ax);
		float ny = applyDead(ay);
		if (nx == 0.0f && ny == 0.0f) return;
		float dt = dtMs / 1000.0f;
		int dx = (int)std::lround(nx * MAX_SPEED_PX * dt);
		int dy = (int)std::lround(ny * MAX_SPEED_PX * dt);
		if (dx == 0 && dy == 0) return;
		int nxPos = (int)gusMouseXPos + dx;
		int nyPos = (int)gusMouseYPos + dy;
		SetUsingTouch(false);
		SetSafeMousePosition(nxPos, nyPos);
		SimulateMouseMovement(gusMouseXPos, gusMouseYPos);
		RefreshMouseRegions();
	};

	// Digital stick -> 4 keys (edge). keys: [up,down,left,right]
	auto updateKeyStick = [&](int ax, int ay, SDL_Keycode kUp, SDL_Keycode kDown,
	                          SDL_Keycode kLeft, SDL_Keycode kRight,
	                          bool held[4]) {
		const bool want[4] = {
			ay < -STICK_KEY_THRESH,
			ay >  STICK_KEY_THRESH,
			ax < -STICK_KEY_THRESH,
			ax >  STICK_KEY_THRESH
		};
		const SDL_Keycode keys[4] = { kUp, kDown, kLeft, kRight };
		for (int i = 0; i < 4; ++i) {
			if (want[i] != held[i]) {
				held[i] = want[i];
				SendKey(keys[i], want[i]);
			}
		}
	};

	static bool s_leftHeld[4] = {};
	static bool s_rightHeld[4] = {};
	static int s_leftMode = -1;
	static int s_rightMode = -1;

	const int lx = SDL_GameControllerGetAxis(g_pad, SDL_CONTROLLER_AXIS_LEFTX);
	const int ly = SDL_GameControllerGetAxis(g_pad, SDL_CONTROLLER_AXIS_LEFTY);
	const int rx = SDL_GameControllerGetAxis(g_pad, SDL_CONTROLLER_AXIS_RIGHTX);
	const int ry = SDL_GameControllerGetAxis(g_pad, SDL_CONTROLLER_AXIS_RIGHTY);

	auto releaseHeld = [&](int prevMode, bool held[4]) {
		if (prevMode != 2 && prevMode != 3) {
			for (int i = 0; i < 4; ++i) held[i] = false;
			return;
		}
		const SDL_Keycode keys[4] = {
			(prevMode == 2) ? SDLK_w : SDLK_UP,
			(prevMode == 2) ? SDLK_s : SDLK_DOWN,
			(prevMode == 2) ? SDLK_a : SDLK_LEFT,
			(prevMode == 2) ? SDLK_d : SDLK_RIGHT
		};
		for (int i = 0; i < 4; ++i) {
			if (held[i]) {
				SendKey(keys[i], false);
				held[i] = false;
			}
		}
	};

	auto runStick = [&](int mode, int ax, int ay, bool held[4], int& lastMode) {
		if (lastMode != mode) {
			releaseHeld(lastMode, held);
			lastMode = mode;
		}
		switch (mode) {
			case 1: // cursor
				moveCursor(ax, ay);
				break;
			case 2: // WASD
				updateKeyStick(ax, ay, SDLK_w, SDLK_s, SDLK_a, SDLK_d, held);
				break;
			case 3: // Arrow
				updateKeyStick(ax, ay, SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT, held);
				break;
			default:
				break;
		}
	};

	runStick(g_leftStickMode, lx, ly, s_leftHeld, s_leftMode);
	runStick(g_rightStickMode, rx, ry, s_rightHeld, s_rightMode);
}

bool GameController_IsActive(void)
{
	return g_enabled && g_pad != nullptr;
}
