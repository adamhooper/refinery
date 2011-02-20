#include "refinery/interpolate.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

#include "refinery/camera.h"
#include "refinery/image.h"
#include "refinery/image_tile.h"

namespace refinery {

namespace {
  void interpolateBorder(RGBImage& rgbImage, const GrayImage& image, int border) {
    const int width = image.width(), height = image.height();
    const int top = 0, left = 0, right = width, bottom = height;

    for (int row = top; row < bottom; row++) {
      for (int col = left; col < right; col++) {
        if (col == border && row >= border && row < bottom - border) {
          col = right - border;
        }

        unsigned int sum[3] = { 0, 0, 0 };
        unsigned int count[3] = { 0, 0, 0 };

        for (int y = row - 1; y <= row + 1; y++) {
          if (y < top || y >= bottom) continue;

          for (int x = col - 1; x <= col + 1; x++) {
            if (x < left || x >= right) continue;

            Point p(y, x);
            RGBImage::ColorType c = image.colorAtPoint(p);
            sum[c] += image.constPixelAtPoint(p).value();
            count[c]++;
          }
        }

        Point curP(row, col);
        RGBImage::ColorType curC = image.colorAtPoint(curP);
        for (RGBImage::ColorType c = 0; c < 3; c++) {
          if (c == curC) {
            rgbImage.pixelAtPoint(curP)[c] =
                image.constPixelAtPoint(curP).value();
          } else if (count[c]) {
            rgbImage.pixelAtPoint(curP)[c] = sum[c] / count[c];
          }
        }
      }
    }
  }
} // namespace {}

class BilinearInterpolator {
  struct PixelInstructions {
    unsigned int adjacentWeights[8];
    unsigned int adjacentColors[8];
    unsigned int otherColors[2];
    unsigned int divisions[2];
  };

  class PixelsInstructions {
    PixelInstructions pixels[16][16];

    public:
    PixelsInstructions(const GrayImage& image)
    {
      for (unsigned int row = 0; row < 16; row++) {
        for (unsigned int col = 0; col < 16; col++) {
          PixelInstructions& instructions = pixels[row & 15][col & 15];

          unsigned int sums[3] = { 0, 0, 0 };

          unsigned int adjIndex = 0;
          for (int y = -1; y <= 1; y++) {
            for (int x = -1; x <= 1; x++) {
              unsigned int shift = (x == 0) + (y == 0);
              if (shift == 2) continue;

              instructions.adjacentWeights[adjIndex] = shift;
              unsigned int color = image.colorAtPoint(row + y, col + x);
              instructions.adjacentColors[adjIndex] = color;

              sums[color] += 1 << shift;

              adjIndex++;
            }
          }

          const unsigned int colorAtPixel = image.colorAtPoint(row, col);
          unsigned int colorIndex = 0;
          for (unsigned int color = 0; color < 3; color++) {
            if (color == colorAtPixel) continue;

            instructions.otherColors[colorIndex] = color;
            instructions.divisions[colorIndex] = 256 / sums[color];

            colorIndex++;
          }
        }
      }
    }

