/*
 * Copyright (c) 2010 Adam Hooper
 *
 * This file is part of refinery (GPL portion).
 *
 * Foobar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "refinery/exif_exiv2.h"

#include <sstream>
#include <stdexcept>

#include <exiv2/exif.hpp>
#include <exiv2/tags.hpp>

namespace refinery {

class Exiv2ExifData::Impl {
  Exiv2::ExifData& exiv2ExifData;

  const Exiv2::Value& getValue(const char* key) const
  {
    Exiv2::ExifKey exifKey(key);
    Exiv2::ExifData::const_iterator it(exiv2ExifData.findKey(exifKey));
    if (it == exiv2ExifData.end()) {
      throw std::logic_error(std::string("Invalid Exif key `") + key + "'");
    }

    return it->value();
  }

public:

  Impl(Exiv2::ExifData& exiv2ExifData) : exiv2ExifData(exiv2ExifData) {}

  bool hasKey(const char* key) const
  {
    Exiv2::ExifKey exifKey(key);
    Exiv2::ExifData::const_iterator it(exiv2ExifData.findKey(exifKey));

    return it != exiv2ExifData.end();
  }

  std::string getString(const char* key) const
  {
    const Exiv2::Value& value(getValue(key));
    std::string ret(value.toString());
    if (!value.ok()) {
      std::stringstream err;
      err << "Exif value should be type string, but has type ID "
          << value.typeId() << ": " << key;
      throw std::logic_error(err.str());
    }
    return ret;
  }

  void getBytes(const char* key, std::vector<byte>& outBytes) const
  {
    const Exiv2::Value& value(getValue(key));
    long nBytes(value.size());
    outBytes.assign(nBytes, 0);
    value.copy(&outBytes[0], Exiv2::bigEndian);
  }

  int getInt(const char* key) const
  {
    const Exiv2::Value& value(getValue(key));
    long ret = value.toLong();
    if (!value.ok()) {
      std::stringstream err;
      err << "Exif value should be type long, but has type ID "
        << value.typeId() << ": " << key;
      throw std::logic_error(err.str());
    }
    return ret;
  }

  float getFloat(const char* key) const
  {
    const Exiv2::Value& value(getValue(key));
    float ret = value.toFloat();
    if (!value.ok()) {
      std::stringstream err;
      err << "Exif value should be type float, but has type ID "
        << value.typeId() << ": " << key;
      throw std::logic_error(err.str());
    }
    return ret;
  }

  void setString(const char* key, const std::string& s)
  {
    exiv2ExifData[key] = s;
  }
};

Exiv2ExifData::Exiv2ExifData(Exiv2::ExifData& exiv2ExifData)
  : impl(new Exiv2ExifData::Impl(exiv2ExifData))
{
}

Exiv2ExifData::~Exiv2ExifData()
{
  delete impl;
}

bool Exiv2ExifData::hasKey(const char* key) const
{
  return impl->hasKey(key);
}

std::string Exiv2ExifData::getString(const char* key) const
{
  return impl->getString(key);
}

void Exiv2ExifData::getBytes(const char* key, std::vector<byte>& outBytes) const
{
  impl->getBytes(key, outBytes);
}

int Exiv2ExifData::getInt(const char* key) const
{
  return impl->getInt(key);
}

float Exiv2ExifData::getFloat(const char* key) const
{
  return impl->getFloat(key);
}

void Exiv2ExifData::setString(const char* key, const std::string& s)
{
  impl->setString(key, s);
}

} // namespace refinery
