/*
 * jpeglib.h
 *
 * This file was part of the Independent JPEG Group's software:
 * Copyright (C) 1991-1998, Thomas G. Lane.
 * Modified 2002-2009 by Guido Vollbeding.
 * libjpeg-turbo Modifications:
 * Copyright (C) 2009-2011, 2013-2014, 2016-2017, D. R. Commander.
 * Copyright (C) 2015, Google, Inc.
 * For conditions of distribution and use, see the accompanying README.ijg
 * file.
 *
 * This file defines the application interface for the JPEG library.
 * Most applications using the library need only include this file,
 * and perhaps jerror.h if they want to know the exact error codes.
 */

#ifndef JPEGLIB_H
#define JPEGLIB_H

/*
 * First we include the configuration files that record how this
 * installation of the JPEG library is set up.  jconfig.h can be
 * generated automatically for many systems.  jmorecfg.h contains
 * manual configuration options that most people need not worry about.
 */

#ifndef JCONFIG_INCLUDED        /* in case jinclude.h already did */
#include "jconfig.h"            /* widely used configuration options */
#endif
#include "jmorecfg.h"           /* seldom changed options */


#ifdef __cplusplus
#ifndef DONT_USE_EXTERN_C
extern "C" {
#endif
#endif


/* Various constants determining the sizes of things.
 * All of these are specified by the JPEG standard, so don't change them
 * if you want to be compatible.
 */

#define DCTSIZE             8   /* The basic DCT block is 8x8 samples */
#define DCTSIZE2            64  /* DCTSIZE squared; # of elements in a block */
#define NUM_QUANT_TBLS      4   /* Quantization tables are numbered 0..3 */
#define NUM_HUFF_TBLS       4   /* Huffman tables are numbered 0..3 */
#define NUM_ARITH_TBLS      16  /* Arith-coding tables are numbered 0..15 */
#define MAX_COMPS_IN_SCAN   4   /* JPEG limit on # of components in one scan */
#define MAX_SAMP_FACTOR     4   /* JPEG limit on sampling factors */
/* Unfortunately, some bozo at Adobe saw no reason to be bound by the standard;
 * the PostScript DCT filter can emit files with many more than 10 blocks/MCU.
 * If you happen to run across such a file, you can up D_MAX_BLOCKS_IN_MCU
 * to handle it.  We even let you do this from the jconfig.h file.  However,
 * we strongly discourage changing C_MAX_BLOCKS_IN_MCU; just because Adobe
 * sometimes emits noncompliant files doesn't mean you should too.
 */
#define C_MAX_BLOCKS_IN_MCU   10 /* compressor's limit on blocks per MCU */
#ifndef D_MAX_BLOCKS_IN_MCU
#define D_MAX_BLOCKS_IN_MCU   10 /* decompressor's limit on blocks per MCU */
#endif


/* Data structures for images (arrays of samples and of DCT coefficients).
 */

typedef _Array_ptr<JSAMPLE> JSAMPROW;      /* ptr to one image row of pixel samples. */
typedef _Array_ptr<JSAMPROW> JSAMPARRAY;   /* ptr to some rows (a 2-D sample array) */
typedef _Ptr<JSAMPARRAY> JSAMPIMAGE; /* a 3-D sample array: top index is color */

typedef JCOEF JBLOCK _Checked[64]; /* one block of coefficients */
typedef _Ptr<JBLOCK> JBLOCKROW;      /* pointer to one row of coefficient blocks */
typedef _Ptr<JBLOCKROW> JBLOCKARRAY;         /* a 2-D array of coefficient blocks */
typedef _Ptr<JBLOCKARRAY> JBLOCKIMAGE;       /* a 3-D array of coefficient blocks */

typedef _Ptr<JCOEF> JCOEFPTR;        /* useful in a couple of places */


/* Types for JPEG compression parameters and working tables. */


/* DCT coefficient quantization tables. */

typedef struct {
  /* This array gives the coefficient quantizers in natural array order
   * (not the zigzag order in which they are stored in a JPEG DQT marker).
   * CAUTION: IJG versions prior to v6a kept this array in zigzag order.
   */
  UINT16 quantval _Checked[DCTSIZE2];    /* quantization step for each coefficient */
  /* This field is used only during compression.  It's initialized FALSE when
   * the table is created, and set TRUE when it's been output to the file.
   * You could suppress output of a table by setting this to TRUE.
   * (See jpeg_suppress_tables for an example.)
   */
  boolean sent_table;           /* TRUE when table has been output */
} JQUANT_TBL;


/* Huffman coding tables. */

typedef struct {
  /* These two fields directly represent the contents of a JPEG DHT marker */
  UINT8 bits _Checked[17];               /* bits[k] = # of symbols with codes of */
                                /* length k bits; bits[0] is unused */
  UINT8 huffval _Checked[256];           /* The symbols, in order of incr code length */
  /* This field is used only during compression.  It's initialized FALSE when
   * the table is created, and set TRUE when it's been output to the file.
   * You could suppress output of a table by setting this to TRUE.
   * (See jpeg_suppress_tables for an example.)
   */
  boolean sent_table;           /* TRUE when table has been output */
} JHUFF_TBL;


/* Basic info about one component (color channel). */

typedef struct {
  /* These values are fixed over the whole image. */
  /* For compression, they must be supplied by parameter setup; */
  /* for decompression, they are read from the SOF marker. */
  int component_id;             /* identifier for this component (0..255) */
  int component_index;          /* its index in SOF or cinfo->comp_info[] */
  int h_samp_factor;            /* horizontal sampling factor (1..4) */
  int v_samp_factor;            /* vertical sampling factor (1..4) */
  int quant_tbl_no;             /* quantization table selector (0..3) */
  /* These values may vary between scans. */
  /* For compression, they must be supplied by parameter setup; */
  /* for decompression, they are read from the SOS marker. */
  /* The decompressor output side may not use these variables. */
  int dc_tbl_no;                /* DC entropy table selector (0..3) */
  int ac_tbl_no;                /* AC entropy table selector (0..3) */

  /* Remaining fields should be treated as private by applications. */

  /* These values are computed during compression or decompression startup: */
  /* Component's size in DCT blocks.
   * Any dummy blocks added to complete an MCU are not counted; therefore
   * these values do not depend on whether a scan is interleaved or not.
   */
  JDIMENSION width_in_blocks;
  JDIMENSION height_in_blocks;
  /* Size of a DCT block in samples.  Always DCTSIZE for compression.
   * For decompression this is the size of the output from one DCT block,
   * reflecting any scaling we choose to apply during the IDCT step.
   * Values from 1 to 16 are supported.
   * Note that different components may receive different IDCT scalings.
   */
#if JPEG_LIB_VERSION >= 70
  int DCT_h_scaled_size;
  int DCT_v_scaled_size;
#else
  int DCT_scaled_size;
#endif
  /* The downsampled dimensions are the component's actual, unpadded number
   * of samples at the main buffer (preprocessing/compression interface), thus
   * downsampled_width = ceil(image_width * Hi/Hmax)
   * and similarly for height.  For decompression, IDCT scaling is included, so
   * downsampled_width = ceil(image_width * Hi/Hmax * DCT_[h_]scaled_size/DCTSIZE)
   */
  JDIMENSION downsampled_width;  /* actual width in samples */
  JDIMENSION downsampled_height; /* actual height in samples */
  /* This flag is used only for decompression.  In cases where some of the
   * components will be ignored (eg grayscale output from YCbCr image),
   * we can skip most computations for the unused components.
   */
  boolean component_needed;     /* do we need the value of this component? */

  /* These values are computed before starting a scan of the component. */
  /* The decompressor output side may not use these variables. */
  int MCU_width;                /* number of blocks per MCU, horizontally */
  int MCU_height;               /* number of blocks per MCU, vertically */
  int MCU_blocks;               /* MCU_width * MCU_height */
  int MCU_sample_width;         /* MCU width in samples, MCU_width*DCT_[h_]scaled_size */
  int last_col_width;           /* # of non-dummy blocks across in last MCU */
  int last_row_height;          /* # of non-dummy blocks down in last MCU */

  /* Saved quantization table for component; NULL if none yet saved.
   * See jdinput.c comments about the need for this information.
   * This field is currently used only for decompression.
   */
  _Ptr<JQUANT_TBL> quant_table;

  /* Private per-component storage for DCT or IDCT subsystem. */
  void *dct_table;
} jpeg_component_info;


/* The script for encoding a multiple-scan file is an array of these: */

typedef struct {
  int comps_in_scan;            /* number of components encoded in this scan */
  int component_index _Checked[MAX_COMPS_IN_SCAN]; /* their SOF/comp_info[] indexes */
  int Ss, Se;                   /* progressive JPEG spectral selection parms */
  int Ah, Al;                   /* progressive JPEG successive approx. parms */
} jpeg_scan_info;

/* The decompressor can save APPn and COM markers in a list of these: */

typedef _Ptr<struct jpeg_marker_struct> jpeg_saved_marker_ptr;

struct jpeg_marker_struct {
  jpeg_saved_marker_ptr next;   /* next in list, or NULL */
  UINT8 marker;                 /* marker code: JPEG_COM, or JPEG_APP0+n */
  unsigned int original_length; /* # bytes of data in the file */
  unsigned int data_length;     /* # bytes of data saved at data[] */
  _Ptr<JOCTET> data;                 /* the data contained in the marker */
  /* the marker length word is not counted in data_length or original_length */
};

/* Known color spaces. */

#define JCS_EXTENSIONS  1
#define JCS_ALPHA_EXTENSIONS  1

typedef enum {
  JCS_UNKNOWN,            /* error/unspecified */
  JCS_GRAYSCALE,          /* monochrome */
  JCS_RGB,                /* red/green/blue as specified by the RGB_RED,
                             RGB_GREEN, RGB_BLUE, and RGB_PIXELSIZE macros */
  JCS_YCbCr,              /* Y/Cb/Cr (also known as YUV) */
  JCS_CMYK,               /* C/M/Y/K */
  JCS_YCCK,               /* Y/Cb/Cr/K */
  JCS_EXT_RGB,            /* red/green/blue */
  JCS_EXT_RGBX,           /* red/green/blue/x */
  JCS_EXT_BGR,            /* blue/green/red */
  JCS_EXT_BGRX,           /* blue/green/red/x */
  JCS_EXT_XBGR,           /* x/blue/green/red */
  JCS_EXT_XRGB,           /* x/red/green/blue */
  /* When out_color_space it set to JCS_EXT_RGBX, JCS_EXT_BGRX, JCS_EXT_XBGR,
     or JCS_EXT_XRGB during decompression, the X byte is undefined, and in
     order to ensure the best performance, libjpeg-turbo can set that byte to
     whatever value it wishes.  Use the following colorspace constants to
     ensure that the X byte is set to 0xFF, so that it can be interpreted as an
     opaque alpha channel. */
  JCS_EXT_RGBA,           /* red/green/blue/alpha */
  JCS_EXT_BGRA,           /* blue/green/red/alpha */
  JCS_EXT_ABGR,           /* alpha/blue/green/red */
  JCS_EXT_ARGB,           /* alpha/red/green/blue */
  JCS_RGB565              /* 5-bit red/6-bit green/5-bit blue */
} J_COLOR_SPACE;

/* DCT/IDCT algorithm options. */

typedef enum {
  JDCT_ISLOW,             /* slow but accurate integer algorithm */
  JDCT_IFAST,             /* faster, less accurate integer method */
  JDCT_FLOAT              /* floating-point: accurate, fast on fast HW */
} J_DCT_METHOD;

#ifndef JDCT_DEFAULT            /* may be overridden in jconfig.h */
#define JDCT_DEFAULT  JDCT_ISLOW
#endif
#ifndef JDCT_FASTEST            /* may be overridden in jconfig.h */
#define JDCT_FASTEST  JDCT_IFAST
#endif

/* Dithering options for decompression. */

typedef enum {
  JDITHER_NONE,           /* no dithering */
  JDITHER_ORDERED,        /* simple ordered dither */
  JDITHER_FS              /* Floyd-Steinberg error diffusion dither */
} J_DITHER_MODE;


/* Common fields between JPEG compression and decompression master structs. */

#define jpeg_common_fields \
  struct jpeg_error_mgr *err : itype(_Ptr<struct jpeg_error_mgr>);   /* Error handler module */ \
  struct jpeg_memory_mgr *mem : itype(_Ptr<struct jpeg_memory_mgr>);  /* Memory manager module */ \
  struct jpeg_progress_mgr *progress; /* Progress monitor, or NULL if none */ \
  void *client_data;            /* Available for use by application */ \
  boolean is_decompressor;      /* So common code can tell which is which */ \
  int global_state              /* For checking call sequence validity */

/* Routines that are to be used by both halves of the library are declared
 * to receive a pointer to this structure.  There are no actual instances of
 * jpeg_common_struct, only of jpeg_compress_struct and jpeg_decompress_struct.
 */
struct jpeg_common_struct {
  jpeg_common_fields;           /* Fields common to both master struct types */
  /* Additional fields follow in an actual jpeg_compress_struct or
   * jpeg_decompress_struct.  All three structs must agree on these
   * initial fields!  (This would be a lot cleaner in C++.)
   */
};

typedef _Ptr<struct jpeg_common_struct> j_common_ptr;
typedef _Ptr<struct jpeg_compress_struct> j_compress_ptr;
typedef _Ptr<struct jpeg_decompress_struct> j_decompress_ptr;


/* Master record for a compression instance */

struct jpeg_compress_struct {
  jpeg_common_fields;           /* Fields shared with jpeg_decompress_struct */

  /* Destination for compressed data */
  _Ptr<struct jpeg_destination_mgr> dest;

  /* Description of source image --- these fields must be filled in by
   * outer application before starting compression.  in_color_space must
   * be correct before you can even call jpeg_set_defaults().
   */

  JDIMENSION image_width;       /* input image width */
  JDIMENSION image_height;      /* input image height */
  int input_components;         /* # of color components in input image */
  J_COLOR_SPACE in_color_space; /* colorspace of input image */

  double input_gamma;           /* image gamma of input image */

  /* Compression parameters --- these fields must be set before calling
   * jpeg_start_compress().  We recommend calling jpeg_set_defaults() to
   * initialize everything to reasonable defaults, then changing anything
   * the application specifically wants to change.  That way you won't get
   * burnt when new parameters are added.  Also note that there are several
   * helper routines to simplify changing parameters.
   */

#if JPEG_LIB_VERSION >= 70
  unsigned int scale_num, scale_denom; /* fraction by which to scale image */

  JDIMENSION jpeg_width;        /* scaled JPEG image width */
  JDIMENSION jpeg_height;       /* scaled JPEG image height */
  /* Dimensions of actual JPEG image that will be written to file,
   * derived from input dimensions by scaling factors above.
   * These fields are computed by jpeg_start_compress().
   * You can also use jpeg_calc_jpeg_dimensions() to determine these values
   * in advance of calling jpeg_start_compress().
   */
#endif

  int data_precision;           /* bits of precision in image data */

  int num_components;           /* # of color components in JPEG image */
  J_COLOR_SPACE jpeg_color_space; /* colorspace of JPEG image */

  _Ptr<jpeg_component_info> comp_info;
  /* comp_info[i] describes component that appears i'th in SOF */

  _Ptr<JQUANT_TBL> quant_tbl_ptrs _Checked[NUM_QUANT_TBLS];
#if JPEG_LIB_VERSION >= 70
  int q_scale_factor _Checked[NUM_QUANT_TBLS];
#endif
  /* ptrs to coefficient quantization tables, or NULL if not defined,
   * and corresponding scale factors (percentage, initialized 100).
   */

  _Ptr<JHUFF_TBL> dc_huff_tbl_ptrs _Checked[NUM_HUFF_TBLS];
  _Ptr<JHUFF_TBL> ac_huff_tbl_ptrs _Checked[NUM_HUFF_TBLS];
  /* ptrs to Huffman coding tables, or NULL if not defined */

  UINT8 arith_dc_L _Checked[NUM_ARITH_TBLS]; /* L values for DC arith-coding tables */
  UINT8 arith_dc_U _Checked[NUM_ARITH_TBLS]; /* U values for DC arith-coding tables */
  UINT8 arith_ac_K _Checked[NUM_ARITH_TBLS]; /* Kx values for AC arith-coding tables */

  int num_scans;                /* # of entries in scan_info array */
  _Ptr<const jpeg_scan_info> scan_info; /* script for multi-scan file, or NULL */
  /* The default value of scan_info is NULL, which causes a single-scan
   * sequential JPEG file to be emitted.  To create a multi-scan file,
   * set num_scans and scan_info to point to an array of scan definitions.
   */

  boolean raw_data_in;          /* TRUE=caller supplies downsampled data */
  boolean arith_code;           /* TRUE=arithmetic coding, FALSE=Huffman */
  boolean optimize_coding;      /* TRUE=optimize entropy encoding parms */
  boolean CCIR601_sampling;     /* TRUE=first samples are cosited */
#if JPEG_LIB_VERSION >= 70
  boolean do_fancy_downsampling; /* TRUE=apply fancy downsampling */
#endif
  int smoothing_factor;         /* 1..100, or 0 for no input smoothing */
  J_DCT_METHOD dct_method;      /* DCT algorithm selector */

  /* The restart interval can be specified in absolute MCUs by setting
   * restart_interval, or in MCU rows by setting restart_in_rows
   * (in which case the correct restart_interval will be figured
   * for each scan).
   */
  unsigned int restart_interval; /* MCUs per restart, or 0 for no restart */
  int restart_in_rows;          /* if > 0, MCU rows per restart interval */

  /* Parameters controlling emission of special markers. */

  boolean write_JFIF_header;    /* should a JFIF marker be written? */
  UINT8 JFIF_major_version;     /* What to write for the JFIF version number */
  UINT8 JFIF_minor_version;
  /* These three values are not used by the JPEG code, merely copied */
  /* into the JFIF APP0 marker.  density_unit can be 0 for unknown, */
  /* 1 for dots/inch, or 2 for dots/cm.  Note that the pixel aspect */
  /* ratio is defined by X_density/Y_density even when density_unit=0. */
  UINT8 density_unit;           /* JFIF code for pixel size units */
  UINT16 X_density;             /* Horizontal pixel density */
  UINT16 Y_density;             /* Vertical pixel density */
  boolean write_Adobe_marker;   /* should an Adobe marker be written? */

  /* State variable: index of next scanline to be written to
   * jpeg_write_scanlines().  Application may use this to control its
   * processing loop, e.g., "while (next_scanline < image_height)".
   */

  JDIMENSION next_scanline;     /* 0 .. image_height-1  */

  /* Remaining fields are known throughout compressor, but generally
   * should not be touched by a surrounding application.
   */

  /*
   * These fields are computed during compression startup
   */
  boolean progressive_mode;     /* TRUE if scan script uses progressive mode */
  int max_h_samp_factor;        /* largest h_samp_factor */
  int max_v_samp_factor;        /* largest v_samp_factor */

#if JPEG_LIB_VERSION >= 70
  int min_DCT_h_scaled_size;    /* smallest DCT_h_scaled_size of any component */
  int min_DCT_v_scaled_size;    /* smallest DCT_v_scaled_size of any component */
#endif

  JDIMENSION total_iMCU_rows;   /* # of iMCU rows to be input to coef ctlr */
  /* The coefficient controller receives data in units of MCU rows as defined
   * for fully interleaved scans (whether the JPEG file is interleaved or not).
   * There are v_samp_factor * DCTSIZE sample rows of each component in an
   * "iMCU" (interleaved MCU) row.
   */

  /*
   * These fields are valid during any one scan.
   * They describe the components and MCUs actually appearing in the scan.
   */
  int comps_in_scan;            /* # of JPEG components in this scan */
  _Ptr<jpeg_component_info> cur_comp_info _Checked[MAX_COMPS_IN_SCAN];
  /* *cur_comp_info[i] describes component that appears i'th in SOS */

  JDIMENSION MCUs_per_row;      /* # of MCUs across the image */
  JDIMENSION MCU_rows_in_scan;  /* # of MCU rows in the image */

  int blocks_in_MCU;            /* # of DCT blocks per MCU */
  int MCU_membership _Checked[C_MAX_BLOCKS_IN_MCU];
  /* MCU_membership[i] is index in cur_comp_info of component owning */
  /* i'th block in an MCU */

  int Ss, Se, Ah, Al;           /* progressive JPEG parameters for scan */

#if JPEG_LIB_VERSION >= 80
  int block_size;               /* the basic DCT block size: 1..16 */
  _Ptr<const int> natural_order;     /* natural-order position array */
  int lim_Se;                   /* min( Se, DCTSIZE2-1 ) */
#endif

  /*
   * Links to compression subobjects (methods and private variables of modules)
   */
  _Ptr<struct jpeg_comp_master> master;
  _Ptr<struct jpeg_c_main_controller> main;
  _Ptr<struct jpeg_c_prep_controller> prep;
  _Ptr<struct jpeg_c_coef_controller> coef;
  _Ptr<struct jpeg_marker_writer> marker;
  _Ptr<struct jpeg_color_converter> cconvert;
  _Ptr<struct jpeg_downsampler> downsample;
  _Ptr<struct jpeg_forward_dct> fdct;
  _Ptr<struct jpeg_entropy_encoder> entropy;
  _Ptr<jpeg_scan_info> script_space; /* workspace for jpeg_simple_progression */
  int script_space_size;
};


/* Master record for a decompression instance */

struct jpeg_decompress_struct {
  jpeg_common_fields;           /* Fields shared with jpeg_compress_struct */

  /* Source of compressed data */
  _Ptr<struct jpeg_source_mgr> src;

  /* Basic description of image --- filled in by jpeg_read_header(). */
  /* Application may inspect these values to decide how to process image. */

  JDIMENSION image_width;       /* nominal image width (from SOF marker) */
  JDIMENSION image_height;      /* nominal image height */
  int num_components;           /* # of color components in JPEG image */
  J_COLOR_SPACE jpeg_color_space; /* colorspace of JPEG image */

  /* Decompression processing parameters --- these fields must be set before
   * calling jpeg_start_decompress().  Note that jpeg_read_header() initializes
   * them to default values.
   */

  J_COLOR_SPACE out_color_space; /* colorspace for output */

  unsigned int scale_num, scale_denom; /* fraction by which to scale image */

  double output_gamma;          /* image gamma wanted in output */

  boolean buffered_image;       /* TRUE=multiple output passes */
  boolean raw_data_out;         /* TRUE=downsampled data wanted */

  J_DCT_METHOD dct_method;      /* IDCT algorithm selector */
  boolean do_fancy_upsampling;  /* TRUE=apply fancy upsampling */
  boolean do_block_smoothing;   /* TRUE=apply interblock smoothing */

  boolean quantize_colors;      /* TRUE=colormapped output wanted */
  /* the following are ignored if not quantize_colors: */
  J_DITHER_MODE dither_mode;    /* type of color dithering to use */
  boolean two_pass_quantize;    /* TRUE=use two-pass color quantization */
  int desired_number_of_colors; /* max # colors to use in created colormap */
  /* these are significant only in buffered-image mode: */
  boolean enable_1pass_quant;   /* enable future use of 1-pass quantizer */
  boolean enable_external_quant;/* enable future use of external colormap */
  boolean enable_2pass_quant;   /* enable future use of 2-pass quantizer */

  /* Description of actual output image that will be returned to application.
   * These fields are computed by jpeg_start_decompress().
   * You can also use jpeg_calc_output_dimensions() to determine these values
   * in advance of calling jpeg_start_decompress().
   */

  JDIMENSION output_width;      /* scaled image width */
  JDIMENSION output_height;     /* scaled image height */
  int out_color_components;     /* # of color components in out_color_space */
  int output_components;        /* # of color components returned */
  /* output_components is 1 (a colormap index) when quantizing colors;
   * otherwise it equals out_color_components.
   */
  int rec_outbuf_height;        /* min recommended height of scanline buffer */
  /* If the buffer passed to jpeg_read_scanlines() is less than this many rows
   * high, space and time will be wasted due to unnecessary data copying.
   * Usually rec_outbuf_height will be 1 or 2, at most 4.
   */

  /* When quantizing colors, the output colormap is described by these fields.
   * The application can supply a colormap by setting colormap non-NULL before
   * calling jpeg_start_decompress; otherwise a colormap is created during
   * jpeg_start_decompress or jpeg_start_output.
   * The map has out_color_components rows and actual_number_of_colors columns.
   */
  int actual_number_of_colors;  /* number of entries in use */
  JSAMPARRAY colormap;          /* The color map as a 2-D pixel array */

  /* State variables: these variables indicate the progress of decompression.
   * The application may examine these but must not modify them.
   */

  /* Row index of next scanline to be read from jpeg_read_scanlines().
   * Application may use this to control its processing loop, e.g.,
   * "while (output_scanline < output_height)".
   */
  JDIMENSION output_scanline;   /* 0 .. output_height-1  */

  /* Current input scan number and number of iMCU rows completed in scan.
   * These indicate the progress of the decompressor input side.
   */
  int input_scan_number;        /* Number of SOS markers seen so far */
  JDIMENSION input_iMCU_row;    /* Number of iMCU rows completed */

  /* The "output scan number" is the notional scan being displayed by the
   * output side.  The decompressor will not allow output scan/row number
   * to get ahead of input scan/row, but it can fall arbitrarily far behind.
   */
  int output_scan_number;       /* Nominal scan number being displayed */
  JDIMENSION output_iMCU_row;   /* Number of iMCU rows read */

  /* Current progression status.  coef_bits[c][i] indicates the precision
   * with which component c's DCT coefficient i (in zigzag order) is known.
   * It is -1 when no data has yet been received, otherwise it is the point
   * transform (shift) value for the most recent scan of the coefficient
   * (thus, 0 at completion of the progression).
   * This pointer is NULL when reading a non-progressive file.
   */
  _Ptr<int _Checked[DCTSIZE2]> coef_bits;   /* -1 or current Al value for each coef */

  /* Internal JPEG parameters --- the application usually need not look at
   * these fields.  Note that the decompressor output side may not use
   * any parameters that can change between scans.
   */

  /* Quantization and Huffman tables are carried forward across input
   * datastreams when processing abbreviated JPEG datastreams.
   */

  _Ptr<JQUANT_TBL> quant_tbl_ptrs _Checked[NUM_QUANT_TBLS];
  /* ptrs to coefficient quantization tables, or NULL if not defined */

  _Ptr<JHUFF_TBL> dc_huff_tbl_ptrs _Checked[NUM_HUFF_TBLS];
  _Ptr<JHUFF_TBL> ac_huff_tbl_ptrs _Checked[NUM_HUFF_TBLS];
  /* ptrs to Huffman coding tables, or NULL if not defined */

  /* These parameters are never carried across datastreams, since they
   * are given in SOF/SOS markers or defined to be reset by SOI.
   */

  int data_precision;           /* bits of precision in image data */

  _Ptr<jpeg_component_info> comp_info;
  /* comp_info[i] describes component that appears i'th in SOF */

#if JPEG_LIB_VERSION >= 80
  boolean is_baseline;          /* TRUE if Baseline SOF0 encountered */
#endif
  boolean progressive_mode;     /* TRUE if SOFn specifies progressive mode */
  boolean arith_code;           /* TRUE=arithmetic coding, FALSE=Huffman */

  UINT8 arith_dc_L _Checked[NUM_ARITH_TBLS]; /* L values for DC arith-coding tables */
  UINT8 arith_dc_U _Checked[NUM_ARITH_TBLS]; /* U values for DC arith-coding tables */
  UINT8 arith_ac_K _Checked[NUM_ARITH_TBLS]; /* Kx values for AC arith-coding tables */

  unsigned int restart_interval; /* MCUs per restart interval, or 0 for no restart */

  /* These fields record data obtained from optional markers recognized by
   * the JPEG library.
   */
  boolean saw_JFIF_marker;      /* TRUE iff a JFIF APP0 marker was found */
  /* Data copied from JFIF marker; only valid if saw_JFIF_marker is TRUE: */
  UINT8 JFIF_major_version;     /* JFIF version number */
  UINT8 JFIF_minor_version;
  UINT8 density_unit;           /* JFIF code for pixel size units */
  UINT16 X_density;             /* Horizontal pixel density */
  UINT16 Y_density;             /* Vertical pixel density */
  boolean saw_Adobe_marker;     /* TRUE iff an Adobe APP14 marker was found */
  UINT8 Adobe_transform;        /* Color transform code from Adobe marker */

  boolean CCIR601_sampling;     /* TRUE=first samples are cosited */

  /* Aside from the specific data retained from APPn markers known to the
   * library, the uninterpreted contents of any or all APPn and COM markers
   * can be saved in a list for examination by the application.
   */
  jpeg_saved_marker_ptr marker_list; /* Head of list of saved markers */

  /* Remaining fields are known throughout decompressor, but generally
   * should not be touched by a surrounding application.
   */

  /*
   * These fields are computed during decompression startup
   */
  int max_h_samp_factor;        /* largest h_samp_factor */
  int max_v_samp_factor;        /* largest v_samp_factor */

#if JPEG_LIB_VERSION >= 70
  int min_DCT_h_scaled_size;    /* smallest DCT_h_scaled_size of any component */
  int min_DCT_v_scaled_size;    /* smallest DCT_v_scaled_size of any component */
#else
  int min_DCT_scaled_size;      /* smallest DCT_scaled_size of any component */
#endif

  JDIMENSION total_iMCU_rows;   /* # of iMCU rows in image */
  /* The coefficient controller's input and output progress is measured in
   * units of "iMCU" (interleaved MCU) rows.  These are the same as MCU rows
   * in fully interleaved JPEG scans, but are used whether the scan is
   * interleaved or not.  We define an iMCU row as v_samp_factor DCT block
   * rows of each component.  Therefore, the IDCT output contains
   * v_samp_factor*DCT_[v_]scaled_size sample rows of a component per iMCU row.
   */

  _Ptr<JSAMPLE> sample_range_limit;  /* table for fast range-limiting */

  /*
   * These fields are valid during any one scan.
   * They describe the components and MCUs actually appearing in the scan.
   * Note that the decompressor output side must not use these fields.
   */
  int comps_in_scan;            /* # of JPEG components in this scan */
  _Ptr<jpeg_component_info> cur_comp_info _Checked[MAX_COMPS_IN_SCAN];
  /* *cur_comp_info[i] describes component that appears i'th in SOS */

  JDIMENSION MCUs_per_row;      /* # of MCUs across the image */
  JDIMENSION MCU_rows_in_scan;  /* # of MCU rows in the image */

  int blocks_in_MCU;            /* # of DCT blocks per MCU */
  int MCU_membership _Checked[D_MAX_BLOCKS_IN_MCU];
  /* MCU_membership[i] is index in cur_comp_info of component owning */
  /* i'th block in an MCU */

  int Ss, Se, Ah, Al;           /* progressive JPEG parameters for scan */

#if JPEG_LIB_VERSION >= 80
  /* These fields are derived from Se of first SOS marker.
   */
  int block_size;               /* the basic DCT block size: 1..16 */
  _Ptr<const int> natural_order; /* natural-order position array for entropy decode */
  int lim_Se;                   /* min( Se, DCTSIZE2-1 ) for entropy decode */
#endif

  /* This field is shared between entropy decoder and marker parser.
   * It is either zero or the code of a JPEG marker that has been
   * read from the data source, but has not yet been processed.
   */
  int unread_marker;

  /*
   * Links to decompression subobjects (methods, private variables of modules)
   */
  _Ptr<struct jpeg_decomp_master> master;
  _Ptr<struct jpeg_d_main_controller> main;
  _Ptr<struct jpeg_d_coef_controller> coef;
  _Ptr<struct jpeg_d_post_controller> post;
  _Ptr<struct jpeg_input_controller> inputctl;
  _Ptr<struct jpeg_marker_reader> marker;
  _Ptr<struct jpeg_entropy_decoder> entropy;
  _Ptr<struct jpeg_inverse_dct> idct;
  _Ptr<struct jpeg_upsampler> upsample;
  _Ptr<struct jpeg_color_deconverter> cconvert;
  _Ptr<struct jpeg_color_quantizer> cquantize;
};


/* "Object" declarations for JPEG modules that may be supplied or called
 * directly by the surrounding application.
 * As with all objects in the JPEG library, these structs only define the
 * publicly visible methods and state variables of a module.  Additional
 * private fields may exist after the public ones.
 */


/* Error handler object */

struct jpeg_error_mgr {
  /* Error exit handler: does not return to caller */
  _Ptr<void (_Ptr<struct jpeg_common_struct> cinfo)> error_exit;
  /* Conditionally emit a trace or warning message */
  _Ptr<void (_Ptr<struct jpeg_common_struct> cinfo, int msg_level)> emit_message;
  /* Routine that actually outputs a trace or error message */
  _Ptr<void (_Ptr<struct jpeg_common_struct> cinfo)> output_message;
  /* Format a message string for the most recent JPEG error or message */
  _Ptr<void (_Ptr<struct jpeg_common_struct> cinfo, _Ptr<char> buffer)> format_message;
#define JMSG_LENGTH_MAX  200    /* recommended size of format_message buffer */
  /* Reset error state variables at start of a new image */
  _Ptr<void (_Ptr<struct jpeg_common_struct> cinfo)> reset_error_mgr;

  /* The message ID code and any parameters are saved here.
   * A message can have one string parameter or up to 8 int parameters.
   */
  int msg_code;
#define JMSG_STR_PARM_MAX  80
  union {
    int i[8];
    char s[JMSG_STR_PARM_MAX];
  } msg_parm;

  /* Standard state variables for error facility */

  int trace_level;              /* max msg_level that will be displayed */

  /* For recoverable corrupt-data errors, we emit a warning message,
   * but keep going unless emit_message chooses to abort.  emit_message
   * should count warnings in num_warnings.  The surrounding application
   * can check for bad data by seeing if num_warnings is nonzero at the
   * end of processing.
   */
  long num_warnings;            /* number of corrupt-data warnings */

  /* These fields point to the table(s) of error message strings.
   * An application can change the table pointer to switch to a different
   * message list (typically, to change the language in which errors are
   * reported).  Some applications may wish to add additional error codes
   * that will be handled by the JPEG library error mechanism; the second
   * table pointer is used for this purpose.
   *
   * First table includes all errors generated by JPEG library itself.
   * Error code 0 is reserved for a "no such error string" message.
   */
  _Ptr<const _Ptr<const char>> jpeg_message_table; /* Library errors */
  int last_jpeg_message;    /* Table contains strings 0..last_jpeg_message */
  /* Second table can be added by application (see cjpeg/djpeg for example).
   * It contains strings numbered first_addon_message..last_addon_message.
   */
  _Ptr<const _Ptr<const char>> addon_message_table; /* Non-library errors */
  int first_addon_message;      /* code for first string in addon table */
  int last_addon_message;       /* code for last string in addon table */
};


/* Progress monitor object */

struct jpeg_progress_mgr {
  _Ptr<void (_Ptr<struct jpeg_common_struct> cinfo)> progress_monitor;

  long pass_counter;            /* work units completed in this pass */
  long pass_limit;              /* total number of work units in this pass */
  int completed_passes;         /* passes completed so far */
  int total_passes;             /* total number of passes expected */
};


/* Data destination object for compression */

struct jpeg_destination_mgr {
  _Ptr<JOCTET> next_output_byte;     /* => next byte to write in buffer */
  size_t free_in_buffer;        /* # of byte spaces remaining in buffer */

  _Ptr<void (_Ptr<struct jpeg_compress_struct> cinfo)> init_destination;
  _Ptr<boolean (_Ptr<struct jpeg_compress_struct> cinfo)> empty_output_buffer;
  _Ptr<void (_Ptr<struct jpeg_compress_struct> cinfo)> term_destination;
};


/* Data source object for decompression */

struct jpeg_source_mgr {
  _Ptr<const JOCTET> next_input_byte; /* => next byte to read from buffer */
  size_t bytes_in_buffer;       /* # of bytes remaining in buffer */

  _Ptr<void (_Ptr<struct jpeg_decompress_struct> cinfo)> init_source;
  _Ptr<boolean (_Ptr<struct jpeg_decompress_struct> cinfo)> fill_input_buffer;
  _Ptr<void (_Ptr<struct jpeg_decompress_struct> cinfo, long num_bytes)> skip_input_data;
  _Ptr<boolean (_Ptr<struct jpeg_decompress_struct> cinfo, int desired)> resync_to_restart;
  _Ptr<void (_Ptr<struct jpeg_decompress_struct> cinfo)> term_source;
};


/* Memory manager object.
 * Allocates "small" objects (a few K total), "large" objects (tens of K),
 * and "really big" objects (virtual arrays with backing store if needed).
 * The memory manager does not allow individual objects to be freed; rather,
 * each created object is assigned to a pool, and whole pools can be freed
 * at once.  This is faster and more convenient than remembering exactly what
 * to free, especially where malloc()/free() are not too speedy.
 * NB: alloc routines never return NULL.  They exit to error_exit if not
 * successful.
 */

#define JPOOL_PERMANENT  0      /* lasts until master record is destroyed */
#define JPOOL_IMAGE      1      /* lasts until done with image/datastream */
#define JPOOL_NUMPOOLS   2

typedef struct jvirt_sarray_control *jvirt_sarray_ptr;
typedef struct jvirt_barray_control *jvirt_barray_ptr;


struct jpeg_memory_mgr {
  /* Method pointers */
  _Ptr<void *(_Ptr<struct jpeg_common_struct> cinfo, int pool_id, size_t sizeofobject)> alloc_small;
  _Ptr<void *(_Ptr<struct jpeg_common_struct> cinfo, int pool_id, size_t sizeofobject)> alloc_large;
  _Ptr<_Array_ptr<JSAMPROW> (_Ptr<struct jpeg_common_struct> cinfo, int pool_id, JDIMENSION samplesperrow, JDIMENSION numrows) : count(numrows)> alloc_sarray;
  _Ptr<_Ptr<JBLOCKROW> (_Ptr<struct jpeg_common_struct> cinfo, int pool_id, JDIMENSION blocksperrow, JDIMENSION numrows)> alloc_barray;
  _Ptr<_Ptr<struct jvirt_sarray_control> (_Ptr<struct jpeg_common_struct> cinfo, int pool_id, boolean pre_zero, JDIMENSION samplesperrow, JDIMENSION numrows, JDIMENSION maxaccess)> request_virt_sarray;
  _Ptr<_Ptr<struct jvirt_barray_control> (_Ptr<struct jpeg_common_struct> cinfo, int pool_id, boolean pre_zero, JDIMENSION blocksperrow, JDIMENSION numrows, JDIMENSION maxaccess)> request_virt_barray;
  _Ptr<void (_Ptr<struct jpeg_common_struct> cinfo)> realize_virt_arrays;
  _Ptr<_Ptr<JSAMPROW> (_Ptr<struct jpeg_common_struct> cinfo, _Ptr<struct jvirt_sarray_control> ptr, JDIMENSION start_row, JDIMENSION num_rows, boolean writable)> access_virt_sarray;
  _Ptr<_Ptr<JBLOCKROW> (_Ptr<struct jpeg_common_struct> cinfo, _Ptr<struct jvirt_barray_control> ptr, JDIMENSION start_row, JDIMENSION num_rows, boolean writable)> access_virt_barray;
  _Ptr<void (_Ptr<struct jpeg_common_struct> cinfo, int pool_id)> free_pool;
  _Ptr<void (_Ptr<struct jpeg_common_struct> cinfo)> self_destruct;

  /* Limit on memory allocation for this JPEG object.  (Note that this is
   * merely advisory, not a guaranteed maximum; it only affects the space
   * used for virtual-array buffers.)  May be changed by outer application
   * after creating the JPEG object.
   */
  long max_memory_to_use;

  /* Maximum allocation request accepted by alloc_large. */
  long max_alloc_chunk;
};


/* Routine signature for application-supplied marker processing methods.
 * Need not pass marker code since it is stored in cinfo->unread_marker.
 */
typedef _Ptr<boolean (j_decompress_ptr cinfo)> jpeg_marker_parser_method;


/* Originally, this macro was used as a way of defining function prototypes
 * for both modern compilers as well as older compilers that did not support
 * prototype parameters.  libjpeg-turbo has never supported these older,
 * non-ANSI compilers, but the macro is still included because there is some
 * software out there that uses it.
 */

#define JPP(arglist)    arglist


/* Default error-management setup */
extern struct jpeg_error_mgr *jpeg_std_error(struct jpeg_error_mgr *err : itype(_Ptr<struct jpeg_error_mgr>)) : itype(_Ptr<struct jpeg_error_mgr>);

/* Initialization of JPEG compression objects.
 * jpeg_create_compress() and jpeg_create_decompress() are the exported
 * names that applications should call.  These expand to calls on
 * jpeg_CreateCompress and jpeg_CreateDecompress with additional information
 * passed for version mismatch checking.
 * NB: you must set up the error-manager BEFORE calling jpeg_create_xxx.
 */
#define jpeg_create_compress(cinfo) \
  jpeg_CreateCompress((cinfo), JPEG_LIB_VERSION, \
                      (size_t)sizeof(struct jpeg_compress_struct))
#define jpeg_create_decompress(cinfo) \
  jpeg_CreateDecompress((cinfo), JPEG_LIB_VERSION, \
                        (size_t)sizeof(struct jpeg_decompress_struct))
EXTERN(void) jpeg_CreateCompress(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr), int version, size_t structsize);
EXTERN(void) jpeg_CreateDecompress(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr), int version, size_t structsize);
/* Destruction of JPEG compression objects */
EXTERN(void) jpeg_destroy_compress(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr));
EXTERN(void) jpeg_destroy_decompress(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr));

