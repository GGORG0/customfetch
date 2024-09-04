#include <getopt.h>

#include <cstdlib>
#include <filesystem>

#include "config.hpp"
#include "display.hpp"
#include "gui.hpp"
#include "switch_fnv1a.hpp"
#include "util.hpp"

// https://cfengine.com/blog/2021/optional-arguments-with-getopt-long/
// because "--opt-arg arg" won't work
// but "--opt-arg=arg" will
#define OPTIONAL_ARGUMENT_IS_PRESENT \
    ((optarg == NULL && optind < argc && argv[optind][0] != '-') \
     ? (bool) (optarg = argv[optind++]) \
     : (optarg != NULL))

using namespace std::string_view_literals;

static void version()
{
    fmt::println("customfetch {} branch {}", VERSION, BRANCH);

#ifdef GUI_MODE
    fmt::println("GUI mode enabled");
#else
    fmt::println("GUI mode IS NOT enabled");
#endif

    // if only everyone would not return error when querying the program version :(
    std::exit(EXIT_SUCCESS);
}

static void help(bool invalid_opt = false)
{
    fmt::println("Usage: cufetch [OPTIONS]...");
    fmt::println(R"(
A command-line system information tool (or neofetch like program), which its focus point is customizability and perfomance

    -n, --no-display		Do not display the ascii art
    -s, --source-path <path>	Path to the ascii art file to display
    -C, --config <path>		Path to the config file to use
    -a, --ascii-logo-type [<name>]
                                The type of ASCII art to apply ("small" or "old").
                                Basically will add "_<type>" to the logo filename.
                                It will return the regular linux ascii art if it doesn't exist.
                                Leave it empty for regular.
    
    -D, --data-dir <path>       Path to the data dir where we'll taking the distros ascii arts (must contain subdirectory called "ascii")
    -d, --distro <name>         Print a custom distro logo (must be the same name, uppercase or lowercase, e.g "windows 11" or "ArCh")
    -f, --font <name>           The font to be used in GUI mode (syntax must be "[FAMILY-LIST] [STYLE-OPTIONS] [SIZE]" without the double quotes and [])
                                An example: [Liberation Mono] [Normal] [12], which can be "Liberation Mono Normal 12"

    -g, --gui                   Use GUI mode instead of priting in the terminal (use -V to check if it was enabled)
    -o, --offset <num>          Offset between the ascii art and the layout
    -l. --list-modules  	Print the list of the modules and its members
    -h, --help			Print this help menu
    -L, --logo-only             Print only the logo
    -V, --version		Print the version along with the git branch it was built

    --bg-image <path>           Path to image to be used in the background in GUI (put "disable" for disabling in the config)
    --logo-padding-top	<num>	Padding of the logo from the top
    --logo-padding-left	<num>	Padding of the logo from the left
    --layout-padding-top <num>  Padding of the layout from the top
    --sep-title <string>        A char (or string) to use in $<user.title_sep>
    --sep-reset <string>        A separetor (or string) that when ecountered, will automatically reset color
    --sep-reset-after [<num>]     Reset color either before of after 'sep-reset' (1 = after && 0 = before)
    --gen-config [<path>]       Generate default config file to config folder (if path, it will generate to the path)
                                Will ask for confirmation if file exists already

    --color <string>            Replace instances of a color with another value.
                                Syntax MUST be "name=value" with no space beetween "=", example: --color "foo=#444333".
				Thus replaces any instance of foo with #444333. Can be done with multiple colors separetly.

Read the manual "cufetch.1" or the autogenerated config file for more infos about customfetch and how it works
)"sv);
    std::exit(invalid_opt);
}

