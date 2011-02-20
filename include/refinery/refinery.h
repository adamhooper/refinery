#ifndef _REFINERY_REFINERY_H
#define _REFINERY_REFINERY_H

#include <refinery/camera.h>
#include <refinery/color.h>
#include <refinery/exif.h>
#include <refinery/filters.h>
#include <refinery/gamma.h>
#include <refinery/image.h>
#include <refinery/histogram.h>
#include <refinery/output.h>
#include <refinery/unpack.h>

namespace refinery {
/**
 * \mainpage refinery
 *
 * \section Introduction
 *
 * Refinery reads RAW photograph files as input and provides usable RGB (or
 * other format) data, which is suitable for combining with other imaging
 * libraries.
 *
 * \section Installing
 *
 * <ol>
 *  <li>Download the source: <tt>git clone git://github.com/adamh/refinery.git</tt>
 *  <li>Install CMake and Boost: <tt>sudo apt-get install libboost-dev cmake</tt>
 *  <li>Change into the source directory and type <tt>cmake -D CMAKE_BUILD_TYPE=Release .</tt>
 *  <li><tt>make</tt> (to change options, install and run <tt>ccmake</tt>).</li>
 *  <li><tt>sudo make install</tt></li>
 * </ol>
 *
 * This will install headers, a shared library, and the <tt>raw2ppm</tt>
 * program.
 *
 * If you also have SWIG 2.0 and Python development headers installed, then
 * Python bindings will be built and installed. With these, you can combine
 * refinery with the Python Imaging Library (PIL) for image-processing power.
 *
 * You may also install Exiv2 and run cmake with <tt>-D LICENCE_GPL=ON</tt> to
 * compile Exiv2 support. Where the built-in DcrawExifData doesn't work, Exiv2
 * might.
 *
 * \section Running
 *
 * This is a library, not a program. You can use <tt>-I/path/to/refinery</tt>
 * and <tt>-lrefinery</tt> to compile programs.
 *
 * One program, <tt>raw2ppm</tt>, accompanies this code. Call it with
 * <tt>raw2ppm [RAWFILE] outfile.ppm</tt>. It converts a RAW file to a PPM and
 * is intended as an example, not an everyday tool. You can link refinery with
 * other libraries (such as a JPEG-writing one) to create more powerful tools.
 *
 * \section Using
 *
 * Use <tt>-I/path/to/refinery</tt> and <tt>-lrefinery</tt> to compile programs.
 * You can include <refinery/refinery.h> for some basic tools, or
 * just include the particular files you need.
 *
 * Use <tt>util/raw2ppm.cc</tt> as an example. There's also
 * <tt>util/licensed/gpl/convert.py</tt> which shows how to use Python to
 * extend classes and take advantage of PIL.
 *
 * Raw decoding entails a series of steps; here's a typical chain:
 *
 * <ol>
 *  <li>Unpack the Exif data (use DcrawExifData or Exiv2ExifData).</li>
 *  <li>Unpack the raw data (use ImageReader). Unless you're using a
 *      Foveon camera, you now have a grayscale Image.</li>
 *  <li>Scale it to fill its data-type (for instance, scaling 12-bit to
 *      16-bit) (use ScaleColorsFilter).</li>
 *  <li>Interpolate the missing colors (use Interpolator). Now you have
 *      multi-color Image.</li>
 *  <li>Convert from the camera colorspace to sRGB (use
 *      ConvertToRgbFilter).</li>
 *  <li>Gamma-correct the image (use Histogram, GammaCurve and
 *      GammaFilter).</li>
 *  <li>Flip and rotate the image if needed (nothing in refinery for this
 *      right now).</li>
 *  <li>Save the image (use ImageWriter).</li>
 * </ol>
 * 
 * Depending on your intents, you might want to add more steps. That's okay,
 * since the image data is easy to copy from Image::pixels().
 *
 * \section License
 *
 * This code is public domain, except for anything that links to Exiv2, which
 * is GPL.
 *
 * \example util/raw2ppm.cc
 * \example util/licensed/gpl/convert.py
 */
}; // namespace refinery

#endif /* _REFINERY_REFINERY_H */
