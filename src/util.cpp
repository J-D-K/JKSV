#include <string>
#include <cstdio>
#include <ctime>
#include <sys/stat.h>
#include <json-c/json.h>

#include "data.h"
#include "gfx.h"
#include "util.h"
#include "file.h"
#include "ui.h"
#include "curlfuncs.h"

static const char verboten[] = { ',', '/', '\\', '<', '>', ':', '"', '|', '?', '*', '�', '�', '�'};

static bool isVerboten(const uint32_t& t)
{
    for(unsigned i = 0; i < 13; i++)
    {
        if(t == verboten[i])
            return true;
    }

    return false;
}

static inline bool isASCII(const uint32_t& t)
{
    return t > 30 && t < 127;
}

void util::replaceStr(std::string& _str, const std::string& _find, const std::string& _rep)
{
    size_t pos = 0;
    while((pos = _str.find(_find)) != _str.npos)
        _str.replace(pos, _find.length(), _rep);
}

//Used to split version tag git
static void getVersionFromTag(const std::string& tag, unsigned& _year, unsigned& _month, unsigned& _day)
{
    _month = strtoul(tag.substr(0, 2).c_str(), NULL, 10);
    _day = strtoul(tag.substr(3, 5).c_str(), NULL, 10);
    _year = strtoul(tag.substr(6, 10).c_str(), NULL, 10);
}

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
    memset(w, 0, sizeof(*w));

    utf8_to_utf16(w->read, (uint8_t *)read, (sizeof(w->read) / sizeof(uint16_t)) - 1);
    utf8_to_utf16(w->word, (uint8_t *)word, (sizeof(w->word) / sizeof(uint16_t)) - 1);
}

uint32_t replaceChar(uint32_t c)
{
    switch(c)
    {
        case '�':
            return 'e';
            break;
    }

    return c;
}

static inline void replaceCharCStr(char *_s, char _find, char _rep)
{
    size_t strLength = strlen(_s);
    for(unsigned i = 0; i < strLength; i++)
    {
        if(_s[i] == _find)
            _s[i] = _rep;
    }
}

std::string util::getDateTime(int fmt)
{
    char ret[128];

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

        case DATE_FMT_JHK:
            sprintf(ret, "%04d%02d%02d_%02d%02d", Time->tm_year + 1900, Time->tm_mon + 1, Time->tm_mday, Time->tm_hour, Time->tm_min);
            break;

        case DATE_FMT_ASC:
            strcpy(ret, asctime(Time));
            replaceCharCStr(ret, ':', '_');
            replaceCharCStr(ret, '\n', 0x00);
            break;
    }

    return std::string(ret);
}

void util::copyDirListToMenu(const fs::dirList& d, ui::menu& m)
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

void util::removeLastFolderFromString(std::string& _path)
{
    unsigned last = _path.find_last_of('/', _path.length() - 2);
    _path.erase(last + 1, _path.length());
}

std::string util::safeString(const std::string& s)
{
    std::string ret = "";
    for(unsigned i = 0; i < s.length(); )
    {
        uint32_t tmpChr = 0;
        ssize_t untCnt = decode_utf8(&tmpChr, (uint8_t *)&s.data()[i]);

        i += untCnt;

        tmpChr = replaceChar(tmpChr);

        if(isVerboten(tmpChr))
            ret += ' ';
        else if(!isASCII(tmpChr))
            return ""; //return empty string so titledata::init defaults to titleID
        else
            ret += (char)tmpChr;
    }

    //Check for spaces at end
    while(ret[ret.length() - 1] == ' ' || ret[ret.length() - 1] == '.')
        ret.erase(ret.length() - 1, 1);

    return ret;
}

static inline std::string getTimeString(const uint32_t& _h, const uint32_t& _m)
{
    char tmp[32];
    sprintf(tmp, "%02d:%02d", _h, _m);
    return std::string(tmp);
}

std::string util::getInfoString(const data::user& u, const data::titledata& d)
{
    std::string ret = d.getTitle() + "\n";

    ret += "TID: " + d.getTIDStr() + "\n";
    ret += "SID: " + d.getSaveIDStr() + "\n";

    uint32_t hours, mins;
    hours = d.getPlayTime() / 60;
    mins = d.getPlayTime() - (hours * 60);

    ret += "Play Time: " + getTimeString(hours, mins) + "\n";
    ret += "Total Launches: " + std::to_string(d.getLaunchCount()) + "\n";

    switch(d.getType())
    {
        case FsSaveDataType_System:
            ret += "System Save\n";
            break;

        case FsSaveDataType_Account:
            ret += "Save Data\n";
            break;

        case FsSaveDataType_Bcat:
            ret += "BCAT\n";
            break;

        case FsSaveDataType_Device:
            ret += "Device Save\n";
            break;

        case FsSaveDataType_Temporary:
            ret += "Temp Storage\n";
            break;

        case FsSaveDataType_Cache:
            ret += "Cache Storage\n";
            break;

        case FsSaveDataType_SystemBcat:
            ret += "System BCAT\n";
            break;
    }

    ret += u.getUsername();

    return ret;
}

