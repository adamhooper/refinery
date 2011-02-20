#include "refinery/exif.h"

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stack>
#include <string>

#include <boost/any.hpp>
#include <boost/cstdint.hpp>
#include <boost/detail/endian.hpp>
#include <boost/tr1/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

#include "c_file_istreambuf.h"

namespace refinery {

class InMemoryExifDataMixin {
protected:
  std::tr1::unordered_map<std::string, boost::any> mData;

public:
  typedef ExifData::byte byte;

  bool hasKey(const char* key) const {
    return mData.find(key) != mData.end();
  }

  std::string getString(const char* key) const {
    boost::any value(mData.at(key));
    return boost::any_cast<std::string>(value);
  }

  void getBytes(const char* key, std::vector<byte>& outBytes) const {
    typedef const std::vector<byte>& ArrayType;

    const boost::any value(mData.at(key));
    ArrayType bytes(boost::any_cast<ArrayType>(value));
    outBytes = bytes;
  }

  int getInt(const char* key) const {
    boost::any value(mData.at(key));
    return boost::any_cast<int>(value);
  }

  int getFloat(const char* key) const {
    boost::any value(mData.at(key));
    return boost::any_cast<float>(value);
  }

  void setString(const char* key, const std::string& s) {
    mData[key] = s;
  }

  void setInt(const char* key, int i) {
    mData[key] = i;
  }

  void setFloat(const char* key, float f) {
    mData[key] = f;
  }

  void setBytes(const char* key, const std::vector<byte>& bytes) {
    mData[key] = bytes;
  }
};

class InMemoryExifData::Impl : public InMemoryExifDataMixin {
};

InMemoryExifData::InMemoryExifData()
  : impl(new Impl())
{
}

InMemoryExifData::~InMemoryExifData()
{
  delete impl;
}

bool InMemoryExifData::hasKey(const char* key) const
{
  return impl->hasKey(key);
}

std::string InMemoryExifData::getString(const char* key) const
{
  return impl->getString(key);
}

void InMemoryExifData::getBytes(
    const char* key, std::vector<byte>& outBytes) const
{
  impl->getBytes(key, outBytes);
}

int InMemoryExifData::getInt(const char* key) const
{
  return impl->getInt(key);
}

float InMemoryExifData::getFloat(const char* key) const
{
  return impl->getFloat(key);
}

void InMemoryExifData::setString(const char* key, const std::string& s)
{
  impl->setString(key, s);
}

/*
 * This code is uncannily similar to dcraw's TIFF-parsing code.
 *
 * The advantage is that it's public-domain. The disadvantages are that
 * this implementation offers a new opportunity for bugs and that the
 * maintainer has to manually incorporate any changes to dcraw.
 *
 * This code is based on dcraw-8.99. If a new version of dcraw comes out,
 * the code and this comment should be updated.
 */
class DcrawExifData::Impl : public InMemoryExifDataMixin {
  std::auto_ptr<c_file_istreambuf> mFileIStream;
  std::streambuf& mIStream;

  struct Sandbox {
    std::streambuf& mIStream;
    // would prefer std::tr1::unique_ptr...
    std::stack<boost::shared_ptr<std::streambuf> > mOtherStreamBufs;

    int* ifp; // dummy variable
#ifndef SEEK_SET
    static const int SEEK_SET = 0xdeadbeef;
#endif
#ifndef SEEK_CUR
    static const int SEEK_CUR = 0xbeefdead;
#endif
    static const int use_camera_wb = 0;
    static const int verbose = 0;
    float d65_white[3];
    const char* ifname;

    typedef unsigned char uchar;
    typedef unsigned short ushort;
    typedef boost::int64_t INT64;
    typedef boost::uint64_t UINT64;
    typedef int FILE;

    struct jhead {
      int bits, high, wide, clrs, sraw, psv, restart, vpred[6];
      unsigned short *huff[6], *free[4], *row;
    };

    struct _tiff_ifd {
        int width, height, bps, comp, phint, offset, flip, samples;
        unsigned int bytes;
    } tiff_ifd[10];

    struct _ph1 {
      int format, key_off, black, black_off, split_col, tag_21a;
      float tag_210;
    } ph1;