    const PixelInstructions& getPixelInstructions(
        unsigned int row, unsigned int col) const
    {
      return pixels[row & 15][col & 15];
    }
  };

public:
  RGBImage* interpolate(const GrayImage& image) {
    std::auto_ptr<RGBImage> rgbImagePtr(
        new RGBImage(image.cameraData(), image.width(), image.height()));
    RGBImage& rgbImage(*rgbImagePtr);

    interpolateBorder(rgbImage, image, 1);

    const unsigned int width = image.width();
    const unsigned int height = image.height();

    const unsigned int top = 1;
    const unsigned int bottom = height - 1;
    const unsigned int left = 1;
    const unsigned int right = width - 1;

    const int adjacentOffsets[8] = {
      -width - 1, -width, -width + 1, -1, 1, width -1 , width, width + 1
    };
    const PixelsInstructions pixelsInstructions(image);

    for (unsigned int row = top; row < bottom; row++) {
      RGBImage::PixelType* pix(rgbImage.pixelsAtPoint(row, left));
      const GrayImage::PixelType* grayPix(image.constPixelsAtPoint(row, left));

      for (unsigned int col = left; col < right; col++, pix++, grayPix++) {
        const PixelInstructions& instructions(
            pixelsInstructions.getPixelInstructions(row, col));

        unsigned int sums[3] = { 0, 0, 0 };

        for (unsigned int adjIndex = 0; adjIndex < 8; adjIndex++) {
          const int adjOffset = adjacentOffsets[adjIndex];
          const unsigned int adjWeight = instructions.adjacentWeights[adjIndex];
          const unsigned int adjColor = instructions.adjacentColors[adjIndex];

          sums[adjColor] +=
              static_cast<unsigned int>(grayPix[adjOffset].value())
              << adjWeight;
        }

        for (unsigned int colorIndex = 0; colorIndex < 2; colorIndex++) {
          const unsigned int color = instructions.otherColors[colorIndex];
          const unsigned int division = instructions.divisions[colorIndex];

          pix[0][color] =
              static_cast<RGBImage::ValueType>(sums[color] * division >> 8);
        }
      }
    }

    return rgbImagePtr.release();
  }
};

namespace { std::vector<float> xyzCbrtLookup; } // FIXME make singleton

class AHDInterpolator {
private:
  typedef RGBImage XYZImage;
  struct HomogeneityPixel {
    typedef char ValueType;
    typedef unsigned int ColorType;
    ValueType h;
    ValueType v;
    ValueType diff;

    HomogeneityPixel() : h(h), v(v), diff(diff) {}
    ValueType& operator[](const ColorType& index) {
      return index == 0 ? h : v;
    }
    const ValueType& at(const ColorType& index) const {
      return index == 0 ? h : v;
    }
  };
  typedef Image<HomogeneityPixel> HomogeneityImage;
  typedef ImageTile<HomogeneityImage> HomogeneityTile;

  typedef ImageTile<RGBImage> RGBImageTile;
  typedef ImageTile<LABImage> LABImageTile;

  typedef RGBImage::ColorType Color;

  static const unsigned int H = 0;
  static const unsigned int V = 1;

  float xyzCbrtMin;
  float xyzCbrtMax;

public:
  AHDInterpolator()
  {
    if (xyzCbrtLookup.size() != 0x20000) {
      // This is thread-safe: worst-case we just set the same value multiple
      // times.
      xyzCbrtLookup.reserve(0x20000);

      for (int i = 0; i < 0x10000; i++) {
        double r = i / 65535.0;
        xyzCbrtLookup[i] = 64.0f * (r > 0.008856 ? std::pow(r, 1.0/3) : 7.787*r + 16.0/116);
      }

      xyzCbrtMin = xyzCbrtLookup[0];
      xyzCbrtMax = xyzCbrtLookup[0xffff];

      for (int i = 0x10000; i <= 0x17fff; i++) {
        xyzCbrtLookup[i] = xyzCbrtMax;
      }
      for (int i = 0x18000; i <= 0x1ffff; i++) {
        xyzCbrtLookup[i] = xyzCbrtMin;
      }
    }
  }

private:

  /**
   * Returns cbrt(f/65536), unless f is <= 580 in which case it's a linear
   * formula.
   *
   * Google how to convert RGB to XYZ to LAB. This formula pops up.
   *
   * The return value is multiplied by 64.0f, because we convert to an integer.
   *
   * If f is a small negative number or a bit over 65535, it will be clamped.
   * This will only work between around -32767 and 98302.
   */
  float xyz64Cbrt(float f) const
  {
    unsigned int i(static_cast<int>(f) & 0x1ffff);

    return xyzCbrtLookup[i];
  }

