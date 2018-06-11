#ifndef UI_H
#define UI_H

#include <vector>
#include <string>

#include "data.h"

namespace ui
{
    class menu
    {
        public:
            void addOpt(const std::string& add);
            ~menu();

            void handleInput(const uint64_t& down, const uint64_t& held);
            int getSelected();
            void print(const unsigned& x, const unsigned& y, const uint32_t& rectWidth);

            void reset();

        private:
            std::vector<std::string> opt;
            int selected = 0, fc = 0, start = 0;
            uint8_t clrSh = 0;
            bool clrAdd = true;
    };

    class progBar
    {
        public:
            progBar(const uint32_t& _max);
            void update(const uint32_t& _prog);
            void draw(const std::string& text);

        private:
            float max, prog, width;
    };

    class key
    {
        public:
            key(const std::string& txt, const char& _let, const unsigned& _txtSz, const unsigned& _x, const unsigned& _y, const unsigned& _w, const unsigned& _h);
            void updateText(const std::string& txt);
            void draw();
            bool isOver(const touchPosition& p);
            bool released(const touchPosition& p);
            char getLet();
            void toCaps();
            void toLower();

        private:
            bool pressed;
            char let;
            unsigned x, y, w, h;
            unsigned tX, tY;
            unsigned txtSz;
            std::string text;
            touchPosition prev;
    };

    class keyboard
    {
        public:
            keyboard();
            ~keyboard();

            void draw();
            const std::string getString();

        private:
            std::vector<key> keys;
            std::string str;
    };

    void init();

    void userMenuInit();
    void titleMenuPrepare(data::user& usr);
    void folderMenuPrepare(data::user& usr, data::titledata& dat);

    void showUserMenu(const uint64_t& down, const uint64_t& held);
    void showTitleMenu(const uint64_t& down, const uint64_t& held);
    void showFolderMenu(const uint64_t& down, const uint64_t& held);
    void runApp(const uint64_t& down, const uint64_t& held);

    void showMessage(const std::string& mess);
    void showError(const std::string& mess, const Result& r);
}

#endif
