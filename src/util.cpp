#include <string>
#include <fstream>
#include <cstdio>
#include <ctime>
#include <sys/stat.h>

#include "data.h"
#include "gfx.h"
#include "util.h"
#include "file.h"
#include "ui.h"

static const char verboten[] = { ',', '/', '\\', '<', '>', ':', '"', '|', '?', '*'};

//Missing swkbd config funcs for now
typedef enum
{
    SwkbdPosStart = 0,
    SwkbdPosEnd   = 1
} SwkbdInitPos;

typedef struct
{
    uint16_t read[0x32 / sizeof(uint16_t)];
    uint16_t word[0x32 / sizeof(uint16_t)];
} dictWord;

void swkbdDictWordCreate(dictWord *w, const char *read, const char *word)
{
    memset(w->read, 0, 0x32);
    memset(w->word, 0, 0x32);

    uint16_t tmp[0x32 / sizeof(uint16_t)];
    memset(tmp, 0, 0x32);

    utf8_to_utf16(tmp, (uint8_t *)read, 0x30);
    memcpy(w->read, tmp, 0x30);

    utf8_to_utf16(tmp, (uint8_t *)word, 0x30);
    memcpy(w->word, tmp, 0x30);
}

uint32_t replaceIfAccent(uint32_t c)
{
    switch(c)
    {
        case 'é':
            return 'e';
            break;
    }

    return c;
}

namespace util
{
    std::string getDateTime(int fmt)
    {
        char ret[64];

        time_t raw;
        time(&raw);
        tm *Time = localtime(&raw);

        switch(fmt)
        {
            case DATE_FMT_YMD:
                sprintf(ret, "%04d.%02d.%02d @ %02d.%02d.%02d", Time->tm_year + 1900, Time->tm_mon + 1, Time->tm_mday, Time->tm_hour, Time->tm_min, Time->tm_sec);
                break;

            case DATE_FMT_YDM:
                sprintf(ret, "%04d.%02d.%02d @ %02d.%02d.%02d", Time->tm_year + 1900, Time->tm_mday, Time->tm_mon + 1, Time->tm_hour, Time->tm_min, Time->tm_sec);
                break;

            case DATE_FMT_HOYSTE:
                sprintf(ret, "%02d.%02d.%04d", Time->tm_mday, Time->tm_mon + 1, Time->tm_year + 1900);
                break;
        }

        return std::string(ret);
    }

    void makeTitleDir(data::user& u, data::titledata& t)
    {
        std::string path = fs::getWorkDir() + t.getTitleSafe();
        mkdir(path.c_str(), 777);
    }

    std::string getTitleDir(data::user& u, data::titledata& t)
    {
        return std::string(fs::getWorkDir() + t.getTitleSafe() + "/");
    }

    void copyDirListToMenu(fs::dirList& d, ui::menu& m)
    {
        m.reset();
        m.addOpt(".");
        m.addOpt("..");
        for(unsigned i = 0; i < d.getCount(); i++)
        {
            if(d.isDir(i))
                m.addOpt("D " + d.getItem(i));
            else
                m.addOpt("F " + d.getItem(i));
        }

        m.adjust();
    }

    void removeLastFolderFromString(std::string& _path)
    {
        unsigned last = _path.find_last_of('/', _path.length() - 2);
        _path.erase(last + 1, _path.length());
    }

    bool isVerboten(uint32_t t)
    {
        for(unsigned i = 0; i < 10; i++)
        {
            if(t == verboten[i])
                return true;
        }

        return false;
    }

    std::string safeString(const std::string& s)
    {
        std::string ret = "";
        for(unsigned i = 0; i < s.length(); )
        {
            uint32_t tmpChr = 0;
            ssize_t untCnt = decode_utf8(&tmpChr, (uint8_t *)&s.data()[i]);

            i += untCnt;

            tmpChr = replaceIfAccent(tmpChr);

            if(isVerboten(tmpChr))
            {
                ret += ' ';
            }
            else if(tmpChr < 31 || tmpChr > 126)
                return ""; //return empty string so titledata::init defaults to titleID
            else
                ret += (char)tmpChr;
        }

        //Check for spaces at end
        if(ret[ret.length() - 1] == ' ')
            ret.erase(ret.end() - 1, ret.end());

        return ret;
    }

