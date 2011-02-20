#ifndef _REFINERY_EXIF_H
#define _REFINERY_EXIF_H

#include <streambuf>
#include <string>
#include <vector>

namespace refinery {

/**
 * Holds Exif information for an image.
 *
 * This data is passed to the TypedImage constructor, after which any Exif
 * information is only accessed through the image's CameraData.
 *
 * This is a virtual class because several strategies exist for parsing Exif
 * data from an image. Extend this class if you want to use a new strategy.
 * Keep in mind Exif parsing for raw files is extremely finicky because each
 * camera format is different. Some winning strategies aren't included because
 * they come in different programming languages or with incompatible licenses.
 * This extensible ExifData class can handle both cases.
 */
class ExifData {
public:
  typedef unsigned char byte; /**< For raw data. */

  virtual ~ExifData() {} /**< destructor. */

  /**
   * True iff the Exif data contains the given \a key.
   *
   * \param[in] key Exif key, for instance "Exif.Image.Orientation".
   * \return True iff the Exif data contains the key.
   */
  virtual bool hasKey(const char* key) const = 0;

  /**
   * Returns the specified Exif value as a string.
   *
   * This throws an error if the Exif data doesn't exist. Use hasKey() to
   * verify that it does before calling this method.
   *
   * \param[in] key Exif key, for instance "Exif.Image.Model".
   * \return A string representation of the Exif value.
   */
  virtual std::string getString(const char* key) const = 0;

  /**
   * Copies the specified Exif value into a byte-array.
   *
   * Existing data will be erased.
   *
   * This throws an error if the Exif data doesn't exist. Use hasKey() to
   * verify that it does before calling this method.
   *
   * \param[in] key Exif key, for instance "Exif.Nikon3.LinearizationTable".
   * \param[out] outBytes Byte-array to fill.
   */
  virtual void getBytes(const char* key, std::vector<byte>& outBytes) const = 0;

  /**
   * Returns the specified Exif value as an int.
   *
   * This throws an error if the Exif data doesn't exist. Use hasKey() to
   * verify that it does before calling this method.
   *
   * \param[in] key Exif key, for instance "Exif.Image.Orientation".
   * \return An int representation of the Exif value.
   */
  virtual int getInt(const char* key) const = 0;

  /**
   * Returns the specified Exif value as a float.
   *
   * This throws an error if the Exif data doesn't exist. Use hasKey() to
   * verify that it does before calling this method.
   *
   * \param[in] key Exif key, for instance "Exif.Image.XResolution".
   * \return A float representation of the Exif value.
   */
  virtual float getFloat(const char* key) const = 0;
};

/**
 * An in-memory, fake Exif data container.
 *
 * This only returns Exif data that has been set in it programmatically. It's
 * useful for testing or as a base class for other Exif parsers.

 * \code
 * InMemoryExifData exifData;
 * exifData.setString("Exif.Image.Model", "NIKON D5000");
 * std::string model(exifData.getString("Exif.Image.Model"));
 * std::cout << "Camera: " << model << std::endl;
 * \endcode
 */
class InMemoryExifData : public ExifData {
  class Impl;
  Impl* impl;

public:
  InMemoryExifData(); /**< constructor. */
  ~InMemoryExifData(); /**< destructor. */

  virtual bool hasKey(const char* key) const;
  virtual std::string getString(const char* key) const;
  virtual void getBytes(const char* key, std::vector<byte>& outBytes) const;
  virtual int getInt(const char* key) const;
  virtual float getFloat(const char* key) const;

  /**
   * Sets a string Exif datum.
   *
   * \param[in] key Exif key, for instance "Exif.Image.Model".
   * \param[in] s String, for instance "NIKON D5000".
   */
  virtual void setString(const char* key, const std::string& s);
};

/**
 * Exif parser copied from dcraw.c.
 *
 * Dcraw has impeccable camera-identifying code, but it doesn't store its
 * in-memory data as Exif. For instance, it sometimes hard-codes the known
 * width and height of a particular camera's sensor without looking at
 * Exif tags, unlike refinery, which uses CameraData for all special cases.
 * This class runs dcraw's calculations and then lets users access the
 * results as if they were Exif data.
 *
 * Example:
 *
 * \code
 * std::ifstream istream("image.NEF");
 * DcrawExifData exifData(istream);
 * std::string model(exifData.getString("Exif.Image.Model"));
 * std::cout << "Camera: " << model << std::endl;
 * \endcode
 */
class DcrawExifData : public ExifData {
  class Impl;
  Impl* impl;

public:
  /**
   * Constructor.
   *
   * The input stream will be forgotten when this method returns.
   *
   * \param[in] istream Input stream to parse.
   */
  DcrawExifData(std::streambuf& istream);
  /**
   * Constructor.
   *
   * The input stream will be forgotten when this method returns.
   *
   * \param[in] f Input file pointer to parse.
   */
  DcrawExifData(FILE* f);

  ~DcrawExifData(); /**< destructor. */

  /**
   * Returns the detected MIME type of this image.
   *
   * This method is included as a convenience, since dcraw calculates the MIME
   * type during parsing.
   *
   * \return A MIME type string, for instance "image/x-nikon-nef".
   */
  const char* mime_type() const;

  virtual bool hasKey(const char* key) const;
  virtual std::string getString(const char* key) const;
  virtual void getBytes(const char* key, std::vector<byte>& outBytes) const;
  virtual int getInt(const char* key) const;
  virtual float getFloat(const char* key) const;
};

} // namespace refinery

#endif /* _REFINERY_EXIF_H */
