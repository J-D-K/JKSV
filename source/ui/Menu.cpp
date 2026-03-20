#include "ui/Menu.hpp"

#include "config/config.hpp"
#include "graphics/ScopedRender.hpp"
#include "graphics/colors.hpp"
#include "mathutil.hpp"
#include "ui/BoundingBox.hpp"

#include <cmath>

//                      ---- Construction ----

ui::Menu::Menu(int x, int y, int width, int fontSize, int renderTargetHeight)
    : m_x(x)
    , m_y(y)
    , m_originalY(y)
    , m_width(width)
    , m_fontSize(fontSize)
    , m_renderTargetHeight(renderTargetHeight)
{
    Menu::calculate_alignments();
    Menu::initialize_transition();
    Menu::initialize_option_target();
    Menu::initialize_ui_elements();
    Menu::initialize_sounds();
}

//                      ---- Public functions ----

void ui::Menu::update(const sdl2::Input &input, bool hasFocus)
{
    if (m_options.empty()) { return; }

    m_boundingBox->update(input, hasFocus);
    m_optionScroll->update(input, hasFocus);

    Menu::handle_input(input);
    Menu::update_scrolling();
    Menu::update_scroll_text();
    m_transition.update();
}

void ui::Menu::render(sdl2::Renderer &renderer, bool hasFocus)
{
    if (m_options.empty()) { return; }

    // I hate doing this.
    const int optionSize = m_options.size();
    const int y          = m_transition.get_y();
    for (int i = 0, tempY = y; i < optionSize; i++, tempY += m_optionHeight)
    {
        if (tempY < -m_fontSize) { continue; }
        else if (tempY > m_renderTargetHeight) { break; }

        {
            graphics::ScopedRender optionScoped{renderer, m_optionTarget};
            renderer.frame_begin(colors::TRANSPARENT);

            if (i == m_selected)
            {
                if (hasFocus)
                {
                    m_boundingBox->set_x(m_x - 4);
                    m_boundingBox->set_y(tempY - 4);
                    m_boundingBox->render(renderer, hasFocus);
                }
                renderer.render_rectangle(8, 8, 4, m_optionHeight - 12, colors::BLUE_GREEN);
                m_optionScroll->render(renderer, hasFocus);
            }
            else
            {
                m_font->render_text(24, m_textY, colors::WHITE, m_options[i]);
            }
        }

        // render target to target
        m_optionTarget->render(m_x, tempY);
    }
}

void ui::Menu::add_option(std::string_view newOption)
{
    // This is needed. The initialization is just a temporary one.
    if (m_options.empty()) { m_optionScroll->set_text(newOption, false); }
    m_options.push_back(newOption.data());
}

void ui::Menu::add_option(std::string &newOption)
{
    if (m_options.empty()) { m_optionScroll->set_text(std::string_view{newOption}, false); }
    m_options.push_back(std::move(newOption));
}

void ui::Menu::edit_option(int index, std::string_view newOption)
{
    const int optionSize = m_options.size();
    if (index < 0 || index >= optionSize) { return; }
    m_options[index] = newOption.data();
}

int ui::Menu::get_selected() const noexcept { return m_selected; }

void ui::Menu::set_selected(int selected)
{
    m_selected = selected;
    Menu::update_scroll_text();
}
void ui::Menu::set_x(int x) noexcept { m_x = x; }

void ui::Menu::set_y(int y) noexcept { m_y = y; }

void ui::Menu::set_width(int width) noexcept { m_width = width; }

void ui::Menu::reset(bool full)
{
    if (full)
    {
        m_selected = 0;
        m_transition.set_y(m_originalY);
        m_transition.set_target_y(m_originalY);
    }

    m_options.clear();
}

bool ui::Menu::is_empty() const noexcept { return m_options.empty(); }

void ui::Menu::play_sound() noexcept { sm_cursor->play(); }

//                      ---- Private functions ----

void ui::Menu::calculate_alignments() noexcept
{
    m_optionHeight      = std::floor(static_cast<double>(m_fontSize) * 1.8f);
    m_maxDisplayOptions = (m_renderTargetHeight - m_originalY) / m_optionHeight;
    m_scrollLength      = std::floor(static_cast<double>(m_maxDisplayOptions) / 2.0f);
    m_textY             = (m_optionHeight / 2) - (m_fontSize / 2);
}

void ui::Menu::initialize_transition() noexcept
{
    m_transition.set_x(m_x);
    m_transition.set_y(m_y);
    m_transition.set_target_x(m_x);
    m_transition.set_target_y(m_y);
    m_transition.set_threshold(m_transition.DEFAULT_THRESHOLD);
}

