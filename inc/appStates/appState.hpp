#pragma once

class appState
{
    public:
        appState(void) { };
        virtual ~appState() { };
        virtual void update(void);
        virtual void render(void);

        // Returns if state can be popped from vector
        bool isActive(void);
        // Sets active to false which means state can be popped
        void deactivateState(void);
        // Tells state it is at the top of the vector
        void giveFocus(void);
        // Tells states it is no longer on top
        void takeFocus(void);
        // Returns if state is at back of vector
        bool hasFocus(void) const;

    private:
        // Whether state is still active or can be popped
        bool m_IsActive = true;
        // Whether state is the one currently at the top
        bool m_HasFocus = false;
};