    short order;
    char cdesc[5], desc[512], make[64], model[64], model2[64], artist[64];
    float flash_used, canon_ev, iso_speed, aperture, shutter, focal_len;
    std::time_t timestamp;
    unsigned int shot_order, kodak_cbpp, unique_id, exif_cfa;
    off_t thumb_offset, strip_offset, data_offset, meta_offset, profile_offset;
    unsigned int thumb_length, meta_length, data_length, profile_length;
    unsigned int thumb_misc, fuji_layout, shot_select;
    unsigned int tiff_nifds, tiff_samples, tiff_bps, tiff_compress;
    unsigned int black, maximum, mix_green, raw_color, zero_is_bad;
    unsigned int zero_after_ff, is_raw, dng_version, is_foveon, data_error;
    unsigned int tile_width, tile_length, gpsdata[32], load_flags;
    unsigned short raw_width, raw_height, width, height, top_margin, left_margin;
    unsigned short thumb_width, thumb_height, fuji_width;
    unsigned int filters;
    float cam_mul[4], pre_mul[4], cmatrix[3][4], rgb_cam[3][4];
    int flip, tiff_flip;
    unsigned int colors;
    double pixel_aspect;
    double gamma_pwr, gamma_ts;
    int gamma_mode, gamma_imax;
    unsigned short white[8][8], curve[0x10000], cr2_slice[3], sraw_mul[4];

    const char* load_raw; // nothing
    const char* thumb_load_raw; // nothing
    const char* write_thumb; // nothing

    char adobe_dng_load_raw_lj;
    char adobe_dng_load_raw_nc;
    char canon_600_load_raw;
    char canon_compressed_load_raw;
    char canon_sraw_load_raw;
    char eight_bit_load_raw;
    char foveon_load_raw;
    char fuji_load_raw;
    char imacon_full_load_raw;
    char hasselblad_load_raw;
    char kodak_262_load_raw;
    char kodak_65000_load_raw;
    char kodak_dc120_load_raw;
    char kodak_jpeg_load_raw;
    char kodak_radc_load_raw;
    char kodak_rgb_load_raw;
    char kodak_thumb_load_raw;
    char kodak_ycbcr_load_raw;
    char kodak_yrgb_load_raw;
    char leaf_hdr_load_raw;
    char lossless_jpeg_load_raw;
    char minolta_rd175_load_raw;
    char olympus_load_raw;
    char nikon_compressed_load_raw;
    char nokia_load_raw;
    char packed_load_raw;
    char panasonic_load_raw;
    char pentax_load_raw;
    char phase_one_load_raw;
    char phase_one_load_raw_c;
    char quicktake_100_load_raw;
    char rollei_load_raw;
    char sinar_4shot_load_raw;
    char smal_v6_load_raw;
    char smal_v9_load_raw;
    char sony_arw_load_raw;
    char sony_arw2_load_raw;
    char sony_load_raw;
    char unpacked_load_raw;

    char foveon_thumb;
    char jpeg_thumb;
    char layer_thumb;
    char ppm_thumb;
    char rollei_thumb;

    Sandbox(std::streambuf& istream)
      : mIStream(istream)
    {
      ifp = 0;
      ifname = "";
      shot_select = 0;
      order = 0;
      d65_white[0] = 0.950456;
      d65_white[1] = 1;
      d65_white[2] = 1.088754;
    }

    std::streambuf& curStream()
    {
      if (mOtherStreamBufs.empty()) {
        return mIStream;
      } else {
        return *mOtherStreamBufs.top();
      }
    }

    void fread(char* s, std::size_t size, std::size_t nmemb, int* unused_ifp)
    {
      curStream().sgetn(s, size * nmemb);
    }
    template<typename T>
    void fread(T* s, std::size_t size, std::size_t nmemb, int* unused_ifp)
    {
      curStream().sgetn(reinterpret_cast<char*>(s), size * nmemb);
    }
    void /* yes, void */ fgets(char* s, int size, int* unused_ifp)
    {
      std::istream istream(&curStream());
      istream.getline(s, size); // fgets() puts a newline but we don't need it
    }

    void /* yes, void */ fscanf(int* unused_ifp, const char* format, int* i)
    {
      // super-simple: dcraw only ever uses "%d" and "%f"
      std::istream(&curStream()) >> *i;
    }
    void /* yes, void */ fscanf(int* unused_ifp, const char* format, float* f)
    {
      std::istream(&curStream()) >> *f;
    }

    FILE* tmpfile()
    {
      mOtherStreamBufs.push(
          boost::shared_ptr<std::streambuf>(new std::stringbuf()));
      return 0;
    }
    FILE* fopen(const char* path, const char* unused_mode)
    {
      boost::shared_ptr<std::filebuf> buf(new std::filebuf());
      buf->open(path, std::ios::in | std::ios::binary); // assume mode == "rb"
      mOtherStreamBufs.push(boost::shared_ptr<std::streambuf>(buf));
      return 0;
    }
    void fclose(int* unused_ifp)
    {
      mOtherStreamBufs.top().reset();
      mOtherStreamBufs.pop();
    }

