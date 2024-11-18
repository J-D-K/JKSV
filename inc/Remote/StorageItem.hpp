#pragma once
#include <string>

namespace Remote
{
    class StorageItem
    {
        public:
            StorageItem(std::string_view ItemName, std::string_view ItemID, std::string_view ParentID, size_t ItemSize, bool IsDirectory);
            // This is for after patch upload.
            void SetItemSize(size_t ItemSize);
            // These return whatever property of the item is specified.
            std::string_view GetItemName(void) const;
            std::string_view GetParentID(void) const;
            std::string_view GetItemID(void) const;
            size_t GetItemSize(void) const;
            bool IsDirectory(void) const;

        private:
            // Name
            std::string m_ItemName;
            // ID
            std::string m_ItemID;
            // Parent
            std::string m_ParentID;
            // Item's size
            size_t m_ItemSize;
            // Whether is directory.
            bool m_IsDirectory = false;
    };
} // namespace Remote