/* Standard data source and destination managers: stdio streams. */
/* Caller is responsible for opening the file before and closing after. */
EXTERN(void) jpeg_stdio_dest(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr), FILE *outfile : itype(_Ptr<FILE>));
EXTERN(void) jpeg_stdio_src(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr), FILE *infile : itype(_Ptr<FILE>));

#if JPEG_LIB_VERSION >= 80 || defined(MEM_SRCDST_SUPPORTED)
/* Data source and destination managers: memory buffers. */
EXTERN(void) jpeg_mem_dest(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr), unsigned char **outbuffer : itype(_Ptr<_Ptr<unsigned char>>), unsigned long *outsize : itype(_Ptr<unsigned long>));
EXTERN(void) jpeg_mem_src(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr), const unsigned char *inbuffer : itype(_Ptr<const unsigned char>), unsigned long insize);
#endif

/* Default parameter setup for compression */
EXTERN(void) jpeg_set_defaults(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr));
/* Compression parameter setup aids */
EXTERN(void) jpeg_set_colorspace(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr), J_COLOR_SPACE colorspace);
EXTERN(void) jpeg_default_colorspace(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr));
EXTERN(void) jpeg_set_quality(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr), int quality, boolean force_baseline);
EXTERN(void) jpeg_set_linear_quality(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr), int scale_factor, boolean force_baseline);
#if JPEG_LIB_VERSION >= 70
EXTERN(void) jpeg_default_qtables(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr), boolean force_baseline);
#endif
EXTERN(void) jpeg_add_quant_table(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr), int which_tbl, const unsigned int *basic_table : itype(_Ptr<const unsigned int>), int scale_factor, boolean force_baseline);
EXTERN(int) jpeg_quality_scaling(int quality);
EXTERN(void) jpeg_simple_progression(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr));
EXTERN(void) jpeg_suppress_tables(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr), boolean suppress);
extern JQUANT_TBL *jpeg_alloc_quant_table(struct jpeg_common_struct *cinfo : itype(j_common_ptr)) : itype(_Ptr<JQUANT_TBL>);
extern JHUFF_TBL *jpeg_alloc_huff_table(struct jpeg_common_struct *cinfo : itype(j_common_ptr)) : itype(_Ptr<JHUFF_TBL>);

