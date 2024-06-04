#include <vector>
#include <SDL2/SDL.h>
#include "config.hpp"
#include "graphics/graphics.hpp"
#include "ui/ui.hpp"
#include "ui/popMessage.hpp"

static const int POPMESSAGE_X_COORDINATE = 24;
static const int POPMESSAGE_Y_COORDINATE = 640;

namespace
{
    // Queue to be processed. Needed to avoid threading graphics issues.
    std::vector<ui::popMessage::pMessage> s_MessageQueue;
    // Actual vector of messages
    std::vector<ui::popMessage::pMessage> s_Messages;
}

void ui::popMessage::newMessage(const std::string &newMessage, const int &tickCount)
{
    // Create message to push to processing queue
    ui::popMessage::pMessage newPopMessage = 
    {
        .message = newMessage,
        .messageTimer = std::make_unique<sys::timer>(tickCount),
        .rectangleWidth = 0, // Will be calculated at update.
        .y = 720
    };

    // Push it to queue
    s_MessageQueue.push_back(std::move(newPopMessage));
}

void ui::popMessage::update(void)
{
    // Loop through queue to make sure they're processed on main thread
    for(ui::popMessage::pMessage &pMessage : s_MessageQueue)
    {

        // Calculate width of message rectangle
        pMessage.rectangleWidth = graphics::systemFont::getTextWidth(pMessage.message, 24) + 32;
        // Move it to actual message vector
        s_Messages.push_back(std::move(pMessage));
    }
    // Clear queue
    s_MessageQueue.clear();

    // Need animation scaling to calculate positions
    float animationScaling = config::getAnimationScaling();

    // Loop through message vector like this because we need current offset
    // targetY is starting target coordinate for messages to be displayed vertically upward
    int targetY = POPMESSAGE_Y_COORDINATE;
    for(unsigned int i = 0; i < s_Messages.size(); i++, targetY -= 52)
    {
        // Get reference to message we're working with
        ui::popMessage::pMessage &currentMessage = s_Messages.at(i);

        // If it has expired, erase, continue
        if(currentMessage.messageTimer->triggered())
        {
            s_Messages.erase(s_Messages.begin() + i);
            continue;
        }

        // Calculate updated Y position
        if(currentMessage.y != targetY)
        {
            currentMessage.y += (targetY - currentMessage.y) / animationScaling;
        }
    }
}

void ui::popMessage::render(void)
{
    // Loop and render messages. Drawn directly to framebuffer
    for(ui::popMessage::pMessage &pMessage : s_Messages)
    {
        ui::renderDialogBox(NULL, POPMESSAGE_X_COORDINATE, pMessage.y, pMessage.rectangleWidth, 44);
        graphics::systemFont::renderText(pMessage.message, NULL, POPMESSAGE_X_COORDINATE + 16, pMessage.y + 10, 24, COLOR_WHITE);
    }
}