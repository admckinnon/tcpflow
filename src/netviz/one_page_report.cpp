/**
 * one_page_report.cpp: 
 * Generate a one-page visualization from TCP packets
 *
 * This source file is public domain, as it is not based on the original tcpflow.
 *
 * Author: Michael Shick <mike@shick.in>
 *
 */

#include "config.h"

#include "plot.h"
#ifdef HAVE_LIBCAIRO
#include "tcpflow.h"
#include "tcpip.h"

#include <ctime>
#include <iomanip>
#include <math.h>

#include "one_page_report.h"

// string constants
const std::string one_page_report::title_version = PACKAGE_NAME " " PACKAGE_VERSION;
const std::vector<std::string> one_page_report::size_suffixes =
        one_page_report::build_size_suffixes();
// ratio constants
const double one_page_report::page_margin_factor = 0.05;
const double one_page_report::line_space_factor = 0.25;
const double one_page_report::histogram_pad_factor_y = 1.0;
const double one_page_report::address_histogram_width_divisor = 2.5;
// size constants
const double one_page_report::bandwidth_histogram_height = 100.0;
const double one_page_report::address_histogram_height = 125.0;
const double one_page_report::port_histogram_height = 100.0;
// color constants
const plot::rgb_t one_page_report::default_color(0.67, 0.67, 0.67);

one_page_report::one_page_report() : 
    source_identifier(), filename("report.pdf"),
    bounds(0.0, 0.0, 611.0, 792.0), header_font_size(8.0),
    top_list_font_size(8.0), histogram_show_top_n_text(3),
    packet_count(0), byte_count(0), earliest(), latest(),
    transport_counts(), bandwidth_histogram(), src_addr_histogram(),
    dst_addr_histogram(), src_port_histogram(), dst_port_histogram(),
    pfall(), src_tree(), dst_tree(), port_aliases(), port_color_map()
{
    earliest = (struct timeval) { 0 };
    latest = (struct timeval) { 0 };

    port_color_map[PORT_HTTP] = plot::rgb_t(0.07, 0.44, 0.87);
    port_color_map[PORT_HTTPS] = plot::rgb_t(0.25, 0.79, 0.40);

    bandwidth_histogram.parent.title = "TCP Packets Received";
    bandwidth_histogram.parent.pad_left_factor = 0.2;
    bandwidth_histogram.parent.y_tick_font_size = 6.0;
    bandwidth_histogram.parent.x_tick_font_size = 6.0;
    bandwidth_histogram.parent.x_axis_font_size = 8.0;
    bandwidth_histogram.parent.x_axis_decoration = plot::AXIS_SPAN_ARROW;
    bandwidth_histogram.colorize(port_color_map[PORT_HTTP], port_color_map[PORT_HTTPS],
            default_color);

    pfall.parent.title = "";
    pfall.parent.subtitle = "";
    pfall.parent.x_label = "";
    pfall.parent.y_label = "";
    pfall.parent.pad_left_factor = 0.2;

    dst_addr_histogram.quick_config("Top Destination Addresses", default_color);
    src_addr_histogram.quick_config("Top Source Addresses", default_color);
    dst_port_histogram.quick_config(port_histogram::DESTINATION, "Top Destination Ports");
    src_port_histogram.quick_config(port_histogram::SOURCE, "Top Source Ports");

    // build null alias map to avoid requiring special handling for unmapped ports
    for(int ii = 0; ii <= 65535; ii++) {
        port_aliases[ii] = ii;
    }
}

void one_page_report::ingest_packet(const be13::packet_info &pi)
{
    if(earliest.tv_sec == 0) {
        earliest = pi.ts;
    }
    if(pi.ts.tv_sec > latest.tv_sec && pi.ts.tv_usec > latest.tv_usec) {
        latest = pi.ts;
    }

    packet_count++;
    byte_count += pi.pcap_hdr->caplen;
    transport_counts[pi.ether_type()]++; // should we handle VLANs?

}