std::string util::getStringInput(const std::string& def, const std::string& head, size_t maxLength, unsigned dictCnt, const std::string dictWords[])
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

std::string util::generateAbbrev(const data::titledata& dat)
{
    size_t titleLength = dat.getTitle().length();

    char temp[titleLength + 1];
    memset(temp, 0, titleLength + 1);
    memcpy(temp, dat.getTitle().c_str(), titleLength);

    std::string ret;
    char *tok = strtok(temp, " ");
    while(tok)
    {
        if(isASCII(tok[0]))
            ret += tok[0];
        tok = strtok(NULL, " ");
    }
    return ret;
}

void util::stripChar(char _c, std::string& _s)
{
    size_t pos = 0;
    while((pos = _s.find(_c)) != _s.npos)
        _s.erase(pos, 1);
}

void util::replaceButtonsInString(std::string& rep)
{
    replaceStr(rep, "[A]", "\ue0e0");
    replaceStr(rep, "[B]", "\ue0e1");
    replaceStr(rep, "[X]", "\ue0e2");
    replaceStr(rep, "[Y]", "\ue0e3");
    replaceStr(rep, "[L]", "\ue0e4");
    replaceStr(rep, "[R]", "\ue0e5");
    replaceStr(rep, "[ZL]", "\ue0e6");
    replaceStr(rep, "[ZR]", "\ue0e7");
    replaceStr(rep, "[SL]", "\ue0e8");
    replaceStr(rep, "[SR]", "\ue0e9");
    replaceStr(rep, "[DPAD]", "\ue0ea");
    replaceStr(rep, "[DUP]", "\ue0eb");
    replaceStr(rep, "[DDOWN]", "\ue0ec");
    replaceStr(rep, "[DLEFT]", "\ue0ed");
    replaceStr(rep, "[DRIGHT]", "\ue0ee");
    replaceStr(rep, "[+]", "\ue0ef");
    replaceStr(rep, "[-]", "\ue0f0");
}

SDL_Texture *util::createIconGeneric(const char *txt, int fontSize)
{
    SDL_Texture *ret = SDL_CreateTexture(gfx::render, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET, 256, 256);
    SDL_SetRenderTarget(gfx::render, ret);
    SDL_SetRenderDrawColor(gfx::render, ui::rectLt.r, ui::rectLt.g, ui::rectLt.b, ui::rectLt.a);
    SDL_RenderClear(gfx::render);
    unsigned int x = 128 - (gfx::getTextWidth(txt, fontSize) / 2);
    unsigned int y = 128 - (fontSize / 2);
    gfx::drawTextf(fontSize, x, y, &ui::txtCont, txt);
    SDL_SetRenderTarget(gfx::render, NULL);
    return ret;
}

void util::setCPU(uint32_t hz)
{
    if(R_FAILED(clkrstInitialize()))
        return;

    ClkrstSession cpu;
    clkrstOpenSession(&cpu, PcvModuleId_CpuBus, 3);
    clkrstSetClockRate(&cpu, hz);
    clkrstCloseSession(&cpu);
    clkrstExit();
}

void util::checkForUpdate()
{
    std::string gitJson = getJSONURL(NULL, "https://api.github.com/repos/J-D-K/JKSV/releases/latest");
    if(gitJson.empty())
    {
        ui::showPopup(POP_FRAME_DEFAULT, ui::errorConnecting.c_str());
        return;
    }

    std::string tagStr;
    unsigned month, day, year;
    json_object *jobj = json_tokener_parse(gitJson.c_str()), *tag;
    json_object_object_get_ex(jobj, "tag_name", &tag);
    tagStr = json_object_get_string(tag);
    getVersionFromTag(tagStr, year, month, day);
    //This can throw false positives as is. need to fix sometime
    if(year > BLD_YEAR || month > BLD_MON || month > BLD_DAY)
    {
        //dunno about NSP yet...
        json_object *assets, *asset0, *dlUrl;
        json_object_object_get_ex(jobj, "assets", &assets);
        asset0 = json_object_array_get_idx(assets, 0);
        json_object_object_get_ex(asset0, "browser_download_url", &dlUrl);

        std::vector<uint8_t> jksvBuff;
        std::string url = json_object_get_string(dlUrl);
        getBinURL(&jksvBuff, url);
        FILE *jksvOut = fopen("sdmc:/switch/JKSV.nro", "wb");
        fwrite(jksvBuff.data(), 1, jksvBuff.size(), jksvOut);
        fclose(jksvOut);
    }
    else
        ui::showPopup(POP_FRAME_DEFAULT, ui::noUpdate.c_str());

    json_object_put(jobj);
}