    std::string getInfoString(data::user& u, data::titledata& d)
    {
        std::string ret = d.getTitle() + "\n";

        char id[18];
        sprintf(id, " %016lX", d.getID());
        ret += std::string(id) + "\n\n";

        switch(d.getType())
        {
            case FsSaveDataType_System:
                ret += "System Save\n\n";
                break;

            case FsSaveDataType_Account:
                ret += "Save Data\n\n";
                break;

            case FsSaveDataType_Bcat:
                ret += "BCAT\n\n";
                break;

            case FsSaveDataType_Device:
                ret += "Device Save\n\n";
                break;

            case FsSaveDataType_Temporary:
                ret += "Temp Storage\n\n";
                break;

            case FsSaveDataType_Cache:
                ret += "Cache Storage\n\n";
                break;

            case FsSaveDataType_SystemBcat:
                ret += "System BCAT";
                break;
        }

        ret += u.getUsername();

        return ret;
    }

    std::string getStringInput(const std::string& def, const std::string& head, size_t maxLength, unsigned dictCnt, const std::string dictWords[])
    {
        SwkbdConfig swkbd;
        swkbdCreate(&swkbd, dictCnt);
        swkbdConfigSetBlurBackground(&swkbd, true);
        swkbdConfigSetInitialText(&swkbd, def.c_str());
        swkbdConfigSetHeaderText(&swkbd, head.c_str());
        swkbdConfigSetGuideText(&swkbd, head.c_str());
        swkbdConfigSetInitialCursorPos(&swkbd, SwkbdPosEnd);
        swkbdConfigSetType(&swkbd, SwkbdType_QWERTY);
        swkbdConfigSetStringLenMax(&swkbd, maxLength);
        swkbdConfigSetKeySetDisableBitmask(&swkbd, SwkbdKeyDisableBitmask_Backslash | SwkbdKeyDisableBitmask_ForwardSlash | SwkbdKeyDisableBitmask_Percent);
        swkbdConfigSetDicFlag(&swkbd, 1);

        if(dictCnt > 0)
        {
            dictWord words[dictCnt];
            for(unsigned i = 0; i < dictCnt; i++)
                swkbdDictWordCreate(&words[i], dictWords[i].c_str(), dictWords[i].c_str());

            swkbdConfigSetDictionary(&swkbd, (SwkbdDictWord *)words, dictCnt);
        }

        char out[maxLength];
        memset(out, 0, maxLength);
        swkbdShow(&swkbd, out, maxLength);
        swkbdClose(&swkbd);

        return std::string(out);
    }

    std::string generateAbbrev(data::titledata& dat)
    {
        size_t titleLength = dat.getTitle().length();

        char temp[titleLength + 1];
        memset(temp, 0, titleLength + 1);
        memcpy(temp, dat.getTitle().c_str(), titleLength);

        std::string ret;
        char *tok = strtok(temp, " ");
        while(tok)
        {
            ret += tok[0];
            tok = strtok(NULL, " ");
        }

        return ret;
    }

    tex *loadDefaultIcon()
    {
        tex *ret;

        std::fstream iconIn("romfs:/img/icn/icnDefault.png", std::ios::in | std::ios::binary);
        iconIn.seekg(0, iconIn.end);
        size_t fileSize = iconIn.tellg();
        iconIn.seekg(0xB50, iconIn.beg);

        size_t iconSize = fileSize - 0xB50;
        uint8_t *tmpBuff = new uint8_t[iconSize];
        iconIn.read((char *)tmpBuff, iconSize);
        iconIn.close();

        ret = texLoadJPEGMem(tmpBuff, iconSize);
        delete[] tmpBuff;

        return ret;
    }

    tex *createIconGeneric(const char *txt)
    {
        tex *ret = texCreate(256, 256);
        texClearColor(ret, ui::rectLt);
        unsigned int x = 128 - (textGetWidth(txt, ui::shared, 32) / 2);
        drawText(txt, ret, ui::shared, x, 112, 32, ui::mnuTxt);
        return ret;
    }
}

