/*
 * Print color quadrant
 *
 */
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "colorUtils.h"
#include "colorPrint.h"

#define HUE_MAX   360.0
#define HUE_MIN   0.0
#define HUE_DEF   180.0
#define SAT_MAX   1.0
#define SAT_MIN   0.0
#define SAT_DEF   0.6
#define LUM_MAX   1.0
#define LUM_MIN   0.0
#define LUM_DEF   0.5
#define RED_DEF   0
#define GREEN_DEF 0
#define BLUE_DEF  0
#define PRINT_COLOR_W    8
#define TOP_HEADER_BG    255, 255, 255
#define TOP_HEADER_FG    0, 0, 255
#define BOTTOM_HEADER_BG 255, 255, 255
#define BOTTOM_HEADER_FG 0, 0, 255
#define LEFT_HEADER_BG   255, 255, 255
#define LEFT_HEADER_FG   0, 0, 255
#define RIGHT_HEADER_BG  255, 255, 255
#define RIGHT_HEADER_FG  0, 0, 255
#define GRABC            "grabc"
#define PERR(args...)  do { printf(args); return 1; } while(0)


enum ColorMode { NO_MODE, RED, GREEN, BLUE, HUE, SAT, LUM };
enum Action    { NO_ACTION, SHOW, PRINT_RGB, PRINT_HSL, GRAB_COLOR };
static struct winsize w;

#define GETI(DESC, V, MIN, MAX) do {                                  \
    char *endp;                                                       \
    if (++i >= ac)                                                    \
        PERR("After switch \"%s\" %s is expected.\n", av[--i], DESC); \
    V = strtol(av[i], &endp, 0);                                      \
    if (*endp)                                                        \
        PERR("\"%s\" is invalid %s\n", av[i], DESC);                  \
    if (V < MIN || V > MAX)                                           \
        PERR("%d is a wrong %s value, must be between %d and %d\n",   \
             V, DESC, MIN, MAX);                                      \
    } while(0)

#define GETD(DESC, V, MIN, MAX) do {                                    \
        char *endp;                                                     \
    if (++arg_idx >= ac)                                                \
        PERR("After switch \"%s\" %s is expected.\n", av[--arg_idx], DESC); \
    V = strtod(av[arg_idx], &endp);                                     \
    if (*endp)                                                          \
        PERR("\"%s\" is invalid %s\n", av[arg_idx], DESC);              \
    if (V < MIN || V > MAX)                                             \
        PERR("%g is a wrong %s value, must be between %g and %g\n",     \
             V, DESC, MIN, MAX);                                        \
    } while(0)

#define GET_HEX(DESC, V, B) do {                 \
    char *endp;                                  \
    char s[3];                                   \
    s[0] = (B)[0];                               \
    s[1] = (B)[1];                               \
    s[2] = 0;                                    \
    V = strtol(s, &endp, 16);                    \
    if (*endp)                                   \
        PERR("\"%s\" is invalid %s\n", s, DESC); \
    } while(0)