/* Main entry points for compression */
EXTERN(void) jpeg_start_compress(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr), boolean write_all_tables);
EXTERN(JDIMENSION) jpeg_write_scanlines(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr), unsigned char **scanlines : itype(JSAMPARRAY), JDIMENSION num_lines);
EXTERN(void) jpeg_finish_compress(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr));

#if JPEG_LIB_VERSION >= 70
/* Precalculate JPEG dimensions for current compression parameters. */
EXTERN(void) jpeg_calc_jpeg_dimensions(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr));
#endif

/* Replaces jpeg_write_scanlines when writing raw downsampled data. */
EXTERN(JDIMENSION) jpeg_write_raw_data(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr), unsigned char ***data : itype(JSAMPIMAGE), JDIMENSION num_lines);

/* Write a special marker.  See libjpeg.txt concerning safe usage. */
EXTERN(void) jpeg_write_marker(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr), int marker, const JOCTET *dataptr : itype(_Ptr<const JOCTET>), unsigned int datalen);
/* Same, but piecemeal. */
EXTERN(void) jpeg_write_m_header(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr), int marker, unsigned int datalen);
EXTERN(void) jpeg_write_m_byte(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr), int val);

/* Alternate compression function: just write an abbreviated table file */
EXTERN(void) jpeg_write_tables(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr));