static void modules_list()
{
    fmt::println(R"(
Syntax:
# maybe comments of the module
module
  member	: description [example of what it prints, maybe another]

Should be used in the config as like as $<module.member>
NOTE: there are modules such as "user.de_version" that may slow down cufetch because of querying things like the DE version
      cufetch is still fast tho :)

os
  name		: OS name (pretty_name) [Ubuntu 22.04.4 LTS, Arch Linux]
  kernel	: kernel name and version [Linux 6.9.3-zen1-1-zen]
  kernel_name	: kernel name [Linux]
  kernel_version: kernel version [6.9.3-zen1-1-zen]
  version_id	: OS version id [22.04.4, 20240101.0.204074]
  version_codename: OS version codename [jammy]
  pkgs		: the count of the installed packages by a package manager [1869 (pacman), 4 (flatpak)]
  uptime	: (auto) uptime of the system [36 mins, 3 hours, 23 days]
  uptime_secs	: uptime of the system in seconds (should be used along with others uptime_ members) [45]
  uptime_mins   : uptime of the system in minutes (should be used along with others uptime_ members) [12]
  uptime_hours  : uptime of the system in hours   (should be used along with others uptime_ members) [34]
  uptime_days	: uptime of the system in days    (should be used along with others uptime_ members) [2]
  hostname	: hostname of the OS [mymainPC]
  initsys_name	: Init system name [systemd]
  initsys_version: Init system version [256.5-1-arch]

# you may ask, why is there a sep_title but no title???
# well, it's kinda a "bug" or "regression" in my spaghetti code.
# It has more to do with coloring than actually implementing it.
# I won't rework the whole codebase for one single line,
# and it's already written in the default config
user
  sep_title	: the separator between the title and the system infos (with the title lenght) [--------]
  name		: name you are currently logged in (not real name) [toni69]
  shell		: login shell name and version [zsh 5.9]
  shell_name	: login shell [zsh]
  shell_path	: login shell (with path) [/bin/zsh]
  shell_version : login shell version (may be not correct) [5.9]
  de_name	: Desktop Enviroment current session name [Plasma]
  wm_name	: Windows manager current session name [dwm, xfwm4]
  term		: Terminal name and version [alacritty 0.13.2]
  term_name	: Terminal name [alacritty]
  term_version	: Terminal version [0.13.2]

# this module is just for generic theme stuff
# such as indeed cursor
# because it is not GTK-Qt specific
theme
  cursor	: cursor name [Bibata-Modern-Ice]
  cursor_size	: cursor size [16]

# the N stands for the gtk version number to query
# so for example if you want to query the gtk3 theme version
# write it like "theme.gtk3"
# note: they may be inaccurate if didn't find anything in the config files
# 	thus because of using as last resort the `gsettings` exacutable
theme-gtkN
  name		: gtk theme name [Arc-Dark]
  icons		: gtk icons theme name [Qogir-Dark]
  font		: gtk font theme name [Noto Sans 10]

# basically as like as the "theme-gtkN" module above
# but with gtk{{2,3,4}} and auto format gkt version
# note: may be slow because of calling "gsettings" if couldn't read from configs
theme-gtk-all
  name          : gtk theme name [Decay-Green [GTK2], Arc-Dark [GTK3/4]]
  icons         : gtk icons theme name [Papirus-Dark [GTK2/3], Qogir [GTK4]]
  font          : gtk font theme name [Cantarell 10 [GTK2], Noto Sans,  10 [GTK3], Noto Sans 10 [GTK4]]

# note: these members are auto displayed in KiB, MiB, GiB and TiB.
# they all (except ram.ram and ram.swap) have a -KiB, -GiB and -MiB variant
# example: if you want to show your 512MiB of used RAM in GiB
# use the used-GiB variant (they don't print the unit tho)
ram
  ram		: used and total amount of RAM (auto) [2.81 GiB / 15.88 GiB]
  used		: used amount of RAM (auto) [2.81 GiB]
  free		: available amount of RAM (auto) [10.46 GiB]
  total		: total amount of RAM (auto) [15.88 GiB]
  ram           : swapfile used and total amount of RAM (auto) [477.68 MiB / 512.00 MiB]
  swap_free	: swapfile available amount of RAM (auto) [34.32 MiB]
  swap_total	: swapfile total amount of RAM (auto) [512.00 MiB]
  swap_used	: swapfile used amount of RAM (auto) [477.68 MiB]

# same thing as RAM (above)
# note: I mean literally /path/to/fs
#	e.g disk(/)
disk(/path/to/fs)
  disk		: used and total amount of disk space (auto) with type of filesystem [360.02 GiB / 438.08 GiB - ext4]
  used          : used amount of disk space (auto) [360.02 GiB]
  free          : available amount of disk space (auto) [438.08 GiB]
  total         : total amount of disk space (auto) [100.08 GiB]
  fs            : type of filesystem [ext4]

# usually people have 1 GPU in their host,
# but if you got more than 1 and want to query it,
# you should call gpu module with a number, e.g gpu1 (default gpu0).
# Infos are gotten from `/sys/class/drm/` and on each cardN directory
gpu
  name		: GPU model name [NVIDIA GeForce GTX 1650]
  vendor	: GPU vendor (UNSTABLE IDK WHY) [NVIDIA Corporation]

cpu
  cpu		: CPU model name with number of virtual proccessors and max freq [AMD Ryzen 5 5500 (12) @ 4.90 GHz]
  name		: CPU model name [AMD Ryzen 5 5500]
  nproc         : CPU number of virtual proccessors [12]
  freq_bios_limit: CPU freq (limited by bios, in GHz) [4.32]
  freq_cur	: CPU freq (current, in GHz) [3.42]
  freq_min	: CPU freq (mininum, in GHz) [2.45]
  freq_max	: CPU freq (maxinum, in GHz) [4.90]

system
  host		: Host (aka. Motherboard) model name with vendor and version [Micro-Star International Co., Ltd. PRO B550M-P GEN3 (MS-7D95) 1.0]
  host_name	: Host (aka. Motherboard) model name [PRO B550M-P GEN3 (MS-7D95)]
  host_version	: Host (aka. Motherboard) model version [1.0]
  host_vendor	: Host (aka. Motherboard) model vendor [Micro-Star International Co., Ltd.]
  arch          : the architecture of the machine [x86_64, aarch64]

)"sv);
    std::exit(EXIT_SUCCESS);
}

