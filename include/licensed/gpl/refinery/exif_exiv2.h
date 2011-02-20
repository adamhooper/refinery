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

class Exiv2ExifData : public ExifData {
  class Impl;
  Impl* impl;

public:
  Exiv2ExifData(Exiv2::ExifData& exiv2ExifData);
  ~Exiv2ExifData();

  virtual bool hasKey(const char* key) const;
  virtual std::string getString(const char* key) const;
  virtual void getBytes(const char* key, std::vector<byte>& outBytes) const;
  virtual int getInt(const char* key) const;
  virtual float getFloat(const char* key) const;
};

}

#endif /* _REFINERY_EXIF_EXIV2_H */
