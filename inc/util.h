#ifndef UTIL_H
#define UTIL_H

#include "data.h"
#include "ui.h"
#include "file.h"

namespace util
{
    enum
    {
        DATE_FMT_YMD,
        DATE_FMT_YDM,
        DATE_FMT_HOYSTE
    };

    //Returns string with date S+ time
    std::string getDateTime(int fmt);

    //Creates Dir 'JKSV/[title]
    void makeTitleDir(data::user& u, data::titledata& t);

    //Returns 'JKSV/[title]/'
    std::string getTitleDir(data::user& u, data::titledata& t);

    //Just returns string with '\n' inserted.
    std::string getWrappedString(const std::string& s, const unsigned& sz, const unsigned& maxWidth);

    //Copys dir list to a menu with 'D: ' + 'F: '
    void copyDirListToMenu(fs::dirList& d, ui::menu& m);

    //Removes last folder from '_path'
    void removeLastFolderFromString(std::string& _path);

    std::string safeString(const std::string& s);

    std::string getInfoString(data::user& u, data::titledata& d);

    std::string getStringInput(const std::string& def, const std::string& head, size_t maxLength, unsigned dictCnt, const std::string dictWords[]);
}
#endif // UTIL_H