static void usage(const char *s, int def_cols, int def_rows)
{
    const char *msg =
        "Usage:\n\n\t%s <COMMAND> <COMMAND_PARAMS> [SWITCHES]\n"
        "\nCommand are\n"
        "\nshow    - Draw slice of HSL cylinder or RGB cube, see comands parameters below:\n"
        "    h HUE   - Print this hue with different saturation/luminosity, must be between %g - %g\n"
        "    s SAT   - Print this saturation with different hue/luminosity, must be between %g - %g or 0%% - 100%%\n"
        "    l LUM   - Print this luminosity with different hue/saturation, must be between %g - %g or 0%% - 100%%\n"
        "    r RED   - Print green/blue matrix with specified red level, must be between 0-255 (0x00-0xff)\n"
        "    g GREEN - Print red/blue matrix with specified green level, must be between 0-255 (0x00-0xff)\n"
        "    b BLUE  - Print red/blue matrix with specified blue level, must be between 0-255 (0x00-0xff)\n"
        "\nrgb     - Show info about specified color in RGB format, command params may be one of the following:\n"
        "    #XXXXXX - Where X is a hexa character\n"
        "    XXXXXX - Where X is a hexa character\n"
        "    R G B   - Three values for R, G and B, values must be between 0 - 255\n"
        "        May be integer, hexa (with a 0x prefix), float or percentage\n"
        "\nhsl     - Show info about specified color in HSL format, command params are:\n"
        "    H S L   - Three values for hue, saturation and luminosity\n"
        "        Hue value must be between 0 - 360, may be either integer or float\n"
        "        saturation and luminosity values may be float (between 0.0 - 1.0)\n"
        "        or percentage between 0%% and 100%%\n"
        "\ngrabc   - Using external \"grabc\" command to grab the color and then prints the color info\n"
        "\nSwitches are:\n"
        "    -h       - print this message\n"
        "    -q       - Quiet mode, print colors only without additional info.\n"
        "    -c COLS  - Amount of columns use for the print, default is %d\n"
        "    -r ROWS  - Amount of rows use for the print, default is %d\n"
        "    -g       - Grab color and print its info after \"show\" command. Note that \"grabc\" external command\n"
        "               should be installed, to install it use \"apt install grabc\" in case of debian based systems\n"
        "    -G       - Same as \"-g\" but grab the color in an indefinite loop\n"
        "\nExamples:\n"
        "    %s show s 0.7\n"
        "        Draw color rectangle with saturation 0.7 and different hue and luminosity on a full screen\n"
        "    %s show l 30%% -c 40 -r 25\n"
        "        Draw color rectangle with luminosity 0.3 and different hue and saturation\n"
        "        using a rectangle with 40 columns and 25 rows\n"
        "    %s show r 0x80 -q -g\n"
        "        Draw color rectangle with red level 0.5 and different green and blue on a full screen\n"
        "        without decoration. Grab color with mouse and print its info after the action\n"
        "    %s rgb 407090\n"
        "        Show info about the #407090 color\n"
        "    %s hsl 180 0.3 60%\n"
        "        Show info about the color with hue 180, saturation 0.3 and luminosity 0.6\n"
        ;
    printf(msg, s, def_cols, def_rows, HUE_MIN, HUE_MAX, SAT_MIN, SAT_MAX, LUM_MIN, LUM_MAX,
           s, s, s, s, s);
    exit(1);
}

/*
 * grab color
 */
bool grab_color(int& red, int& green, int& blue)
{
    (void)red;
    (void)green;
    (void)blue;
    const int BSIZE = 512;
    std::string reply;

    char cmd[strlen(GRABC) + 8];
    sprintf(cmd, "%s 2>&1", GRABC);
    FILE *f = popen(cmd, "r");
    if (f) {
        static char b[BSIZE];
        while (fgets(b, BSIZE-1, f))
            reply.append(b);
        pclose(f);
        size_t n = reply.find_last_not_of(" \n\r\t");
        if (n != std::string::npos)
            reply.erase(n+1);
        n = reply.find_first_of("\n\r");
        if (n != std::string::npos)
            reply.erase(n);

        bool ok = true;
        if (reply.length() != 7 ||  reply[0] != '#')
            ok = false;
        else {
            for (unsigned i=1; i<7; i++)
                if (!isxdigit(reply[i]))
                    ok = false;
        }

        if (ok) {
            GET_HEX("red",   red, &reply.c_str()[1]);
            GET_HEX("green", green, &reply.c_str()[3]);
            GET_HEX("blue",  blue, &reply.c_str()[5]);
            return true;
        } else {
            printf("\"%s\" error: %s\n", GRABC, reply.c_str());
            return false;
        }
    }
    printf("Can't run \"%s\": %s\n", GRABC, strerror(errno));
    return false;
}

/*
 * show detail of the specified color
 */