  /**
   * Returns v bounded by bound1 and bound2.
   *
   * bound1 is the min and bound2 is the max, or vice-versa.
   */
  RGBImage::ValueType bound(
      const RGBImage::ValueType& v,
      const RGBImage::ValueType& bound1, const RGBImage::ValueType& bound2)
  {
    return std::min(
      std::max(bound1, std::min(v, bound2)),
      std::max(bound2, std::min(v, bound1))
      );
  }

  /*
   * Make G values from hImageTile and vImageTile be the approximated Gs we
   * get after looking at original grayscale image.
   *
   * The final image will have a checkerboard pattern of G estimates. The
   * original image's G pixels aren't copied (that'll happen later, only so we
   * get a tiny speedup).
   */
  void createGreenDirectionalImages(
      const GrayImage& image,
      RGBImageTile& hImageTile, RGBImageTile& vImageTile)
  {
    const unsigned int top = hImageTile.top();
    const unsigned int left = hImageTile.left();
    const unsigned int right = hImageTile.right();
    const unsigned int bottom = hImageTile.bottom();

    const int width = image.width();

    for (unsigned int row = top; row < bottom; row++) {
      unsigned int col =
          left + (image.colorAtPoint(Point(row, left)) & 1); // 1st R or B

      const GrayImage::PixelType* pix(image.constPixelsAtPoint(row, col));

      const GrayImage::PixelType* pixAbove(&pix[-width]);
      const GrayImage::PixelType* pix2Above(&pix[-2 * width]);
      const GrayImage::PixelType* pixBelow(&pix[width]);
      const GrayImage::PixelType* pix2Below(&pix[2 * width]);

      RGBImageTile::PixelType* hPix(hImageTile.pixelsAtImageCoords(row, col));
      RGBImageTile::PixelType* vPix(vImageTile.pixelsAtImageCoords(row, col));

      for (; col < right;
          col += 2, pix += 2, hPix += 2, vPix += 2,
          pixAbove += 2, pix2Above += 2, pixBelow += 2, pix2Below += 2) {
        RGBImage::ValueType hValue =
          ((pix[-1].value() + pix[0].value() + pix[1].value()) * 2 // G, c, G
           - pix[-2].value() - pix[2].value()) >> 2; // c, c
        hPix[0].g() = bound(hValue, pix[-1].value(), pix[1].value()); // G, G

        RGBImage::ValueType vValue =
          ((pixAbove[0].value() + pix[0].value() + pixBelow[0].value()) * 2 // G, c, G
            - pix2Above[0].value() - pix2Below[0].value()) >> 2; // c, c
        vPix[0].g() = bound(vValue, pixAbove[0].value(), pixBelow[0].value()); // G, G
      }
    }
  }

  RGBImage::ValueType clamp16(int val) {
    // Assume right-shift of signed values leaves 1s, not 0s
    const unsigned int trashBits = val >> 16;
    return trashBits ? (~trashBits >> 16) : val;
  }

  inline void incrPointers(
      int n, const GrayImage::PixelType* (&pix),
      const GrayImage::PixelType* (&pixAbove),
      const GrayImage::PixelType* (&pixBelow),
      const RGBImageTile::PixelType* (&dPixAbove),
      const RGBImageTile::PixelType* (&dPixBelow))
  {
    pix += n;
    pixAbove += n;
    pixBelow += n;
    dPixAbove += n;
    dPixBelow += n;
  }

  inline void fillRandBinGPixel(
      RGBImageTile::PixelType* dPix, const Color rowC, const Color colC,
      const GrayImage::PixelType* pix,
      const GrayImage::PixelType* pixAbove,
      const GrayImage::PixelType* pixBelow,
      const RGBImageTile::PixelType* dPixAbove,
      const RGBImageTile::PixelType* dPixBelow)
  {
    const int colCValue =
        pix[0].value() + // G
        ((pixAbove[0].value() + pixBelow[0].value() // colC, colC
          - dPixAbove[0].g() - dPixBelow[0].g()) >> 1); // G, G
    dPix[0][colC] = clamp16(colCValue);

    const int rowCValue =
        pix[0].value() + // G
        ((pix[-1].value() + pix[1].value() // rowC, rowC
          - dPix[-1].g() - dPix[1].g()) >> 1); // G, G
    dPix[0][rowC] = clamp16(rowCValue);
  }

