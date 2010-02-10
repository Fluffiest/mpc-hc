LAVC_DIR=libavcodec
LAVU_DIR=libavutil
LSWS_DIR=libswscale
LIBV_DIR=libvo
PNG_DIR =../../../../libpng
ifeq ($(64BIT),yes)
	ZLIB_DIR=../../../../zlib
else
	ZLIB_DIR=../../../../zlib
endif

CFLAGS+= -I. -I.. -I$(LAVC_DIR) -I$(LAVU_DIR) -I$(LSWS_DIR) -I$(ZLIB_DIR) -I$(PNG_DIR) \
		-DHAVE_AV_CONFIG_H -D_ISOC99_SOURCE -D_POSIX_C_SOURCE=200112 -std=gnu99

SRCS_C=\
		$(LAVC_DIR)/aac_ac3_parser.c \
		$(LAVC_DIR)/ac3.c \
		$(LAVC_DIR)/ac3_parser.c \
		$(LAVC_DIR)/ac3dec.c \
		$(LAVC_DIR)/ac3dec_data.c \
		$(LAVC_DIR)/ac3tab.c \
		$(LAVC_DIR)/adpcm.c \
		$(LAVC_DIR)/allcodecs.c \
		$(LAVC_DIR)/bitstream.c \
		$(LAVC_DIR)/cabac.c \
		$(LAVC_DIR)/CompilatorVersion.c\
		$(LAVC_DIR)/dsputil.c \
		$(LAVC_DIR)/eac3dec.c \
		$(LAVC_DIR)/eac3dec_data.c \
		$(LAVC_DIR)/error_resilience.c \
		$(LAVC_DIR)/faandct.c \
		$(LAVC_DIR)/faanidct.c \
		$(LAVC_DIR)/fft.c \
		$(LAVC_DIR)/flvdec.c \
		$(LAVC_DIR)/golomb.c \
		$(LAVC_DIR)/h263.c \
		$(LAVC_DIR)/h263_parser.c \
		$(LAVC_DIR)/h263dec.c \
		$(LAVC_DIR)/h264.c \
		$(LAVC_DIR)/h264_cabac.c \
		$(LAVC_DIR)/h264_cavlc.c \
		$(LAVC_DIR)/h264_direct.c \
		$(LAVC_DIR)/h264_loopfilter.c \
		$(LAVC_DIR)/h264_parser.c \
		$(LAVC_DIR)/h264_ps.c \
		$(LAVC_DIR)/h264_ps.c \
		$(LAVC_DIR)/h264_refs.c \
		$(LAVC_DIR)/h264_sei.c \
		$(LAVC_DIR)/h264idct.c \
		$(LAVC_DIR)/h264pred.c \
		$(LAVC_DIR)/huffman.c \
		$(LAVC_DIR)/imgconvert.c \
		$(LAVC_DIR)/intelh263dec.c \
		$(LAVC_DIR)/intrax8.c \
		$(LAVC_DIR)/intrax8dsp.c \
		$(LAVC_DIR)/ituh263dec.c \
		$(LAVC_DIR)/jpegls.c \
		$(LAVC_DIR)/jpeglsdec.c \
		$(LAVC_DIR)/jrevdct.c \
		$(LAVC_DIR)/libamr.c \
		$(LAVC_DIR)/mdct.c \
		$(LAVC_DIR)/mdct_tablegen.c \
		$(LAVC_DIR)/mjpeg.c \
		$(LAVC_DIR)/mjpegdec.c \
		$(LAVC_DIR)/mlp.c \
		$(LAVC_DIR)/mlp_parser.c \
		$(LAVC_DIR)/mlpdec.c \
		$(LAVC_DIR)/mlpdsp.c \
		$(LAVC_DIR)/mpc_helper.c \
		$(LAVC_DIR)/mpeg12.c \
		$(LAVC_DIR)/mpeg12data.c \
		$(LAVC_DIR)/mpeg4video.c \
		$(LAVC_DIR)/mpeg4video_parser.c \
		$(LAVC_DIR)/mpeg4videodec.c \
		$(LAVC_DIR)/mpegvideo.c \
		$(LAVC_DIR)/msmpeg4.c \
		$(LAVC_DIR)/msmpeg4data.c \
		$(LAVC_DIR)/nellymoser.c \
		$(LAVC_DIR)/nellymoserdec.c \
		$(LAVC_DIR)/options.c \
		$(LAVC_DIR)/parser.c \
		$(LAVC_DIR)/rv10.c \
		$(LAVC_DIR)/rv30.c \
		$(LAVC_DIR)/rv30dsp.c \
		$(LAVC_DIR)/rv34.c \
		$(LAVC_DIR)/rv40.c \
		$(LAVC_DIR)/rv40dsp.c \
		$(LAVC_DIR)/simple_idct.c \
		$(LAVC_DIR)/sp5xdec.c \
		$(LAVC_DIR)/svq1.c \
		$(LAVC_DIR)/svq1dec.c \
		$(LAVC_DIR)/svq3.c \
		$(LAVC_DIR)/tableprint.c \
		$(LAVC_DIR)/utils.c \
		$(LAVC_DIR)/vc1.c \
		$(LAVC_DIR)/vc1dec.c \
		$(LAVC_DIR)/vc1data.c \
		$(LAVC_DIR)/vc1dsp.c \
		$(LAVC_DIR)/vp3.c \
		$(LAVC_DIR)/vp3dsp.c \
		$(LAVC_DIR)/vp5.c \
		$(LAVC_DIR)/vp56.c \
		$(LAVC_DIR)/vp56data.c \
		$(LAVC_DIR)/vp6.c \
		$(LAVC_DIR)/vp6dsp.c \
		$(LAVC_DIR)/w32thread.c \
		$(LAVC_DIR)/wmv2.c \
		$(LAVC_DIR)/wmv2dec.c \
		$(LAVC_DIR)/xiph.c\
		$(LAVC_DIR)/amr_float/interf_dec.c \
		$(LAVC_DIR)/amr_float/sp_dec.c \
	\
		$(LAVU_DIR)/crc.c \
		$(LAVU_DIR)/log.c \
		$(LAVU_DIR)/mathematics.c \
		$(LAVU_DIR)/mem.c \
		$(LAVU_DIR)/random_seed.c \
		$(LAVU_DIR)/rational.c \
		$(LAVU_DIR)/lfg.c \
		$(LAVU_DIR)/md5.c \
	\
		$(LAVC_DIR)/x86/cpuid.c \
		$(LAVC_DIR)/x86/dsputil_mmx.c \
		$(LAVC_DIR)/x86/fdct_mmx.c \
		$(LAVC_DIR)/x86/idct_mmx.c \
		$(LAVC_DIR)/x86/idct_mmx_xvid.c \
		$(LAVC_DIR)/x86/idct_sse2_xvid.c \
		$(LAVC_DIR)/x86/mlpdsp.c \
		$(LAVC_DIR)/x86/mpegvideo_mmx.c \
		$(LAVC_DIR)/x86/simple_idct_mmx.c \
		$(LAVC_DIR)/x86/vc1dsp_mmx.c \
		$(LAVC_DIR)/x86/vp3dsp_mmx.c \
		$(LAVC_DIR)/x86/vp3dsp_sse2.c \
		$(LAVC_DIR)/x86/vp6dsp_mmx.c \
		$(LAVC_DIR)/x86/vp6dsp_sse2.c