    template<typename T>
    int fwrite(const T* buf, std::size_t size, std::size_t nmemb, int* unused_ifp)
    {
      const char* s = reinterpret_cast<const char*>(buf);
      return curStream().sputn(s, size * nmemb);
    }

    int fgetc(int* unused_ifp)
    {
      return curStream().sbumpc();
    }
    int getc(int* unused_ifp)
    {
      return curStream().sbumpc();
    }

    void fseek(int* unused_ifp, std::ios::streamoff off, int whence)
    {
      std::ios::seekdir way = std::ios::cur;
      if (whence == SEEK_SET) way = std::ios::beg;
      curStream().pubseekoff(off, way);
    }

    long ftell(int* unused_ifp)
    {
      return curStream().pubseekoff(0, std::ios::cur);
    }

    unsigned short sget2(char* s)
    {
      if (order == 0x4949) {
        return (s[1] << 8) | s[0];
      } else {
        return (s[0] << 8) | s[1];
      }
    }

    unsigned short sget2(unsigned char* us)
    {
      return sget2(reinterpret_cast<char*>(us));
    }

    unsigned short get2()
    {
      unsigned char c1 = curStream().sbumpc();
      unsigned char c2 = curStream().sbumpc();

      if (order == 0x4949) {
        return (c2 << 8) | c1;
      } else {
        return (c1 << 8) | c2;
      }
    }

    unsigned int get4()
    {
      unsigned char c1 = curStream().sbumpc();
      unsigned char c2 = curStream().sbumpc();
      unsigned char c3 = curStream().sbumpc();
      unsigned char c4 = curStream().sbumpc();

      if (order == 0x4949) {
        return (c4 << 24) | (c3 << 16) | (c2 << 8) | c1;
      } else {
        return (c1 << 24) | (c2 << 16) | (c3 << 8) | c4;
      }
    }

    unsigned int getint(int type)
    {
      return type == 3 ? get2() : get4();
    }

    float int_to_float(int i)
    {
      union { int i; float f; } u;
      u.i = i;
      return u.f;
    }

    boost::uint32_t htonl(boost::uint32_t hostlong)
    {
#ifdef BOOST_LITTLE_ENDIAN
      union { unsigned char c[4]; boost::uint32_t i; } u;
      u.i = hostlong;
      return (u.c[3] << 24) | (u.c[2] << 16) | (u.c[1] << 8) | u.c[0];
#else
      return hostlong;
#endif
    }

    double getreal(int type)
    {
      switch (type) {
        case 3: return get2();
        case 4: return get4();
        case 5:
          {
            double d = get4();
            return d / get4();
          }
        case 8: return static_cast<short>(get2());
        case 9: return static_cast<int>(get4());
        case 10:
          {
            double d = static_cast<int>(get4());
            return d / static_cast<int>(get4());
          }
        case 12:
          {
            const unsigned short tiff_style_machine_endianness =
#ifdef BOOST_LITTLE_ENDIAN
              0x4949
#else
# ifdef BOOST_BIG_ENDIAN
              0x4d4d
# else
#  error Need to #include <boost/detail/endian.hpp>
# endif
#endif
              ;
            int rev = 7 * (order != tiff_style_machine_endianness);
            union { char c[8]; double d; } u;
            for (int i = 0; i < 8; i++) {
              u.c[i ^ rev] = curStream().sbumpc();
            }
            return u.d;
          }
        default:
          return static_cast<unsigned char>(curStream().sbumpc());
      }
    }

    unsigned short* make_decoder_ref(const unsigned char** p)
    {
      // normally *p would be incremented, but we don't care
      return 0;
    }

    void gamma_curve(double pwr, double ts, int mode, int imax)
    {
      gamma_pwr = pwr;
      gamma_ts = ts;
      gamma_mode = mode;
      gamma_imax = imax;
    }

    void cam_xyz_coeff(const double (&cam_xyz)[4][3])
    {
      return;
    }

    void read_shorts(unsigned short* out, int n)
    {
      while (n-- > 0) {
        *out = get2();
        out++;
      }
    }

    template<typename T, typename U>
    T MIN(T v1, U v2)
    {
      T v2_t = static_cast<T>(v2);
      return v1 < v2_t ? v1 : v2_t;
    }

    template<typename T, typename U>
    void SWAP(T& v1, U& v2)
    {
      T t = v1;
      v1 = v2;
      v2 = t;
    }

    template<typename T>
    T SQR(T x)
    {
      return x * x;
    }