  inline void fillRandBinBorRPixel(
      RGBImageTile::PixelType* dPix, const Color rowC, const Color colC,
      const GrayImage::PixelType* pix,
      const GrayImage::PixelType* pixAbove,
      const GrayImage::PixelType* pixBelow,
      const RGBImageTile::PixelType* dPixAbove,
      const RGBImageTile::PixelType* dPixBelow)
  {
    const int colCValue =
        dPix[0].g() + // G
        ((pixAbove[-1].value() + pixAbove[1].value() // colC, colC
          + pixBelow[-1].value() + pixBelow[1].value() // colC, colC
          - dPixAbove[-1].g() - dPixAbove[1].g() // G, G
          - dPixBelow[-1].g() - dPixBelow[1].g() // G, G
          + 1) >> 2);
    dPix[0][colC] = clamp16(colCValue);
  }

  void fillDirectionalImage(const GrayImage& image, RGBImageTile& dirImageTile)
  {
    const unsigned int top = dirImageTile.top() + 1;
    const unsigned int left = dirImageTile.left() + 1;
    const unsigned int right = dirImageTile.right() - 1;
    const unsigned int bottom = dirImageTile.bottom() - 1;

    const int width = image.width();
    const int dWidth = dirImageTile.width();

    const Color G = 1;

    for (unsigned int row = top; row < bottom; row++) {
      /*
       * We assume the bayer pattern in the filter looks like one of these
       * (copied from dcraw comments):
       *
       *    0 1 2 3 4 5    0 1 2 3 4 5    0 1 2 3 4 5    0 1 2 3 4 5
       *    0 B G B G B G  0 G R G R G R  0 G B G B G B  0 R G R G R G
       *    1 G R G R G R  1 B G B G B G  1 R G R G R G  1 G B G B G B
       *    2 B G B G B G  2 G R G R G R  2 G B G B G B  2 R G R G R G
       *    3 G R G R G R  3 B G B G B G  3 R G R G R G  3 G B G B G B
       *
       * So we know:
       * - every 2nd row starts with the same color; the other is green
       * - horizontally, we only see two colors on a row (one is green)
       * - vertically, we only see two colors on a column (one is green)
       * - if we're looking at a green pixel and we know the row's other
       *   color c, the column's color is 2-c
       */

      Color rowC;
      Color colC;

      Color c(image.colorAtPoint(Point(row, left)));
      if (c == G) {
        rowC = image.colorAtPoint(Point(row, left + 1));
        colC = 2 - rowC;
      } else {
        rowC = c;
        colC = 2 - c;
      }

      RGBImageTile::PixelType* dPix(
          &dirImageTile.pixelsAtImageCoords(row, left + (c != G))[0]);
      const RGBImageTile::PixelType* dPixAbove(&dPix[-dWidth]);
      const RGBImageTile::PixelType* dPixBelow(&dPix[dWidth]);

      const GrayImage::PixelType* pix(
          image.constPixelsAtPoint(row, left + (c != G)));
      const GrayImage::PixelType* pixAbove(&pix[-width]);
      const GrayImage::PixelType* pixBelow(&pix[width]);

      for (unsigned int col = left + (c != G); col < right; col += 2) {
        dPix[0].g() = pix[0].value(); // see createGreenDirectionalImages comment

        fillRandBinGPixel(
            dPix, rowC, colC, pix, pixAbove, pixBelow, dPixAbove, dPixBelow);
        dPix += 2;
        incrPointers(2, pix, pixAbove, pixBelow, dPixAbove, dPixBelow);
      }

      dPix = dirImageTile.pixelsAtImageCoords(row, left + (c == G));
      dPixAbove = &dPix[-dWidth];
      dPixBelow = &dPix[dWidth];

      pix = &image.constPixelsAtRow(row)[left + (c == G)];
      pixAbove = &pix[-width];
      pixBelow = &pix[width];

      for (unsigned int col = left + (c == G); col < right; col += 2) {
        dPix[0][rowC] = pix[0].value();

        fillRandBinBorRPixel(
            dPix, rowC, colC, pix, pixAbove, pixBelow, dPixAbove, dPixBelow);

        dPix += 2;
        incrPointers(2, pix, pixAbove, pixBelow, dPixAbove, dPixBelow);
      }
    }
  }

