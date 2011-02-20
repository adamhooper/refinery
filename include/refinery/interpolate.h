#ifndef _REFINERY_INTERPOLATE_H
#define _REFINERY_INTERPOLATE_H

namespace refinery {

template<typename T> class Image;
template<typename T> class RGBPixel;
typedef Image<RGBPixel<unsigned short> > RGBImage;
template<typename T> class GrayPixel;
typedef Image<GrayPixel<unsigned short> > GrayImage;

/**
 * Transforms a sensor image to an RGB or CMYK image.
 *
 * When most cameras capture an image, their sensors only gather one color
 * value per pixel. The other values must be added in software, by the
 * Interpolator.
 *
 * Crucial in the incoming image is the Image::filters() return value,
 * which describes which colors go where on the camera sensor array.
 *
 * There are several strategies for guessing the missing color values on each
 * pixel. One, INTERPOLATE_BILINEAR, takes an average from the color values of
 * adjacent pixels. Another, INTERPOLATE_AHD, chooses either vertical or
 * horizontal averages per pixel, depending on which one gives the crispest
 * image.
 *
 * It's straightforward to interpolate a camera-sensor image:
 * \code
 * GrayImage gray = somehowGetGrayImage();
 * Interpolator interpolator(Interpolator.INTERPOLATE_AHD);
 * RGBImage* rgb = interpolator.interpolate(gray);
 * \endcode
 */
class Interpolator {
public:
  enum Type {
    /**
     * For each pixel, takes the best average.
     *
     * Two averages are calculated: one with pixels which line up
     * horizontally, and the other with pixels lined up vertically. For each
     * pixel, an average is chosen which gives the least blurry result.
     */
    INTERPOLATE_AHD,

    /**
     * For each missing color value, take the average of its neighbors.
     *
     * For example, on a typical RGB sensor any non-green pixel will have
     * a green pixel above, below, to the left and to the right. Sum those and
     * divide by four, and call that the green for that pixel.
     *
     * This can make slightly blurry pictures, but it's fast.
     */
    INTERPOLATE_BILINEAR
  };

private:
  Type mType;

public:
  /**
   * Constructor.
   *
   * \param[in] type Type of interpolator this is.
   */
  Interpolator(const Type& type);

  /**
   * Produce a colorful image from a gray one.
   *
   * This relies upon GrayImage::filters() to determine what the sensor colors
   * are. This ought to be set automatically from the Exif data, but if the
   * output has cartoony color problems, that might have failed.
   *
   * It's up to the caller to free the resulting image, with \c delete.
   *
   * \param[in] image Grayscale (from sensor data) image to interpolate.
   * \return A new, colorful image with the same width, height and Exif data.
   */
  RGBImage* interpolate(const GrayImage& image);
};

} /* namespace */

#endif /* _REFINERY_INTERPOLATE_H */
