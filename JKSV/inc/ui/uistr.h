#pragma once

#include <map>

//Strings since translation support
namespace ui
{
    void initStrings();
    void loadTrans();
    void saveTranslationFiles(void *a);
    extern std::map<std::pair<std::string, int>, std::string> strings;

    inline std::string getUIString(const std::string& _name, int ind){ return strings[std::make_pair(_name, ind)]; }
    inline const char *getUICString(const std::string& _name, int ind){ return strings[std::make_pair(_name, ind)].c_str(); }
}
