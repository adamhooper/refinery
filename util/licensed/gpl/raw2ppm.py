#!/usr/bin/env python

# Copyright (c) 2010 Adam Hooper
#
# This file is part of refinery (GPL portion).
#
# refinery (GPL portion) is free software: you can redistribute it and/or
# modify it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# refinery (GPL portion) is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with refinery (GPL portion).  If not, see
# <http://www.gnu.org/licenses/>.

import refinery
import pyexiv2 # A GPLed dependency, hence we're GPL too

# refinery doesn't use Exiv2::ExifData pointers directly (because of
# licensing), so we can't create a refinery::Exiv2ExifData pointer as we would
# in C++.
#
# The solution: create its parent, a refinery::ExifData. This Python subclass
# is functionally equivalent to a refinery::Exiv2ExifData pointer. We'll pass
# it to all our C++ methods as if it were a C++ pointer. (To them, it is one.)
class PyExifData(refinery.ExifData):
  def __init__(self, exivImage):
    refinery.ExifData.__init__(self)
    self.exivImage = exivImage

  def hasKey(self, key):
    return key in self.exivImage

  def getBytes(self, key, bytes):
    # bytes is an std::vector<unsigned char>
    tag = self.exivImage[key]
    value = None
    if tag.type == 'Undefined':
      value = [ord(c) for c in tag.value]
    else:
      value = [int(s) for s in tag.value.split()]
    bytes.clear()
    bytes.reserve(len(value))
    for b in value:
      bytes.append(b)

  def getInt(self, key):
    value = self.exivImage[key].value
    return int(value)

  def getFloat(self, key):
    value = self.exivImage[key].value
    return float(value)

  def getString(self, key):
    value = self.exivImage[key].value
    return str(value)

  def setString(self, key, s):
    exivImage[key] = s

# Here's the image processing chain, from read to write
def convert(infile, outfile):
  print('Loading metadata from %s...' % infile)

  exivImage = pyexiv2.ImageMetadata(infile)
  exivImage.read()
  exifData = PyExifData(exivImage)

  reader = refinery.ImageReader()
  width = exivImage['Exif.SubImage2.ImageWidth'].value
  height = exivImage['Exif.SubImage2.ImageLength'].value
  mimeType = 'image/x-nikon-nef'

  print('Reading grayscale image (%s, %dx%d)' % (mimeType, width, height))
  f = open(infile)
  grayImage = reader.readGrayImage(f, mimeType, width, height, exifData)

  print('Scaling to full 16-bit color...')
  refinery.ScaleColorsFilter().filter(grayImage)

  print('Interpolating RGB values...')
  interpolator = refinery.Interpolator(refinery.Interpolator.INTERPOLATE_AHD)
  image = interpolator.interpolate(grayImage)

  print('Converting from camera RGB to sRGB...')
  refinery.ConvertToRgbFilter().filter(image)

  print('Writing image to %s... (8-bit PPM)' % outfile)
  writer = refinery.ImageWriter()
  writer.writeImage(image, 'out3.ppm', 'PPM', 8)

  print('Done!')

if __name__ == '__main__':
  import sys

  if len(sys.argv) != 3:
    print >>sys.stderr, 'Usage: %s <infile> <outfile>' % sys.argv[0]
    sys.exit(1)

  convert(sys.argv[1], sys.argv[2])
