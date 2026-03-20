#include "ui/PopMessageManager.hpp"

#include "config/config.hpp"
#include "graphics/colors.hpp"
#include "logging/logger.hpp"
#include "sdl.hpp"
#include "ui/PopMessage.hpp"

#include <cstdarg>

//                      ---- Construction ----

ui::PopMessageManager::PopMessageManager() { PopMessageManager::initialize_pop_sound(); }

//                      ---- Public functions ----

void ui::PopMessageManager::update()
{
    static constexpr double COORD_INIT_Y = 594.0f;

    // Grab instance.
    PopMessageManager &manager = PopMessageManager::get_instance();

    auto &messageQueue       = manager.m_messageQueue;
    auto &messages           = manager.m_messages;
    std::mutex &messageMutex = manager.m_messageMutex;
    std::mutex &queueMutex   = manager.m_queueMutex;

    {
        std::lock_guard<std::mutex> queueGuard{queueMutex};
        if (!messageQueue.empty())
        {
            // Loop through the queue and process it so we don't wind up with black characters.
            for (auto &[displayTicks, currentMessage] : messageQueue) { messages.emplace_back(displayTicks, currentMessage); }
            messageQueue.clear();
        }
    }

    // Update all the messages.
    // This is the first Y position a message should be displayed at.;
    double currentY = COORD_INIT_Y;
    std::lock_guard<std::mutex> messageGuard{messageMutex};
    for (auto message = messages.begin(); message != messages.end();)
    {
        if (message->finished())
        {
            message = messages.erase(message);
            continue;
        }
        message->update(currentY);

        currentY -= 56;
        ++message;
    }
}

void ui::PopMessageManager::render(sdl2::Renderer &renderer)
{
    PopMessageManager &manager = PopMessageManager::get_instance();
    auto &messages             = manager.m_messages;
    std::mutex &messageMutex   = manager.m_messageMutex;

    std::lock_guard<std::mutex> messageGuard{messageMutex};
    for (auto &message : messages) { message.render(renderer); }
}

void ui::PopMessageManager::push_message(int displayTicks, std::string_view message)
{
    PopMessageManager &manager = PopMessageManager::get_instance();
    std::mutex &queueMutex     = manager.m_queueMutex;
    std::mutex &messageMutex   = manager.m_messageMutex;
    auto &messageQueue         = manager.m_messageQueue;
    auto &messages             = manager.m_messages;
    auto &popSound             = manager.m_popSound;

    {
        std::lock_guard messageGuard{messageMutex};
        if (!messages.empty())
        {
            ui::PopMessage &back               = messages.back();
            const std::string_view lastMessage = back.get_message();
            if (lastMessage == message) { return; }
        }
    }

    std::lock_guard queueGuard(queueMutex);
    auto queuePair = std::make_pair(displayTicks, std::string{message});
    messageQueue.push_back(std::move(queuePair));
    popSound->play();
}

void ui::PopMessageManager::push_message(int displayTicks, std::string &message)
{
    PopMessageManager &manager = PopMessageManager::get_instance();
    std::mutex &queueMutex     = manager.m_queueMutex;
    std::mutex &messageMutex   = manager.m_messageMutex;
    auto &messageQueue         = manager.m_messageQueue;
    auto &messages             = manager.m_messages;
    auto &popSound             = manager.m_popSound;

    {
        std::lock_guard messageGuard{messageMutex};
        if (!messages.empty())
        {
            ui::PopMessage &back               = messages.back();
            const std::string_view lastMessage = back.get_message();
            if (lastMessage == message) { return; }
        }
    }

    std::lock_guard queueGuard(queueMutex);
    auto queuePair = std::make_pair(displayTicks, std::move(message));
    messageQueue.push_back(std::move(queuePair));
    popSound->play();
}

//                      ---- Private functions ----

void ui::PopMessageManager::initialize_pop_sound()
{
    static constexpr std::string_view POP_PATH = "romfs:/Sound/PopMessage.wav";

    if (m_popSound) { return; }

    m_popSound = sdl2::SoundManager::create_load_resource(POP_PATH, POP_PATH);
}