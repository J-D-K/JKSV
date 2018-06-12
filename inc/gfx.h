#ifndef GFX_H
#define GFX_H

#include <string>

namespace gfx
{
	//Inits graphics and shared font. Code for shared font is from switch-portlibs examples
	bool init(const uint32_t& _fontSize);
	bool fini();

	//Changes gfx mode to linear double
	void switchMode();

	void handleBuffs();

	//Clears framebuffer to clr. RGBA8
	void clearBufferColor(const uint32_t& clr);

	//Draws text using shared font. No alpha blending yet.
	void drawText(const std::string& str, unsigned x, unsigned y, const uint32_t& sz, const uint32_t& clr);

	//Returns width of str
	unsigned getTextWidth(const std::string& str, const uint32_t& sz);

	//Returns height of text if using sz
	unsigned getTextHeight(const uint32_t& sz);

	//Draws a rectangle. clr = RGBA8. Be careful not to go outside framebuffer
	void drawRectangle(uint32_t x, uint32_t y, const uint32_t& width, const uint32_t& height, const uint32_t& clr);

	//Old 3DS code for loading raw RGBA8 data
	class tex
	{
		public:
			/*
			Loads binary data from path
			First 4 bytes are data size
			Next 2 are width
			Next 2 are height
			The rest is raw RGBA8 data
			*/
			void loadFromFile(const std::string& path);
			//Frees memory used by data
			~tex();
			void draw(uint32_t x, uint32_t y);

		private:
			uint32_t sz;
			uint16_t width, height;
			uint8_t *data = NULL;
	};
}

#endif // GFX_H
