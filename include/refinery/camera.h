#ifndef _REFINERY_CAMERA_H
#define _REFINERY_CAMERA_H

namespace refinery {

class ExifData;

/**
 * Holds static data about an actual camera model.
 *
 * Each camera model behaves slightly differently: different compression, for
 * instance, or different color profiles. That's what Camera represents. Most
 * of the time users really want CameraData, which uses a Camera to make sense
 * of Exif data.
 *
 * Some Camera methods accept an ExifData& as a parameter. That's
 * because different cameras treat the same Exif tags differently.
 */
class Camera {
public:
  /**
   * These values are used in mapping from sensors to RGB, CMYK and XYZ.
   */
  struct ColorConversionData {
    short black; /**< Black level, the camera's conceptual 0. */
    short maximum; /**< Maximum sensor value. */
    double xyzToCamera[4][3]; /**< Matrix to convert XYZ to sensor values. */
    double rgbToCamera[4][3]; /**< Matrix to convert sRGB to sensor values. */
    double cameraToRgb[3][4]; /**< Matrix to convert sensor values to sRGB. */
    double cameraToXyz[3][4]; /**< Matrix to convert sensor values to XYZ. */
    double cameraMultipliers[4]; /**< Multipliers for making other matrices. */
    double scalingMultipliers[4]; /**< To scale sensor values for max 65535. */
  };

  /**
   * The make of the camera, for example "NIKON".
   *
   * \return An immutable C string, like "NIKON".
   */
  virtual const char* make() const = 0;

  /**
   * The model of the camera, for example "D5000".
   *
   * \return An immutable C string, like "D5000".
   */
  virtual const char* model() const = 0;

  /**
   * The make + model of the camera, for example "NIKON D5000".
   *
   * \return An immutable C string, like "NIKON D5000".
   */
  virtual const char* name() const = 0;

  /**
   * The number of sensor colors, typically 3 or 4.
   *
   * \return The number of colors, 3 or 4.
   */
  virtual unsigned int colors() const = 0;

  /**
   * The width of the raw image, in pixels.
   *
   * This is different from width(), which is the width of the output image.
   * Some cameras capture margin pixels which have no value at all, and this
   * width includes those.
   *
   * \param[in] exifData the Exif data.
   * \return How many pixels are in each sensor row.
   */
  virtual unsigned int rawWidth(const ExifData& exifData) const = 0;

  /**
   * The height of the raw image, in pixels.
   *
   * This is different from height(), which is the height of the output image.
   * Some cameras capture margin pixels which have no value at all, and this
   * height includes those.
   *
   * \param[in] exifData the Exif data.
   * \return How many pixels are in each sensor column.
   */
  virtual unsigned int rawHeight(const ExifData& exifData) const = 0;

  /**
   * Some multipliers for changing color spaces.
   *
   * \return Color conversion data.
   */
  virtual ColorConversionData colorConversionData() const = 0;

  /**
   * The Exif "Orientation", from 1 to 8, saying how one image is flipped.
   *
   * \param[in] exifData the Exif data.
   * \return An integer from 1 to 8.
   */
  virtual unsigned int orientation(const ExifData& exifData) const = 0;