/* Write ICC profile.  See libjpeg.txt for usage information. */
EXTERN(void) jpeg_write_icc_profile(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr), const JOCTET *icc_data_ptr : itype(_Ptr<const JOCTET>), unsigned int icc_data_len);


/* Decompression startup: read start of JPEG datastream to see what's there */
EXTERN(int) jpeg_read_header(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr), boolean require_image);
/* Return value is one of: */
#define JPEG_SUSPENDED           0 /* Suspended due to lack of input data */
#define JPEG_HEADER_OK           1 /* Found valid image datastream */
#define JPEG_HEADER_TABLES_ONLY  2 /* Found valid table-specs-only datastream */
/* If you pass require_image = TRUE (normal case), you need not check for
 * a TABLES_ONLY return code; an abbreviated file will cause an error exit.
 * JPEG_SUSPENDED is only possible if you use a data source module that can
 * give a suspension return (the stdio source module doesn't).
 */

/* Main entry points for decompression */
EXTERN(boolean) jpeg_start_decompress(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr));
EXTERN(JDIMENSION) jpeg_read_scanlines(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr), unsigned char **scanlines : itype(JSAMPARRAY), JDIMENSION max_lines);
EXTERN(JDIMENSION) jpeg_skip_scanlines(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr), JDIMENSION num_lines);
EXTERN(void) jpeg_crop_scanline(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr), JDIMENSION *xoffset : itype(_Ptr<JDIMENSION>), JDIMENSION *width : itype(_Ptr<JDIMENSION>));
EXTERN(boolean) jpeg_finish_decompress(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr));

