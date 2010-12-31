#include "refinery/camera.h"

#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <boost/foreach.hpp>

#include "refinery/exif.h"

namespace refinery {

namespace CameraModels {
  const double RgbToXyz[3][3] = {
    { 0.412453, 0.357580, 0.180423 },
    { 0.212671, 0.715160, 0.072169 },
    { 0.019334, 0.119193, 0.950227 }
  };

  const double D65White[3] = { 0.950456, 1, 1.088754 };

  class AdobeColorConversionCamera : public virtual Camera {
    void dcraw_pseudoinverse(double (*in)[3], double (*out)[3], int size) const
    {
      double work[3][6], num;
      int i, j, k;

      for (i=0; i < 3; i++) {
        for (j=0; j < 6; j++)
          work[i][j] = j == i+3;
        for (j=0; j < 3; j++)
          for (k=0; k < size; k++)
            work[i][j] += in[k][i] * in[k][j];
      }
      for (i=0; i < 3; i++) {
        num = work[i][i];
        for (j=0; j < 6; j++)
          work[i][j] /= num;
        for (k=0; k < 3; k++) {
          if (k==i) continue;
          num = work[k][i];
          for (j=0; j < 6; j++)
            work[k][j] -= work[i][j] * num;
        }
      }
      for (i=0; i < size; i++)
        for (j=0; j < 3; j++)
          for (out[i][j]=k=0; k < 3; k++)
            out[i][j] += work[j][k+3] * in[i][k];
    }