// clang-format off
// parseargs() but only for parsing the user config path trough args
// and so we can directly construct Config
static std::string parse_config_path(int argc, char* argv[], const std::string& configDir)
{
    int opt = 0;
    int option_index = 0;
    opterr = 0;
    const char *optstring = "-C:";
    static const struct option opts[] =
    {
        {"config", required_argument, 0, 'C'},
        {0,0,0,0}
    };

    while ((opt = getopt_long(argc, argv, optstring, opts, &option_index)) != -1)
    {
        switch (opt)
        {
            // skip errors or anything else
            case 0:
            case '?':
                break;

            case 'C': 
                if (!std::filesystem::exists(optarg))
                    die("config file '{}' doesn't exist", optarg);

                return optarg;
                break;
        }
    }

    return configDir + "/config.toml";
}

static bool parseargs(int argc, char* argv[], Config& config, const std::string_view configFile)
{
    int opt = 0;
    int option_index = 0;
    opterr = 1; // re-enable since before we disabled for "invalid option" error
    const char *optstring = "-VhnLlga::f:o:C:d:D:s:";
    static const struct option opts[] =
    {
        {"version",          no_argument,       0, 'V'},
        {"help",             no_argument,       0, 'h'},
        {"no-display",       no_argument,       0, 'n'},
        {"list-modules",     no_argument,       0, 'l'},
        {"logo-only",        no_argument,       0, 'L'},
        {"gui",              no_argument,       0, 'g'},
        {"ascii-logo-type",  optional_argument, 0, 'a'},
        {"offset",           required_argument, 0, 'o'},
        {"font",             required_argument, 0, 'f'},
        {"config",           required_argument, 0, 'C'},
        {"data-dir",         required_argument, 0, 'D'},
        {"distro",           required_argument, 0, 'd'},
        {"source-path",      required_argument, 0, 's'},

        {"sep-reset",          required_argument, 0, "sep-reset"_fnv1a16},
        {"sep-title",          required_argument, 0, "sep-title"_fnv1a16},
        {"sep-reset-after",    optional_argument, 0, "sep-reset-after"_fnv1a16},
        {"logo-padding-top",   required_argument, 0, "logo-padding-top"_fnv1a16},
        {"logo-padding-left",  required_argument, 0, "logo-padding-left"_fnv1a16},
        {"layout-padding-top", required_argument, 0, "layout-padding-top"_fnv1a16},
        {"bg-image",           required_argument, 0, "bg-image"_fnv1a16},
        {"color",              required_argument, 0, "color"_fnv1a16},
        {"gen-config",         optional_argument, 0, "gen-config"_fnv1a16},
        
        {0,0,0,0}
    };

    /* parse operation */
    optind = 0;
    while ((opt = getopt_long(argc, argv, optstring, opts, &option_index)) != -1)
    {
        switch (opt)
        {
            case 0:
                break;
            case '?':
                help(EXIT_FAILURE); break;

            case 'V':
                version(); break;
            case 'h':
                help(); break;
            case 'n':
                config.m_disable_source = true; break;
            case 'l':
                modules_list(); break;
            case 'f':
                config.font = optarg; break;
            case 'L':
                config.m_print_logo_only = true; break;
            case 'g':
                config.gui = true; break;
            case 'o':
                config.offset = std::atoi(optarg); break;
            case 'C': // we have already did it in parse_config_path()
                break;
            case 'D':
                config.data_dir = optarg; break;
            case 'd':
                config.m_custom_distro = str_tolower(optarg); break;
            case 's':
                config.source_path = optarg; break;
            case 'a':
                if (OPTIONAL_ARGUMENT_IS_PRESENT)
                    config.ascii_logo_type = optarg;
                else
                    config.ascii_logo_type.clear();
                break;

            case "logo-padding-top"_fnv1a16:
                config.logo_padding_top = std::atoi(optarg); break;

            case "logo-padding-left"_fnv1a16:
                config.logo_padding_left = std::atoi(optarg); break;

            case "layout-padding-top"_fnv1a16:
                config.layout_padding_top = std::atoi(optarg); break;

            case "bg-image"_fnv1a16:
                config.gui_bg_image = optarg; break;

            case "color"_fnv1a16:
            {
                const std::string& optarg_str = optarg;
                const size_t& pos = optarg_str.find('=');
                if (pos == std::string::npos)
                    die("argument color '{}' does NOT have an equal sign '=' for separiting color name and value.\n"
                        "for more check with --help", optarg_str);

                const std::string& name = optarg_str.substr(0, pos);
                const std::string& value = optarg_str.substr(pos + 1);
                config.m_arg_colors_name.push_back(name);
                config.m_arg_colors_value.push_back(value);
            }
            break;

            case "gen-config"_fnv1a16:
                if (OPTIONAL_ARGUMENT_IS_PRESENT)
                    config.generateConfig(optarg);
                else
                    config.generateConfig(configFile);
                exit(EXIT_SUCCESS);

            case "sep-reset"_fnv1a16:
                config.sep_reset = optarg; break;

            case "sep-title"_fnv1a16:
                config.user_sep_title = optarg; break;

            case "sep-reset-after"_fnv1a16:
                if (OPTIONAL_ARGUMENT_IS_PRESENT)
                    config.sep_reset_after = std::stoi(optarg);
                else
                    config.sep_reset_after = true;
                break;

            default:
                return false;
        }
    }

    return true;
}