/* Replaces jpeg_read_scanlines when reading raw downsampled data. */
EXTERN(JDIMENSION) jpeg_read_raw_data(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr), unsigned char ***data : itype(JSAMPIMAGE), JDIMENSION max_lines);

/* Additional entry points for buffered-image mode. */
EXTERN(boolean) jpeg_has_multiple_scans(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr));
EXTERN(boolean) jpeg_start_output(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr), int scan_number);
EXTERN(boolean) jpeg_finish_output(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr));
EXTERN(boolean) jpeg_input_complete(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr));
EXTERN(void) jpeg_new_colormap(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr));
EXTERN(int) jpeg_consume_input(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr));
/* Return value is one of: */
/* #define JPEG_SUSPENDED       0    Suspended due to lack of input data */
#define JPEG_REACHED_SOS        1 /* Reached start of new scan */
#define JPEG_REACHED_EOI        2 /* Reached end of image */
#define JPEG_ROW_COMPLETED      3 /* Completed one iMCU row */
#define JPEG_SCAN_COMPLETED     4 /* Completed last iMCU row of a scan */

/* Precalculate output dimensions for current decompression parameters. */
#if JPEG_LIB_VERSION >= 80
EXTERN(void) jpeg_core_output_dimensions(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr));
#endif
EXTERN(void) jpeg_calc_output_dimensions(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr));