void show_color_details(double red, double green, double blue,
                        double hue, double saturation, double luminosity)
{
    printf("\n\n  ");

    // Hexa
    rgb_bg(red, green, blue);
    printf("%*s", PRINT_COLOR_W, " ");
    normal();
    printf("  Hexa: #%02X%02X%02X\n", (int)round(red), (int)round(green), (int)round(blue));

    // RGB
    printf("  ");
    rgb_bg(red, green, blue);
    printf("%*s", PRINT_COLOR_W, " ");
    normal();
    printf("  RGB:  %d %d %d\n", (int)round(red), (int)round(green), (int)round(blue));

    // HSL
    printf("  ");
    rgb_bg(red, green, blue);
    printf("%*s", PRINT_COLOR_W, " ");
    normal();
    printf("  HSL:  %g %g %g\n", hue, saturation, luminosity);

    // HSL in %%
    printf("  ");
    rgb_bg(red, green, blue);
    printf("%*s", PRINT_COLOR_W, " ");
    normal();
    printf("        %d %d%% %d%%\n", (int)round(hue), (int)round(saturation * 100.0), (int)round(luminosity * 100.0));

    printf("\n");
}
void show_rgb_color_details(double red, double green, double blue)
{
    double hue, saturation, luminosity;
    RGBToHSL(red, green, blue, hue, saturation, luminosity);
    show_color_details(red, green, blue, hue, saturation, luminosity);
}

void show_hsl_color_details(double hue, double saturation, double luminosity)
{
    double red, green, blue;
    HSLToRGB(hue, saturation, luminosity, red, green, blue);
    show_color_details(red, green, blue, hue, saturation, luminosity);
}

/*
 * Draw color rectangle
 */
