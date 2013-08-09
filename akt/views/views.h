// -*- Mode:C++ -*-

#pragma once

#include "akt/ring.h"
#include "akt/assert.h"

#include "ch.h"
#include "hal.h"

#include <algorithm>

namespace akt {
  namespace views {
    typedef int16_t coord;
    typedef uint16_t pixel;

    class FontBase;
    class Canvas;
    class Size;

    struct Point {
      coord x, y;

      Point();
      Point(int x, int y);

      Point operator+(Point p) const;
      Point operator-(Point p) const;
      Point operator+(Size s)  const;
      Point operator-(Size s)  const;
      Point operator+(int n)   const;
      Point operator-(int n)   const;

      Point &operator +=(const Point &p);
    };

    inline bool operator==(const Point &p1, const Point &p2) {
      return p1.x == p2.x && p1.y == p2.y;
    }

    struct Size {
      coord w, h;

      Size();
      Size(int w, int h);
      inline bool empty() const {return w*h == 0;}
      inline Size operator/(int n) const {return Size(w/n, h/n);}
      inline Size operator*(int n) const {return Size(w*n, h*n);}
      inline Size operator+(int n) const {return Size(w+n, h+n);}
      inline Size operator-(int n) const {return Size(w-n, h-n);}
    };

    inline Point Point::operator+(Point p) const {return Point(x+p.x, y+p.y);}
    inline Point Point::operator-(Point p) const {return Point(x-p.x, y-p.y);}
    inline Point Point::operator+(Size s)  const {return Point(x+s.w, y+s.h);}
    inline Point Point::operator-(Size s)  const {return Point(x-s.w, y-s.h);}
    inline Point Point::operator+(int n)   const {return Point(x+n,   y+n);}
    inline Point Point::operator-(int n)   const {return Point(x-n,   y-n);}


    inline bool operator==(const Size &s1, const Size &s2) {
      return s1.w == s2.w && s1.h == s2.h;
    }

    class Rect {
      enum {
        INSIDE = 0,
        LEFT   = 1 << 0,
        RIGHT  = 1 << 1,
        BOTTOM = 1 << 2,
        TOP    = 1 << 3
      };
  
      int outcode(const Point &p) const;

    public:
      Point min, max;

      Rect();
      Rect(int x, int y, int w, int h);
      Rect(const Point &p0, const Point &p1);
      Rect(const Point &p, const Size &s);

      inline coord width()  const {return max.x - min.x;}
      inline coord height() const {return max.y - min.y;}
      inline Size  size()   const {return Size(width(), height());}
      inline bool  empty()  const {return width()*height() == 0;}
      inline bool  normal() const {return (max.x >= min.x) && (max.y >= min.y);}

      void normalize();
      bool intersects(const Rect &r) const;
      bool contains(const Point &p) const;
      bool contains(const Rect &r) const;

      Rect &operator +=(const Point &p);
      Rect &operator =(const Point &p);
      Rect operator &(const Rect &right) const;
      Rect operator +(Size s);
      Rect operator -(Size s);
      Point center() const;

      bool clip(Point &p0, Point &p1) const;
    };

    bool operator ==(const Rect &left, const Rect &right);
    Rect operator |(const Rect &left, const Rect &right);

#define BYTESWAP(u16) ((((u16) & 0xff) << 8) | ((u16) >> 8))
#define RGB565(r,g,b) (((((uint16_t) r) & 0xf8) << 8) | ((((uint16_t) g) & 0xfc) << 3) | (((uint16_t) b) >> 3))