/* Control saving of COM and APPn markers into marker_list. */
EXTERN(void) jpeg_save_markers(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr), int marker_code, unsigned int length_limit);

/* Install a special processing method for COM or APPn markers. */
EXTERN(void) jpeg_set_marker_processor(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr), int marker_code, boolean ((*routine)(struct jpeg_decompress_struct *)) : itype(jpeg_marker_parser_method));

/* Read or write raw DCT coefficients --- useful for lossless transcoding. */
extern jvirt_barray_ptr *jpeg_read_coefficients(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr)) : itype(_Ptr<jvirt_barray_ptr>);
EXTERN(void) jpeg_write_coefficients(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr), jvirt_barray_ptr *coef_arrays : itype(_Ptr<jvirt_barray_ptr>));
EXTERN(void) jpeg_copy_critical_parameters(struct jpeg_decompress_struct *srcinfo : itype(j_decompress_ptr), struct jpeg_compress_struct *dstinfo : itype(j_compress_ptr));

/* If you choose to abort compression or decompression before completing
 * jpeg_finish_(de)compress, then you need to clean up to release memory,
 * temporary files, etc.  You can just call jpeg_destroy_(de)compress
 * if you're done with the JPEG object, but if you want to clean it up and
 * reuse it, call this:
 */
EXTERN(void) jpeg_abort_compress(struct jpeg_compress_struct *cinfo : itype(j_compress_ptr));
EXTERN(void) jpeg_abort_decompress(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr));

