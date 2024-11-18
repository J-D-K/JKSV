#include "Remote/StorageItem.hpp"

Remote::StorageItem::StorageItem(std::string_view ItemName,
                                 std::string_view ItemID,
                                 std::string_view ParentID,
                                 size_t ItemSize,
                                 bool IsDirectory)
    : m_ItemName(ItemName), m_ItemID(ItemID), m_ParentID(ParentID), m_ItemSize(ItemSize), m_IsDirectory(IsDirectory)
{
}

void Remote::StorageItem::SetItemSize(size_t ItemSize)
{
    m_ItemSize = ItemSize;
}

std::string_view Remote::StorageItem::GetItemName(void) const
{
    return m_ItemName;
}

std::string_view Remote::StorageItem::GetParentID(void) const
{
    return m_ParentID;
}

std::string_view Remote::StorageItem::GetItemID(void) const
{
    return m_ItemName;
}

size_t Remote::StorageItem::GetItemSize(void) const
{
    return m_ItemSize;
}

bool Remote::StorageItem::IsDirectory(void) const
{
    return m_IsDirectory;
}
