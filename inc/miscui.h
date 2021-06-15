#ifndef MISCUI_H
#define MISCUI_H

#include "gfx.h"

#define PROG_MAX_WIDTH_DEFAULT 576
#define POP_FRAME_DEFAULT 130

//For smaller classes that aren't easy to get lost in and general functions
namespace ui
{
    //Progress bar for showing loading. Mostly so people know it didn't freeze
    class progBar
    {
        public:
            progBar() = default;

            //Constructor. _max is the maximum value
            progBar(const uint64_t& _max, const uint64_t& _maxWidth) { max = _max; maxWidth = _maxWidth; }

            void setMax(const uint64_t& _max, const uint64_t& _maxWidth){ max = _max; maxWidth = _maxWidth; }

            //Updates progress
            void update(const uint64_t& _prog);

            void incProgress(unsigned inc){ prog += inc; }

            //Draws with text at top
            void draw(const std::string& text, const std::string& head);

            //Draws without dialog box
            void drawNoDialog(int x, int y);

        private:
            uint64_t max, prog, maxWidth;
            float width;
    };

    //General use
    void showMessage(const char *head, const char *fmt, ...);
    bool confirm(bool hold, const char *fmt, ...);
    bool confirmTransfer(const std::string& f, const std::string& t);
    bool confirmDelete(const std::string& p);
    void drawTextbox(int x, int y, int w, int h);

    //Popup from freebird
    void showPopup(unsigned frames, const char *fmt, ...);
    void drawPopup(const uint64_t& down);
}

#endif // MISCUI_H
