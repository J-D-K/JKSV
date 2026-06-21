#pragma once
#include "remote/Storage.hpp"
#include "remote/URL.hpp"

#include <string>

namespace remote
{
    class WebDav final : public remote::Storage
    {
        public:
            /// @brief Loads the WebDav config from the SD card and loads the listing.
            WebDav();

            /// @brief Creates a new directory on the WebDav server.
            /// @param name Name of the directory to create.
            bool create_directory(std::string_view name) override;

            /// @brief Uploads a file to the webdav server. File name is retrieved from the path.
            /// @param source Local path of the file to upload.
            bool upload_file(const fslib::Path &source,
                             std::string_view remoteName,
                             sys::ProgressTask *task = nullptr) override;

            /// @brief Patches or updates a file on the WebDav server.
            /// @param file Pointer to the file to update.
            /// @param source Path of the source file to update with.
            bool patch_file(remote::Item *file, const fslib::Path &source, sys::ProgressTask *task = nullptr) override;

            /// @brief Downloads the passed file from the WebDav server.
            /// @param file Pointer to the file to download.
            /// @param destination Path to write the downloaded data from.
            bool download_file(const remote::Item *item,
                               const fslib::Path &destination,
                               sys::ProgressTask *task = nullptr) override;

            /// @brief Deletes the target item from the WebDav server.
            /// @param item Item to delete.
            bool delete_item(const remote::Item *item) override;

            /// @brief Renames an item on WebDav.
            /// @param item Item to rename.
            /// @param newName New name of the item.
            bool rename_item(remote::Item *item, std::string_view newName) override;

        private:
            /// @brief Origin or server address.
            std::string m_origin{};

            /// @brief Path component of the origin URL, used to normalize PROPFIND hrefs.
            std::string m_originPath{};

            /// @brief Username for curl requests.
            std::string m_username{};

            /// @brief Password for curl requests.
            std::string m_password{};

            /// @brief Appends the username and password to a WebDav curl request.
            void append_credentials();

            /// @brief Requests PROPFIND to the url passed.
            /// @param url URL to PROPFIND with.
            /// @param xml String to record XML response to.
            bool prop_find(const remote::URL &url, std::string &xml);

            /// @brief Processes a PROPFIND XML response.
            /// @param xml XML response.
            /// @param queriedPath The path that was queried to produce this XML, used as parentID for children.
            bool process_listing(std::string_view xml, std::string_view queriedPath);
    };
} // namespace remote
