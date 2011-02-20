#ifndef _REFINERY_UNPACK_H
#define _REFINERY_UNPACK_H

#include <iosfwd>
#include <cstdio>

namespace refinery {

class ExifData;

template<typename T> class Image;
template<typename T> class GrayPixel;
typedef Image<GrayPixel<unsigned short> > GrayImage;
template<typename T> class RGBPixel;
typedef Image<RGBPixel<unsigned short> > RGBImage;

/**
 * Returns Image instances based on stream input and Exif data.
 *
 * When reading is done, the stream will be seeked one byte past the last
 * byte in the image.
 *
 * You must know ahead of time whether the input image is grayscale or RGB.
 * This can be calculated using Exif data. Most modern cameras use Bayer
 * filter arrays over monochrome sensors and so they are grayscale, \e not
 * RGB.
 *
 * Here's what you can do to create a GrayImage:
 *
 * \code
 * std::filebuf file;
 * file.open("image.NEF", std::ios::in);
 * refinery::DcrawExifData exifData(file);
 * refinery::ImageReader reader;
 * std::auto_ptr<GrayImage*> grayImage(
 *   reader.readGrayImage(file, exifData));
 * \endcode
 */
class ImageReader {
public:
  /**
   * Reads and returns a GrayImage.
   *
   * \param[in] istream Input streambuf, such as an std::filebuf.
   * \param[in] exifData Image Exif data.
   * \return A newly-allocated GrayImage which the caller must free later.
   */
  GrayImage* readGrayImage(std::streambuf& istream, const ExifData& exifData);
  /**
   * Reads and returns a GrayImage.
   *
   * This doesn't use C++ streams, so it's suitable for language bindings.
   *
   * \param[in] istream Input file pointer.
   * \param[in] exifData Image Exif data.
   * \return A newly-allocated GrayImage which the caller must free later.
   */
  GrayImage* readGrayImage(FILE* istream, const ExifData& exifData);

  /**
   * Reads and returns an RGBImage.
   *
   * Aside from reading RAW files, this can read 8-bit or 16-bit PPM files.
   *
   * \param[in] istream Input streambuf, such as an std::filebuf.
   * \param[in] exifData Image Exif data.
   * \return A newly-allocated RGBImage which the caller must free later.
   */
  RGBImage* readRgbImage(std::streambuf& istream, const ExifData& exifData);
  /**
   * Reads and returns an RGBImage.
   *
   * Aside from reading RAW files, this can read 8-bit or 16-bit PPM files.
   *
   * This doesn't use C++ streams, so it's suitable for language bindings.
   *
   * \param[in] istream Input file pointer.
   * \param[in] exifData Image Exif data.
   * \return A newly-allocated RGBImage which the caller must free later.
   */
  RGBImage* readRgbImage(FILE* istream, const ExifData& exifData);
};

}

#endif /* _REFINERY_UNPACK_H */
