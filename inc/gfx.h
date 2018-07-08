#ifndef GFX_H
#define GFX_H

#include <string>

namespace gfx
{
	class color
	{
		public:
			void fromRGBA(const uint8_t& _r, const uint8_t& _g, const uint8_t& _b, const uint8_t& _a);
			void fromU32(const uint32_t& _px);
			void invert();

			void setR(const uint8_t& _r) { rgb[3] = _r; }
			void setG(const uint8_t& _g) { rgb[2] = _g; }
			void setB(const uint8_t& _b) { rgb[1] = _b; }
			void setA(const uint8_t& _a) { rgb[0] = _a; }

			uint8_t r() { return rgb[3]; }
			uint8_t g() { return rgb[2]; }
			uint8_t b() { return rgb[1]; }
			uint8_t a() { return rgb[0]; }
			uint32_t clr() { return rgb[0] << 24 | rgb[1] << 16 | rgb[2] << 8 | rgb[3]; }

		private:
			uint8_t rgb[4];
	};

	//Old 3DS code for loading raw RGBA8 data
	class tex
	{
		public:
			/*
			Now uses libpng and png files
			Only accepts RGBA8 PNGs
			Others will have issues.
			*/
			void loadPNGFile(const std::string& path);
			void loadJpegMem(const uint8_t *txData, const size_t& jpegSz);
			void loadJpegFile(const std::string& path);
			//Frees memory used by data
			void deleteData();
			uint16_t getWidth() { return width;}
			uint16_t getHeight() {return height;}
			const uint32_t *getDataPointer() {return data;}

			void draw(uint32_t x, uint32_t y);
			void drawInvert(uint32_t x, uint32_t y);
			void drawNoBlend(uint32_t x, uint32_t y);

			//For lazy-scaling icons
			//Skips every-other pixel and row, so 1/2 scale
			void drawNoBlendSkip(unsigned x, unsigned y);
			void drawNoBlendSkipSmooth(unsigned x, unsigned y);

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


	//Inits graphics and shared font. Code for shared font is from switch-portlibs examples
	bool init();
	bool exit();

	//Changes gfx mode to linear double
	void switchMode();

	void handleBuffs();

	//Clears framebuffer to clr. RGBA8
	void clearBufferColor(const uint32_t& clr);

	//Blends two pixels. Clr is the color to be drawn. FB should be the framebuffer pixel
	uint32_t blend(color& px, color& fb);

	//Draws text using shared font.
	void drawText(const std::string& str, unsigned x, unsigned y, const uint32_t& sz, const uint32_t& clr);

	//Returns width of str
	unsigned getTextWidth(const std::string& str, const uint32_t& sz);

	//Returns height of text if using sz
	unsigned getTextHeight(const uint32_t& sz);

	//Draws a rectangle. clr = RGBA8. Be careful not to go outside framebuffer
	void drawRectangle(uint32_t x, uint32_t y, const uint32_t& width, const uint32_t& height, const uint32_t& clr);
}

#endif // GFX_H