  public:
    virtual ColorConversionData colorConversionData() const {
      // All matrices are from Adobe DNG Converter unless otherwise noted.
      static const struct {
        const char *model;
        unsigned short black;
        unsigned short maximum;
        short trans[12];
      } table[] = {
        { "AGFAPHOTO DC-833m", 0, 0,	/* DJC */
          { 11438,-3762,-1115,-2409,9914,2497,-1227,2295,5300 } },
        { "Apple QuickTake", 0, 0,		/* DJC */
          { 21392,-5653,-3353,2406,8010,-415,7166,1427,2078 } },
        { "Canon EOS D2000", 0, 0,
          { 24542,-10860,-3401,-1490,11370,-297,2858,-605,3225 } },
        { "Canon EOS D6000", 0, 0,
          { 20482,-7172,-3125,-1033,10410,-285,2542,226,3136 } },
        { "Canon EOS D30", 0, 0,
          { 9805,-2689,-1312,-5803,13064,3068,-2438,3075,8775 } },
        { "Canon EOS D60", 0, 0xfa0,
          { 6188,-1341,-890,-7168,14489,2937,-2640,3228,8483 } },
        { "Canon EOS 5D Mark II", 0, 0x3cf0,
          { 4716,603,-830,-7798,15474,2480,-1496,1937,6651 } },
        { "Canon EOS 5D", 0, 0xe6c,
          { 6347,-479,-972,-8297,15954,2480,-1968,2131,7649 } },
        { "Canon EOS 7D", 0, 0x3510,
          { 6844,-996,-856,-3876,11761,2396,-593,1772,6198 } },
        { "Canon EOS 10D", 0, 0xfa0,
          { 8197,-2000,-1118,-6714,14335,2592,-2536,3178,8266 } },
        { "Canon EOS 20Da", 0, 0,
          { 14155,-5065,-1382,-6550,14633,2039,-1623,1824,6561 } },
        { "Canon EOS 20D", 0, 0xfff,
          { 6599,-537,-891,-8071,15783,2424,-1983,2234,7462 } },
        { "Canon EOS 30D", 0, 0,
          { 6257,-303,-1000,-7880,15621,2396,-1714,1904,7046 } },
        { "Canon EOS 40D", 0, 0x3f60,
          { 6071,-747,-856,-7653,15365,2441,-2025,2553,7315 } },
        { "Canon EOS 50D", 0, 0x3d93,
          { 4920,616,-593,-6493,13964,2784,-1774,3178,7005 } },
        { "Canon EOS 300D", 0, 0xfa0,
          { 8197,-2000,-1118,-6714,14335,2592,-2536,3178,8266 } },
        { "Canon EOS 350D", 0, 0xfff,
          { 6018,-617,-965,-8645,15881,2975,-1530,1719,7642 } },
        { "Canon EOS 400D", 0, 0xe8e,
          { 7054,-1501,-990,-8156,15544,2812,-1278,1414,7796 } },
        { "Canon EOS 450D", 0, 0x390d,
          { 5784,-262,-821,-7539,15064,2672,-1982,2681,7427 } },
        { "Canon EOS 500D", 0, 0x3479,
          { 4763,712,-646,-6821,14399,2640,-1921,3276,6561 } },
        { "Canon EOS 1000D", 0, 0xe43,
          { 6771,-1139,-977,-7818,15123,2928,-1244,1437,7533 } },
        { "Canon EOS-1Ds Mark III", 0, 0x3bb0,
          { 5859,-211,-930,-8255,16017,2353,-1732,1887,7448 } },
        { "Canon EOS-1Ds Mark II", 0, 0xe80,
          { 6517,-602,-867,-8180,15926,2378,-1618,1771,7633 } },
        { "Canon EOS-1D Mark IV", 0, 0x3bb0,
          { 6014,-220,-795,-4109,12014,2361,-561,1824,5787 } },
        { "Canon EOS-1D Mark III", 0, 0x3bb0,
          { 6291,-540,-976,-8350,16145,2311,-1714,1858,7326 } },
        { "Canon EOS-1D Mark II N", 0, 0xe80,
          { 6240,-466,-822,-8180,15825,2500,-1801,1938,8042 } },
        { "Canon EOS-1D Mark II", 0, 0xe80,
          { 6264,-582,-724,-8312,15948,2504,-1744,1919,8664 } },
        { "Canon EOS-1DS", 0, 0xe20,
          { 4374,3631,-1743,-7520,15212,2472,-2892,3632,8161 } },
        { "Canon EOS-1D", 0, 0xe20,
          { 6806,-179,-1020,-8097,16415,1687,-3267,4236,7690 } },
        { "Canon EOS", 0, 0,
          { 8197,-2000,-1118,-6714,14335,2592,-2536,3178,8266 } },
        { "Canon PowerShot A530", 0, 0,
          { 0 } },	/* don't want the A5 matrix */	
        { "Canon PowerShot A50", 0, 0,
          { -5300,9846,1776,3436,684,3939,-5540,9879,6200,-1404,11175,217 } },
        { "Canon PowerShot A5", 0, 0,
          { -4801,9475,1952,2926,1611,4094,-5259,10164,5947,-1554,10883,547 } },
        { "Canon PowerShot G10", 0, 0,
          { 11093,-3906,-1028,-5047,12492,2879,-1003,1750,5561 } },
        { "Canon PowerShot G11", 0, 0,
          { 12177,-4817,-1069,-1612,9864,2049,-98,850,4471 } },
        { "Canon PowerShot G1", 0, 0,
          { -4778,9467,2172,4743,-1141,4344,-5146,9908,6077,-1566,11051,557 } },
        { "Canon PowerShot G2", 0, 0,
          { 9087,-2693,-1049,-6715,14382,2537,-2291,2819,7790 } },
        { "Canon PowerShot G3", 0, 0,
          { 9212,-2781,-1073,-6573,14189,2605,-2300,2844,7664 } },
        { "Canon PowerShot G5", 0, 0,
          { 9757,-2872,-933,-5972,13861,2301,-1622,2328,7212 } },
        { "Canon PowerShot G6", 0, 0,
          { 9877,-3775,-871,-7613,14807,3072,-1448,1305,7485 } },
        { "Canon PowerShot G9", 0, 0,
          { 7368,-2141,-598,-5621,13254,2625,-1418,1696,5743 } },
        { "Canon PowerShot Pro1", 0, 0,
          { 10062,-3522,-999,-7643,15117,2730,-765,817,7323 } },
        { "Canon PowerShot Pro70", 34, 0,
          { -4155,9818,1529,3939,-25,4522,-5521,9870,6610,-2238,10873,1342 } },
        { "Canon PowerShot Pro90", 0, 0,
          { -4963,9896,2235,4642,-987,4294,-5162,10011,5859,-1770,11230,577 } },
        { "Canon PowerShot S30", 0, 0,
          { 10566,-3652,-1129,-6552,14662,2006,-2197,2581,7670 } },
        { "Canon PowerShot S40", 0, 0,
          { 8510,-2487,-940,-6869,14231,2900,-2318,2829,9013 } },
        { "Canon PowerShot S45", 0, 0,
          { 8163,-2333,-955,-6682,14174,2751,-2077,2597,8041 } },
        { "Canon PowerShot S50", 0, 0,
          { 8882,-2571,-863,-6348,14234,2288,-1516,2172,6569 } },
        { "Canon PowerShot S60", 0, 0,
          { 8795,-2482,-797,-7804,15403,2573,-1422,1996,7082 } },
        { "Canon PowerShot S70", 0, 0,
          { 9976,-3810,-832,-7115,14463,2906,-901,989,7889 } },
        { "Canon PowerShot S90", 0, 0,
          { 12374,-5016,-1049,-1677,9902,2078,-83,852,4683 } },
        { "Canon PowerShot A470", 0, 0,	/* DJC */
          { 12513,-4407,-1242,-2680,10276,2405,-878,2215,4734 } },
        { "Canon PowerShot A610", 0, 0,	/* DJC */
          { 15591,-6402,-1592,-5365,13198,2168,-1300,1824,5075 } },
        { "Canon PowerShot A620", 0, 0,	/* DJC */
          { 15265,-6193,-1558,-4125,12116,2010,-888,1639,5220 } },
        { "Canon PowerShot A630", 0, 0,	/* DJC */
          { 14201,-5308,-1757,-6087,14472,1617,-2191,3105,5348 } },
        { "Canon PowerShot A640", 0, 0,	/* DJC */
          { 13124,-5329,-1390,-3602,11658,1944,-1612,2863,4885 } },
        { "Canon PowerShot A650", 0, 0,	/* DJC */
          { 9427,-3036,-959,-2581,10671,1911,-1039,1982,4430 } },
        { "Canon PowerShot A720", 0, 0,	/* DJC */
          { 14573,-5482,-1546,-1266,9799,1468,-1040,1912,3810 } },
        { "Canon PowerShot S3 IS", 0, 0,	/* DJC */
          { 14062,-5199,-1446,-4712,12470,2243,-1286,2028,4836 } },
        { "Canon PowerShot SX1 IS", 0, 0,
          { 6578,-259,-502,-5974,13030,3309,-308,1058,4970 } },
        { "Canon PowerShot SX110 IS", 0, 0,	/* DJC */
          { 14134,-5576,-1527,-1991,10719,1273,-1158,1929,3581 } },
        { "CASIO EX-S20", 0, 0,		/* DJC */
          { 11634,-3924,-1128,-4968,12954,2015,-1588,2648,7206 } },
        { "CASIO EX-Z750", 0, 0,		/* DJC */
          { 10819,-3873,-1099,-4903,13730,1175,-1755,3751,4632 } },
        { "CINE 650", 0, 0,
          { 3390,480,-500,-800,3610,340,-550,2336,1192 } },
        { "CINE 660", 0, 0,
          { 3390,480,-500,-800,3610,340,-550,2336,1192 } },
        { "CINE", 0, 0,
          { 20183,-4295,-423,-3940,15330,3985,-280,4870,9800 } },
        { "Contax N Digital", 0, 0xf1e,
          { 7777,1285,-1053,-9280,16543,2916,-3677,5679,7060 } },
        { "EPSON R-D1", 0, 0,
          { 6827,-1878,-732,-8429,16012,2564,-704,592,7145 } },
        { "FUJIFILM FinePix E550", 0, 0,
          { 11044,-3888,-1120,-7248,15168,2208,-1531,2277,8069 } },
        { "FUJIFILM FinePix E900", 0, 0,
          { 9183,-2526,-1078,-7461,15071,2574,-2022,2440,8639 } },
        { "FUJIFILM FinePix F8", 0, 0,
          { 11044,-3888,-1120,-7248,15168,2208,-1531,2277,8069 } },
        { "FUJIFILM FinePix F7", 0, 0,
          { 10004,-3219,-1201,-7036,15047,2107,-1863,2565,7736 } },
        { "FUJIFILM FinePix S100FS", 514, 0,
          { 11521,-4355,-1065,-6524,13767,3058,-1466,1984,6045 } },
        { "FUJIFILM FinePix S20Pro", 0, 0,
          { 10004,-3219,-1201,-7036,15047,2107,-1863,2565,7736 } },
        { "FUJIFILM FinePix S2Pro", 128, 0,
          { 12492,-4690,-1402,-7033,15423,1647,-1507,2111,7697 } },
        { "FUJIFILM FinePix S3Pro", 0, 0,
          { 11807,-4612,-1294,-8927,16968,1988,-2120,2741,8006 } },
        { "FUJIFILM FinePix S5Pro", 0, 0,
          { 12300,-5110,-1304,-9117,17143,1998,-1947,2448,8100 } },
        { "FUJIFILM FinePix S5000", 0, 0,
          { 8754,-2732,-1019,-7204,15069,2276,-1702,2334,6982 } },
        { "FUJIFILM FinePix S5100", 0, 0x3e00,
          { 11940,-4431,-1255,-6766,14428,2542,-993,1165,7421 } },
        { "FUJIFILM FinePix S5500", 0, 0x3e00,
          { 11940,-4431,-1255,-6766,14428,2542,-993,1165,7421 } },
        { "FUJIFILM FinePix S5200", 0, 0,
          { 9636,-2804,-988,-7442,15040,2589,-1803,2311,8621 } },
        { "FUJIFILM FinePix S5600", 0, 0,
          { 9636,-2804,-988,-7442,15040,2589,-1803,2311,8621 } },
        { "FUJIFILM FinePix S6", 0, 0,
          { 12628,-4887,-1401,-6861,14996,1962,-2198,2782,7091 } },
        { "FUJIFILM FinePix S7000", 0, 0,
          { 10190,-3506,-1312,-7153,15051,2238,-2003,2399,7505 } },
        { "FUJIFILM FinePix S9000", 0, 0,
          { 10491,-3423,-1145,-7385,15027,2538,-1809,2275,8692 } },
        { "FUJIFILM FinePix S9500", 0, 0,
          { 10491,-3423,-1145,-7385,15027,2538,-1809,2275,8692 } },
        { "FUJIFILM FinePix S9100", 0, 0,
          { 12343,-4515,-1285,-7165,14899,2435,-1895,2496,8800 } },
        { "FUJIFILM FinePix S9600", 0, 0,
          { 12343,-4515,-1285,-7165,14899,2435,-1895,2496,8800 } },
        { "FUJIFILM IS-1", 0, 0,
          { 21461,-10807,-1441,-2332,10599,1999,289,875,7703 } },
        { "FUJIFILM IS Pro", 0, 0,
          { 12300,-5110,-1304,-9117,17143,1998,-1947,2448,8100 } },
        { "Imacon Ixpress", 0, 0,		/* DJC */
          { 7025,-1415,-704,-5188,13765,1424,-1248,2742,6038 } },
        { "KODAK NC2000", 0, 0,
          { 13891,-6055,-803,-465,9919,642,2121,82,1291 } },
        { "Kodak DCS315C", 8, 0,
          { 17523,-4827,-2510,756,8546,-137,6113,1649,2250 } },
        { "Kodak DCS330C", 8, 0,
          { 20620,-7572,-2801,-103,10073,-396,3551,-233,2220 } },
        { "KODAK DCS420", 0, 0,
          { 10868,-1852,-644,-1537,11083,484,2343,628,2216 } },
        { "KODAK DCS460", 0, 0,
          { 10592,-2206,-967,-1944,11685,230,2206,670,1273 } },
        { "KODAK EOSDCS1", 0, 0,
          { 10592,-2206,-967,-1944,11685,230,2206,670,1273 } },
        { "KODAK EOSDCS3B", 0, 0,
          { 9898,-2700,-940,-2478,12219,206,1985,634,1031 } },
        { "Kodak DCS520C", 180, 0,
          { 24542,-10860,-3401,-1490,11370,-297,2858,-605,3225 } },
        { "Kodak DCS560C", 188, 0,
          { 20482,-7172,-3125,-1033,10410,-285,2542,226,3136 } },
        { "Kodak DCS620C", 180, 0,
          { 23617,-10175,-3149,-2054,11749,-272,2586,-489,3453 } },
        { "Kodak DCS620X", 185, 0,
          { 13095,-6231,154,12221,-21,-2137,895,4602,2258 } },
        { "Kodak DCS660C", 214, 0,
          { 18244,-6351,-2739,-791,11193,-521,3711,-129,2802 } },
        { "Kodak DCS720X", 0, 0,
          { 11775,-5884,950,9556,1846,-1286,-1019,6221,2728 } },
        { "Kodak DCS760C", 0, 0,
          { 16623,-6309,-1411,-4344,13923,323,2285,274,2926 } },
        { "Kodak DCS Pro SLR", 0, 0,
          { 5494,2393,-232,-6427,13850,2846,-1876,3997,5445 } },
        { "Kodak DCS Pro 14nx", 0, 0,
          { 5494,2393,-232,-6427,13850,2846,-1876,3997,5445 } },
        { "Kodak DCS Pro 14", 0, 0,
          { 7791,3128,-776,-8588,16458,2039,-2455,4006,6198 } },
        { "Kodak ProBack645", 0, 0,
          { 16414,-6060,-1470,-3555,13037,473,2545,122,4948 } },
        { "Kodak ProBack", 0, 0,
          { 21179,-8316,-2918,-915,11019,-165,3477,-180,4210 } },
        { "KODAK P712", 0, 0,
          { 9658,-3314,-823,-5163,12695,2768,-1342,1843,6044 } },
        { "KODAK P850", 0, 0xf7c,
          { 10511,-3836,-1102,-6946,14587,2558,-1481,1792,6246 } },
        { "KODAK P880", 0, 0xfff,
          { 12805,-4662,-1376,-7480,15267,2360,-1626,2194,7904 } },
        { "KODAK EasyShare Z980", 0, 0,
          { 11313,-3559,-1101,-3893,11891,2257,-1214,2398,4908 } },
        { "KODAK EASYSHARE Z1015", 0, 0xef1,
          { 11265,-4286,-992,-4694,12343,2647,-1090,1523,5447 } },
        { "Leaf CMost", 0, 0,
          { 3952,2189,449,-6701,14585,2275,-4536,7349,6536 } },
        { "Leaf Valeo 6", 0, 0,
          { 3952,2189,449,-6701,14585,2275,-4536,7349,6536 } },
        { "Leaf Aptus 54S", 0, 0,
          { 8236,1746,-1314,-8251,15953,2428,-3673,5786,5771 } },
        { "Leaf Aptus 65", 0, 0,
          { 7914,1414,-1190,-8777,16582,2280,-2811,4605,5562 } },
        { "Leaf Aptus 75", 0, 0,
          { 7914,1414,-1190,-8777,16582,2280,-2811,4605,5562 } },
        { "Leaf", 0, 0,
          { 8236,1746,-1314,-8251,15953,2428,-3673,5786,5771 } },
        { "Mamiya ZD", 0, 0,
          { 7645,2579,-1363,-8689,16717,2015,-3712,5941,5961 } },
        { "Micron 2010", 110, 0,		/* DJC */
          { 16695,-3761,-2151,155,9682,163,3433,951,4904 } },
        { "Minolta DiMAGE 5", 0, 0xf7d,
          { 8983,-2942,-963,-6556,14476,2237,-2426,2887,8014 } },
        { "Minolta DiMAGE 7Hi", 0, 0xf7d,
          { 11368,-3894,-1242,-6521,14358,2339,-2475,3056,7285 } },
        { "Minolta DiMAGE 7", 0, 0xf7d,
          { 9144,-2777,-998,-6676,14556,2281,-2470,3019,7744 } },
        { "Minolta DiMAGE A1", 0, 0xf8b,
          { 9274,-2547,-1167,-8220,16323,1943,-2273,2720,8340 } },
        { "MINOLTA DiMAGE A200", 0, 0,
          { 8560,-2487,-986,-8112,15535,2771,-1209,1324,7743 } },
        { "Minolta DiMAGE A2", 0, 0xf8f,
          { 9097,-2726,-1053,-8073,15506,2762,-966,981,7763 } },
        { "Minolta DiMAGE Z2", 0, 0,	/* DJC */
          { 11280,-3564,-1370,-4655,12374,2282,-1423,2168,5396 } },
        { "MINOLTA DYNAX 5", 0, 0xffb,
          { 10284,-3283,-1086,-7957,15762,2316,-829,882,6644 } },
        { "MINOLTA DYNAX 7", 0, 0xffb,
          { 10239,-3104,-1099,-8037,15727,2451,-927,925,6871 } },
        { "MOTOROLA PIXL", 0, 0,		/* DJC */
          { 8898,-989,-1033,-3292,11619,1674,-661,3178,5216 } },
        { "NIKON D100", 0, 0,
          { 5902,-933,-782,-8983,16719,2354,-1402,1455,6464 } },
        { "NIKON D1H", 0, 0,
          { 7577,-2166,-926,-7454,15592,1934,-2377,2808,8606 } },
        { "NIKON D1X", 0, 0,
          { 7702,-2245,-975,-9114,17242,1875,-2679,3055,8521 } },
        { "NIKON D1", 0, 0, /* multiplied by 2.218750, 1.0, 1.148438 */
          { 16772,-4726,-2141,-7611,15713,1972,-2846,3494,9521 } },
        { "NIKON D200", 0, 0xfbc,
          { 8367,-2248,-763,-8758,16447,2422,-1527,1550,8053 } },
        { "NIKON D2H", 0, 0,
          { 5710,-901,-615,-8594,16617,2024,-2975,4120,6830 } },
        { "NIKON D2X", 0, 0,
          { 10231,-2769,-1255,-8301,15900,2552,-797,680,7148 } },
        { "NIKON D3000", 0, 0,
          { 8736,-2458,-935,-9075,16894,2251,-1354,1242,8263 } },
        { "NIKON D300", 0, 0,
          { 9030,-1992,-715,-8465,16302,2255,-2689,3217,8069 } },
        { "NIKON D3X", 0, 0,
          { 7171,-1986,-648,-8085,15555,2718,-2170,2512,7457 } },
        { "NIKON D3S", 0, 0,
          { 8828,-2406,-694,-4874,12603,2541,-660,1509,7587 } },
        { "NIKON D3", 0, 0,
          { 8139,-2171,-663,-8747,16541,2295,-1925,2008,8093 } },
        { "NIKON D40X", 0, 0,
          { 8819,-2543,-911,-9025,16928,2151,-1329,1213,8449 } },
        { "NIKON D40", 0, 0,
          { 6992,-1668,-806,-8138,15748,2543,-874,850,7897 } },
        { "NIKON D5000", 0, 0xf00,
          { 7309,-1403,-519,-8474,16008,2622,-2433,2826,8064 } },
        { "NIKON D50", 0, 0,
          { 7732,-2422,-789,-8238,15884,2498,-859,783,7330 } },
        { "NIKON D60", 0, 0,
          { 8736,-2458,-935,-9075,16894,2251,-1354,1242,8263 } },
        { "NIKON D700", 0, 0,
          { 8139,-2171,-663,-8747,16541,2295,-1925,2008,8093 } },
        { "NIKON D70", 0, 0,
          { 7732,-2422,-789,-8238,15884,2498,-859,783,7330 } },
        { "NIKON D80", 0, 0,
          { 8629,-2410,-883,-9055,16940,2171,-1490,1363,8520 } },
        { "NIKON D90", 0, 0xf00,
          { 7309,-1403,-519,-8474,16008,2622,-2434,2826,8064 } },
        { "NIKON E950", 0, 0x3dd,		/* DJC */
          { -3746,10611,1665,9621,-1734,2114,-2389,7082,3064,3406,6116,-244 } },
        { "NIKON E995", 0, 0,	/* copied from E5000 */
          { -5547,11762,2189,5814,-558,3342,-4924,9840,5949,688,9083,96 } },
        { "NIKON E2100", 0, 0,	/* copied from Z2, new white balance */
          { 13142,-4152,-1596,-4655,12374,2282,-1769,2696,6711} },
        { "NIKON E2500", 0, 0,
          { -5547,11762,2189,5814,-558,3342,-4924,9840,5949,688,9083,96 } },
        { "NIKON E4300", 0, 0,	/* copied from Minolta DiMAGE Z2 */
          { 11280,-3564,-1370,-4655,12374,2282,-1423,2168,5396 } },
        { "NIKON E4500", 0, 0,
          { -5547,11762,2189,5814,-558,3342,-4924,9840,5949,688,9083,96 } },
        { "NIKON E5000", 0, 0,
          { -5547,11762,2189,5814,-558,3342,-4924,9840,5949,688,9083,96 } },
        { "NIKON E5400", 0, 0,
          { 9349,-2987,-1001,-7919,15766,2266,-2098,2680,6839 } },
        { "NIKON E5700", 0, 0,
          { -5368,11478,2368,5537,-113,3148,-4969,10021,5782,778,9028,211 } },
        { "NIKON E8400", 0, 0,
          { 7842,-2320,-992,-8154,15718,2599,-1098,1342,7560 } },
        { "NIKON E8700", 0, 0,
          { 8489,-2583,-1036,-8051,15583,2643,-1307,1407,7354 } },
        { "NIKON E8800", 0, 0,
          { 7971,-2314,-913,-8451,15762,2894,-1442,1520,7610 } },
        { "NIKON COOLPIX P6000", 0, 0,
          { 9698,-3367,-914,-4706,12584,2368,-837,968,5801 } },
        { "OLYMPUS C5050", 0, 0,
          { 10508,-3124,-1273,-6079,14294,1901,-1653,2306,6237 } },
        { "OLYMPUS C5060", 0, 0,
          { 10445,-3362,-1307,-7662,15690,2058,-1135,1176,7602 } },
        { "OLYMPUS C7070", 0, 0,
          { 10252,-3531,-1095,-7114,14850,2436,-1451,1723,6365 } },
        { "OLYMPUS C70", 0, 0,
          { 10793,-3791,-1146,-7498,15177,2488,-1390,1577,7321 } },
        { "OLYMPUS C80", 0, 0,
          { 8606,-2509,-1014,-8238,15714,2703,-942,979,7760 } },
        { "OLYMPUS E-10", 0, 0xffc0,
          { 12745,-4500,-1416,-6062,14542,1580,-1934,2256,6603 } },
        { "OLYMPUS E-1", 0, 0xfff0,
          { 11846,-4767,-945,-7027,15878,1089,-2699,4122,8311 } },
        { "OLYMPUS E-20", 0, 0xffc0,
          { 13173,-4732,-1499,-5807,14036,1895,-2045,2452,7142 } },
        { "OLYMPUS E-300", 0, 0,
          { 7828,-1761,-348,-5788,14071,1830,-2853,4518,6557 } },
        { "OLYMPUS E-330", 0, 0,
          { 8961,-2473,-1084,-7979,15990,2067,-2319,3035,8249 } },
        { "OLYMPUS E-30", 0, 0xfbc,
          { 8144,-1861,-1111,-7763,15894,1929,-1865,2542,7607 } },
        { "OLYMPUS E-3", 0, 0xf99,
          { 9487,-2875,-1115,-7533,15606,2010,-1618,2100,7389 } },
        { "OLYMPUS E-400", 0, 0xfff0,
          { 6169,-1483,-21,-7107,14761,2536,-2904,3580,8568 } },
        { "OLYMPUS E-410", 0, 0xf6a,
          { 8856,-2582,-1026,-7761,15766,2082,-2009,2575,7469 } },
        { "OLYMPUS E-420", 0, 0xfd7,
          { 8746,-2425,-1095,-7594,15612,2073,-1780,2309,7416 } },
        { "OLYMPUS E-450", 0, 0xfd2,
          { 8745,-2425,-1095,-7594,15613,2073,-1780,2309,7416 } },
        { "OLYMPUS E-500", 0, 0xfff0,
          { 8136,-1968,-299,-5481,13742,1871,-2556,4205,6630 } },
        { "OLYMPUS E-510", 0, 0xf6a,
          { 8785,-2529,-1033,-7639,15624,2112,-1783,2300,7817 } },
        { "OLYMPUS E-520", 0, 0xfd2,
          { 8344,-2322,-1020,-7596,15635,2048,-1748,2269,7287 } },
        { "OLYMPUS E-620", 0, 0xfb9,
          { 8453,-2198,-1092,-7609,15681,2008,-1725,2337,7824 } },
        { "OLYMPUS E-P1", 0, 0xffd,
          { 8343,-2050,-1021,-7715,15705,2103,-1831,2380,8235 } },
        { "OLYMPUS SP350", 0, 0,
          { 12078,-4836,-1069,-6671,14306,2578,-786,939,7418 } },
        { "OLYMPUS SP3", 0, 0,
          { 11766,-4445,-1067,-6901,14421,2707,-1029,1217,7572 } },
        { "OLYMPUS SP500UZ", 0, 0xfff,
          { 9493,-3415,-666,-5211,12334,3260,-1548,2262,6482 } },
        { "OLYMPUS SP510UZ", 0, 0xffe,
          { 10593,-3607,-1010,-5881,13127,3084,-1200,1805,6721 } },
        { "OLYMPUS SP550UZ", 0, 0xffe,
          { 11597,-4006,-1049,-5432,12799,2957,-1029,1750,6516 } },
        { "OLYMPUS SP560UZ", 0, 0xff9,
          { 10915,-3677,-982,-5587,12986,2911,-1168,1968,6223 } },
        { "OLYMPUS SP570UZ", 0, 0,
          { 11522,-4044,-1146,-4736,12172,2904,-988,1829,6039 } },
        { "PENTAX *ist DL2", 0, 0,
          { 10504,-2438,-1189,-8603,16207,2531,-1022,863,12242 } },
        { "PENTAX *ist DL", 0, 0,
          { 10829,-2838,-1115,-8339,15817,2696,-837,680,11939 } },
        { "PENTAX *ist DS2", 0, 0,
          { 10504,-2438,-1189,-8603,16207,2531,-1022,863,12242 } },
        { "PENTAX *ist DS", 0, 0,
          { 10371,-2333,-1206,-8688,16231,2602,-1230,1116,11282 } },
        { "PENTAX *ist D", 0, 0,
          { 9651,-2059,-1189,-8881,16512,2487,-1460,1345,10687 } },
        { "PENTAX K10D", 0, 0,
          { 9566,-2863,-803,-7170,15172,2112,-818,803,9705 } },
        { "PENTAX K1", 0, 0,
          { 11095,-3157,-1324,-8377,15834,2720,-1108,947,11688 } },
        { "PENTAX K20D", 0, 0,
          { 9427,-2714,-868,-7493,16092,1373,-2199,3264,7180 } },
        { "PENTAX K200D", 0, 0,
          { 9186,-2678,-907,-8693,16517,2260,-1129,1094,8524 } },
        { "PENTAX K2000", 0, 0,
          { 11057,-3604,-1155,-5152,13046,2329,-282,375,8104 } },
        { "PENTAX K-m", 0, 0,
          { 11057,-3604,-1155,-5152,13046,2329,-282,375,8104 } },
        { "PENTAX K-x", 0, 0,
          { 8843,-2837,-625,-5025,12644,2668,-411,1234,7410 } },
        { "PENTAX K-7", 0, 0,
          { 9142,-2947,-678,-8648,16967,1663,-2224,2898,8615 } },
        { "Panasonic DMC-FZ8", 0, 0xf7f0,
          { 8986,-2755,-802,-6341,13575,3077,-1476,2144,6379 } },
        { "Panasonic DMC-FZ18", 0, 0,
          { 9932,-3060,-935,-5809,13331,2753,-1267,2155,5575 } },
        { "Panasonic DMC-FZ28", 15, 0xfff,
          { 10109,-3488,-993,-5412,12812,2916,-1305,2140,5543 } },
        { "Panasonic DMC-FZ30", 0, 0xf94c,
          { 10976,-4029,-1141,-7918,15491,2600,-1670,2071,8246 } },
        { "Panasonic DMC-FZ35", 147, 0xfff,
          { 9938,-2780,-890,-4604,12393,2480,-1117,2304,4620 } },
        { "Panasonic DMC-FZ50", 0, 0xfff0,	/* aka "LEICA V-LUX1" */
          { 7906,-2709,-594,-6231,13351,3220,-1922,2631,6537 } },
        { "Panasonic DMC-L10", 15, 0xf96,
          { 8025,-1942,-1050,-7920,15904,2100,-2456,3005,7039 } },
        { "Panasonic DMC-L1", 0, 0xf7fc,	/* aka "LEICA DIGILUX 3" */
          { 8054,-1885,-1025,-8349,16367,2040,-2805,3542,7629 } },
        { "Panasonic DMC-LC1", 0, 0,	/* aka "LEICA DIGILUX 2" */
          { 11340,-4069,-1275,-7555,15266,2448,-2960,3426,7685 } },
        { "Panasonic DMC-LX1", 0, 0xf7f0,	/* aka "LEICA D-LUX2" */
          { 10704,-4187,-1230,-8314,15952,2501,-920,945,8927 } },
        { "Panasonic DMC-LX2", 0, 0,	/* aka "LEICA D-LUX3" */
          { 8048,-2810,-623,-6450,13519,3272,-1700,2146,7049 } },
        { "Panasonic DMC-LX3", 15, 0xfff,	/* aka "LEICA D-LUX4" */
          { 8128,-2668,-655,-6134,13307,3161,-1782,2568,6083 } },
        { "Panasonic DMC-FX150", 15, 0xfff,
          { 9082,-2907,-925,-6119,13377,3058,-1797,2641,5609 } },
        { "Panasonic DMC-G1", 15, 0xfff,
          { 8199,-2065,-1056,-8124,16156,2033,-2458,3022,7220 } },
        { "Panasonic DMC-GF1", 15, 0xf92,
          { 7888,-1902,-1011,-8106,16085,2099,-2353,2866,7330 } },
        { "Panasonic DMC-GH1", 15, 0xf92,
          { 6299,-1466,-532,-6535,13852,2969,-2331,3112,5984 } },
        { "Phase One H 20", 0, 0,		/* DJC */
          { 1313,1855,-109,-6715,15908,808,-327,1840,6020 } },
        { "Phase One P 2", 0, 0,
          { 2905,732,-237,-8134,16626,1476,-3038,4253,7517 } },
        { "Phase One P 30", 0, 0,
          { 4516,-245,-37,-7020,14976,2173,-3206,4671,7087 } },
        { "Phase One P 45", 0, 0,
          { 5053,-24,-117,-5684,14076,1702,-2619,4492,5849 } },
        { "Phase One P65", 0, 0,		/* DJC */
          { 8522,1268,-1916,-7706,16350,1358,-2397,4344,4923 } },
        { "SAMSUNG GX-1", 0, 0,
          { 10504,-2438,-1189,-8603,16207,2531,-1022,863,12242 } },
        { "SAMSUNG S85", 0, 0,		/* DJC */
          { 11885,-3968,-1473,-4214,12299,1916,-835,1655,5549 } },
        { "Sinar", 0, 0,			/* DJC */
          { 16442,-2956,-2422,-2877,12128,750,-1136,6066,4559 } },
        { "SONY DSC-F828", 491, 0,
          { 7924,-1910,-777,-8226,15459,2998,-1517,2199,6818,-7242,11401,3481 } },
        { "SONY DSC-R1", 512, 0,
          { 8512,-2641,-694,-8042,15670,2526,-1821,2117,7414 } },
        { "SONY DSC-V3", 0, 0,
          { 7511,-2571,-692,-7894,15088,3060,-948,1111,8128 } },
        { "SONY DSLR-A100", 0, 0xfeb,
          { 9437,-2811,-774,-8405,16215,2290,-710,596,7181 } },
        { "SONY DSLR-A200", 0, 0,
          { 9847,-3091,-928,-8485,16345,2225,-715,595,7103 } },
        { "SONY DSLR-A230", 0, 0,	/* copied */
          { 9847,-3091,-928,-8485,16345,2225,-715,595,7103 } },
        { "SONY DSLR-A300", 0, 0,
          { 9847,-3091,-928,-8485,16345,2225,-715,595,7103 } },
        { "SONY DSLR-A330", 0, 0,
          { 9847,-3091,-929,-8485,16346,2225,-714,595,7103 } },
        { "SONY DSLR-A350", 0, 0xffc,
          { 6038,-1484,-578,-9146,16746,2513,-875,746,7217 } },
        { "SONY DSLR-A380", 0, 0,
          { 6038,-1484,-579,-9145,16746,2512,-875,746,7218 } },
        { "SONY DSLR-A5", 254, 0x1ffe,
          { 4950,-580,-103,-5228,12542,3029,-709,1435,7371 } },
        { "SONY DSLR-A700", 254, 0x1ffe,
          { 5775,-805,-359,-8574,16295,2391,-1943,2341,7249 } },
        { "SONY DSLR-A850", 256, 0x1ffe,
          { 5413,-1162,-365,-5665,13098,2866,-608,1179,8440 } },
        { "SONY DSLR-A900", 254, 0x1ffe,
          { 5209,-1072,-397,-8845,16120,2919,-1618,1803,8654 } }
      };

      ColorConversionData ret;

      const char* aName = this->name();

      bool fail = true;
      for (unsigned int i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        if (!strcmp(aName, table[i].model)) {
          fail = false;
          ret.black = table[i].black;
          ret.maximum = table[i].maximum;
          double* xyz(reinterpret_cast<double*>(ret.xyzToCamera));
          for (int j = 0; j < 12; j++) {
            xyz[j] = table[i].trans[j] / 10000.0;
          }
        }
      }
      if (fail) throw std::logic_error(
          std::string("Missing Adobe coefficients for camera: ") + aName);

      const int aColors = colors();
      // Multiply out rgbToCamera ...
      for (int i = 0; i < aColors; i++) {
        for (int j = 0; j < 3; j++) {
          ret.rgbToCamera[i][j] = 0;
          for (int k = 0; k < 3; k++) {
            ret.rgbToCamera[i][j] += ret.xyzToCamera[i][k] * RgbToXyz[k][j];
          }
        }
      }
      // ... and normalize it (so rgbToCamera * (1,1,1) == (1,1,1))
      for (int i = 0; i < aColors; i++) {
        double sum = 0;
        for (int j = 0; j < 3; j++) {
          sum += ret.rgbToCamera[i][j];
        }
        for (int j = 0; j < 3; j++) {
          ret.rgbToCamera[i][j] /= sum;
        }
        ret.cameraMultipliers[i] = 1 / sum;
      }

      // set scalingMultipliers, adjust cameraMultipliers
      double cameraMultipliersMin = std::numeric_limits<double>::max();
      for (int i = 0; i < aColors; i++) {
        if (cameraMultipliersMin > ret.cameraMultipliers[i]) {
          cameraMultipliersMin = ret.cameraMultipliers[i];
        }
      }
      for (int i = 0; i < aColors; i++) {
        ret.cameraMultipliers[i] =
            ret.cameraMultipliers[i] / cameraMultipliersMin;
        ret.scalingMultipliers[i] =
            ret.cameraMultipliers[i] * 65535.0 / ret.maximum;
      }

      // set cameraToRgb
      double inverse[4][3];
      dcraw_pseudoinverse(ret.rgbToCamera, inverse, aColors);
      for (int i = 0; i < 3; i++) {
        for (int j = 0; j < aColors; j++) {
          ret.cameraToRgb[i][j] = inverse[j][i];
        }
      }

      // set cameraToXyz
      for (int i = 0; i < 3; i++) {
        for (int j = 0; j < aColors; j++) {
          ret.cameraToXyz[i][j] = 0;
          for (int k = 0; k < 3; k++) {
            ret.cameraToXyz[i][j] +=
                RgbToXyz[i][k] * ret.cameraToRgb[k][j] / D65White[i];
          }
        }
      }

      return ret;
    }
  };