  void rgbToLab(
      const RGBImage::PixelType& rgb, LABImage::PixelType& lab,
      const float (&cameraToXyz)[3][4])
  {
    float cbrtX = xyz64Cbrt(0.5f
        + cameraToXyz[0][0] * rgb.r()
        + cameraToXyz[0][1] * rgb.g()
        + cameraToXyz[0][2] * rgb.b());
    float cbrtY = xyz64Cbrt(0.5f
        + cameraToXyz[1][0] * rgb.r()
        + cameraToXyz[1][1] * rgb.g()
        + cameraToXyz[1][2] * rgb.b());
    float cbrtZ = xyz64Cbrt(0.5f
        + cameraToXyz[2][0] * rgb.r()
        + cameraToXyz[2][1] * rgb.g()
        + cameraToXyz[2][2] * rgb.b());

    lab.l() = static_cast<LABImage::ValueType>(116.0f * cbrtY - (64.0f*16.0f));
    lab.a() = static_cast<LABImage::ValueType>(500.0f * (cbrtX - cbrtY));
    lab.b() = static_cast<LABImage::ValueType>(200.0f * (cbrtY - cbrtZ));
  }

  void createCielabImage(
      const RGBImageTile& imageTile, LABImageTile& labImageTile,
      const float (&cameraToXyz)[3][4])
  {
    const unsigned int top = imageTile.top() + 1;
    const unsigned int left = imageTile.left() + 1;
    const unsigned int right = imageTile.right() - 1;
    const unsigned int bottom = imageTile.bottom() - 1;

    for (unsigned int row = top; row < bottom; row++) {
      const RGBImageTile::PixelType* pix(
          &imageTile.constPixelsAtImageCoords(row, left)[0]);

      LABImageTile::PixelType* lPix(
          &labImageTile.pixelsAtImageCoords(row, left)[0]);

      for (unsigned int col = left; col < right; col++) {
        rgbToLab(pix[0], lPix[0], cameraToXyz);
        pix++;
        lPix++;
      }
    }
  }

  unsigned int epsilon(const unsigned int (&diff)[2][4])
  {
    return std::min(
        std::max(diff[H][0], diff[H][1]),
        std::max(diff[V][2], diff[V][3])) + 1;
  }

