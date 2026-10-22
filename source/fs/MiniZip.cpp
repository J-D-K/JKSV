#include "fs/MiniZip.hpp"

#include "config/config.hpp"
#include "error.hpp"
#include "logging/logger.hpp"

#include <ctime>

// Definition at bottom.
static zip_fileinfo create_zip_file_info();

//                      ---- Construction ----

fs::MiniZip::MiniZip(const fslib::Path &path)
    : m_level(config::get_by_key(config::keys::ZIP_COMPRESSION_LEVEL))
{
    MiniZip::open(path);
}

fs::MiniZip::~MiniZip() { MiniZip::close(); }

//                      ---- Public functions ----

bool fs::MiniZip::is_open() const noexcept { return m_isOpen; }

bool fs::MiniZip::open(const fslib::Path &path)
{
    MiniZip::close();

    const std::string pathString = path.string();
    m_zip                        = zipOpen64(pathString.c_str(), APPEND_STATUS_CREATE);
    if (error::is_null(m_zip)) { return false; }
    m_isOpen = true;
    return true;
}

void fs::MiniZip::close()
{
    if (!m_isOpen) { return; }
    zipClose(m_zip, nullptr);
    m_isOpen = false;
}

void fs::MiniZip::set_root_prefix(std::string_view prefix)
{
    m_rootPrefix.assign(prefix);
    while (!m_rootPrefix.empty() && m_rootPrefix.back() == '/') { m_rootPrefix.pop_back(); }
}

std::string_view fs::MiniZip::strip_path(std::string_view path) const
{
    if (!m_rootPrefix.empty() && path.size() >= m_rootPrefix.size() &&
        path.compare(0, m_rootPrefix.size(), m_rootPrefix) == 0 &&
        (path.size() == m_rootPrefix.size() || path[m_rootPrefix.size()] == '/'))
    {
        path.remove_prefix(m_rootPrefix.size());
        while (!path.empty() && path.front() == '/') { path.remove_prefix(1); }
        return path;
    }

    const size_t pathBegin = path.find_first_of('/');
    if (pathBegin != path.npos) { path.remove_prefix(pathBegin + 1); }
    return path;
}

void fs::MiniZip::add_directory(std::string_view filename)
{
    std::string zipPath{strip_path(filename)};
    if (zipPath.empty()) { return; }
    if (zipPath.back() != '/') { zipPath.append("/"); }

    MiniZip::open_new_file_raw(zipPath);
    MiniZip::close_current_file();
}

bool fs::MiniZip::open_new_file(std::string_view filename, bool trimPath, size_t trimPlaces)
{
    (void)trimPath;
    (void)trimPlaces;
    return MiniZip::open_new_file_raw(strip_path(filename));
}

bool fs::MiniZip::open_new_file_raw(std::string_view filename)
{
    if (filename.empty()) { return false; }

    std::string entryName{filename};
    const zip_fileinfo fileInfo = create_zip_file_info();
    return zipOpenNewFileInZip64(m_zip,
                                 entryName.c_str(),
                                 &fileInfo,
                                 nullptr,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 Z_DEFLATED,
                                 m_level,
                                 0) == ZIP_OK;
}

bool fs::MiniZip::close_current_file() { return zipCloseFileInZip(m_zip) == ZIP_OK; }

bool fs::MiniZip::write(const void *buffer, size_t dataSize)
{
    if (!m_isOpen) { return false; }
    return zipWriteInFileInZip(m_zip, buffer, dataSize) == ZIP_OK;
}

//                      ---- Static functions ----

static zip_fileinfo create_zip_file_info()
{
    const std::time_t currentTime = std::time(nullptr);
    const std::tm *local          = std::localtime(&currentTime);
    const zip_fileinfo fileInfo   = {.tmz_date    = {.tm_sec  = local->tm_sec,
                                                     .tm_min  = local->tm_min,
                                                     .tm_hour = local->tm_hour,
                                                     .tm_mday = local->tm_mday,
                                                     .tm_mon  = local->tm_mon,
                                                     .tm_year = local->tm_year + 1900},
                                     .dosDate     = 0,
                                     .internal_fa = 0,
                                     .external_fa = 0};

    return fileInfo;
}