  class NullCamera : public Camera {
  public:
    virtual const char* name() const { return "(null)"; }
    virtual const char* make() const { return "(null)"; }
    virtual const char* model() const { return "(null)"; }
    virtual unsigned int orientation(const ExifData& exifData) const
    {
      return 1;
    }
    virtual unsigned int colors() const { return 3; }
    virtual ColorConversionData colorConversionData() const
    {
      return ColorConversionData();
    }
    virtual bool canHandle(const ExifData& exifData) const { return true; }
  };

  class NikonD5000 : public virtual Camera, public virtual AdobeColorConversionCamera {
  public:
    virtual const char* name() const { return "NIKON D5000"; }
    virtual const char* make() const { return "NIKON"; }
    virtual const char* model() const { return "D5000"; }
    virtual unsigned int colors() const { return 3; }
    virtual unsigned int orientation(const ExifData& exifData) const {
      return exifData.getInt("Exif.Image.Orientation");
    }
    virtual bool canHandle(const ExifData& exifData) const {
      const char* key = "Exif.Image.Model";
      const char* test = "NIKON D5000";
      return exifData.hasKey(key) && exifData.getString(key) == test;
    }
  };
} // namespace CameraModels

unsigned int CameraData::orientation() const
{
  return mCamera.orientation(mExifData);
}

unsigned int CameraData::colors() const
{
  return mCamera.colors();
}

Camera::ColorConversionData CameraData::colorConversionData() const
{
  return mCamera.colorConversionData();
}

namespace CameraFactoryData {
  std::vector<Camera*> cameraRegistry;