void ui::Menu::initialize_option_target()
{
    static int MENU_ID{};

    const std::string optionTargetName = "MENU_TARGET_" + std::to_string(MENU_ID++);
    m_optionTarget =
        sdl2::TextureManager::create_load_resource(optionTargetName, m_width, m_optionHeight, SDL_TEXTUREACCESS_TARGET);
}

void ui::Menu::initialize_ui_elements()
{
    // This is empty by default.
    static constexpr std::string_view EMPTY = {};

    m_boundingBox  = ui::BoundingBox::create(0, 0, m_width + 12, m_optionHeight + 12);
    m_optionScroll = ui::TextScroll::create(EMPTY,
                                            16,
                                            0,
                                            m_width,
                                            m_optionHeight,
                                            m_fontSize,
                                            colors::BLUE_GREEN,
                                            colors::TRANSPARENT,
                                            false);
}

void ui::Menu::initialize_sounds()
{
    static constexpr std::string_view CURSOR_PATH = "romfs:/Sound/MenuCursor.wav";

    if (sm_cursor) { return; }
    sm_cursor = sdl2::SoundManager::create_load_resource(CURSOR_PATH, CURSOR_PATH);
}

void ui::Menu::update_scroll_text()
{
    const std::string_view text   = m_optionScroll->get_text();
    const std::string_view option = m_options[m_selected];
    if (text != option) { m_optionScroll->set_text(std::string_view{option}, false); }
}

void ui::Menu::handle_input(const sdl2::Input &input)
{
    // Get the length of the menu.
    const int optionsSize = m_options.size();

    // Control bools.
    const bool upPressed        = input.button_pressed(HidNpadButton_AnyUp);
    const bool downPressed      = input.button_pressed(HidNpadButton_AnyDown);
    const bool leftPressed      = input.button_pressed(HidNpadButton_AnyLeft);
    const bool rightPressed     = input.button_pressed(HidNpadButton_AnyRight);
    const bool lShoulderPressed = input.button_pressed(HidNpadButton_L);
    const bool rShoulderPressed = input.button_pressed(HidNpadButton_R);

    // Wrapping conditions.
    const bool wrapEnd   = upPressed && m_selected - 1 < 0;
    const bool wrapBegin = downPressed && m_selected + 1 >= optionsSize;

    // This is used to tell whether or not to play the sound.
    const int previousSelected = m_selected;

    if (wrapEnd) { m_selected = optionsSize - 1; }
    else if (wrapBegin) { m_selected = 0; }
    else if (upPressed) { --m_selected; }
    else if (downPressed) { ++m_selected; }
    else if (leftPressed) { m_selected -= m_scrollLength; }
    else if (rightPressed) { m_selected += m_scrollLength; }
    else if (lShoulderPressed) { m_selected -= m_scrollLength * 3; }
    else if (rShoulderPressed) { m_selected += m_scrollLength * 3; }

    if (m_selected < 0) { m_selected = 0; }
    else if (m_selected >= optionsSize) { m_selected = optionsSize - 1; }

    if (m_selected != previousSelected) { Menu::play_sound(); }
}

void ui::Menu::update_scrolling()
{
    // Get the option count. Bail if the menu isn't long enough to need scrolling.
    const int optionsSize = m_options.size();
    if (optionsSize <= m_maxDisplayOptions) { return; }

    // This is where scrolling menu down (or upward) ends.
    const int endScrollPoint = optionsSize - (m_maxDisplayOptions - m_scrollLength);

    // This is the total number of items scrolled passed the length where scrolling needs to occur.
    const int scrolledItems = m_selected - m_scrollLength;

    // This is our updated target.
    int targetY{};
    // We need this for comparison later.
    const int previousTarget = m_transition.get_target_y();

    if (m_selected < m_scrollLength) { targetY = m_originalY; } // Don't bother. There's no point.
    // If the selected index is passed the point where we stop scrolling, set it to the position where we need to stop to keep
    // the menu in place.
    else if (m_selected >= endScrollPoint) { targetY = m_originalY - (optionsSize - m_maxDisplayOptions) * m_optionHeight; }
    // If the selected option is passed our scrolling length, calculate the new target.
    else if (m_selected >= m_scrollLength) { targetY = m_originalY - (scrolledItems * m_optionHeight); }

    // If our current target doesn't match the previous, update the target.
    if (targetY != previousTarget) { m_transition.set_target_y(targetY); }
}