void draw_color_rect(ColorMode color_mode, double hue, double saturation, double luminosity,
                     int red, int green, int blue, int rows, int columns, int width_original,
                     bool quiet)
{
    // Header
    std::vector<std::string> left_header;
    std::vector<std::string> right_header;
    std::string top_header;
    int top_margin = 0;
    int left_maxlen = 0;
    int left_idx = 0;
    int right_idx = 0;
    int left_start = 0;
    int right_start = 0;
    if (!quiet) {
        // Top and left headers
        rows -= 4;
        switch(color_mode) {
        case NO_MODE:
            return;
        case RED:
            top_header = "Green:  0 --> 255";
            left_header.emplace_back("BLU");
            break;
        case GREEN:
            top_header = "Red:  0 --> 255";
            left_header.emplace_back("BLU");
            break;
        case BLUE:
            top_header = "Red:  0 --> 255";
            left_header.emplace_back("GRN");
            break;
        case HUE:
            top_header = "Saturation:  0.0 --> 1.0";
            left_header.emplace_back("LUM");
            break;
        case SAT:
            top_header = "Hue:  0.0 --> 360.0";
            left_header.emplace_back("LUM");
            break;
        case LUM:
            top_header = "Hue:  0.0 --> 360.0";
            left_header.emplace_back("SAT");
            break;
        }
        top_margin = (width_original - top_header.length()) / 2;

        // More left header
        if (color_mode == HUE || color_mode == SAT || color_mode == LUM) {
            left_header.emplace_back("");
            left_header.emplace_back("0.0");
            left_header.emplace_back("");
            left_header.emplace_back(" |");
            left_header.emplace_back(" |");
            left_header.emplace_back(" V");
            left_header.emplace_back("");
            left_header.emplace_back("1.0");
        } else {
            left_header.emplace_back("");
            left_header.emplace_back(" 0");
            left_header.emplace_back("");
            left_header.emplace_back(" |");
            left_header.emplace_back(" |");
            left_header.emplace_back(" V");
            left_header.emplace_back("");
            left_header.emplace_back("255");
        }
        right_header.emplace_back("|");
        right_header.emplace_back("|");
        right_header.emplace_back("|");
        right_header.emplace_back("|");
        right_header.emplace_back("|");
        right_header.emplace_back("|");
        right_header.emplace_back("|");
        right_header.emplace_back("|");
        right_header.emplace_back("|");
        right_header.emplace_back("V");
        right_start = (rows - right_header.size()) / 2;

        // Maximal lenght of left header
        for (auto i : left_header)
            if ((int)i.length() > left_maxlen)
                left_maxlen = i.length();
        columns -= left_maxlen + 1;
        left_start = (rows - left_header.size()) / 2;

        // Print top header
        rgb_bg(TOP_HEADER_BG);
        rgb_fg(TOP_HEADER_FG);
        printf("%*s%s%*s", top_margin, " ", top_header.c_str(), width_original - top_margin - (int)top_header.length(), " ");

    }

    // Draw left header of the second line
    normal();
    printf("\n");
    if (!quiet) {
        rgb_bg(LEFT_HEADER_BG);
        rgb_fg(LEFT_HEADER_FG);
        printf("%*s", left_maxlen, " ");
        normal();
        printf("%*s", width_original - left_maxlen - 1, " ");
        rgb_bg(RIGHT_HEADER_BG);
        rgb_fg(RIGHT_HEADER_FG);
        printf(" \n");
        normal();
    }

    // Main loop by rows/columns
    for (int y = 0; y < rows; y++) {
        if (!quiet) {
            // Left header
            rgb_bg(LEFT_HEADER_BG);
            rgb_fg(LEFT_HEADER_FG);
            if (y < left_start || left_idx >= (int)left_header.size())
                printf("%*s", left_maxlen, " ");
            else {
                int pad = left_maxlen - (int)left_header[left_idx].length();
                if (pad)
                    printf("%s%*s", left_header[left_idx].c_str(), pad, " ");
                else
                    printf("%s", left_header[left_idx].c_str());
                left_idx++;
            }
            normal();
        }
        printf(" ");
        for (int x = 0; x < columns; x++) {
            double rx = (double)x / columns;
            double ry = (double)y / rows;
            double h = 0.0;
            double s = 0.0;
            double l = 0.0;
            double r = 0;
            double g = 0;
            double b = 0;

            switch(color_mode) {
            case NO_MODE:
                return;
            case RED:
                r = red;
                g = 255.0 * rx;
                b = 255.0 * ry;
                rgb_bg(r, g, b);
                break;
            case GREEN:
                r = 255.0 * rx;
                g = green;
                b = 255.0 * ry;
                rgb_bg(r, g, b);
                break;
            case BLUE:
                r = 255.0 * rx;
                g = 255.0 * ry;
                b = blue;
                rgb_bg(r, g, b);
                break;
            case HUE:
                h = hue;
                s = rx;
                l = ry;
                hsl_bg(h, s, l);
                break;
            case SAT:
                h = 360.0 * rx;
                s = saturation;
                l = ry;
                hsl_bg(h, s, l);
                break;
            case LUM:
                h = 360.0 * rx;
                s = ry;
                l = luminosity;
                hsl_bg(h, s, l);
                break;
            }

            printf(" ");
            normal();
        }
        // Right header
        if (quiet) {
            printf(" ");
            normal();
        } else {
            normal();
            printf(" ");

            rgb_bg(RIGHT_HEADER_BG);
            rgb_fg(RIGHT_HEADER_FG);

            if (y < right_start || right_idx >= (int)right_header.size())
                printf(" ");
            else
                printf("%s", right_header[right_idx++].c_str());

            normal();
        }
        if (width_original < w.ws_col)
            printf("%*s", w.ws_col - width_original, " ");
        printf("\n");
    }

    if (!quiet) {
        // Footer
        rgb_bg(LEFT_HEADER_BG);
        rgb_fg(LEFT_HEADER_FG);
        printf("%*s", left_maxlen, " ");
        normal();
        printf("%*s", width_original - left_maxlen - 1, " ");
        rgb_bg(RIGHT_HEADER_BG);
        rgb_fg(RIGHT_HEADER_FG);
        printf(" ");
        normal();
        printf("\n");

        rgb_bg(BOTTOM_HEADER_BG);
        rgb_fg(BOTTOM_HEADER_FG);
        printf("%*s%s%*s", top_margin, " ", top_header.c_str(), width_original - top_margin - (int)top_header.length(), " ");
    }
    normal();
    printf("\n");
}