  void fillHomogeneityMap(
      const LABImageTile& hLabImageTile, const LABImageTile& vLabImageTile,
      HomogeneityTile& homoTile)
  {
    const unsigned int top = hLabImageTile.top() + 2;
    const unsigned int left = hLabImageTile.left() + 2;
    const unsigned int right = hLabImageTile.right() - 2;
    const unsigned int bottom = hLabImageTile.bottom() - 2;

    const int width = hLabImageTile.width();

    const int ADJ[4] = { -1, 1, -width, width };

    for (unsigned int row = top; row < bottom; row++) {
      HomogeneityTile::PixelType* homoPix(
          &homoTile.pixelsAtImageCoords(row, left)[0]);

      const LABImageTile::PixelType* labPix[2] = {
        &hLabImageTile.constPixelsAtImageCoords(row, left)[0],
        &vLabImageTile.constPixelsAtImageCoords(row, left)[0]
      };

      const LABImageTile::PixelType* labAdjPix[2][4];
      for (unsigned int adjDir = 0; adjDir < 4; adjDir++) {
        labAdjPix[H][adjDir] = &labPix[H][ADJ[adjDir]];
        labAdjPix[V][adjDir] = &labPix[V][ADJ[adjDir]];
      }

      for (unsigned int col = left; col < right; col++, homoPix++) {
        unsigned int lDiff[2][4], abDiff[2][4];

        for (unsigned int dir = H; dir <= V; dir++) {
          const LABImageTile::PixelType* dirLabPix(labPix[dir]);

          for (unsigned int adjDir = 0; adjDir < 4; adjDir++) {
            const LABImageTile::PixelType* adjLabPix(labAdjPix[dir][adjDir]);

            const int adjDiffL = dirLabPix[0].l() - adjLabPix[0].l();
            lDiff[dir][adjDir] = std::abs(adjDiffL);

            const int adjDiffA = dirLabPix[0].a() - adjLabPix[0].a();
            const int adjDiffB = dirLabPix[0].b() - adjLabPix[0].b();
            abDiff[dir][adjDir] = adjDiffA * adjDiffA + adjDiffB * adjDiffB;

            labAdjPix[dir][adjDir]++;
          }

          labPix[dir]++;
        }

        const unsigned int lEps = epsilon(lDiff);
        const unsigned int abEps = epsilon(abDiff);

        for (unsigned int dir = H; dir <= V; dir++) {
          unsigned int homogeneity = 0;
          for (unsigned int adjDir = 0; adjDir < 4; adjDir++) {
#if 0
            /* This is the slow way--it uses branches */
            if (lDiff[dir][adjDir] < lEps && abDiff[dir][adjDir] < abEps)
            {
              homogeneity++;
            }
#else
            /*
             * Here's a branch-free equivalent. It assumes negative numbers
             * start with a 1 bit (which the C standard mandates)
             */
            homogeneity += (
                ((lDiff[dir][adjDir] - lEps) & (abDiff[dir][adjDir] - abEps))
                >> (sizeof(lEps) * 8 - 1));
#endif
          }

          homoPix[0][dir] = homogeneity;
        }
      }
    }
  }