    char* calloc(std::size_t nmemb, std::size_t size)
    {
      std::auto_ptr<char> ret(new char[nmemb * size]);
      std::fill_n(ret.get(), nmemb * size, 0);
      return ret.release();
    }

    char* malloc(std::size_t size)
    {
      return new char[size];
    }

    template<typename T>
    void free(T* buf)
    {
      char* s = reinterpret_cast<char*>(buf);
      delete [] s;
    }

    template<typename T>
    void merror(T* ptr, const char* msg)
    {
      // no-op. *our* malloc() and calloc() use exceptions.
    }

    template<typename T>
    void memset(T* ptr, int c, size_t n)
    {
      char* s = reinterpret_cast<char*>(ptr);
      std::fill_n(s, n, c);
    }

    const char* _(const char* s)
    {
      return s;
    }

    template<typename T>
    T ABS(const T& val)
    {
      return std::abs(val);
    }

#define CLASS
#define FORC(cnt) for (c = 0; c < cnt; c++)
#define FORCC FORC(colors)
#define FORC3 FORC(3)
#define FORC4 FORC(4)
#include "dcraw_exif.ccpart"
#undef CLASS
#undef FORC
#undef FORCC
#undef FORC3
#undef FORC4
  }; // struct Sandbox
  std::auto_ptr<Sandbox> mSandbox;

  void init() {
    mSandbox.reset(new Sandbox(mIStream));
    mSandbox->identify();

    if (mSandbox->load_raw == &mSandbox->nikon_compressed_load_raw) {
      const unsigned int LONGEST_NEF_CURVE_SIZE = 683;
      const unsigned int LONGEST_NEF_CURVE_DATA_SIZE
        = 14 + LONGEST_NEF_CURVE_SIZE * 2;
      // The unpacker won't use the extra bytes if the curve is shorter
      mIStream.pubseekoff(mSandbox->meta_offset, std::ios::beg);
      std::vector<unsigned char> linearizationTable(
          LONGEST_NEF_CURVE_DATA_SIZE);
      mIStream.sgetn(reinterpret_cast<char*>(
            &linearizationTable[0]), LONGEST_NEF_CURVE_DATA_SIZE);
      this->setBytes("Exif.Nikon3.LinearizationTable", linearizationTable);
    }

    std::vector<unsigned char> cfaPattern(4, 0);
    // No idea if this is right; just know Nikon (1 2 0 1) is 0x49494949
    unsigned char filterPart = mSandbox->filters & 0xff;
    cfaPattern[0] = (filterPart >> 6) & 3;
    cfaPattern[1] = (filterPart >> 2) & 3;
    cfaPattern[2] = (filterPart >> 4) & 3;
    cfaPattern[3] = ((filterPart & 3) == 3 ? 1 : filterPart) & 3;

    this->setBytes("Exif.SubImage2.CFAPattern", cfaPattern);
    this->setString("Exif.Image.Model", std::string(mSandbox->make) + " " + mSandbox->model);
    this->setInt("Exif.SubImage2.BitsPerSample", mSandbox->tiff_bps);
    this->setInt("Exif.SubImage2.StripOffsets", mSandbox->data_offset);
    this->setInt("Exif.Image.Orientation", mSandbox->flip);
    this->setInt("Exif.SubImage2.ImageWidth", mSandbox->raw_width);
    this->setInt("Exif.SubImage2.ImageLength", mSandbox->raw_height);
  }

public:
  Impl(std::streambuf& istream) : InMemoryExifDataMixin(), mIStream(istream) {
    this->init();
  }

  Impl(FILE* f)
    : InMemoryExifDataMixin()
    , mFileIStream(new c_file_istreambuf(f)), mIStream(*mFileIStream) {
    this->init();
  }
};

DcrawExifData::DcrawExifData(std::streambuf& istream)
  : impl(new Impl(istream))
{
}

DcrawExifData::DcrawExifData(FILE* f)
  : impl(new Impl(f))
{
}

DcrawExifData::~DcrawExifData()
{
  delete impl;
}

const char* DcrawExifData::mime_type() const
{
  return "image/x-nikon-nef";
}

bool DcrawExifData::hasKey(const char* key) const
{
  return impl->hasKey(key);
}

std::string DcrawExifData::getString(const char* key) const
{
  return impl->getString(key);
}

void DcrawExifData::getBytes(
    const char* key, std::vector<byte>& outBytes) const
{
  impl->getBytes(key, outBytes);
}

int DcrawExifData::getInt(const char* key) const
{
  return impl->getInt(key);
}

float DcrawExifData::getFloat(const char* key) const
{
  return impl->getFloat(key);
}

} // namespace refinery
