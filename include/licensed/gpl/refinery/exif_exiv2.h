/*
 * Copyright (c) 2010 Adam Hooper
 *
 * This file is part of refinery (GPL portion).
 *
 * refinery (GPL portion) is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * refinery (GPL portion) is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with refinery (GPL portion).  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef _REFINERY_EXIF_EXIV2_H
#define _REFINERY_EXIF_EXIV2_H

#include <refinery/exif.h>

namespace Exiv2 {
  class ExifData;
}

namespace refinery {

/**
 * Uses Exiv2 to supply Exif data.
 *
 * Exiv2 is complete and has a useful API, but its license is GPL and so any
 * code that links to it must use a GPL-compatible license.
 *
 * This is a shallow wrapper around the Exiv2 data, so the original Exiv2 data
 * must exist for the lifetime of this object.
 *
 * When the header is included, Exiv2ExifData can be initialized automatically
 * where an ExifData is expected, as in the following example:
 *
 * \code
 * Exiv2::Image::AutoPtr exivImage(Exiv2::ImageFactory::open("image.NEF"));
 * // assert exivImage.get() != 0
 * exivImage->readMetadata();
 * const Exiv2::ExifData& exivData(exivImage->exifData());
 * // assert !exivData.empty()
 * refinery::ImageReader reader;
 * std::auto_ptr<GrayImage> grayImagePtr(
 *   reader.readGrayImage(fb, exivData)); // exivData converts to an ExifData
 * \endcode
 */
class Exiv2ExifData : public ExifData {
  class Impl;
  Impl* impl;

public:
  /**
   * constructor.
   *
   * \param exiv2ExifData The Exiv2 data.
   */
  Exiv2ExifData(Exiv2::ExifData& exiv2ExifData);
  ~Exiv2ExifData(); /**< destructor. */

  virtual bool hasKey(const char* key) const;
  virtual std::string getString(const char* key) const;
  virtual void getBytes(const char* key, std::vector<byte>& outBytes) const;
  virtual int getInt(const char* key) const;
  virtual float getFloat(const char* key) const;
};

}

#endif /* _REFINERY_EXIF_EXIV2_H */
