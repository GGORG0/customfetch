#ifndef QUERY_HPP
#define QUERY_HPP

#include "util.hpp"
#include "config.hpp"
#include <cstdint>
#include <fstream>
#include <unordered_map>
#include <variant>
#include <string>

extern "C" {
#include <pwd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <mntent.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>
}

using systemInfo_t = std::unordered_map<std::string, std::unordered_map<std::string, std::variant<std::string, size_t, float>>>;
using variant = std::variant<std::string, size_t, float>;

namespace Query {

class System {
public:
    struct System_t {
        std::string os_pretty_name{UNKNOWN};
        std::string os_name{UNKNOWN};
        std::string os_id{UNKNOWN};
        std::string os_version_id{UNKNOWN};
        std::string os_version_codename{UNKNOWN};
        std::string os_initsys_name{UNKNOWN};
        std::string os_initsys_version{UNKNOWN};

        std::string host_modelname;
        std::string host_version;
        std::string host_vendor;

        std::string pkgs_installed;
    };

    struct pkg_managers_t {
        std::uint16_t pacman_pkgs = 0;
        std::uint16_t flatpak_pkgs = 0;
    };

    System();
    std::string kernel_name();
    std::string kernel_version();
    std::string hostname();
    std::string arch();
    std::string os_pretty_name();
    std::string os_name();
    std::string os_id();
    std::string os_initsys_name();
    std::string os_versionid();
    std::string os_version_codename();
    long        uptime();
    
    // motherboard (host)
    std::string host_modelname();
    std::string host_vendor();
    std::string host_version();

    std::string pkgs_installed(const Config& config);

private:
    static System_t m_system_infos;
    static bool m_bInit;
    static struct utsname m_uname_infos;
    static struct sysinfo m_sysInfos;
};

class User {
public:
    struct User_t {
        std::string name{UNKNOWN};
        std::string shell_name{UNKNOWN};
        std::string shell_version{UNKNOWN};
        std::string wm_name{MAGIC_LINE};
        std::string de_name{MAGIC_LINE};
        std::string de_version{UNKNOWN};
        std::string term_name{MAGIC_LINE};
        std::string term_version{MAGIC_LINE};
    };

    User();
    std::string name() noexcept;
    std::string shell_name() noexcept;
    std::string shell_path() noexcept;
    std::string shell_version(const std::string_view shell_name) noexcept;
    std::string wm_name(bool dont_query_dewm, const std::string_view term_name);
    std::string de_name(bool dont_query_dewm, const std::string_view term_name, const std::string_view wm_name);
    std::string de_version(const std::string_view de_name);

    std::string term_name();
    std::string term_version(const std::string_view term_name);

    static bool m_bDont_query_dewm;

private:
    static bool m_bCut_de;
    static bool m_bInit;
    static User_t m_users_infos;
    static struct passwd *m_pPwd;
};

class CPU {
public:
    struct CPU_t {
        std::string name{UNKNOWN};
        std::string nproc{UNKNOWN};
        
        float freq_max = 0;
        float freq_min = 0;
        float freq_cur = 0;
        float freq_bios_limit = 0;

        // private:
        float freq_max_cpuinfo = 0;
    };

    CPU();
    std::string name();
    std::string nproc();

    float freq_max();
    float freq_min();
    float freq_cur();
    float freq_bios_limit();

private:
    static bool m_bInit;
    static CPU_t m_cpu_infos;
};

class GPU {
public:
    struct GPU_t {
        std::string name{UNKNOWN};
        std::string vendor{UNKNOWN};
    };

    GPU(u_short id = 0);
    std::string name();
    std::string vendor();

private:
    uint16_t    m_vendor_id;
    uint16_t    m_device_id;
    std::string m_vendor_id_s;
    std::string m_device_id_s;

    static GPU_t m_gpu_infos;
    static bool m_bInit;
};

class Disk {
public:
    // no need for a struct because we'll use m_statvfs members
    Disk(const std::string_view path);
    float total_amount();
    float free_amount();
    float used_amount();
    std::string typefs();

private:
    static bool m_bInit;
    static struct statvfs m_statvfs;
    static std::string m_typefs;
};

class RAM {
public:
    struct RAM_t {
        float total_amount = 0;
        float free_amount = 0;
        float used_amount = 0;
        float swap_free_amount = 0;
        float swap_total_amount = 0;
    };

    RAM();
    float total_amount();
    float free_amount();
    float used_amount();
    float swap_free_amount();
    float swap_total_amount();

private:
    static bool m_bInit;
    static RAM_t m_memory_infos;
};

};

//inline Query::System query_system;
//inline Query::CPU query_cpu;
//inline Query::GPU query_gpu;
//inline Query::RAM query_ram;

#endif
