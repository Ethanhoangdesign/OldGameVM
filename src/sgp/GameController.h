/* OldGameVM addition
 * OGVM-CONTROLLER: native gamepad support (SDL_GameController).
 * Bien tay cam thanh chuot/ban phim ngay trong vong lap input cua game.
 * Khong process ngoai, chay Mac/Windows/Linux.
 */
#ifndef OGVM_GAME_CONTROLLER_H
#define OGVM_GAME_CONTROLLER_H

#include "SDL.h"

/* Khoi tao he thong tay cam. Doc co "enabled" tu <home>/controller.ini;
 * neu tat thi khong lam gi. Goi sau SDL_Init trong SGP.cc. */
void GameController_Init(void);

/* Dong controller + tat subsystem. Goi luc thoat. */
void GameController_Shutdown(void);

/* Xu ly event tay cam (device added/removed, button down/up).
 * Goi tu switch trong MainLoop. */
void GameController_HandleEvent(const SDL_Event* event);

/* Goi moi frame: doc analog, di chuyen con tro chuot. */
void GameController_Update(void);

/* Da bat va co tay cam dang mo? (cho launcher/UI neu can) */
bool GameController_IsActive(void);

#endif
