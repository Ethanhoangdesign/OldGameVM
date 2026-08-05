/* OldGameVM addition
 * OGVM-CONTROLLER: FLTK gamepad diagram — real PNG if present, else fl_* sketch.
 * Header-only so CMake GLOB picks it up without adding a .cc.
 * layout: 0=Xbox, 1=PS5. highlightToken = SDL button/axis name (sketch only).
 */
#pragma once

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/fl_draw.H>
#include <cstring>

class ControllerView : public Fl_Box {
public:
	int layout = 0; // 0 Xbox, 1 PS5
	// SDL name: "a","b","leftshoulder","lefttrigger",... or empty
	char highlightToken[32] = {};

	ControllerView(int X, int Y, int W, int H, const char* L = 0)
		: Fl_Box(X, Y, W, H, L)
	{
		box(FL_FLAT_BOX);
		highlightToken[0] = '\0';
	}

	~ControllerView() override
	{
		delete padImg[0];
		delete padImg[1];
	}

	void setHighlight(const char* token)
	{
		if (!token) { highlightToken[0] = '\0'; return; }
		std::strncpy(highlightToken, token, sizeof(highlightToken) - 1);
		highlightToken[sizeof(highlightToken) - 1] = '\0';
	}

	/* Load real pad art (path from findPathFromAssetsDir). Null/fail → sketch. */
	void setPadImage(int idx, const char* path)
	{
		if (idx < 0 || idx > 1) return;
		delete padImg[idx];
		padImg[idx] = nullptr;
		if (!path || !*path) return;
		Fl_PNG_Image* p = new Fl_PNG_Image(path);
		if (!p || p->w() <= 0 || p->h() <= 0) {
			delete p;
			return;
		}
		padImg[idx] = p;
	}

protected:
	void draw() override
	{
		draw_box();
		draw_label();

		if (drawPadImage()) return;

		const int cx = x() + w() / 2;
		const int cy = y() + h() / 2 + 6;
		const bool ps = (layout == 1);

		// Body
		fl_color(fl_rgb_color(70, 74, 82));
		fl_pie(cx - 95, cy - 45, 190, 100, 0, 360);
		fl_rectf(cx - 80, cy - 20, 160, 55);
		fl_pie(cx - 105, cy + 5, 50, 55, 0, 360);
		fl_pie(cx + 55,  cy + 5, 50, 55, 0, 360);

		// Triggers above bumpers (LT/RT or L2/R2)
		drawBtn(cx - 55, cy - 66, 40, 11, ps ? "L2" : "LT", hi("lefttrigger"));
		drawBtn(cx + 15, cy - 66, 40, 11, ps ? "R2" : "RT", hi("righttrigger"));
		// Bumpers
		drawBtn(cx - 55, cy - 52, 40, 12, ps ? "L1" : "LB", hi("leftshoulder"));
		drawBtn(cx + 15, cy - 52, 40, 12, ps ? "R1" : "RB", hi("rightshoulder"));

		// D-pad left
		drawDpad(cx - 60, cy + 5);

		// Face buttons right
		if (ps) {
			// PS diamond: top triangle, right circle, bottom cross, left square
			drawRound(cx + 60, cy - 18, 11, "T", hi("y"));
			drawRound(cx + 78, cy + 0,  11, "O", hi("b"));
			drawRound(cx + 60, cy + 18, 11, "X", hi("a"));
			drawRound(cx + 42, cy + 0,  11, "[]", hi("x"));
		} else {
			drawRound(cx + 60, cy + 18, 11, "A", hi("a"));
			drawRound(cx + 78, cy + 0,  11, "B", hi("b"));
			drawRound(cx + 42, cy + 0,  11, "X", hi("x"));
			drawRound(cx + 60, cy - 18, 11, "Y", hi("y"));
		}

		// Sticks
		drawStick(cx - 30, cy + 28);
		drawStick(cx + 30, cy + 28);

		// Center: Back/View + Start/Options
		drawBtn(cx - 20, cy - 8, 18, 10, ps ? "Cr" : "Vw", hi("back"));
		drawBtn(cx + 4,  cy - 8, 18, 10, ps ? "Op" : "Mn", hi("start"));
	}

private:
	Fl_Image* padImg[2] = {nullptr, nullptr}; // 0 Xbox, 1 PS5

	bool drawPadImage()
	{
		const int idx = (layout == 1) ? 1 : 0;
		Fl_Image* src = padImg[idx];
		if (!src) return false;

		const int maxw = w() > 4 ? w() - 4 : w();
		const int maxh = h() > 4 ? h() - 4 : h();
		if (maxw <= 0 || maxh <= 0) return false;

		const int iw = src->w();
		const int ih = src->h();
		if (iw <= 0 || ih <= 0) return false;

		int dw = maxw, dh = maxh;
		if (iw * maxh > ih * maxw) dh = ih * maxw / iw;
		else dw = iw * maxh / ih;

		Fl_Image* scaled = src->copy(dw, dh);
		if (!scaled) return false;
		scaled->draw(x() + (w() - dw) / 2, y() + (h() - dh) / 2);
		delete scaled;
		return true;
	}

	bool hi(const char* token) const
	{
		return highlightToken[0] && std::strcmp(highlightToken, token) == 0;
	}

	void drawRound(int cx, int cy, int r, const char* lab, bool on)
	{
		fl_color(on ? fl_rgb_color(0, 162, 232) : fl_rgb_color(200, 200, 205));
		fl_pie(cx - r, cy - r, r * 2, r * 2, 0, 360);
		fl_color(FL_BLACK);
		fl_font(FL_HELVETICA_BOLD, 9);
		fl_draw(lab, cx - 8, cy - 5, 16, 12, FL_ALIGN_CENTER);
	}

	void drawBtn(int x, int y, int w, int h, const char* lab, bool on)
	{
		fl_color(on ? fl_rgb_color(0, 162, 232) : fl_rgb_color(160, 164, 172));
		fl_rectf(x, y, w, h);
		fl_color(FL_BLACK);
		fl_font(FL_HELVETICA, 8);
		fl_draw(lab, x, y, w, h, FL_ALIGN_CENTER);
	}

	void drawStick(int cx, int cy)
	{
		fl_color(fl_rgb_color(50, 52, 58));
		fl_pie(cx - 14, cy - 14, 28, 28, 0, 360);
		fl_color(fl_rgb_color(120, 124, 132));
		fl_pie(cx - 8, cy - 8, 16, 16, 0, 360);
	}

	void drawDpad(int cx, int cy)
	{
		const int s = 12;
		drawBtn(cx - s / 2, cy - s * 2, s, s, "^", hi("dpup"));
		drawBtn(cx - s / 2, cy + s,     s, s, "v", hi("dpdown"));
		drawBtn(cx - s * 2, cy - s / 2, s, s, "<", hi("dpleft"));
		drawBtn(cx + s,     cy - s / 2, s, s, ">", hi("dpright"));
		fl_color(fl_rgb_color(90, 94, 100));
		fl_rectf(cx - s / 2, cy - s / 2, s, s);
	}
};
