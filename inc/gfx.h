#ifndef GFX_H
#define GFX_H

#include <string>

namespace gfx
{
	//Inits graphics and shared font. Code for shared font is from switch-portlibs examples
	bool init();
	bool exit();

	//Changes gfx mode to linear double
	void switchMode();

	void handleBuffs();

	//Clears framebuffer to clr. RGBA8
	void clearBufferColor(const uint32_t& clr);

	//Blends two pixels. Clr is the color to be drawn. FB should be the framebuffer pixel
	uint32_t blend(const uint32_t& clr, const uint32_t& fb);

	//Draws text using shared font.
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
			Now uses libpng and png files
			Only accepts RGBA8 PNGs
			Others will have issues.
			*/
			void loadFromFile(const std::string& path);
			//Icons
			void loadJpegMem(const uint8_t *txData, const uint32_t& jpegSz);
			//Frees memory used by data
			void deleteData();
			uint16_t getWidth();
			uint16_t getHeight();
			const uint32_t *getDataPointer();

			void draw(uint32_t x, uint32_t y);
			void drawNoBlend(uint32_t x, uint32_t y);

			//For lazy-scaling icons
			//Skips every-other pixel and row, so 1/2 scale
			void drawNoBlendSkip(unsigned x, unsigned y);

			//These only repeat pixels.
			void drawRepeatHori(uint32_t x, uint32_t y, uint32_t w);
			void drawRepeatHoriNoBlend(uint32_t x, uint32_t y, uint32_t w);

			void drawRepeatVert(uint32_t x, uint32_t y, uint32_t h);
			void drawRepeatVertNoBlend(uint32_t x, uint32_t y, uint32_t h);

		private:
			uint32_t sz;
			uint16_t width, height;
			uint32_t *data = NULL;
	};
}

#endif // GFX_H