    namespace rgb565 {
      enum color {
        BLACK        = BYTESWAP(RGB565(0,   0,   0)),
        NAVY         = BYTESWAP(RGB565(0,   0,   128)),
        DARK_GREEN   = BYTESWAP(RGB565(0,   128, 0)),
        DARK_CYAN    = BYTESWAP(RGB565(0,   128, 128)),
        MAROON       = BYTESWAP(RGB565(128, 0,   0)),
        PURPLE       = BYTESWAP(RGB565(128, 0,   128)),
        OLIVE        = BYTESWAP(RGB565(128, 128, 0)),
        LIGHT_GRAY   = BYTESWAP(RGB565(192, 192, 192)),
        DARK_GRAY    = BYTESWAP(RGB565(128, 128, 128)),
        BLUE         = BYTESWAP(RGB565(0,   0,   255)),
        GREEN        = BYTESWAP(RGB565(0,   255, 0)),
        CYAN         = BYTESWAP(RGB565(0,   255, 255)),
        RED          = BYTESWAP(RGB565(255, 0,   0)),
        MAGENTA      = BYTESWAP(RGB565(255, 0,   255)),
        YELLOW       = BYTESWAP(RGB565(255, 255, 0)),
        WHITE        = BYTESWAP(RGB565(255, 255, 255)),
        ORANGE       = BYTESWAP(RGB565(255, 165, 0)),
        GREEN_YELLOW = BYTESWAP(RGB565(173, 255, 47)),
      };
    };

    namespace b_and_w {
      enum color {
        WHITE = 0,
        BLACK = 1
      };
    };

    class FontBase {
    public:
      Size size;
      uint16_t offset;

      virtual Size measure(char c) const = 0;
      virtual Size measure(const char *str) const = 0;
      virtual void draw_char(Canvas *c, Point p, char ch, pixel value) const = 0;
      virtual void draw_string(Canvas *c, Point p, const char *str, pixel value) const = 0;
    };

    /*
     * This class handles fonts generated by the MikroElektronica GLCD Font Creator
     * program found at http://www.mikroe.com/glcd-font-creator/
     * 
     * The output of the program is an array of byte values that define the width
     * of each glyph in the font and which bits are set. Black pixels in the editor UI
     * correspond to '1' bits in the data, and '0' means white. The bit data for
     * each glyph is preceeded by a one byte integer specifying the rendered width
     * of that particular glyph. This allows proportionally spaced fonts.
     *
     * Glyphs are rasterized in column major order with the least significant bits
     * at the top. Consider the following 5x7 glyph for the numeral '1':
     *
     *
     *      0 0 1 0 0                 5 bytes of bit data
     *      0 1 1 0 0               ______________________
     *      0 0 1 0 0              v                      v
     *      0 0 1 0 0  ==> 0x04, 0x00, 0x42, 0x7f, 0x40, 0x00
     *      0 0 1 0 0        ^
     *      0 0 1 0 0         \
     *      0 1 1 1 0           1 byte of width        
     */

    class MikroFont : public FontBase {
      enum {
        GAP = 1, // minimum space between rendered glyphs
      };

      const uint8_t *data;
      uint16_t height_in_bytes;
      uint16_t glyph_stride;
      unsigned char first, last;

    public:
      typedef struct {
        uint8_t width;
        uint8_t data[];
      } glyph_t;

      MikroFont(const uint8_t *data, const Size &size, uint16_t offset, char first, char last);

      inline const glyph_t *glyph(unsigned char c) const {
        if (c < first || c > last) c = first;
        return (glyph_t *) (data + (c-first)*glyph_stride);
      }

      Size measure(char c) const;
      Size measure(const char *str) const;
      uint16_t draw_char1(Canvas *c, Point p, char ch, pixel value) const;
      void draw_char(Canvas *c, Point p, char ch, pixel value) const;
      void draw_string(Canvas *c, Point p, const char *str, pixel value) const;
    };

    struct PlaneBase {
      const Size size;

      PlaneBase(Size s) : size(s) {}

      virtual void  set_pixel(Point p, pixel value) = 0;
      virtual void  set_pixels(Point p, unsigned int n, pixel value);
      virtual pixel get_pixel(Point p) const = 0;
    };

    template<class T>
    class AddressablePlane : public PlaneBase {
    public:
      T *storage;