int main (int argc, char *argv[]) {
#ifdef PARSER_TEST
    // test
    fmt::println("=== PARSER TEST! ===");

    std::string test_1 = "Hello, World!";
    std::string test_2 = "Hello, $(echo \"World\")!";
    std::string test_3 = "Hello, \\$(echo \"World\")!";
    std::string test_4 = "Hello, $\\(echo \"World\")!";
    std::string test_5 = "Hello, \\\\$(echo \"World\")!";
    std::string test_6 = "$(echo \"World\")!";
    systemInfo_t systemInfo;
    std::unique_ptr<std::string> pureOutput = std::make_unique<std::string>();
    std::string clr = "#d3dae3";

    fmt::print("Useless string (input: {}): ", test_1);
    parse(test_1, systemInfo, pureOutput, clr);
    fmt::println("\t{}", test_1);
    
    fmt::print("Exec string (input: {}): ", test_2);
    parse(test_2, systemInfo, pureOutput, clr);
    fmt::println("\t{}", test_2);
    
    fmt::print("Bypassed exec string #1 (input: {}): ", test_3);
    parse(test_3, systemInfo, pureOutput, clr);
    fmt::println("\t{}", test_3);
    
    fmt::print("Bypassed exec string #2 (input: {}): ", test_4);
    parse(test_4, systemInfo, pureOutput, clr);
    fmt::println("\t{}", test_4);
    
    fmt::print("Escaped backslash before exec string (input: {}): ", test_5);
    parse(test_5, systemInfo, pureOutput, clr);
    fmt::println("\t{}", test_5);
    
    fmt::print("Exec string at start of the string (input: {}): ", test_6);
    parse(test_6, systemInfo, pureOutput, clr);
    fmt::println("\t{}", test_6);
#endif

#ifdef VENDOR_TEST
    // test
    fmt::println("=== VENDOR TEST! ===");

    fmt::println("Intel: {}", binarySearchPCIArray("8086"));
    fmt::println("AMD: {}", binarySearchPCIArray("1002"));
    fmt::println("NVIDIA: {}", binarySearchPCIArray("10de"));
#endif

#ifdef DEVICE_TEST
    // test
    fmt::println("=== DEVICE TEST! ===");

    fmt::println("an Intel iGPU: {}", binarySearchPCIArray("8086", "0f31"));
    fmt::println("RX 7700 XT: {}", binarySearchPCIArray("1002", "747e"));
    fmt::println("GTX 1650: {}", binarySearchPCIArray("10de", "1f0a"));
    fmt::println("?: {}", binarySearchPCIArray("1414", "0006"));
#endif

    // clang-format on
    colors_t colors;

    const std::string& configDir  = getConfigDir();
    const std::string& configFile = parse_config_path(argc, argv, configDir);

    Config config(configFile, configDir, colors);

    if (!parseargs(argc, argv, config, configFile))
        return 1;

    if (config.source_path.empty() || config.source_path == "off")
        config.m_disable_source = true;

    config.m_display_distro = (config.source_path == "os");

    std::string path = config.m_display_distro ? Display::detect_distro(config) : config.source_path;

    if (!config.ascii_logo_type.empty())
    {
        const size_t& pos = path.rfind('.');
        if (pos != std::string::npos)
            path.insert(pos, "_" + config.ascii_logo_type);
        else
            path += "_" + config.ascii_logo_type;
    }

    if (!std::filesystem::exists(path) &&
        !std::filesystem::exists((path = config.data_dir + "/ascii/linux.txt")))
        die("'{}' doesn't exist. Can't load image/text file", path);

#ifdef GUI_MODE
    if (config.gui)
    {
        const Glib::RefPtr<Gtk::Application>& app = Gtk::Application::create("org.toni.customfetch");
        GUI::Window window(config, colors, path);
        return app->run(window);
    }
#else
    if (config.gui)
        die("Can't run in GUI mode because it got disabled at compile time\n"
            "Compile customfetch with GUI_MODE=1 or contact your distro to enable it");
#endif

    Display::display(Display::render(config, colors, false, path));

    return 0;
}
