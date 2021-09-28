#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/virtual_locator.hpp>
#include <exiv2/exiv2.hpp>
#include <fstream>

#include <iostream>

namespace gil = boost::gil;

template <typename UnderlyingImage> struct overlapped {
    typedef gil::point2<ptrdiff_t> point_t;

    typedef overlapped const_t;
    typedef gil::rgb8_pixel_t value_type;
    typedef value_type reference;
    typedef value_type const_reference;
    typedef point_t argument_type;
    typedef reference result_type;
    BOOST_STATIC_CONSTANT(bool, is_mutable = false);

    using underlying_image_t = UnderlyingImage;

    overlapped(underlying_image_t img,
               value_type edge_color = value_type(0xff, 0xff, 0xff))
        : img_(img), edge_color_(edge_color) {}

    result_type operator()(point_t p) const {
        if (p.x < 0 || p.x >= img_.width() || p.y < 0 || p.y >= img_.height()) {
            return edge_color_;
        }
        return img_(p);
    }

private:
    UnderlyingImage img_;
    value_type edge_color_;
};

template <typename Img>
using overlapped_image_view =
    gil::image_view<gil::virtual_2d_locator<overlapped<Img>, false>>;

template <typename SrcImageView>
auto padded_image(SrcImageView img, int top_pad, int left_pad, int bottom_pad,
                  int right_pad) {

    using fn = overlapped<SrcImageView>;
    using point_t = typename fn::point_t;
    using locator = gil::virtual_2d_locator<fn, false>;
    using image_view = gil::image_view<locator>;

    return image_view(
        point_t(img.width() + left_pad + right_pad,
                img.height() + top_pad + bottom_pad),
        locator(point_t(-left_pad, -top_pad), point_t(1, 1), fn(img)));
}

void copy_metadata(Exiv2::Image &src, Exiv2::Image &dst) {
    if (src.iccProfileDefined() && src.iccProfile()) {
        dst.setIccProfile(*src.iccProfile(), false);
    }
    dst.setIptcData(src.iptcData());
    dst.setExifData(src.exifData());
    dst.setComment(src.comment());
    dst.setXmpData(src.xmpData());
}

extern "C" {
int run(const char *infile, const char *outfile) {
    try {
    std::cout << "loading " << infile << std::endl;
    std::ifstream img_file(infile);

    gil::rgb8_image_t img;
    gil::read_image(img_file, img, gil::jpeg_tag());

    Exiv2::XmpParser::initialize();
    ::atexit(Exiv2::XmpParser::terminate);

    auto in_exif = Exiv2::ImageFactory::open(infile, false);
    if (!in_exif.get()) {
        std::cerr << "failed to load exif from " << infile << std::endl;
        return -1;
    }
    in_exif->readMetadata();

    // relative to short side of final image
    // polaroid: 1/15 padding left, right and top. 1/4 bottom
    // source:
    // https://support.polaroid.com/hc/en-us/articles/115012363647-What-are-Polaroid-photo-dimensions-

    auto short_side = std::min(img.height(), img.width());

    int rpad = short_side / 15, lpad = short_side / 15, tpad = short_side / 15,
        bpad = short_side / 15;

    auto exif_orientation = Exiv2::orientation(in_exif->exifData());
    int orientation = 1;
    if (exif_orientation != in_exif->exifData().end()) {
        orientation = exif_orientation->toLong();
        std::cout << "orientation: " << orientation << std::endl;
    }

    switch (orientation) {
    case 3:
    case 4: tpad = short_side / 4; break; // 0th row is bottom

    case 5:
    case 6: rpad = short_side / 4; break; // 0th column is top

    case 7:
    case 8: lpad = short_side / 4; break; // 0th column is bottom

    case 1:
    case 2:
    default: bpad = short_side / 4; break; // 0th row is top
    }

    auto v = padded_image(gil::view(img), tpad, lpad, bpad, rpad);

    std::ofstream img_file_out(outfile);

    std::cout << "created outstream" << std::endl;
    gil::write_view(img_file_out, v, gil::image_write_info<gil::jpeg_tag>(100));
    img_file_out.close();

    auto out_exif = Exiv2::ImageFactory::open(outfile, false);
    if (!out_exif.get()) {
        std::cerr << "failed to load exif from " << outfile << std::endl;
        return -1;
    }
    copy_metadata(*in_exif, *out_exif);
    out_exif->writeMetadata();

    } catch (std::exception &err) {
        std::cerr << err.what() << std::endl;
        return -1;
    }
    return 0;
}
}

#ifndef NO_MAIN

int main(int argc, const char **argv) {
    if (argc < 3) {
        return -1;
    }

    return run(argv[1], argv[2]);
}

#endif
