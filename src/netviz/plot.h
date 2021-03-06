#ifndef PLOT_H
#define PLOT_H

#include "config.h"

#ifdef HAVE_LIBCAIRO

#ifdef HAVE_CAIRO_H
#include <cairo.h>
#elif defined HAVE_CAIRO_CAIRO_H
#include <cairo/cairo.h>
#endif
#ifdef HAVE_CAIRO_PDF_H
#include <cairo-pdf.h>
#elif defined HAVE_CAIRO_CAIRO_PDF_H
#include <cairo/cairo-pdf.h>
#endif

#include <vector>
#include <string>
#include <math.h>

class plot {
public:
    plot() :
        filename("graph"), title("graph of things"), subtitle("x vs y"),
        x_label("x axis"), y_label("y axis"), width(161.803), height(100.000),
        title_on_bottom(false), title_font_size(8.0), x_axis_font_size(8.0),
        y_axis_font_size(8.0), title_max_width_ratio(0.8),
        title_y_pad_factor(2.0), subtitle_y_pad_factor(0.2),
        subtitle_font_size_factor(0.4), axis_thickness_factor(0.002),
        tick_length_factor(0.0124), tick_width_factor(0.002),
        x_tick_label_pad_factor(4.0), y_tick_label_pad_factor(2.0),
        y_tick_font_size(3.0), x_tick_font_size(3.0), pad_left_factor(0.148),
        pad_top_factor(0.2), pad_bottom_factor(0.2), pad_right_factor(0.148),
        legend_chip_factor(1.2), legend_font_size(2.5),
        x_axis_decoration(AXIS_NO_DECO), y_axis_decoration(AXIS_NO_DECO) {}

    typedef enum {
        AXIS_NO_DECO = 0, AXIS_SPAN_ARROW, AXIS_SPAN_STOP
    } axis_decoration_t;

    class rgb_t {
    public:
        rgb_t() : r(0.0), g(0.0), b(0.0) {}
        rgb_t(const double r_, const double g_, const double b_) :
        r(r_), g(g_), b(b_) {}
        double r;
        double g;
        double b;
        static const double epsilon = 1.0 / 256.0;
    };

    class ticks_t {
    public:
        ticks_t() :
        x_labels(), y_labels() {}
        std::vector<std::string> x_labels;
        std::vector<std::string> y_labels;
    };

    class labels_t {
    public:
        labels_t() :
            x_label(), y_label() {}
        std::string x_label;
        std::string y_label;
    };

    class legend_entry_t {
    public:
        legend_entry_t(const rgb_t color_, const std::string label_) :
        color(color_), label(label_) {}
        rgb_t color;
        std::string label;
    };
    typedef std::vector<legend_entry_t> legend_t;

    class bounds_t {
    public:
        bounds_t() :
            x(0.0), y(0.0), width(0.0), height(0.0) {}
        bounds_t(const double x_, const double y_, const double width_,
                const double height_) :
            x(x_), y(y_), width(width_), height(height_) {}
        double x;
        double y;
        double width;
        double height;
    };

    std::string filename;
    std::string title;
    std::string subtitle;
    std::string x_label;
    std::string y_label;
    // width and height are in pt
    double width;
    double height;
    bool title_on_bottom;
    double title_font_size;
    double x_axis_font_size;
    double y_axis_font_size;
    // Title text will be shrunk if needed such that it takes up no more
    // than this ratio of the image width
    double title_max_width_ratio;
    // multiple of title height to be allocated above graph
    double title_y_pad_factor;
    // multiple of the subtitle height that will separate the subtitle from
    // the title
    double subtitle_y_pad_factor;
    // multiple of the title font size for the subtitle font size
    double subtitle_font_size_factor;
    // axis scale
    double axis_thickness_factor;
    // size of scale ticks, in pt
    double tick_length_factor;
    double tick_width_factor;
    // multiple of label dummy text length to allocate for spacing
    double x_tick_label_pad_factor;
    double y_tick_label_pad_factor;
    double y_tick_font_size;
    double x_tick_font_size;
    // non-dynamic padding for the right and bottom of graph
    double pad_left_factor;
    double pad_top_factor;
    double pad_bottom_factor;
    double pad_right_factor;
    // legend
    double legend_chip_factor;
    double legend_font_size;
    // axis decoration
    axis_decoration_t x_axis_decoration;
    axis_decoration_t y_axis_decoration;

    void render(cairo_t *cr, const bounds_t &bounds,
            const ticks_t &ticks, const legend_t &legend,
            bounds_t &content_bounds);

    // constants
    static const double text_line_base_width = 0.05;
    static const double span_arrow_angle = M_PI / 4.0;
    static const double span_stop_angle = M_PI / 2.0;
};

inline bool operator==(const plot::rgb_t &a, const plot::rgb_t &b)
{
    return fabs(a.r - b.r) < plot::rgb_t::epsilon &&
        fabs(a.g - b.g) < plot::rgb_t::epsilon &&
        fabs(a.b - b.b) < plot::rgb_t::epsilon;
}

#endif
#endif
