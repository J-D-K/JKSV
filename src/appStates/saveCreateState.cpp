#include <algorithm>
#include <string>

#include "appStates/saveCreateState.hpp"
#include "system/input.hpp"
#include "system/task.hpp"
#include "stringUtil.hpp"

namespace
{
    // This is to make typing some stuff easier.
    using titleIDPair = std::pair<uint64_t, std::string>;

    // Panel name
    const std::string PANEL_NAME = "saveCreationPanel";

    // Strings used
    const std::string THREAD_STATUS_CREATING_SAVE_DATA = "threadStatusCreatingSaveData";
    const std::string SAVE_DATA_CREATED_FOR_USER = "saveDataCreatedForUser";
    const std::string SAVE_DATA_CREATION_FAILED = "saveDataCreationFailed";
    const std::string SWKBD_ENTER_SAVE_INDEX = "swkbdSaveIndex";
}

static bool sortTitleList(const titleIDPair &titlePairA, const titleIDPair &titlePairB)
{
    // This is bad. I'm assuming there will be a difference before end is hit...
    int titleLength = titlePairA.second.length();
    // Codepoints from title.
    uint32_t codepointA;
    uint32_t codepointB;

    for(int i = 0, j = 0; i < titleLength; )
    {
        ssize_t titleAUnitCount = decode_utf8(&codepointA, reinterpret_cast<const uint8_t *>(&titlePairA.second.c_str()[i]));
        ssize_t titleBUnitCount = decode_utf8(&codepointB, reinterpret_cast<const uint8_t *>(&titlePairB.second.c_str()[j]));

        if(codepointA != codepointB)
        {
            return codepointA < codepointB;
        }

        i += titleAUnitCount;
        j += titleBUnitCount;
    }
    return false;
}

// This is the data needed for the task
struct saveCreationData : sys::taskData
{
    uint64_t titleID;
    FsSaveDataType saveDataType;
    data::user *currentUser;
};

// This is the actual task
void createSaveData(sys::task *task, sys::sharedTaskData sharedData)
{
    if(sharedData == nullptr)
    {
        task->finished();
        return;
    }

    // Cast to correct type
    std::shared_ptr<saveCreationData> dataIn = static_pointer_cast<saveCreationData>(sharedData);

    // Get pointer to titleInfo
    data::titleInfo *workingTitleInfo = data::getTitleInfoByTitleID(dataIn->titleID);

    // Set task status
    std::string taskStatus = stringUtil::getFormattedString(ui::strings::getCString(THREAD_STATUS_CREATING_SAVE_DATA, 0), workingTitleInfo->getTitle().c_str());
    task->setThreadStatus(taskStatus);

    // Variables needed to create some types of saves.
    uint16_t cacheSaveIndex = 0;

    if(dataIn->saveDataType == FsSaveDataType_Cache)
    {
        std::string promptText = ui::strings::getString(SWKBD_ENTER_SAVE_INDEX, 0);
        std::string indexString = sys::input::getString(SwkbdType_NumPad, "0", promptText, 2);
        cacheSaveIndex = std::stoul(indexString, NULL, 10);
    }

    // Attributes needed to create save data
    FsSaveDataAttribute saveAttributes = 
    {
        .application_id = dataIn->titleID,
        .uid = dataIn->currentUser->getAccountID(),
        .system_save_data_id = 0,
        .save_data_type = dataIn->saveDataType,
        .save_data_rank = 0,
        .save_data_index = cacheSaveIndex
    };

    // Save and journal Size
    int64_t saveDataSize = 0;
    int64_t journalSize = 0;

    switch(dataIn->saveDataType)
    {
        case FsSaveDataType_Account:
        {
            saveDataSize = workingTitleInfo->getSaveDataSize(dataIn->saveDataType);
        }
        break;

        default:
        break;
    }

    task->finished();
}

saveCreateState::saveCreateState(data::user *currentUser, FsSaveDataType saveDataType) :
m_CurrentUser(currentUser),
m_SaveCreatePanel(std::make_unique<ui::slidePanel>(PANEL_NAME, 512, ui::PANEL_SIDE_RIGHT)),
m_SaveCreateMenu(std::make_unique<ui::menu>(8, 32, 492, 20, 6)),
m_SaveDataType(saveDataType)
{
    // Get title map
    data::titleMap titleMap = data::getTitleMap();

    // Loop through and make vector
    for(auto &titleInfo : titleMap)
    {
        switch(saveDataType)
        {
            case FsSaveDataType_Account:
            {
                if(titleInfo.second.hasAccountSaveData())
                {
                    m_TitleList.push_back(std::make_pair(titleInfo.first, titleInfo.second.getTitle()));
                }
            }
            break;
        
            case FsSaveDataType_Bcat:
            {
                if(titleInfo.second.hasBCATSaveData())
                {
                    m_TitleList.push_back(std::make_pair(titleInfo.first, titleInfo.second.getTitle()));
                }
            }
            break;

            case FsSaveDataType_Device:
            {
                if(titleInfo.second.hasDeviceSaveData())
                {
                    m_TitleList.push_back(std::make_pair(titleInfo.first, titleInfo.second.getTitle()));
                }
            }
            break;

            case FsSaveDataType_Cache:
            {
                if(titleInfo.second.hasCacheSaveData())
                {
                    m_TitleList.push_back(std::make_pair(titleInfo.first, titleInfo.second.getTitle()));
                }
            }
            break;

            default:
            break;
        }
    }

    // Sort vector
    std::sort(m_TitleList.begin(), m_TitleList.end(), sortTitleList);

    // Copy to menu... 
    for(auto &pair : m_TitleList)
    {
        m_SaveCreateMenu->addOption(pair.second);
    }
}

saveCreateState::~saveCreateState() { }

void saveCreateState::update(void)
{
    m_SaveCreatePanel->update();
    m_SaveCreateMenu->update();

    if(sys::input::buttonDown(HidNpadButton_B))
    {
        m_SaveCreatePanel->closePanel();
    }
    else if(m_SaveCreatePanel->isClosed())
    {
        appState::deactivateState();
    }
}

void saveCreateState::render(void)
{
    // Get render target
    graphics::sdlTexture renderTarget = m_SaveCreatePanel->getPanelRenderTarget();
    // Clear
    graphics::textureClear(renderTarget.get(), COLOR_SLIDE_PANEL_TARGET);
    // Render menu to it
    m_SaveCreateMenu->render(renderTarget.get());
    // Render
    m_SaveCreatePanel->render();
}