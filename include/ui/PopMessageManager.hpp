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
            static void render();

            /// @brief Pushes a new message to the queue for processing.
            /// @param displayTicks Number of ticks to display the message.
            /// @param message Text to display.
            /// @param kind Severity used for the accent color.
            /// @param silent Suppresses the pop sound when true.
            static void push_message(int displayTicks,
                                     std::string_view message,
                                     ui::PopKind kind = ui::PopKind::Neutral,
                                     bool silent = false);

            /// @brief Move version of above.
            /// @param displayTicks Number of ticks to display the message.
            /// @param message Text to display.
            /// @param kind Severity used for the accent color.
            /// @param silent Suppresses the pop sound when true.
            static void push_message(int displayTicks,
                                     std::string &message,
                                     ui::PopKind kind = ui::PopKind::Neutral,
                                     bool silent = false);

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

            /// @brief A message waiting to be turned into a PopMessage.
            struct QueuedMessage
            {
                    /// @brief Number of ticks to display the message.
                    int ticks{};

                    /// @brief Text to display.
                    std::string text{};

                    /// @brief Severity used for the accent color.
                    ui::PopKind kind{};

                    /// @brief Suppresses the pop sound when true.
                    bool silent{};
            };

            // The queue for processing. SDL can't handle things being rendered in multiple threads.
            std::vector<QueuedMessage> m_messageQueue{};

            // Actual vector of messages
            std::vector<ui::PopMessage> m_messages{};

            // Mutex to attempt to make this thread safe.
            std::mutex m_messageMutex{};

            std::mutex m_queueMutex{};

            /// @brief The little chirp that's played when messages pop.
            sdl::SharedSound m_popSound{};

            /// @brief Loads the pop message sound into memory.
            void initialize_pop_sound();
    };
} // namespace ui
