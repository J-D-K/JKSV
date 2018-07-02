#ifndef MISCUI_H
#define MISCUI_H

enum touchTracks
{
	TOUCH_SWIPE_UP,
	TOUCH_SWIPE_DOWN,
	TOUCH_SWIPE_LEFT,
	TOUCH_SWIPE_RIGHT,
	TOUCH_NOTHING
};

//For smaller classes that aren't easy to get lost in and general functions
namespace ui
{
	//Progress bar for showing loading. Mostly so people know it didn't freeze
	class progBar
	{
		public:
			//Constructor. _max is the maximum value
			progBar(const uint32_t& _max);

			//Updates progress
			void update(const uint32_t& _prog);

			//Draws with text at top
			void draw(const std::string& text);

		private:
			float max, prog, width;
	};

	class button
	{
		public:
			button(const std::string& _txt, unsigned _x, unsigned _y, unsigned _w, unsigned _h);
			void draw();

			unsigned getX();
			unsigned getY();

			bool isOver(const touchPosition& p);
			bool released(const touchPosition& p);

		private:
			bool pressed = false;
			unsigned x, y, w, h;
			unsigned tx, ty;
			std::string text;
			touchPosition prev;
	};

	//Not ready yet.
	class touchTrack
	{
		public:
			void trackTouch(const touchPosition& p);

			int getDifX();
			int getDifY();

		private:
			touchPosition prev, cur;
			int curPos = 0, avX = 0, avY = 0;
	};

	//General use
	void showMessage(const std::string& mess);
	void showError(const std::string& mess, const Result& r);
	bool confirm(const std::string& q);
	bool confirmTransfer(const std::string& f, const std::string& t);
	bool confirmDelete(const std::string& p);
	void debShowTex(gfx::tex tx);
	void drawTextbox(unsigned x, unsigned y, unsigned w, unsigned h);
}

#endif // MISCUI_H