  /**
   * Returns a bitmask representing the camera sensor array.
   *
   * Here are some possible return values and what they represent:
   *
   * <pre> PowerShot 600      PowerShot A50      PowerShot Pro70    Pro90 & G1
   * 0xe1e4e1e4:        0x1b4e4b1e:        0x1e4b4e1b:        0xb4b4b4b4:
   *
   *   0 1 2 3 4 5        0 1 2 3 4 5        0 1 2 3 4 5        0 1 2 3 4 5
   * 0 G M G M G M      0 C Y C Y C Y      0 Y C Y C Y C      0 G M G M G M
   * 1 C Y C Y C Y      1 M G M G M G      1 M G M G M G      1 Y C Y C Y C
   * 2 M G M G M G      2 Y C Y C Y C      2 C Y C Y C Y
   * 3 C Y C Y C Y      3 G M G M G M      3 G M G M G M
   *                    4 C Y C Y C Y      4 Y C Y C Y C
   * PowerShot A5       5 G M G M G M      5 G M G M G M
   * 0x1e4e1e4e:        6 Y C Y C Y C      6 C Y C Y C Y
   *                    7 M G M G M G      7 M G M G M G
   *   0 1 2 3 4 5
   * 0 C Y C Y C Y
   * 1 G M G M G M
   * 2 C Y C Y C Y
   * 3 M G M G M G
   *
   * All RGB cameras use one of these Bayer grids:
   *
   * 0x16161616:        0x61616161:        0x49494949:        0x94949494:
   *
   *   0 1 2 3 4 5        0 1 2 3 4 5        0 1 2 3 4 5        0 1 2 3 4 5
   * 0 B G B G B G      0 G R G R G R      0 G B G B G B      0 R G R G R G
   * 1 G R G R G R      1 B G B G B G      1 R G R G R G      1 G B G B G B
   * 2 B G B G B G      2 G R G R G R      2 G B G B G B      2 R G R G R G
   * 3 G R G R G R      3 B G B G B G      3 R G R G R G      3 G B G B G B</pre>
   *
   * \param[in] exifData the Exif data.
   * \return The camera's sensor color pattern.
   */
  virtual unsigned int filters(const ExifData& exifData) const = 0;

  /**
   * Returns \c true if this is the right Camera for exifData.
   *
   * \param[in] exifData the Exif data.
   * \return \c true if this Camera shot a photo with this ExifData.
   */
  virtual bool canHandle(const ExifData& exifData) const = 0;
};

/**
 * A Camera coupled with one photo's ExifData.
 *
 * This is the public face that callers care about. Callers shouldn't need to
 * know whether a default color profile comes hard-coded or embedded in Exif.
 * This class abstracts everything.
 *
 * CameraData instances are lightweight. Shallow-copy them at will.
 */
class CameraData {
  const Camera& mCamera;
  const ExifData& mExifData;

public:
  /**
   * Creates a CameraData for a given photo.
   *
   * The Camera and ExifData are both passed by reference and must exist for
   * the lifetime of this CameraData.
   *
   * \param[in] camera The static camera data.
   * \param[in] exifData The photograph's Exif data.
   */
  CameraData(const Camera& camera, const ExifData& exifData)
      : mCamera(camera), mExifData(exifData) {}

  /**
   * Copy constructor.
   *
   * The Camera and ExifData are both passed by reference.
   *
   * \param[in] rhs Original CameraData.
   */
  CameraData(const CameraData& rhs)
      : mCamera(rhs.camera()), mExifData(rhs.exifData()) {}

  /**
   * The Camera object.
   */
  const Camera& camera() const { return mCamera; }

  /**
   * The ExifData object particular to this photo.
   */
  const ExifData& exifData() const { return mExifData; }

  /**
   * The Exif "Orientation", from 1 to 8, saying how one image is flipped.
   */
  unsigned int orientation() const;

