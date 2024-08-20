#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstring>
#include <filesystem>

#include "config.hpp"
#include "query.hpp"
#include "util.hpp"
#include "utils/packages.hpp"

using namespace Query;

static void get_host_paths(System::System_t& paths)
{
    const std::string syspath = "/sys/devices/virtual/dmi/id";

    if (std::filesystem::exists(syspath + "/board_name"))
    {
        paths.host_modelname = read_by_syspath(syspath + "/board_name");
        paths.host_version   = read_by_syspath(syspath + "/board_version");
        paths.host_vendor    = read_by_syspath(syspath + "/board_vendor");

        if (paths.host_vendor == "Micro-Star International Co., Ltd.")
            paths.host_vendor = "MSI";
    }

    else if (std::filesystem::exists(syspath + "/product_name"))
    {
        paths.host_modelname = read_by_syspath(syspath + "/product_name");
        if (hasStart(paths.host_modelname, "Standard PC"))
        {
            // everyone does it like "KVM/QEMU Standard PC (...) (host_version)" so why not
            paths.host_vendor  = "KVM/QEMU";
            paths.host_version = fmt::format("({})", read_by_syspath(syspath + "/product_version"));
        }
        else
            paths.host_version = read_by_syspath(syspath + "/product_version");
    }

    if (paths.host_version.back() == '\n')
        paths.host_version.pop_back();
}

static System::System_t get_system_infos()
{
    System::System_t ret;

    debug("calling in System {}", __PRETTY_FUNCTION__);
    std::string_view os_release_path;
    constexpr std::array<std::string_view, 3> os_paths = { "/etc/os-release", "/usr/lib/os-release", "/usr/share/os-release" };
    for (const std::string_view path : os_paths)
    {
        if (std::filesystem::exists(path))
        {
            os_release_path = path;
            break;
        }
    }

    std::ifstream os_release_file(os_release_path.data());
    if (!os_release_file.is_open())
    {
        error("Could not open {}\nFailed to get OS infos", os_release_path);
        return ret;
    }

    // get OS /etc/os-release infos
    static u_short iter_index = 0;
    std::string    line;
    while (std::getline(os_release_file, line) && iter_index < 5)
    {
        if (hasStart(line, "PRETTY_NAME="))
            getFileValue(iter_index, line, ret.os_pretty_name, "PRETTY_NAME="_len);

        if (hasStart(line, "NAME="))
            getFileValue(iter_index, line, ret.os_name, "NAME="_len);

        if (hasStart(line, "ID="))
            getFileValue(iter_index, line, ret.os_id, "ID="_len);

        if (hasStart(line, "VERSION_ID="))
            getFileValue(iter_index, line, ret.os_version_id, "VERSION_ID="_len);

        if (hasStart(line, "VERSION_CODENAME="))
            getFileValue(iter_index, line, ret.os_version_codename, "VERSION_CODENAME="_len);
    }

    return ret;
}

System::System()
{
    debug("Constructing {}", __func__);
    if (!m_bInit)
    {
        if (uname(&m_uname_infos) != 0)
            die("uname() failed: {}\nCould not get system infos", strerror(errno));

        if (sysinfo(&m_sysInfos) != 0)
            die("sysinfo() failed: {}\nCould not get system infos", strerror(errno));

        m_system_infos = get_system_infos();
        get_host_paths(m_system_infos);
        m_bInit = true;
    }
}

// clang-format off
std::string System::kernel_name()
{ return m_uname_infos.sysname; }

std::string System::kernel_version()
{ return m_uname_infos.release; }

std::string System::hostname()
{ return m_uname_infos.nodename; }

std::string System::arch()
{ return m_uname_infos.machine; }

long& System::uptime()
{ return m_sysInfos.uptime; }

std::string& System::os_pretty_name()
{ return m_system_infos.os_pretty_name; }

std::string& System::os_name()
{ return m_system_infos.os_name; }

std::string& System::os_id()
{ return m_system_infos.os_id; }

std::string& System::os_versionid()
{ return m_system_infos.os_version_id; }

std::string& System::os_version_codename()
{ return m_system_infos.os_version_codename; }

std::string& System::host_modelname()
{ return m_system_infos.host_modelname; }

std::string& System::host_vendor()
{ return m_system_infos.host_vendor; }

std::string& System::host_version()
{ return m_system_infos.host_version; }

// clang-format on
std::string& System::os_initsys_name()
{
    static bool done = false;
    if (!done)
    {
        // there's no way PID 1 doesn't exist.
        // This will always succeed.
        std::ifstream f_initsys("/proc/1/comm", std::ios::binary);
        if (!f_initsys.is_open())
        {
            die("/proc/1/comm doesn't exist! (what?)");
        }

        std::string initsys;
        std::getline(f_initsys, initsys);
        size_t pos = 0;

        if ((pos = initsys.find('\0')) != std::string::npos)
            initsys.erase(pos);

        if ((pos = initsys.rfind('/')) != std::string::npos)
            initsys.erase(0, pos + 1);

        m_system_infos.os_initsys_name = initsys;

        done = true;
    }

    return m_system_infos.os_initsys_name;
}

std::string& System::pkgs_installed(const Config& config)
{
    static bool done = false;
    if (!done)
    {
        m_system_infos.pkgs_installed = get_all_pkgs(config);

        done = true;
    }

    return m_system_infos.pkgs_installed;
}
