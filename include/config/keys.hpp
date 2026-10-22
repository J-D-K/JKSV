#pragma once
#include <string_view>

namespace config::keys
{
    inline constexpr std::string_view WORKING_DIRECTORY       = "WorkingDirectory";
    inline constexpr std::string_view INCLUDE_DEVICE_SAVES    = "IncludeDeviceSaves";
    inline constexpr std::string_view AUTO_BACKUP_ON_RESTORE  = "AutoBackupOnRestore";
    inline constexpr std::string_view AUTO_NAME_BACKUPS       = "AutoNameBackups";
    inline constexpr std::string_view AUTO_UPLOAD             = "AutoUploadToRemote";
    inline constexpr std::string_view KEEP_LOCAL_BACKUPS      = "KeepLocalBackups";
    inline constexpr std::string_view USE_TITLE_IDS           = "AlwaysUseTitleID";
    inline constexpr std::string_view ENGLISH_SAFE_TITLES     = "EnglishSafeTitles";
    inline constexpr std::string_view HOLD_FOR_DELETION       = "HoldForDeletion";
    inline constexpr std::string_view HOLD_FOR_RESTORATION    = "HoldForRestoration";
    inline constexpr std::string_view HOLD_FOR_OVERWRITE      = "HoldForOverWrite";
    inline constexpr std::string_view ONLY_LIST_MOUNTABLE     = "OnlyListMountable";
    inline constexpr std::string_view LIST_ACCOUNT_SYS_SAVES  = "ListAccountSystemSaves";
    inline constexpr std::string_view ALLOW_WRITING_TO_SYSTEM = "AllowSystemSaveWriting";
    inline constexpr std::string_view EXPORT_TO_ZIP           = "ExportToZip";
    inline constexpr std::string_view ZIP_COMPRESSION_LEVEL   = "ZipCompressionLevel";
    inline constexpr std::string_view TITLE_SORT_TYPE         = "TitleSortType";
    inline constexpr std::string_view JKSM_TEXT_MODE          = "JKSMTextMode";
    inline constexpr std::string_view FORCE_ENGLISH           = "ForceEnglish";
    inline constexpr std::string_view SHOW_DEVICE_USER        = "ShowDevice";
    inline constexpr std::string_view SHOW_BCAT_USER          = "ShowBCAT";
    inline constexpr std::string_view SHOW_CACHE_USER         = "ShowCache";
    inline constexpr std::string_view SHOW_SYSTEM_USER        = "ShowSystem";
    inline constexpr std::string_view ENABLE_TRASH_BIN        = "EnableTrash";
    inline constexpr std::string_view UI_ANIMATION_SCALE      = "UIAnimationScaling";
    inline constexpr std::string_view FAVORITES               = "Favorites";
    inline constexpr std::string_view BLACKLIST               = "BlackList";
    // Peer save transfer (LAN transport; LDN disabled by default after the spike).
    inline constexpr std::string_view PEER_LAST_IP = "PeerLastIP";
}