int get_int_perc(const char *descr, const char *s, int &v, int min, int max)
{
    char *endp;
    v = strtol(s, &endp, 0);
    if (!*endp) {
        if (v < min || v > max)
            PERR("%d is a wrong %s value, must be between %d and %d\n", v, descr, min, max);
        return 0;
    }
    if (strcmp(endp, "%"))
        PERR("\"%s\" is invalid %s\n", s, descr);
    if (v < 0 || v > 100)
        PERR("%d is a wrong %s percentage, must be between 0 - 100\n", v, descr);
    v = (int)round((((float)max - (float)min) * v / 100.0) + (float)min);
    return 0;
}

int get_double_perc(const char *descr, const char *s, double &v, double min, double max)
{
    char *endp;
    v = strtod(s, &endp);
    if (!*endp) {
        if (v < min || v > max)
            PERR("%g is a wrong %s value, must be between %g and %g\n", v, descr, min, max);
        return 0;
    }
    int perc = strtol(s, &endp, 0);
    if (strcmp(endp, "%"))
        PERR("\"%s\" is invalid %s\n", s, descr);
    if (perc < 0 || perc > 100)
        PERR("%d is a wrong %s percentage, must be between 0 - 100\n", perc, descr);
    v = ((max - min) * perc / 100.0) + min;
    return 0;
}

bool starts_with(const std::string& str, const std::string& prefix)
{
    return str.find(prefix) == 0;
}

