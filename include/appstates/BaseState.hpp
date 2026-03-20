#pragma once

#include "sdl.hpp"

#include <memory>

class BaseState
{
    public:
        /// @brief Base application state class.
        /// @param isClosable Optional. Controls whether or not the state should allow JKSV to close.
        BaseState(bool isClosable = true);

        /// @brief Ends homebutton and plus locking.
        virtual ~BaseState();

        /// @brief Every derived class is required to have this function.
        virtual void update(const sdl2::Input &input) = 0;

        /// @brief Sub update routine. Meant to handle minor background tasks. Not meant for full update routines.
        virtual void sub_update() {};

        /// @brief Every derived class is required to have this function.
        virtual void render(sdl2::Renderer &renderer) = 0;

        /// @brief Deactivates state and allows JKSV to purge it from the vector.
        void deactivate();

        /// @brief Allows a state to be reactivated and pushed to the vector.
        void reactivate();

        /// @brief Returns if the state is still active.
        /// @return Whether state is still active or can be purged.
        bool is_active() const;

        /// @brief Tells the state it's at the back of the vector and has focus.
        void give_focus();

        /// @brief Takes the focus away and tells the state it's no long back();
        void take_focus();

        /// @brief Allows the state to know whether it has focus.
        /// @return Whether state has focus or not.
        bool has_focus() const;

        /// @brief Returns whether or not JKSV should allow closing while state is active.
        /// @return True if closable. False if not.
        bool is_closable() const;

    private:
        /// @brief Stores whether or not the state is currently active.
        bool m_isActive{true};

        /// @brief Stores whether or not the state has focus.
        bool m_hasFocus{};

        /// @brief Stores whether or not the state allows closing.
        bool m_isClosable{};
};