void one_page_report::render(const std::string &outdir)
{
    cairo_t *cr;
    cairo_surface_t *surface;
    std::string fname = outdir + "/" + filename;

    surface = cairo_pdf_surface_create(fname.c_str(),
				 bounds.width,
				 bounds.height);
    cr = cairo_create(surface);

    double pad_size = bounds.width * page_margin_factor;
    plot::bounds_t pad_bounds(bounds.x + pad_size,
            bounds.y + pad_size, bounds.width - pad_size * 2,
            bounds.height - pad_size * 2);
    cairo_translate(cr, pad_bounds.x, pad_bounds.y);

    render_pass pass(*this, cr, pad_bounds);

    pass.render_header();
    pass.render_bandwidth_histogram();
    pass.render_map();
    pass.render_packetfall();
    pass.render_address_histograms();
    pass.render_port_histograms();

    // cleanup
    cairo_destroy (cr);
    cairo_surface_destroy(surface);
}

plot::rgb_t one_page_report::port_color(uint16_t port) const
{
    uint16_t true_port = port;
    std::map<uint16_t, uint16_t>::const_iterator port_it = port_aliases.find(port);
    if(port_it != port_aliases.end()) {
        true_port = port_it->second;
    }

    plot::rgb_t color = default_color;
    std::map<uint16_t, plot::rgb_t>::const_iterator color_it = port_color_map.find(true_port);
    if(color_it != port_color_map.end()) {
        color = color_it->second;
    }

    return color;
}

std::string one_page_report::pretty_byte_total(uint64_t byte_count)
{
    //// packet count/size
    uint64_t size_log_1000 = (uint64_t) (log(byte_count) / log(1000));
    if(size_log_1000 >= size_suffixes.size()) {
        size_log_1000 = 0;
    }
    return ssprintf("%.2f %s", (double) byte_count / pow(1000.0, (double) size_log_1000),
            size_suffixes.at(size_log_1000).c_str());
}

void one_page_report::render_pass::render_header()
{
    std::string formatted;
    // title
    double title_line_space = report.header_font_size * line_space_factor;
    //// version
    render_text_line(title_version, report.header_font_size,
            title_line_space);
    //// input
    formatted = ssprintf("Input: %s", report.source_identifier.c_str());
    render_text_line(formatted.c_str(), report.header_font_size,
            title_line_space);
    //// date generated
    time_t gen_unix = time(0);
    struct tm gen_time = *localtime(&gen_unix);
    formatted = ssprintf("Generated: %04d-%02d-%02d %02d:%02d:%02d",
            1900 + gen_time.tm_year, 1 + gen_time.tm_mon, gen_time.tm_mday,
            gen_time.tm_hour, gen_time.tm_min, gen_time.tm_sec);
    render_text_line(formatted.c_str(), report.header_font_size,
            title_line_space);
    //// trailing pad
    end_of_content += title_line_space * 4;
    // quick stats
    //// date range
    struct tm start = *localtime(&report.earliest.tv_sec);
    struct tm stop = *localtime(&report.latest.tv_sec);
    formatted = ssprintf("Date range: %04d-%02d-%02d %02d:%02d:%02d -- %04d-%02d-%02d %02d:%02d:%02d",
            1900 + start.tm_year, 1 + start.tm_mon, start.tm_mday,
            start.tm_hour, start.tm_min, start.tm_sec,
            1900 + stop.tm_year, 1 + stop.tm_mon, stop.tm_mday,
            stop.tm_hour, stop.tm_min, stop.tm_sec);
    render_text_line(formatted.c_str(), report.header_font_size,
            title_line_space);
    //// packet count/size
    formatted = ssprintf("Packets analyzed: %s (%s)",
            comma_number_string(report.packet_count).c_str(),
            pretty_byte_total(report.byte_count).c_str());
    render_text_line(formatted.c_str(), report.header_font_size,
            title_line_space);
    //// protocol breakdown
    uint64_t transport_total = 0;
    for(std::map<uint32_t, uint64_t>::const_iterator ii =
                report.transport_counts.begin();
            ii != report.transport_counts.end(); ii++) {
        transport_total += ii->second;
    }

    formatted = ssprintf("Transports: IPv4 %.2f%% IPv6 %.2f%% ARP %.2f%% Other %.2f%%",
            ((double) report.transport_counts[ETHERTYPE_IP] / (double) transport_total) * 100.0,
            ((double) report.transport_counts[ETHERTYPE_IPV6] / (double) transport_total) * 100.0,
            ((double) report.transport_counts[ETHERTYPE_ARP] / (double) transport_total) * 100.0,
            (1.0 - ((double) (report.transport_counts[ETHERTYPE_IP] +
                              report.transport_counts[ETHERTYPE_IPV6] +
                              report.transport_counts[ETHERTYPE_ARP]) /
                    (double) transport_total)) * 100.0);
    render_text_line(formatted.c_str(), report.header_font_size,
            title_line_space);
    // trailing pad for entire header
    end_of_content += title_line_space * 4;
}

