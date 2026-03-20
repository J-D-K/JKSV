#pragma once
#include "appstates/BaseState.hpp"
#include "ui/ControlGuide.hpp"

/// @brief Class that both view types are derived from.
class TitleSelectCommon : public BaseState
{
    public:
        /// @brief Constructs a new TitleSelectCommon. Basically just calculates the X coordinate of the control if it wasn't
        /// already.
        TitleSelectCommon();

        /// @brief Required destructor.
        virtual ~TitleSelectCommon() {};

        /// @brief Required, inherited.
        virtual void update(const sdl2::Input &input) = 0;

        /// @brief Sub-update routine. Normally in a file, but I didn't feel like it really needed one.
        void sub_update() override;

        /// @brief Required, inherited.
        virtual void render(sdl2::Renderer &renderer) = 0;

        /// @brief Both derived classes need this function.
        virtual void refresh() = 0;

    protected:
        /// @brief This is the control guide shared by all title selects.
        static inline std::shared_ptr<ui::ControlGuide> sm_controlGuide{};

    private:
        /// @brief Initializes the control guide if it hasn't been already.
        void initialize_control_guide();
};