  void fillImage(
      RGBImage& rgbImage,
      const RGBImageTile& hImageTile, const RGBImageTile& vImageTile,
      HomogeneityTile& homoTile)
  {
    const unsigned int top = hImageTile.top() + 3;
    const unsigned int left = hImageTile.left() + 3;
    const unsigned int right = hImageTile.right() - 3;
    const unsigned int bottom = hImageTile.bottom() - 3;

    const int tileWidth = hImageTile.width();

    for (unsigned int row = top; row < bottom; row++) {
      HomogeneityTile::PixelType* homoPix(
          &homoTile.pixelsAtImageCoords(row, left)[0]);
      const HomogeneityTile::PixelType* homoPixAbove(&homoPix[-tileWidth]);
      const HomogeneityTile::PixelType* homoPixBelow(&homoPix[tileWidth]);

      for (unsigned int col = left; col < right;
          col++, homoPix++, homoPixAbove++, homoPixBelow++) {

        unsigned int hm[2] = { 0, 0 };
        for (unsigned int dir = H; dir <= V; dir++) {
          for (int i = -1; i <= 1; i++) {
            hm[dir] += homoPixAbove[i].at(dir);
            hm[dir] += homoPix[i].at(dir);
            hm[dir] += homoPixBelow[i].at(dir);
          }
        }

        homoPix[0].diff = hm[H] - hm[V];
      }
    }

    for (unsigned int row = top; row < bottom; row++) {
      RGBImage::PixelType* pix(rgbImage.pixelsAtPoint(row, left));

      const HomogeneityTile::PixelType* homoPix(
          homoTile.constPixelsAtImageCoords(row, left));
      const RGBImageTile::PixelType* hPix(
          hImageTile.constPixelsAtImageCoords(row, left));
      const RGBImageTile::PixelType* vPix(
          vImageTile.constPixelsAtImageCoords(row, left));

      for (unsigned int col = left; col < right;
          col++, pix++, hPix++, vPix++, homoPix++) {
        if (homoPix[0].diff > 0) {
          pix[0] = hPix[0];
        } else if (homoPix[0].diff < 0) {
          pix[0] = vPix[0];
        } else {
          pix[0].r() = (hPix[0].r() + vPix[0].r()) >> 1;
          pix[0].g() = (hPix[0].g() + vPix[0].g()) >> 1;
          pix[0].b() = (hPix[0].b() + vPix[0].b()) >> 1;
        }
      }
    }
  }

public:
  RGBImage* interpolate(const GrayImage& image) {
    const unsigned int border = 5;

    const Camera::ColorConversionData colorData(
        image.cameraData().colorConversionData());
    float cameraToXyz[3][4];
    for (unsigned int i = 0; i < 3; i++) {
      for (unsigned int j = 0; j < image.cameraData().colors(); j++) {
        // convert from double to float, for speed
        cameraToXyz[i][j] = colorData.cameraToXyz[i][j];
      }
    }

    std::auto_ptr<RGBImage> rgbImagePtr(new RGBImage(
          image.cameraData(), image.width(), image.height()));
    RGBImage& rgbImage(*rgbImagePtr);

    interpolateBorder(rgbImage, image, border);

    const unsigned int height = image.height();
    const unsigned int width = image.width();

    const unsigned int tileHeight = 256;
    const unsigned int tileWidth = 256;

    const unsigned int margin = 3;
    const unsigned int left = border - margin;
    const unsigned int top = border - margin;
    const unsigned int bottom = height - border;
    const unsigned int right = width - border;

    Point imageSize(height, width);
    Point tileSize(tileHeight, tileWidth);

#if _OPENMP
#pragma omp parallel
#endif /* _OPENMP */
    {
      Point tileTopLeft(border - margin, border - margin);

      RGBImageTile hImageTile(imageSize, tileTopLeft, tileSize, border, margin);
      RGBImageTile vImageTile(imageSize, tileTopLeft, tileSize, border, margin);
      LABImageTile hLabImageTile(
          imageSize, tileTopLeft, tileSize, border, margin);
      LABImageTile vLabImageTile(
          imageSize, tileTopLeft, tileSize, border, margin);
      HomogeneityTile homoTile(imageSize, tileTopLeft, tileSize, border, margin);

#if _OPENMP
#pragma omp for schedule(dynamic)
#endif /* _OPENMP */
      for (unsigned int row = top; row < bottom; row += tileHeight - 2*margin) {
        tileTopLeft.row = row;

        for (unsigned int col = left; col < right; col += tileWidth - 2*margin) {
          tileTopLeft.col = col;

          hImageTile.setTopLeft(tileTopLeft);
          vImageTile.setTopLeft(tileTopLeft);
          hLabImageTile.setTopLeft(tileTopLeft);
          vLabImageTile.setTopLeft(tileTopLeft);
          homoTile.setTopLeft(tileTopLeft);

          createGreenDirectionalImages(image, hImageTile, vImageTile);

          fillDirectionalImage(image, hImageTile);
          fillDirectionalImage(image, vImageTile);

          createCielabImage(hImageTile, hLabImageTile, cameraToXyz);
          createCielabImage(vImageTile, vLabImageTile, cameraToXyz);

          fillHomogeneityMap(hLabImageTile, vLabImageTile, homoTile);

          fillImage(rgbImage, hImageTile, vImageTile, homoTile);
        }
      }
    }

    return rgbImagePtr.release();
  }
};

Interpolator::Interpolator(const Interpolator::Type& type)
  : mType(type)
{
}

RGBImage* Interpolator::interpolate(const GrayImage& image)
{
  switch (mType) {
    case INTERPOLATE_AHD:
      {
        AHDInterpolator ahdInterpolator;
        return ahdInterpolator.interpolate(image);
      }
    case INTERPOLATE_BILINEAR:
      {
        BilinearInterpolator bilinearInterpolator;
        return bilinearInterpolator.interpolate(image);
      }
  }
}

}
