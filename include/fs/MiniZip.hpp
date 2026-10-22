#pragma once
#include "fslib.hpp"

#include <minizip/zip.h>
#include <string>
#include <string_view>

namespace fs
{
    class MiniZip final
    {
        public:
            MiniZip() = default;

            /// @brief Constructor. Calls open upon construction;
            /// @param path
            MiniZip(const fslib::Path &path);

            /// @brief Closes the zipFile.
            ~MiniZip();

            /// @brief Returns whether or not the zip file was successfully opened.
            /// @return
            bool is_open() const noexcept;

            /// @brief Opens a Zip file at path
            bool open(const fslib::Path &path);

            /// @brief Manual call for closing the zipFile.
            void close();

            /// @brief Sets a path prefix to remove from entries added to the ZIP.
            /// @param prefix Absolute path of the directory whose contents are being archived.
            /// @note Used by peer folder transfers so the archive contains paths relative to
            ///       the backup directory instead of the full SD-card path.
            void set_root_prefix(std::string_view prefix);

            /// @brief Creates a new dummy directory entry in the ZIP.
            /// @param Path Path of the directory. Will be appended with a trailing slash if needed.
            void add_directory(std::string_view path);

            /// @brief Opens a new file with filename as the path.
            bool open_new_file(std::string_view filename, bool trimPath = false, size_t trimPlaces = 0);

            /// @brief Closes the currently open file in the Zip.
            bool close_current_file();

            /// @brief Attempts to write the buffer passed to the currently opened file.
            bool write(const void *buffer, size_t dataSize);

        private:
            /// @brief Removes the configured root prefix, or the first path component by default.
            std::string_view strip_path(std::string_view path) const;

            /// @brief Opens an entry without applying path trimming.
            bool open_new_file_raw(std::string_view filename);

            /// @brief Stores whether or not the zipFile was opened successfully.
            bool m_isOpen{};

            /// @brief Stores the compression level from config to avoid repeated calls.
            int m_level{};

            /// @brief Prefix removed from entries when creating a relative archive.
            std::string m_rootPrefix;

            /// @brief Underlying ZIP file.
            zipFile m_zip{};
    };
}