  /**
   * A bitmask representing the camera sensor array.
   *
   * Here are some possible return values and what they represent:
   *
   * <pre> PowerShot 600      PowerShot A50      PowerShot Pro70    Pro90 & G1
   * 0xe1e4e1e4:        0x1b4e4b1e:        0x1e4b4e1b:        0xb4b4b4b4:
   *
   *   0 1 2 3 4 5        0 1 2 3 4 5        0 1 2 3 4 5        0 1 2 3 4 5
   * 0 G M G M G M      0 C Y C Y C Y      0 Y C Y C Y C      0 G M G M G M
   * 1 C Y C Y C Y      1 M G M G M G      1 M G M G M G      1 Y C Y C Y C
   * 2 M G M G M G      2 Y C Y C Y C      2 C Y C Y C Y
   * 3 C Y C Y C Y      3 G M G M G M      3 G M G M G M
   *                    4 C Y C Y C Y      4 Y C Y C Y C
   * PowerShot A5       5 G M G M G M      5 G M G M G M
   * 0x1e4e1e4e:        6 Y C Y C Y C      6 C Y C Y C Y
   *                    7 M G M G M G      7 M G M G M G
   *   0 1 2 3 4 5
   * 0 C Y C Y C Y
   * 1 G M G M G M
   * 2 C Y C Y C Y
   * 3 M G M G M G
   *
   * All RGB cameras use one of these Bayer grids:
   *
   * 0x16161616:        0x61616161:        0x49494949:        0x94949494:
   *
   *   0 1 2 3 4 5        0 1 2 3 4 5        0 1 2 3 4 5        0 1 2 3 4 5
   * 0 B G B G B G      0 G R G R G R      0 G B G B G B      0 R G R G R G
   * 1 G R G R G R      1 B G B G B G      1 R G R G R G      1 G B G B G B
   * 2 B G B G B G      2 G R G R G R      2 G B G B G B      2 R G R G R G
   * 3 G R G R G R      3 B G B G B G      3 R G R G R G      3 G B G B G B</pre>
   *
   * \return The camera's sensor color pattern.
   */
  virtual unsigned int filters() const;

  /**
   * The number of sensor colors, typically 3 or 4.
   */
  unsigned int colors() const;

  /**
   * The width of the raw image, in pixels.
   *
   * This is different from width(), which is the width of the output image.
   * Some cameras capture margin pixels which have no value at all, and this
   * width includes those.
   *
   * \return How many pixels are in each sensor row.
   */
  unsigned int rawWidth() const;

  /**
   * The height of the raw image, in pixels.
   *
   * This is different from height(), which is the height of the output image.
   * Some cameras capture margin pixels which have no value at all, and this
   * height includes those.
   *
   * \return How many pixels are in each sensor column.
   */
  unsigned int rawHeight() const;

  /**
   * Some multipliers for changing color spaces.
   */
  Camera::ColorConversionData colorConversionData() const;
};

/**
 * Returns Camera instances.
 *
 * Call CameraFactory::instance().detectCamera(exifData) to return a Camera
 * instance representing the camera model that shot the photo which includes
 * exifData.
 *
 * Each Camera is initialized once, statically. You can only pass Camera
 * instances by reference.
 *
 * If you want to find out anything about an actual photograph, you should look
 * to CameraDataFactory, not CameraFactory.
 */
class CameraFactory {
private:
  CameraFactory();
  ~CameraFactory();

public:
  /**
   * The global CameraFactory.
   */
  static CameraFactory& instance();

  /**
   * Figures out which const Camera& applies to the given Exif data, and
   * returns it.
   *
   * The returned const Camera& should not be memory-managed. It will last the
   * duration of the program execution.
   *
   * If no known real-world camera could have produced the given Exif data
   * (that is, if a computer program did it), a Camera will still be returned.
   * It's called the NullCamera, and there's nothing special about it.
   *
   * \param[in] exifData A photograph's Exif data.
   * \return A Camera which could take this photograph.
   */
  const Camera& detectCamera(const ExifData& exifData) const;
};

/**
 * Returns CameraData instances.
 *
 * Example usage:
 *
 * \code
 * std::auto_ptr<refinery::ExifData> exifData(somehowFindExifData());
 *
 * CameraData cameraData = CameraDataFactory::instance().getCameraData(*exifData);
 *
 * std::cout << "Photo orientation: " << cameraData.orientation() << std::endl;
 * \endcode
 */
class CameraDataFactory {
public:
  /**
   * The global CameraDataFactory.
   */
  static CameraDataFactory& instance();

  /**
   * Returns a CameraData object which helps explain a photo's properties.
   *
   * The ExifData must exist for the lifetime of the returned CameraData.
   *
   * \param[in] exifData A photograph's Exif data.
   * \return A CameraData for this photograph.
   */
  CameraData getCameraData(const ExifData& exifData) const;
};

} // namespace refinery

#endif /* _REFINERY_CAMERA_H */