void one_page_report::render_pass::render_text(std::string text,
        double font_size, double x_offset,
        cairo_text_extents_t &rendered_extents)
{
    cairo_set_font_size(surface, font_size);
    cairo_set_source_rgb(surface, 0.0, 0.0, 0.0);
    cairo_text_extents(surface, text.c_str(), &rendered_extents);
    cairo_move_to(surface, x_offset, end_of_content + rendered_extents.height);
    cairo_show_text(surface, text.c_str());
}

void one_page_report::render_pass::render_text_line(std::string text,
        double font_size, double line_space)
{
    cairo_text_extents_t extents;
    render_text(text, font_size, 0.0, extents);
    end_of_content += extents.height + line_space;
}

void one_page_report::render_pass::render_bandwidth_histogram()
{
    plot::bounds_t bounds(0.0, end_of_content, surface_bounds.width,
            bandwidth_histogram_height);

    report.bandwidth_histogram.render(surface, bounds);

    end_of_content += bounds.height * histogram_pad_factor_y;
}

void one_page_report::render_pass::render_packetfall()
{
    plot::bounds_t bounds(0.0, end_of_content, surface_bounds.width,
            bandwidth_histogram_height);

    report.pfall.render(surface, bounds);

    end_of_content += bounds.height * histogram_pad_factor_y;
}

void one_page_report::render_pass::render_map()
{
}

void one_page_report::render_pass::render_address_histograms()
{
    report.src_addr_histogram.from_iptree(report.src_tree);
    report.dst_addr_histogram.from_iptree(report.dst_tree);
    // histograms
    std::vector<iptree::addr_elem> top_src_addrs;
    std::vector<iptree::addr_elem> top_dst_addrs;
    uint64_t total_datagrams = report.src_addr_histogram.get_ingest_count();

    report.src_addr_histogram.get_top_addrs(top_src_addrs);
    report.dst_addr_histogram.get_top_addrs(top_dst_addrs);

    double width = surface_bounds.width / address_histogram_width_divisor;

    plot::bounds_t left_bounds(0.0, end_of_content, width,
            address_histogram_height);
    report.src_addr_histogram.render(surface, left_bounds);

    plot::bounds_t right_bounds(surface_bounds.width - width, end_of_content,
            width, address_histogram_height);
    report.dst_addr_histogram.render(surface, right_bounds);

    end_of_content += max(left_bounds.height, right_bounds.height);

    // text stats
    for(size_t ii = 0; ii < report.histogram_show_top_n_text; ii++) {
        cairo_text_extents_t left_extents, right_extents;

        if(top_src_addrs.size() > ii) {
            iptree::addr_elem addr = top_src_addrs.at(ii);
            uint8_t percentage = 0;

            percentage = (uint8_t) (((double) addr.count / (double) total_datagrams) * 100.0);

            std::string str = ssprintf("%d. %s - %s (%d%%)", ii + 1, addr.str().c_str(),
                    comma_number_string(addr.count).c_str(), percentage);

            render_text(str.c_str(), report.top_list_font_size, left_bounds.x,
                    left_extents);
        }

        if(top_dst_addrs.size() > ii) {
            iptree::addr_elem addr = top_dst_addrs.at(ii);
            uint8_t percentage = 0;

            percentage = (uint8_t) (((double) addr.count / (double) total_datagrams) * 100.0);

            std::string str = ssprintf("%d. %s - %s (%d%%)", ii + 1, addr.str().c_str(),
                    comma_number_string(addr.count).c_str(), percentage);

            render_text(str.c_str(), report.top_list_font_size, right_bounds.x,
                    right_extents);
        }

        end_of_content += max(left_extents.height, right_extents.height) * 1.5;
    }

    end_of_content += max(left_bounds.height, right_bounds.height) *
        (histogram_pad_factor_y - 1.0);
}

