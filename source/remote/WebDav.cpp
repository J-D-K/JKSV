#include "remote/WebDav.hpp"

#include "curl/curl.hpp"
#include "error.hpp"
#include "json.hpp"
#include "logging/logger.hpp"
#include "remote/remote.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"
#include "ui/PopMessageManager.hpp"

#include <tinyxml2.h>

// Declarations here. Definitions at bottom.
/// @brief Gets a XML element by name. This is namespace agnostic.
/// @param parent Parent XML element.
/// @param name Name of the element to get. No namespace needed.
static tinyxml2::XMLElement *get_element_by_name(tinyxml2::XMLElement *parent, std::string_view name);

/// @brief Returns the starting point of the tag name without the namespace.
/// @param tag Tag to get the name of.
static std::string_view get_tag_begin(std::string_view tag);

/// @brief Ensures the parent is a valid path. I guess some servers don't have trailing slashes for directories.
/// @param parent Parent string.
static std::string ensure_valid_dir_path(std::string_view parent);

/// @brief Slices and unescapes the directory or filename from the href.
/// @param handle Curl handle to use to unescape.
/// @param href HREF to slice.
/// @note This seemed like a better alternative than relying on servers having displayname.
static std::string slice_name_from_href(curl::Handle &handle, std::string_view href);

//                      ---- Construction ----

remote::WebDav::WebDav()
    : Storage("[WD]")
{
    static constexpr const char *STRING_CONFIG_READ_ERROR = "Error initializing WebDav: %s";

    json::Object config = json::new_object(json_object_from_file, remote::PATH_WEBDAV_CONFIG.data());
    if (!config)
    {
        logger::log(STRING_CONFIG_READ_ERROR, "Error reading configuration file!");
        return;
    }

    // Let's just get this all out of the way at once.
    json_object *origin   = json::get_object(config, "origin");
    json_object *basepath = json::get_object(config, "basepath");
    json_object *username = json::get_object(config, "username");
    json_object *password = json::get_object(config, "password");
    if (!origin)
    {
        logger::log(STRING_CONFIG_READ_ERROR, "Config is missing origin!");
        return;
    }
    m_origin = json_object_get_string(origin);

    // Extract the path component from the origin URL for normalizing PROPFIND hrefs.
    {
        const size_t schemeEnd = m_origin.find("://");
        if (schemeEnd != std::string::npos)
        {
            const size_t pathStart = m_origin.find('/', schemeEnd + 3);
            if (pathStart != std::string::npos)
            {
                m_originPath = m_origin.substr(pathStart);
                if (m_originPath.back() != '/') { m_originPath += '/'; }
            }
        }
    }

    if (basepath)
    {
        // The root is both in the beginning. I want this to work as closely as the original just not as poorly written
        // or thought out as the original JKSV dav code.
        m_root   = stringutil::get_formatted_string("/%s/", json_object_get_string(basepath));
        m_parent = m_root;
    }

    if (username) { m_username = json_object_get_string(username); }
    if (password) { m_password = json_object_get_string(password); }

    // This is the starting point. This will read the entire basepath listing in one go.
    remote::URL url{m_origin};
    url.append_path(m_root).append_slash();

    // This'll recursively get the full listing of the basepath for the WebDav server.
    std::string xml{};
    const bool propFind     = WebDav::prop_find(url, xml);
    const bool xmlProcessed = propFind && WebDav::process_listing(xml, m_root);
    if (!propFind || !xmlProcessed) { return; }

    m_isInitialized = true;
}

bool remote::WebDav::create_directory(std::string_view name)
{
    static constexpr const char *STRING_CREATE_DIR_ERROR = "Error creating WebDav directory: %s";

    std::string escapedName{};
    const bool nameEscaped = curl::escape_string(m_curl, name, escapedName);
    if (!nameEscaped)
    {
        logger::log(STRING_CREATE_DIR_ERROR, "Error escaping directory name!");
        return false;
    }

    remote::URL url{m_origin};
    url.append_path(m_parent).append_path(escapedName).append_slash();

    curl::reset_handle(m_curl);
    WebDav::append_credentials();
    curl::set_option(m_curl, CURLOPT_URL, url.get());
    curl::set_option(m_curl, CURLOPT_CUSTOMREQUEST, "MKCOL");

    if (!curl::perform(m_curl)) { return false; }

    const long code = curl::get_response_code(m_curl);
    if (code != 201)
    {
        logger::log(STRING_CREATE_DIR_ERROR, name.data());
        return false;
    }

    // This is the ID string so we can make WebDav work within the same framework as Google Drive.
    std::string id = m_parent + "/" + escapedName + "/";
    m_list.emplace_back(name, id, m_parent, 0, true);

    return true;
}

