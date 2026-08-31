#include "Local.h"
#include "SysUtil.h"
#include "VSurface.h"
#include "UILayout.h"


SGPVSurface* guiSAVEBUFFER = nullptr;
SGPVSurface* guiEXTRABUFFER = nullptr;


void InitializeGameVideoObjects()
{
	guiSAVEBUFFER  = AddVideoSurface(SCREEN_WIDTH, SCREEN_HEIGHT, PIXEL_DEPTH);
	guiEXTRABUFFER = AddVideoSurface(SCREEN_WIDTH, SCREEN_HEIGHT, PIXEL_DEPTH);
}


void ShutdownGameVideoObjects()
{
	DeleteVideoSurface(guiSAVEBUFFER);
	DeleteVideoSurface(guiEXTRABUFFER);
	guiSAVEBUFFER  = nullptr;
	guiEXTRABUFFER = nullptr;
}