      AddressablePlane(T *storage, Size s) : PlaneBase(s), storage(storage) {}
      virtual void set_pixel(Point p, pixel value) {
        assert(p.x >= 0 && p.x < size.w);
        assert(p.y >= 0 && p.y < size.h);
        storage[p.y*size.w + p.x] = (T) value;
      }
      virtual void set_pixels(Point p, unsigned int n, pixel value) {
        assert(p.x >= 0 && p.x < size.w);
        assert(p.y >= 0 && p.y < size.h);

        T *s = &storage[p.y*size.w + p.x];
        while (n-- > 0) *s++ = (T) value;
      }
      virtual pixel get_pixel(Point p) const {
        return (pixel) storage[p.y*size.w + p.x];
      }
    };

    template<class T, unsigned W, unsigned H>
    class Plane : public AddressablePlane<T> {
      T data[W*H];

    public:
      Plane() : AddressablePlane<T>(data, Size(W, H)) {}
    };

    class BitPlaneBase : public PlaneBase {
      const unsigned stride;
      uint8_t *storage;

    public:
      BitPlaneBase(uint8_t *s, Size size) :
        PlaneBase(size),
        stride((size.w+7)/8),
        storage(s)
      {}

      BitPlaneBase(uint8_t *s, Size size, unsigned stride) :
        PlaneBase(size),
        stride(stride),
        storage(s)
      {}

      virtual void set_pixel(Point p, pixel value);
      virtual pixel get_pixel(Point p) const;
    };

    template<unsigned W, unsigned H>
    class BitPlane : public BitPlaneBase {
      uint8_t data[((W+7)*H)/8];

    public:
      BitPlane() : BitPlaneBase(data, Size(W,H)) {}
    };

    class Canvas {
      PlaneBase * const plane;

    public:
      const Rect bounds;
      Rect clip;

      Canvas(PlaneBase *pb, Size s);

      virtual void init();
      virtual void reset();
      virtual void flush(const Rect &r) = 0;
      virtual void fill_rect(const Rect &r, pixel value);
      virtual void draw_pixel(Point p, pixel value);
      virtual void draw_line(Point p0, Point p1, pixel value);
      virtual void draw_string(Point p, const char *str, const FontBase &f, pixel value);
    };

    struct RingBase {
      RingBase *left;
      RingBase *right;

      static void join(RingBase *dst, RingBase *src) { // src joins dst
        // remove src from its current ring
        src->left->right = src->right;
        src->right->left = src->left;

        // form a singleton ring so that join(x,x) works
        src->left = src->right = src;

        // set up local pointers
        src->right = dst->right;
        src->left = dst;

        // splice src into dest
        dst->right->left = src;
        dst->right = src;
      }

    public:
      RingBase() { left = right = this; }
      bool empty() const { return left == this; } // only need to check one side
    };

    class View : public Ring<View> {
    public:
      View *superview;
      View *subviews;
      Rect frame;

      View();
      virtual void set_frame(const Rect &r);
      void add_subview(View &v);
      void remove_from_superview();
      virtual void draw_self(Canvas &c);
      void draw_all(Canvas &c);
      int count_subviews() const;
      void remove_all_subviews();
    };

    class Screen : public View {
    protected:
      Canvas &root;

    public:
      Screen(Canvas &c);
      void init();
      void draw_all();
      void flush();
    };

    class SPIDisplay {
    public:
      virtual void init();
      virtual void reset();

    protected:
      SPIDriver &spi;
      SPIConfig config;
      uint16_t dc_bit;
      uint16_t reset_bit;

      inline void begin_spi() {spiSelect(&spi);}
      inline void end_spi() {spiUnselect(&spi);}
      inline void send_commands() {palClearPad(config.ssport, dc_bit);}
      inline void send_data() {palSetPad(config.ssport, dc_bit);}
      virtual void send(uint8_t *data, size_t len) {spiSend(&spi, len, data);}
      virtual void send(uint8_t data) {spiSend(&spi, 1, &data);}

      SPIDisplay(SPIDriver &d, const SPIConfig &c, uint16_t dc, uint16_t reset);
    };
  };
};