SRCS_C+=\
		$(LIBV_DIR)/aclib.c \
		$(LIBV_DIR)/libvoinit.c \
		$(LSWS_DIR)/rgb2rgb.c \
		$(LSWS_DIR)/yuv2rgb.c \
		$(LSWS_DIR)/swscale.c \
		$(LSWS_DIR)/libswscalew32thread.c
		
ifneq ($(64BIT),yes)
SRCS_C+=\
      $(LAVC_DIR)/x86/fft.c \
      $(LAVC_DIR)/x86/fft_3dn.c \
      $(LAVC_DIR)/x86/fft_3dn2.c \
      $(LAVC_DIR)/x86/fft_sse.c
endif

ifneq ($(64BIT),yes)
SRCS_YASM=\
      $(LAVC_DIR)/x86/dsputil_yasm.asm \
      $(LAVC_DIR)/x86/fft_mmx.asm \
      $(LAVC_DIR)/x86/h264_deblock_sse2.asm \
      $(LAVC_DIR)/x86/h264_idct_sse2.asm \
      $(LAVC_DIR)/x86/x86util.asm
endif


ifeq ($(64BIT),yes)
	CFLAGS+=-DARCH_X86_64
	SLIB=../../../../../lib64/libavcodec_gcc.lib
else
	CFLAGS+=-DARCH_X86_32
	SLIB=../../../../../lib/libavcodec_gcc.lib
endif

DEF=libavcodec_dec.def

NOINTRIN=1

include makefile_c.inc

OPTFLAGS+= -fno-math-errno

.all: $(SLIB)

clean:
	rm -f *.o *.d *~ $(LAVC_DIR)/*.o $(LAVC_DIR)/*.d $(LAVC_DIR)/x86/*.o $(LAVC_DIR)/amr_float/*.o $(LAVC_DIR)/amr_float/*.d $(LAVU_DIR)/*.o $(LAVU_DIR)/*.d $(LSWS_DIR)/*.o $(LSWS_DIR)/*.d $(LIBV_DIR)/*.o $(LIBV_DIR)/*.d  $(SLIB) $(ZLIB_DIR)/*.o $(PNG_DIR)/*.o $(LAVC_DIR)/x86/*.d $(ZLIB_DIR)/*.d

#
# include dependency files if they exist
#
-include $(SRCS:.cpp=.d) $(SRCS_C:.c=.d)