void one_page_report::render_pass::render_port_histograms()
{
    std::vector<port_histogram::port_count> top_src_ports;
    std::vector<port_histogram::port_count> top_dst_ports;
    uint64_t total_segments = report.src_port_histogram.get_ingest_count();

    report.src_port_histogram.get_top_ports(top_src_ports);
    report.dst_port_histogram.get_top_ports(top_dst_ports);

    double width = surface_bounds.width / address_histogram_width_divisor;

    plot::bounds_t left_bounds(0.0, end_of_content, width,
            port_histogram_height);
    report.src_port_histogram.render(surface, left_bounds, report);

    plot::bounds_t right_bounds(surface_bounds.width - width, end_of_content,
            width, port_histogram_height);
    report.dst_port_histogram.render(surface, right_bounds, report);

    end_of_content += max(left_bounds.height, right_bounds.height);

    // text stats
    for(size_t ii = 0; ii < report.histogram_show_top_n_text; ii++) {
        cairo_text_extents_t left_extents, right_extents;

        if(top_src_ports.size() > ii) {
            port_histogram::port_count port = top_src_ports.at(ii);
            uint8_t percentage = 0;

            percentage = (uint8_t) (((double) port.count / (double) total_segments) * 100.0);

            std::string str = ssprintf("%d. %d - %s (%d%%)", ii + 1, port.port,
                    pretty_byte_total(port.count).c_str(), percentage);

            render_text(str.c_str(), report.top_list_font_size, left_bounds.x,
                    left_extents);
        }

        if(top_dst_ports.size() > ii) {
            port_histogram::port_count port = top_dst_ports.at(ii);
            uint8_t percentage = 0;

            percentage = (uint8_t) (((double) port.count / (double) total_segments) * 100.0);

            std::string str = ssprintf("%d. %d - %s (%d%%)", ii + 1, port.port,
                    pretty_byte_total(port.count).c_str(), percentage);

            render_text(str.c_str(), report.top_list_font_size, right_bounds.x,
                    right_extents);
        }

        end_of_content += max(left_extents.height, right_extents.height) * 1.5;
    }

    end_of_content += max(left_bounds.height, right_bounds.height) *
        (histogram_pad_factor_y - 1.0);
}

/* SLG - Should the prefixes be in a structure where the structure encodes both the
   prefix and its multiplier? It seems that the position encodes the multiplier here,
   but I would like to see that explicit. ALso the prefixes are "", "K", "M", etc,
   the B is really not a prefix...
*/
   
   
std::vector<std::string> one_page_report::build_size_suffixes()
{
    std::vector<std::string> v;
    v.push_back("B");
    v.push_back("KB");
    v.push_back("MB");
    v.push_back("GB");
    v.push_back("TB");
    v.push_back("PB");
    v.push_back("EB");
    return v;
}
#endif
