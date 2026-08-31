#ifndef _OPTIONS_SCREEN__H_
#define _OPTIONS_SCREEN__H_

#include "ScreenIDs.h"


#ifdef __ANDROID__
// Humanist heavier than Arial (no 16pt bold STI); white on metal chrome.
#define OPT_BUTTON_FONT		FONT14HUMANIST
#define OPT_BUTTON_ON_COLOR	FONT_MCOLOR_WHITE
#define OPT_BUTTON_OFF_COLOR	FONT_MCOLOR_WHITE
#else
#define OPT_BUTTON_FONT		FONT14ARIAL
#define OPT_BUTTON_ON_COLOR	73
#define OPT_BUTTON_OFF_COLOR	73
#endif


//Record the previous screen the user was in.
extern ScreenID guiPreviousOptionScreen;


ScreenID OptionsScreenHandle(void);

#endif