  CameraFactory* cameraFactoryInstance;
  CameraDataFactory* cameraDataFactoryInstance;
} // namespace CameraFactoryData

CameraFactory::CameraFactory() {
  std::vector<Camera*>& registry(CameraFactoryData::cameraRegistry);

  // INSERT YOUR CAMERA CLASS HERE...
  registry.push_back(new CameraModels::NikonD5000);
  registry.push_back(new CameraModels::NullCamera);
}

CameraFactory::~CameraFactory() {
  BOOST_FOREACH(Camera* camera, CameraFactoryData::cameraRegistry) {
    delete camera;
  }
}

CameraFactory& CameraFactory::instance()
{
  if (!CameraFactoryData::cameraFactoryInstance) {
#pragma omp critical(CameraFactory_instance)
    {
      if (!CameraFactoryData::cameraFactoryInstance) {
        CameraFactoryData::cameraFactoryInstance = new CameraFactory();
      }
    }
  }

  return *CameraFactoryData::cameraFactoryInstance;
}

const Camera& CameraFactory::detectCamera(const ExifData& exifData) const
{
  BOOST_FOREACH(const Camera* camera, CameraFactoryData::cameraRegistry) {
    if (camera->canHandle(exifData)) {
      return *camera;
    }
  }

  // The last camera should be a NullCamera which accepts anything, so we'll
  // never get here
  return *CameraFactoryData::cameraRegistry[0];
}

CameraDataFactory& CameraDataFactory::instance()
{
  if (!CameraFactoryData::cameraDataFactoryInstance) {
#pragma omp critical(CameraDataFactory_instance)
    {
      if (!CameraFactoryData::cameraDataFactoryInstance) {
        CameraFactoryData::cameraDataFactoryInstance = new CameraDataFactory();
      }
    }
  }

  return *CameraFactoryData::cameraDataFactoryInstance;
}

CameraData CameraDataFactory::getCameraData(const ExifData& exifData) const
{
  const Camera& camera(CameraFactory::instance().detectCamera(exifData));
  return CameraData(camera, exifData);
}

} // namespace refinery