/* Generic versions of jpeg_abort and jpeg_destroy that work on either
 * flavor of JPEG object.  These may be more convenient in some places.
 */
EXTERN(void) jpeg_abort(struct jpeg_common_struct *cinfo : itype(j_common_ptr));
EXTERN(void) jpeg_destroy(struct jpeg_common_struct *cinfo : itype(j_common_ptr));

/* Default restart-marker-resync procedure for use by data source modules */
EXTERN(boolean) jpeg_resync_to_restart(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr), int desired);

/* Read ICC profile.  See libjpeg.txt for usage information. */
EXTERN(boolean) jpeg_read_icc_profile(struct jpeg_decompress_struct *cinfo : itype(j_decompress_ptr), JOCTET **icc_data_ptr : itype(_Ptr<_Ptr<JOCTET>>), unsigned int *icc_data_len : itype(_Ptr<unsigned int>));


/* These marker codes are exported since applications and data source modules
 * are likely to want to use them.
 */

#define JPEG_RST0       0xD0    /* RST0 marker code */
#define JPEG_EOI        0xD9    /* EOI marker code */
#define JPEG_APP0       0xE0    /* APP0 marker code */
#define JPEG_COM        0xFE    /* COM marker code */


/* If we have a brain-damaged compiler that emits warnings (or worse, errors)
 * for structure definitions that are never filled in, keep it quiet by
 * supplying dummy definitions for the various substructures.
 */

