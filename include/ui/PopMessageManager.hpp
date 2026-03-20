#pragma once
#include "sdl.hpp"
#include "sys/sys.hpp"
#include "ui/DialogBox.hpp"
#include "ui/PopMessage.hpp"

#include <mutex>
#include <string>
#include <vector>

namespace ui
{
    class PopMessageManager final
    {
        public:
            // No copying.
            PopMessageManager(const PopMessageManager &)            = delete;
            PopMessageManager(PopMessageManager &&)                 = delete;
            PopMessageManager &operator=(const PopMessageManager &) = delete;
            PopMessageManager &operator=(PopMessageManager &&)      = delete;

            /// @brief Updates and processes message queue.
            static void update();

            /// @brief Renders messages to screen.
            static void render(sdl2::Renderer &renderer);

            /// @brief Pushes a new message to the queue for processing.
            static void push_message(int displayTicks, std::string_view message);

            /// @brief Move version of above.
            static void push_message(int displayTicks, std::string &message);

            /// @brief The default duration of ticks for messages to be shown.
            static constexpr int DEFAULT_TICKS = 2500;

        private:
            // Only one instance allowed.
            PopMessageManager();

            // Returns the only instance.
            static PopMessageManager &get_instance()
            {
                static PopMessageManager manager;
                return manager;
            }

            // The queue for processing. SDL can't handle things being rendered in multiple threads.
            std::vector<std::pair<int, std::string>> m_messageQueue{};

            // Actual vector of messages
            std::vector<ui::PopMessage> m_messages{};

            // Mutex to attempt to make this thread safe.
            std::mutex m_messageMutex{};

            std::mutex m_queueMutex{};

            /// @brief The little chirp that's played when messages pop.
            sdl2::SharedSound m_popSound{};

            /// @brief Loads the pop message sound into memory.
            void initialize_pop_sound();
    };
} // namespace ui
