#ifndef UTILS_HPP
#define UTILS_HPP

#include "fmt/color.h"
#include "fmt/core.h"

#include <vector>
#include <string>
#include <sys/types.h>

#define BOLD_TEXT(x)    (fmt::emphasis::bold | fmt::fg(x))
#define NOCOLOR         "\033[0m"
#define UNKNOWN         "(unknown)"

// magic line to be sure that I don't cut the wrong line 
#define MAGIC_LINE      "(cut this shit NOW!! RAHHH)"

bool hasEnding(const std::string_view fullString, const std::string_view ending);
bool hasStart(const std::string_view fullString, const std::string_view start);
std::string name_from_entry(size_t dev_entry_pos);
std::string vendor_from_entry(size_t vendor_entry_pos, const std::string_view vendor_id);
std::string binarySearchPCIArray(const std::string_view vendor_id, const std::string_view pci_id);
std::string binarySearchPCIArray(const std::string_view vendor_id);
std::string shell_exec(const std::string_view cmd);
std::vector<std::string> split(const std::string_view text, char delim);
bool is_file_image(const unsigned char *bytes);
std::string expandVar(const std::string_view str);
// Replace string inplace
void replace_str(std::string &str, const std::string& from, const std::string& to);
bool read_exec(std::vector<const char *> cmd, std::string& output, bool useStdErr = false);
std::string str_tolower(const std::string str);
std::string str_toupper(const std::string str);
void strip(std::string& input);
std::string read_by_syspath(const std::string_view path);
fmt::rgb hexStringToColor(const std::string_view hexstr);
std::string getHomeConfigDir();
std::string getConfigDir();

constexpr std::size_t operator""_len(const char*,std::size_t ln) noexcept{
    return ln;
}

template <typename... Args>
void error(const std::string_view fmt, Args&&... args) {
    fmt::println(stderr, BOLD_TEXT(fmt::rgb(fmt::color::red)), "ERROR: {}", fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

template <typename... Args>
void die(const std::string_view fmt, Args&&... args) {
    fmt::println(stderr, BOLD_TEXT(fmt::rgb(fmt::color::red)), "ERROR: {}", fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
    std::exit(1);
}

template <typename... Args>
void debug(const std::string_view fmt, Args&&... args) {
#if DEBUG
    fmt::println(BOLD_TEXT(fmt::rgb(fmt::color::hot_pink)), "[DEBUG]: {}", fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
#endif
}

template <typename... Args>
void warn(const std::string_view fmt, Args&&... args) {
    fmt::println(BOLD_TEXT(fmt::rgb(fmt::color::yellow)), "WARNING: {}", fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

#endif
