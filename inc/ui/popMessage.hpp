#pragma once
#include <string>
#include <memory>

#include "system/timer.hpp"

namespace ui
{
    namespace popMessage
    {
        // This is the default and only used timer count for these messages.
        static const int POPMESSAGE_DEFAULT_TICKS = 2500;

        // Actual message struct
        typedef struct
        {
            // Message string
            std::string message;
            // Timer for expiration
            std::unique_ptr<sys::timer> messageTimer;
            // Width of rectangle to render
            int rectangleWidth = 0;
            // Current Y
            int y = 0;
        } pMessage;

        // Adds a message to the queue to be processed. tickLength is how long in ticks for it to be displayed.
        void newMessage(const std::string &newMessage, int tickLength);
        // Updates the message queue and vector
        void update(void);
        // Renders messages to screen
        void render(void);
    }
}