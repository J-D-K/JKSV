#ifndef UISTRMAP_H
#define UISTRMAP_H

#include <unordered_map>
//This is only included in UI so I don't have a nasty if else if else if else if else if
//Might be a shitty way to pull this off, but meh
static std::unordered_map<std::string, unsigned> uistrdef;

inline void transLoadInit()
{
    uistrdef["author"] = 0;
    uistrdef["userHelp"] = 1;
    uistrdef["titleHelp"] = 2;
    uistrdef["folderHelp"] = 3;
    uistrdef["optHelp"] = 4;
    uistrdef["yt"] = 5;
    uistrdef["nt"] = 6;
    uistrdef["on"] = 7;
    uistrdef["off"] = 8;
    uistrdef["confirmBlacklist"] = 9;
    uistrdef["confirmOverwrite"] = 10;
    uistrdef["confirmRestore"] = 11;
    uistrdef["confirmDelete"] = 12;
    uistrdef["confirmCopy"] = 13;
    uistrdef["confirmEraseNand"] = 14;
    uistrdef["confirmEraseFolder"] = 15;
    uistrdef["confirmHead"] = 16;
    uistrdef["copyHead"] = 17;
    uistrdef["noSavesFound"] = 18;
    uistrdef["advMenu"] = 19;
    uistrdef["extMenu"] = 20;
    uistrdef["optMenu"] = 21;
    uistrdef["optMenuExp"] = 22;
    uistrdef["holdingText"] = 23;
}

inline void transLoadExit()
{
    uistrdef.clear();
}

#endif
