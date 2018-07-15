#include <string>
#include <vector>
#include <switch.h>

#include "ui.h"
#include "data.h"
#include "file.h"
#include "util.h"

static ui::menu userMenu, titleMenu;

namespace ui
{
    void clsUserPrep()
    {
        userMenu.reset();

        userMenu.setParams(16, 88, 424);

        for(unsigned i = 0; i < data::users.size(); i++)
            userMenu.addOpt(data::users[i].getUsername());
    }

    void clsTitlePrep(data::user& u)
    {
        titleMenu.reset();
        titleMenu.setParams(16, 88, 424);

        for(unsigned i = 0; i < u.titles.size(); i++)
            titleMenu.addOpt(u.titles[i].getTitle());
    }

    void classicUserMenuUpdate(const uint64_t& down, const uint64_t& held, const touchPosition& p)
    {
        userMenu.handleInput(down, held, p);
        userMenu.draw(mnuTxt);

        if(down & KEY_A)
        {
            data::curUser = data::users[userMenu.getSelected()];
            clsTitlePrep(data::curUser);

            mstate = 5;
        }
        else if(down & KEY_Y)
        {
            for(unsigned i = 0; i < data::users.size(); i++)
                fs::dumpAllUserSaves(data::users[i]);
        }
        else if(down & KEY_X)
        {
            std::remove(std::string(fs::getWorkDir() + "cls.txt").c_str());
            clsMode = false;
            mstate = USR_SEL;
        }
    }

    void classicTitleMenuUpdate(const uint64_t& down, const uint64_t& held, const touchPosition& p)
    {
        titleMenu.handleInput(down, held, p);
        titleMenu.draw(mnuTxt);

        if(down & KEY_A)
        {
            data::curData = data::curUser.titles[titleMenu.getSelected()];

            if(fs::mountSave(data::curUser, data::curData))
            {
                util::makeTitleDir(data::curUser, data::curData);
                folderMenuPrepare(data::curUser, data::curData);
                folderMenuInfo = util::getWrappedString(util::getInfoString(data::curUser, data::curData), 18, 256);

                mstate = FLD_SEL;
            }
        }
        else if(down & KEY_Y)
        {
            fs::dumpAllUserSaves(data::curUser);
        }
        else if(down & KEY_B)
            mstate = CLS_USR;
    }
}
