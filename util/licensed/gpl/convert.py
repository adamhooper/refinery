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
using_pil = False
try:
  import PIL.Image
  print('Will use Python Imaging Library (PIL) to save the image')
  using_pil = True
except ImportError:
  print('Python Imaging Library (PIL) not found; will save as possibly-misoriented PPM')

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
    # Sometimes the value is erroneously a list and not an int
    # example: Exif.Image.Orientation
    if isinstance(value, list): value = value[0]
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

  print('Reading grayscale image...')
  reader = refinery.ImageReader()
  f = open(infile)
  grayImage = reader.readGrayImage(f, exifData)

  print('Scaling to full 16-bit color...')
  refinery.ScaleColorsFilter().filter(grayImage)

  print('Interpolating RGB values...')
  interpolator = refinery.Interpolator(refinery.Interpolator.INTERPOLATE_AHD)
  rgbImage = interpolator.interpolate(grayImage)

  print('Converting from camera RGB to sRGB...')
  refinery.ConvertToRgbFilter().filter(rgbImage)

  print('Gamma-correcting...')
  histogram = refinery.RGBHistogram(rgbImage)
  gammaCurve = refinery.GammaCurveU16(histogram)
  refinery.GammaFilter().filter(rgbImage, gammaCurve)

  if using_pil:
    print('Transforming into Python Imaging Library (PIL) Image...')
    width = rgbImage.width()
    height = rgbImage.height()
    ba = bytes('\0') * (width * height * 3)
    rgbImage.fillRgb8(ba)
    pilImage = PIL.Image.fromstring('RGB', (width, height), ba)

    print('Finding image orientation...')
    orientation = exifData.getInt('Exif.Image.Orientation')
    if orientation == 1:
      print('(original is fine, no reorientation needed)')
    elif orientation == 2:
      print('(original is flipped horizontally)')
      pilImage = pilImage.transpose(PIL.Image.FLIP_LEFT_RIGHT)
    elif orientation == 3:
      print('(original is rotated 180 degrees)')
      pilImage = pilImage.transpose(PIL.Image.ROTATE_180)
    elif orientation == 4:
      print('(original is flipped vertically)')
      pilImage = pilImage.transpose(PIL.Image.FLIP_TOP_BOTTOM)
    elif orientation == 5:
      print('(original is flipped by a top-left/bottom-right mirror')
      pilImage = pilImage.transpose(PIL.Image.ROTATE_270).transpose(PIL.Image.FLIP_TOP_BOTTOM)
    elif orientation == 6:
      print('(original is rotated 90 degrees counter-clockwise)')
      pilImage = pilImage.transpose(PIL.Image.ROTATE_90)
    elif orientation == 7:
      print('(original is flipped horizontally and rotated 90 degrees clockwise)')
      pilImage = pilImage.transpose(PIL.Image.ROTATE_90).transpose(PIL.Image.FLIP_TOP_BOTTOM)
    elif orientation == 8:
      print('(original is rotated 90 degrees clockwise)')
      pilImage = pilImage.transpose(PIL.Image.ROTATE_270)
    else:
      print('Error! Invalid orientation "%r", ignoring' % orientation)

    print('Writing image to %s...' % outfile)
    pilImage.save(outfile)
  else:
    print('Writing image to %s... (8-bit PPM)' % outfile)
    writer = refinery.ImageWriter()
    writer.writeImage(rgbImage, outfile, 8)

  print('Done!')

if __name__ == '__main__':
  import sys

  if len(sys.argv) != 3:
    print >>sys.stderr, 'Usage: %s <infile> <outfile>' % sys.argv[0]
    sys.exit(1)

  convert(sys.argv[1], sys.argv[2])