bool remote::WebDav::upload_file(const fslib::Path &source, std::string_view remoteName, sys::ProgressTask *task)
{
    static constexpr const char *STRING_ERROR_UPLOADING = "Error uploading to WebDav: %s";

    fslib::File sourceFile{source, FsOpenMode_Read};
    if (error::fslib(sourceFile.is_open())) { return false; }

    std::string escapedName{};
    const bool nameEscaped = curl::escape_string(m_curl, remoteName, escapedName);
    if (!nameEscaped)
    {
        logger::log(STRING_ERROR_UPLOADING, "Failed to escape filename!");
        return false;
    }

    // This is used multiple multiple places. No need to recall.
    const int64_t fileSize = sourceFile.get_size();
    if (task) { task->reset(static_cast<double>(fileSize)); }

    remote::URL url{m_origin};
    url.append_path(m_parent).append_path(escapedName);

    curl::UploadStruct uploadData{.source = &sourceFile, .task = task};
    curl::reset_handle(m_curl);
    WebDav::append_credentials();
    curl::set_option(m_curl, CURLOPT_URL, url.get());
    curl::set_option(m_curl, CURLOPT_UPLOAD, 1L);
    curl::set_option(m_curl, CURLOPT_UPLOAD_BUFFERSIZE, Storage::SIZE_UPLOAD_BUFFER);
    curl::set_option(m_curl, CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(fileSize));
    curl::set_option(m_curl, CURLOPT_READFUNCTION, curl::read_data_from_file);
    curl::set_option(m_curl, CURLOPT_READDATA, &uploadData);

    if (!curl::perform(m_curl)) { return false; }

    const std::string id = m_parent + "/" + escapedName;
    m_list.emplace_back(remoteName, id, m_parent, fileSize, false);

    return true;
}

bool remote::WebDav::patch_file(remote::Item *item, const fslib::Path &source, sys::ProgressTask *task)
{
    static constexpr const char *STRING_ERROR_PATCHING = "Error patching file: %s";

    fslib::File sourceFile{source, FsOpenMode_Read};
    if (!sourceFile)
    {
        logger::log(STRING_ERROR_PATCHING, fslib::error::get_string());
        return false;
    }

    if (task) { task->reset(static_cast<double>(sourceFile.get_size())); }

    remote::URL url{m_origin};
    url.append_path(item->get_id());

    curl::UploadStruct uploadData{.source = &sourceFile, .task = task};
    curl::reset_handle(m_curl);
    WebDav::append_credentials();
    curl::set_option(m_curl, CURLOPT_URL, url.get());
    curl::set_option(m_curl, CURLOPT_UPLOAD, 1L);
    curl::set_option(m_curl, CURLOPT_UPLOAD_BUFFERSIZE, Storage::SIZE_UPLOAD_BUFFER);
    curl::set_option(m_curl, CURLOPT_READFUNCTION, curl::read_data_from_file);
    curl::set_option(m_curl, CURLOPT_READDATA, &uploadData);

    if (!curl::perform(m_curl)) { return false; }

    // Just update the size this time.
    item->set_size(sourceFile.get_size());

    return true;
}

bool remote::WebDav::download_file(const remote::Item *item, const fslib::Path &destination, sys::ProgressTask *task)
{
    static constexpr const char *STRING_ERROR_DOWNLOADING = "Error downloading file: %s";

    const int64_t itemSize = item->get_size();
    fslib::File destFile{destination, FsOpenMode_Create | FsOpenMode_Write, itemSize};
    if (!destFile)
    {
        logger::log(STRING_ERROR_DOWNLOADING, fslib::error::get_string());
        return false;
    }

    if (task) { task->reset(static_cast<double>(itemSize)); }

    remote::URL url{m_origin};
    url.append_path(item->get_id());

    auto download = curl::create_download_struct(destFile, task, itemSize);
    curl::reset_handle(m_curl);
    WebDav::append_credentials();
    curl::set_option(m_curl, CURLOPT_HTTPGET, 1L);
    curl::set_option(m_curl, CURLOPT_URL, url.get());
    curl::set_option(m_curl, CURLOPT_WRITEFUNCTION, curl::download_file_threaded);
    curl::set_option(m_curl, CURLOPT_WRITEDATA, download.get());

    // Copied from gd.cpp implementation.
    // TODO: Not sure how a thread helps if this parent waits here.
    // TODO: Read and understand what's actually happening before making comments on other's choices.
    sys::threadpool::push_job(curl::download_write_thread_function, download);
    if (!curl::perform(m_curl)) { return false; }

    download->writeComplete.acquire();

    return true;
}