int main(int ac, char *av[])
{
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) < 0)
        PERR("Can't ioctl to get terminal size: %s\n", strerror(errno));

    bool quiet = false;
    bool grabc_after_show = false;
    bool grabc_forever = false;
    int columns = w.ws_col - 2;
    int width_original = w.ws_col;
    int rows = w.ws_row - 4;
    double hue = HUE_DEF;
    double saturation = SAT_DEF;
    double luminosity = LUM_DEF;
    int    red = RED_DEF;
    int    green = GREEN_DEF;
    int    blue = BLUE_DEF;
    int    arg_idx = 1;

    ColorMode color_mode = NO_MODE;
    Action    act = NO_ACTION;

    if (ac < 2)
        usage(av[0], w.ws_col - 2, w.ws_row - 4);

    const char *cmd = av[arg_idx++];
    if (starts_with("show", cmd)) {
        if (ac < 4)
            usage(av[0], w.ws_col - 2, w.ws_row - 4);
        act = SHOW;
        const char *sub1 = av[arg_idx];
        switch (*sub1) {
        case 'r':
        case 'R':
            if (++arg_idx >= ac)
                PERR("After switch \"%s\" red level is expected.\n", av[--arg_idx]);
            if (get_int_perc("Red level", av[arg_idx], red, 0, 255))
                return 1;
            color_mode = RED;
            break;
        case 'g':
        case 'G':
            if (++arg_idx >= ac)
                PERR("After switch \"%s\" gree level is expected.\n", av[--arg_idx]);
            if (get_int_perc("Green level", av[arg_idx], green, 0, 255))
                return 1;
            color_mode = GREEN;
            break;
        case 'b':
        case 'B':
            if (++arg_idx >= ac)
                PERR("After switch \"%s\" blue level is expected.\n", av[--arg_idx]);
            if (get_int_perc("Blue level", av[arg_idx], blue, 0, 255))
                return 1;
            color_mode = BLUE;
            break;
        case 'h':
        case 'H':
            GETD("hue value", hue, HUE_MIN, HUE_MAX);
            color_mode = HUE;
            break;
        case 's':
        case 'S':
            if (++arg_idx >= ac)
                PERR("After switch \"%s\" saturation level is expected.\n", av[--arg_idx]);
            if (get_double_perc("Saturation level", av[arg_idx], saturation, 0.0, 1.0))
                return 1;
            color_mode = SAT;
            break;
        case 'l':
        case 'L':
            if (++arg_idx >= ac)
                PERR("After switch \"%s\" luminosity level is expected.\n", av[--arg_idx]);
            if (get_double_perc("Luminosity level", av[arg_idx], luminosity, 0.0, 1.0))
                return 1;
            color_mode = LUM;
            break;
        default:
            PERR("Invalid color specifications for \"show\" command\n");
        }

    } else if (starts_with("rgb", cmd)) {
        if (ac < 3)
            usage(av[0], w.ws_col - 2, w.ws_row - 4);
        act = PRINT_RGB;
        if (ac - arg_idx == 1) {
            const char *color = av[arg_idx];
            int start_ind = (color[0] == '#') ? 1 : 0;
            if (strlen(&color[start_ind]) != 6)
                PERR("%s is a wrong color, should be \"#NNNNNN\"\n", av[1]);
            for (unsigned i=start_ind; i<strlen(&color[start_ind]); i++)
                if (!isxdigit(color[i]))
                    PERR("W%s is a wrong color, should be \"#NNNNNN\"\n", av[1]);
            GET_HEX("red",   red, &color[start_ind]);
            GET_HEX("green", green, &color[start_ind+2]);
            GET_HEX("blue",  blue, &color[start_ind+4]);
        } else if (ac - arg_idx >= 3) {
            if (get_int_perc("Red level", av[arg_idx++], red, 0, 255))
                return 1;
            if (get_int_perc("Green level", av[arg_idx++], green, 0, 255))
                return 1;
            if (get_int_perc("Blue level", av[arg_idx], blue, 0, 255))
                return 1;
        } else
            PERR("Wrong number of parameters for \"rgb\" command\n");

    } else if (starts_with("hsl", cmd)) {
        act = PRINT_HSL;
        if (ac - arg_idx >= 3) {
            if (get_double_perc("Hue", av[arg_idx++], hue, 0.0, 360.0))
                return 1;
            if (get_double_perc("Saturation", av[arg_idx++], saturation, 0.0, 1.0))
                return 1;
            if (get_double_perc("Luminosity", av[arg_idx++], luminosity, 0.0, 1.0))
                return 1;
        } else
            PERR("Wrong number of parameters for \"hsl\" command\n");
    } else if (starts_with("grabc", cmd))
        act = GRAB_COLOR;
    else
        PERR("\"%s\" is invalid command\n", cmd);

    for (int i=1; i<ac; i++)
    {
        if (*av[i] == '-')
        {
            switch (*++av[i])
            {
            case 'q':
                quiet = true;
                break;
            case 'c':
                GETI("columns amount", columns, 4, w.ws_col - 2);
                width_original = columns;
                columns -= 2;
                break;
            case 'r':
                GETI("rows amount", rows, 4, 999);
                break;
            case 'h':
                usage(av[0], w.ws_col - 2, w.ws_row - 4);
                break;
            case 'g':
                grabc_after_show = true;
                break;
            case 'G':
                grabc_after_show = true;
                grabc_forever = true;
                break;
            default:
                PERR("\"%c\" is invalid flag\n", *av[i]);
            }
        }
    }

    switch (act) {
    case SHOW:
        draw_color_rect(color_mode, hue, saturation, luminosity, red, green, blue,
                        rows, columns, width_original, quiet);
        do {
            if (grabc_after_show && grab_color(red, green, blue))
                show_rgb_color_details(red, green, blue);
        } while (grabc_forever);
        break;
    case PRINT_RGB:
        show_rgb_color_details(red, green, blue);
        break;
    case PRINT_HSL:
        show_hsl_color_details(hue, saturation, luminosity);
        break;
    case GRAB_COLOR:
        do {
            if (grab_color(red, green, blue))
                show_rgb_color_details(red, green, blue);
        } while (grabc_forever);
        break;
    case NO_ACTION:
        break;
    }
}