#ifdef INCOMPLETE_TYPES_BROKEN
#ifndef JPEG_INTERNALS          /* will be defined in jpegint.h */
struct jvirt_sarray_control { long dummy; };
struct jvirt_barray_control { long dummy; };
struct jpeg_comp_master { long dummy; };
struct jpeg_c_main_controller { long dummy; };
struct jpeg_c_prep_controller { long dummy; };
struct jpeg_c_coef_controller { long dummy; };
struct jpeg_marker_writer { long dummy; };
struct jpeg_color_converter { long dummy; };
struct jpeg_downsampler { long dummy; };
struct jpeg_forward_dct { long dummy; };
struct jpeg_entropy_encoder { long dummy; };
struct jpeg_decomp_master { long dummy; };
struct jpeg_d_main_controller { long dummy; };
struct jpeg_d_coef_controller { long dummy; };
struct jpeg_d_post_controller { long dummy; };
struct jpeg_input_controller { long dummy; };
struct jpeg_marker_reader { long dummy; };
struct jpeg_entropy_decoder { long dummy; };
struct jpeg_inverse_dct { long dummy; };
struct jpeg_upsampler { long dummy; };
struct jpeg_color_deconverter { long dummy; };
struct jpeg_color_quantizer { long dummy; };
#endif /* JPEG_INTERNALS */
#endif /* INCOMPLETE_TYPES_BROKEN */


/*
 * The JPEG library modules define JPEG_INTERNALS before including this file.
 * The internal structure declarations are read only when that is true.
 * Applications using the library should not include jpegint.h, but may wish
 * to include jerror.h.
 */

#ifdef JPEG_INTERNALS
#include "jpegint.h"            /* fetch private declarations */
#include "jerror.h"             /* fetch error codes too */
#endif

#ifdef __cplusplus
#ifndef DONT_USE_EXTERN_C
}
#endif
#endif

#endif /* JPEGLIB_H */
