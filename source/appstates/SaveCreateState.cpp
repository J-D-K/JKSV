#include "appstates/SaveCreateState.hpp"

#include "StateManager.hpp"
#include "appstates/ConfirmState.hpp"
#include "data/data.hpp"
#include "error.hpp"
#include "fs/fs.hpp"
#include "keyboard/keyboard.hpp"
#include "logging/logger.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"
#include "sys/sys.hpp"
#include "tasks/savecreate.hpp"
#include "ui/PopMessageManager.hpp"

#include <algorithm>
#include <string>
#include <switch.h>
#include <vector>

// This is the sorting function.
static bool compare_info(data::TitleInfo *infoA, data::TitleInfo *infoB);

//                      ---- Construction ----

SaveCreateState::SaveCreateState(data::User *user, TitleSelectCommon *titleSelect)
    : m_user(user)
    , m_titleSelect(titleSelect)
    , m_saveMenu(ui::Menu::create(8, 8, 624, 23, graphics::SCREEN_HEIGHT))
    , m_dataStruct(std::make_shared<SaveCreateState::DataStruct>())
{
    SaveCreateState::initialize_static_members();
    SaveCreateState::initialize_title_info_vector();
    SaveCreateState::initialize_menu();
    SaveCreateState::initialize_data_struct();
}

//                      ---- Public functions ----

void SaveCreateState::update(const sdl2::Input &input)
{
    const bool hasFocus = BaseState::has_focus();

    sm_slidePanel->update(input, hasFocus);

    const bool aPressed    = input.button_pressed(HidNpadButton_A);
    const bool bPressed    = input.button_pressed(HidNpadButton_B);
    const bool panelClosed = sm_slidePanel->is_closed();

    if (m_refreshRequired.load())
    {
        m_user->load_user_data();
        m_titleSelect->refresh();
        m_refreshRequired.store(false);
    }

    if (aPressed) { SaveCreateState::create_save_data_for(); }
    else if (bPressed) { sm_slidePanel->close(); }
    else if (panelClosed) { SaveCreateState::deactivate_state(); }
}

void SaveCreateState::render(sdl2::Renderer &renderer)
{
    const bool hasFocus = BaseState::has_focus();

    // Clear slide target, render menu, render slide to frame buffer.
    sm_slidePanel->render(renderer, hasFocus);
}

void SaveCreateState::refresh_required() { m_refreshRequired.store(true); }

//                      ---- Private functions ----

void SaveCreateState::initialize_static_members()
{
    if (!sm_slidePanel) { sm_slidePanel = std::make_unique<ui::SlideOutPanel>(640, ui::SlideOutPanel::Side::Right); }
}

void SaveCreateState::initialize_title_info_vector()
{
    const FsSaveDataType saveType = m_user->get_account_save_type();
    data::get_title_info_by_type(saveType, m_titleInfoVector);

    std::sort(m_titleInfoVector.begin(), m_titleInfoVector.end(), compare_info);
}

void SaveCreateState::initialize_menu()
{
    for (data::TitleInfo *titleInfo : m_titleInfoVector)
    {
        const std::string_view title = titleInfo->get_title();
        m_saveMenu->add_option(title);
    }
    sm_slidePanel->push_new_element(m_saveMenu);
}

void SaveCreateState::initialize_data_struct()
{
    m_dataStruct->user          = m_user;
    m_dataStruct->spawningState = this;
}

void SaveCreateState::create_save_data_for()
{
    static constexpr std::string_view DEFAULT_INDEX = "0";

    char cacheIndex[3]         = {0};
    const int selected         = m_saveMenu->get_selected();
    data::TitleInfo *titleInfo = m_titleInfoVector[selected];
    m_dataStruct->titleInfo    = titleInfo;

    const char *keyboardString = strings::get_by_name(strings::names::KEYBOARD, 1);
    const bool isCache         = m_user->get_account_save_type() == FsSaveDataType_Cache;
    const bool indexInput      = isCache && keyboard::get_input(SwkbdType_QWERTY, DEFAULT_INDEX, keyboardString, cacheIndex, 3);
    if (isCache && !indexInput) { return; }
    else if (isCache && indexInput) { m_dataStruct->saveDataIndex = std::strtoul(cacheIndex, nullptr, 10); }

    if (!isCache) { TaskState::create_and_push(tasks::savecreate::create_save_data_for, m_dataStruct); }
    else
    {
        const char *confirmString = strings::get_by_name(strings::names::SAVECREATE_CONFS, 0);
        ConfirmTask::create_push_fade(confirmString,
                                      false,
                                      tasks::savecreate::create_save_data_for_sd,
                                      tasks::savecreate::create_save_data_for,
                                      m_dataStruct);
    }
}

void SaveCreateState::deactivate_state()
{
    sm_slidePanel->clear_elements();
    sm_slidePanel->reset();
    BaseState::deactivate();
}

//                      ---- Static functions ----

static bool compare_info(data::TitleInfo *infoA, data::TitleInfo *infoB)
{
    // Get pointers to both titles.
    const char *titleA = infoA->get_title();
    const char *titleB = infoB->get_title();

    const size_t titleALength  = std::char_traits<char>::length(titleA);
    const size_t titleBLength  = std::char_traits<char>::length(titleB);
    const size_t shortestTitle = titleALength < titleBLength ? titleALength : titleBLength;
    // To do: This doesn't take into account which is the shortest title. This can still go out-of-bounds.
    for (size_t i = 0, j = 0; i < shortestTitle;)
    {
        uint32_t codepointA = 0;
        uint32_t codepointB = 0;

        const uint8_t *pointA    = reinterpret_cast<const uint8_t *>(&titleA[i]);
        const uint8_t *pointB    = reinterpret_cast<const uint8_t *>(&titleB[j]);
        const ssize_t unitCountA = decode_utf8(&codepointA, pointA);
        const ssize_t unitCountB = decode_utf8(&codepointB, pointB);
        if (unitCountA <= 0 || unitCountB <= 0) { return false; }

        if (codepointA != codepointB) { return codepointA < codepointB; }

        i += unitCountA;
        j += unitCountB;
    }
    return false;
}