bool remote::WebDav::delete_item(const remote::Item *item)
{
    static constexpr const char *STRING_ERROR_DELETING = "Error deleting item: %s";

    remote::URL url{m_origin};
    url.append_path(item->get_id());
    if (item->is_directory()) { url.append_slash(); }

    curl::reset_handle(m_curl);
    WebDav::append_credentials();
    curl::set_option(m_curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl::set_option(m_curl, CURLOPT_URL, url.get());

    if (!curl::perform(m_curl)) { return false; }

    const long code = curl::get_response_code(m_curl);
    if (code != 204) // This can be any of these, I think?
    {
        logger::log(STRING_ERROR_DELETING, "Deletion failed!");
        return false;
    }

    auto findFile = Storage::find_file_by_name(item->get_name());
    if (findFile == m_list.end()) { return false; }

    m_list.erase(findFile);
    return true;
}

bool remote::WebDav::rename_item(remote::Item *item, std::string_view newName)
{
    std::string escapedName{};
    const bool escaped = curl::escape_string(m_curl, newName, escapedName);
    if (!escaped) { return false; }

    remote::URL url{m_origin};
    url.append_path(item->get_id());

    remote::URL destLocation{m_origin};
    destLocation.append_path(m_parent).append_path(escapedName);
    if (item->is_directory())
    {
        url.append_slash();
        destLocation.append_slash();
    }

    const std::string destHeader = stringutil::get_formatted_string("Destination: %s", destLocation.get());
    curl::HeaderList header      = curl::new_header_list();
    curl::append_header(header, destHeader);

    curl::reset_handle(m_curl);
    WebDav::append_credentials();
    curl::set_option(m_curl, CURLOPT_CUSTOMREQUEST, "MOVE");
    curl::set_option(m_curl, CURLOPT_HTTPHEADER, header.get());
    curl::set_option(m_curl, CURLOPT_URL, url.get());

    if (!curl::perform(m_curl)) { return false; }

    const long code = curl::get_response_code(m_curl);
    if (code != 201 && code != 204) { return false; }

    std::string newId{};
    if (item->is_directory())
    {
        newId = m_parent + escapedName + "/";

        // Need to make sure the parents match too.
        remote::Storage::DirectoryListing dirListing{};
        Storage::get_directory_listing_with_parent(item, dirListing);
        for (remote::Item *item : dirListing) { item->set_parent_id(newId); }
    }
    else { newId = m_parent + escapedName; }

    item->set_name(newName);
    item->set_id(newId);

    return false;
}

//                      ---- Private functions ----

void remote::WebDav::append_credentials()
{
    if (!m_username.empty()) { curl::set_option(m_curl, CURLOPT_USERNAME, m_username.c_str()); }
    if (!m_password.empty()) { curl::set_option(m_curl, CURLOPT_PASSWORD, m_password.c_str()); }
}

bool remote::WebDav::prop_find(const remote::URL &url, std::string &xml)
{
    // Some servers block Depth: Infinity.
    curl::HeaderList header = curl::new_header_list();
    curl::append_header(header, "Depth: 1");

    curl::reset_handle(m_curl);
    WebDav::append_credentials();
    curl::set_option(m_curl, CURLOPT_CUSTOMREQUEST, "PROPFIND");
    curl::set_option(m_curl, CURLOPT_HTTPHEADER, header.get());
    curl::set_option(m_curl, CURLOPT_URL, url.get());
    curl::set_option(m_curl, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl::set_option(m_curl, CURLOPT_WRITEDATA, &xml);

    return curl::perform(m_curl);
}

bool remote::WebDav::process_listing(std::string_view xml, std::string_view queriedPath)
{
    static constexpr const char *STRING_ERROR_PROCESSING_XML = "Error processing XML: %s";
    // These are so string_views aren't constructed every loop.
    static constexpr std::string_view tagHref{"href"};
    static constexpr std::string_view tagPropStat{"propstat"};
    static constexpr std::string_view tagProp{"prop"};
    static constexpr std::string_view tagResourceType{"resourcetype"};
    static constexpr std::string_view tagCollection{"collection"};
    static constexpr std::string_view tagGetContentLength{"getcontentlength"};

    tinyxml2::XMLDocument listing{};
    if (listing.Parse(xml.data(), xml.length()))
    {
        logger::log(STRING_ERROR_PROCESSING_XML, "Couldn't parse XML!");
        return false;
    }

    tinyxml2::XMLElement *root   = listing.RootElement();
    tinyxml2::XMLElement *parent = root->FirstChildElement();

    // Use the queried path for parentID instead of extracting from the XML href,
    // because XML hrefs contain full server-relative paths that don't match
    // our internal m_parent/m_root format.
    const std::string parentID = ensure_valid_dir_path(queriedPath);
    tinyxml2::XMLElement *current{};
    for (current = parent->NextSiblingElement(); current; current = current->NextSiblingElement())
    {
        // Parsing XML is actually annoying. Even with tinyxml2.
        tinyxml2::XMLElement *href         = get_element_by_name(current, tagHref);
        tinyxml2::XMLElement *propstat     = get_element_by_name(current, tagPropStat);
        tinyxml2::XMLElement *prop         = get_element_by_name(propstat, tagProp);
        tinyxml2::XMLElement *resourceType = get_element_by_name(prop, tagResourceType);
        if (!href || !propstat || !prop || !resourceType) { continue; }

        // Avoid redundant calls the GetText later.
        const char *hrefText             = href->GetText();
        const std::string name           = slice_name_from_href(m_curl, hrefText);
        tinyxml2::XMLElement *collection = get_element_by_name(resourceType, tagCollection);
        if (collection)
        {
            // Build a normalized ID relative to the parent. Re-escape the name
            // so the ID format matches what create_directory() produces.
            std::string escapedName{};
            curl::escape_string(m_curl, name, escapedName);
            const std::string idString = ensure_valid_dir_path(parentID + escapedName);

            m_list.emplace_back(name, idString, parentID, 0, true);

            remote::URL nextUrl{m_origin};
            nextUrl.append_path(idString);

            std::string xml{};
            const bool propFind   = WebDav::prop_find(nextUrl, xml);
            const bool processXml = propFind && WebDav::process_listing(xml, idString);
            if (!propFind || !processXml) { logger::log(STRING_ERROR_PROCESSING_XML, hrefText); }
        }
        else
        {
            tinyxml2::XMLElement *getContentLength = get_element_by_name(prop, tagGetContentLength);
            if (!getContentLength) { continue; }

            const char *lengthString    = getContentLength->GetText();
            const int64_t contentLength = std::strtoll(lengthString, nullptr, 10);

            // Build a normalized file ID relative to the parent.
            std::string escapedName{};
            curl::escape_string(m_curl, name, escapedName);
            const std::string fileID = parentID + escapedName;

            m_list.emplace_back(name, fileID, parentID, contentLength, false);
        }
    }
    return true;
}

//                      ---- Static functions ----

static tinyxml2::XMLElement *get_element_by_name(tinyxml2::XMLElement *parent, std::string_view name)
{
    tinyxml2::XMLElement *current = parent->FirstChildElement();
    if (!current) { return nullptr; }

    do {
        if (get_tag_begin(current->Name()) == name) { return current; }
    } while ((current = current->NextSiblingElement()));
    return nullptr;
}

static std::string_view get_tag_begin(std::string_view tag)
{
    size_t colon = tag.find_first_of(':');
    if (colon == tag.npos) { return tag; }
    return tag.substr(colon + 1);
}

static std::string ensure_valid_dir_path(std::string_view parent)
{
    std::string returnParent{parent};
    if (returnParent.back() != '/') { returnParent.append("/"); }
    return returnParent;
}

static std::string slice_name_from_href(curl::Handle &handle, std::string_view href)
{
    std::string name{};

    if (href.back() == '/')
    {
        // To do: This might not be the safest.
        size_t end   = href.find_last_of('/') - 1;
        size_t begin = href.find_last_of('/', end);
        if (begin == href.npos) { name = href; }
        else { name = href.substr(begin + 1, (end - begin)); }
    }
    else
    {
        size_t begin = href.find_last_of('/');
        if (begin == href.npos) { name = href; }
        else { name = href.substr(begin + 1); }
    }
    curl::unescape_string(handle, name, name);

    return name;
}
