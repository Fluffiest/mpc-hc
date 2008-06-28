#ifndef _FFCODECS_H_
#define _FFCODECS_H_

#define FFDSHOW_CODECS \
 CODEC_OP(CODEC_ID_UNSUPPORTED    ,-1,"unsupported") \
 CODEC_OP(CODEC_ID_NONE           , 0,"none") \
 \
 /* Well-known video codecs */ \
 CODEC_OP(CODEC_ID_H261              , 1,"h261") \
 CODEC_OP(CODEC_ID_H263              , 2,"h263") \
 CODEC_OP(CODEC_ID_H263I             , 3,"h263i") \
 CODEC_OP(CODEC_ID_H263P             , 4,"h263+") \
 CODEC_OP(CODEC_ID_H264              , 5,"h264") \
 CODEC_OP(CODEC_ID_MPEG1VIDEO        , 6,"mpeg1video") \
 CODEC_OP(CODEC_ID_MPEG2VIDEO        , 7,"mpeg2video") \
 CODEC_OP(CODEC_ID_MPEG4             , 8,"mpeg4") \
 CODEC_OP(CODEC_ID_MSMPEG4V1         , 9,"msmpeg4v1") \
 CODEC_OP(CODEC_ID_MSMPEG4V2         ,10,"msmpeg4v2") \
 CODEC_OP(CODEC_ID_MSMPEG4V3         ,11,"msmpeg4v3") \
 CODEC_OP(CODEC_ID_WMV1              ,12,"wmv1") \
 CODEC_OP(CODEC_ID_WMV2              ,13,"wmv2") \
 CODEC_OP(CODEC_ID_WMV3              ,14,"wmv3") \
 CODEC_OP(CODEC_ID_VC1               ,15,"vc1") \
 CODEC_OP(CODEC_ID_SVQ1              ,16,"svq1") \
 CODEC_OP(CODEC_ID_SVQ3              ,17,"svq3") \
 CODEC_OP(CODEC_ID_FLV1              ,18,"flv1") \
 CODEC_OP(CODEC_ID_VP3               ,19,"vp3") \
 CODEC_OP(CODEC_ID_VP5               ,20,"vp5") \
 CODEC_OP(CODEC_ID_VP6               ,21,"vp6") \
 CODEC_OP(CODEC_ID_VP6F              ,22,"vp6f") \
 CODEC_OP(CODEC_ID_RV10              ,23,"rv10") \
 CODEC_OP(CODEC_ID_RV20              ,24,"rv20") \
 CODEC_OP(CODEC_ID_DVVIDEO           ,25,"dvvideo") \
 CODEC_OP(CODEC_ID_MJPEG             ,26,"mjpeg") \
 CODEC_OP(CODEC_ID_MJPEGB            ,27,"mjpegb") \
 CODEC_OP(CODEC_ID_INDEO2            ,28,"indeo2") \
 CODEC_OP(CODEC_ID_INDEO3            ,29,"indeo3") \
 CODEC_OP(CODEC_ID_MSVIDEO1          ,30,"msvideo1") \
 CODEC_OP(CODEC_ID_CINEPAK           ,31,"cinepak") \
 CODEC_OP(CODEC_ID_THEORA            ,32,"theora") \
 \
 /* Lossless video codecs */ \
 CODEC_OP(CODEC_ID_HUFFYUV           ,33,"huffyuv") \
 CODEC_OP(CODEC_ID_FFVHUFF           ,34,"ffvhuff") \
 CODEC_OP(CODEC_ID_FFV1              ,35,"ffv1") \
 CODEC_OP(CODEC_ID_ZMBV              ,36,"zmbv") \
 CODEC_OP(CODEC_ID_PNG               ,37,"png") \
 CODEC_OP(CODEC_ID_COREPNG           ,38,"corepng") \
 CODEC_OP(CODEC_ID_LJPEG             ,39,"ljpeg") \
 CODEC_OP(CODEC_ID_JPEGLS            ,40,"jpegls") \
 CODEC_OP(CODEC_ID_CSCD              ,41,"camstudio") \
 CODEC_OP(CODEC_ID_QPEG              ,42,"qpeg") \
 CODEC_OP(CODEC_ID_LOCO              ,43,"loco") \
 CODEC_OP(CODEC_ID_MSZH              ,44,"mszh") \
 CODEC_OP(CODEC_ID_ZLIB              ,45,"zlib") \
 CODEC_OP(CODEC_ID_SP5X              ,46,"sp5x") \
 \
 /* Other video codecs */ \
 CODEC_OP(CODEC_ID_AVS               ,47,"avs") \
 CODEC_OP(CODEC_ID_8BPS              ,48,"8bps") \
 CODEC_OP(CODEC_ID_QTRLE             ,49,"qtrle") \
 CODEC_OP(CODEC_ID_RPZA              ,50,"qtrpza") \
 CODEC_OP(CODEC_ID_TRUEMOTION1       ,51,"truemotion") \
 CODEC_OP(CODEC_ID_TRUEMOTION2       ,52,"truemotion2") \
 CODEC_OP(CODEC_ID_TSCC              ,53,"tscc") \
 CODEC_OP(CODEC_ID_CYUV              ,55,"cyuv") \
 CODEC_OP(CODEC_ID_VCR1              ,56,"vcr1") \
 CODEC_OP(CODEC_ID_MSRLE             ,57,"msrle") \
 CODEC_OP(CODEC_ID_ASV1              ,58,"asv1") \
 CODEC_OP(CODEC_ID_ASV2              ,59,"asv2") \
 CODEC_OP(CODEC_ID_VIXL              ,60,"vixl") \
 CODEC_OP(CODEC_ID_WNV1              ,61,"wnv1") \
 CODEC_OP(CODEC_ID_FRAPS             ,62,"fraps") \
 CODEC_OP(CODEC_ID_MPEG2TS           ,63,"mpeg2ts") \
 CODEC_OP(CODEC_ID_AASC              ,64,"aasc") \
 CODEC_OP(CODEC_ID_ULTI              ,65,"ulti") \
 CODEC_OP(CODEC_ID_CAVS              ,66,"cavs") \
 CODEC_OP(CODEC_ID_SNOW              ,67,"snow") \
 CODEC_OP(CODEC_ID_VP6A              ,68,"vp6a") \
 CODEC_OP(CODEC_ID_RV30              ,69,"rv30") \
 CODEC_OP(CODEC_ID_RV40              ,70,"rv40") \
 CODEC_OP(CODEC_ID_AMV               ,71,"amv") \
 \
 /* Well-known audio codecs */ \
 CODEC_OP(CODEC_ID_MP2               ,100,"") \
 CODEC_OP(CODEC_ID_MP3               ,101,"") \
 CODEC_OP(CODEC_ID_VORBIS            ,102,"vorbis") \
 CODEC_OP(CODEC_ID_AC3               ,103,"") \
 CODEC_OP(CODEC_ID_WMAV1             ,104,"wmav1") \
 CODEC_OP(CODEC_ID_WMAV2             ,105,"wmav2") \
 CODEC_OP(CODEC_ID_AAC               ,106,"") \
 CODEC_OP(CODEC_ID_DTS               ,107,"") \
 CODEC_OP(CODEC_ID_IMC               ,108,"imc") \
 CODEC_OP(CODEC_ID_PCM_U16LE         ,109,"") \
 CODEC_OP(CODEC_ID_PCM_U16BE         ,110,"") \
 CODEC_OP(CODEC_ID_PCM_S8            ,111,"") \
 CODEC_OP(CODEC_ID_PCM_U8            ,112,"") \
 CODEC_OP(CODEC_ID_PCM_MULAW         ,113,"mulaw") \
 CODEC_OP(CODEC_ID_PCM_ALAW          ,114,"alaw") \
 CODEC_OP(CODEC_ID_ADPCM_IMA_QT      ,115,"adpcm ima qt") \
 CODEC_OP(CODEC_ID_ADPCM_IMA_WAV     ,116,"adpcm ima wav") \
 CODEC_OP(CODEC_ID_ADPCM_MS          ,117,"adpcm ms") \
 CODEC_OP(CODEC_ID_ADPCM_IMA_DK3     ,118,"adpcm ima dk3") \
 CODEC_OP(CODEC_ID_ADPCM_IMA_DK4     ,119,"adpcm ima dk4") \
 CODEC_OP(CODEC_ID_ADPCM_IMA_WS      ,120,"adpcm ima ws") \
 CODEC_OP(CODEC_ID_ADPCM_IMA_SMJPEG  ,121,"adpcm ima smjpeg") \
 CODEC_OP(CODEC_ID_ADPCM_4XM         ,122,"adpcm 4xm") \
 CODEC_OP(CODEC_ID_ADPCM_XA          ,123,"adpcm xa") \
 CODEC_OP(CODEC_ID_ADPCM_EA          ,124,"adpcm ea") \
 CODEC_OP(CODEC_ID_ADPCM_G726        ,125,"adpcm g726") \
 CODEC_OP(CODEC_ID_ADPCM_CT          ,126,"adpcm ct") \
 CODEC_OP(CODEC_ID_ADPCM_SWF         ,127,"adpcm swf") \
 CODEC_OP(CODEC_ID_ADPCM_YAMAHA      ,128,"adpcm yamaha") \
 CODEC_OP(CODEC_ID_ADPCM_SBPRO_2     ,129,"") \
 CODEC_OP(CODEC_ID_ADPCM_SBPRO_3     ,130,"") \
 CODEC_OP(CODEC_ID_ADPCM_SBPRO_4     ,131,"") \
 CODEC_OP(CODEC_ID_ADPCM_IMA_AMV     ,132,"adpcm ima amv") \
 CODEC_OP(CODEC_ID_XAN_DPCM          ,133,"adpcm ima xan") \
 CODEC_OP(CODEC_ID_FLAC              ,134,"flac") \
 CODEC_OP(CODEC_ID_AMR_NB            ,135,"amr nb") \
 CODEC_OP(CODEC_ID_GSM_MS            ,136,"gsm ms") \
 CODEC_OP(CODEC_ID_TTA               ,137,"tta") \
 CODEC_OP(CODEC_ID_MACE3             ,138,"mace3") \
 CODEC_OP(CODEC_ID_MACE6             ,139,"mace6") \
 CODEC_OP(CODEC_ID_QDM2              ,140,"qdm2") \
 CODEC_OP(CODEC_ID_COOK              ,141,"cook") \
 CODEC_OP(CODEC_ID_TRUESPEECH        ,142,"truespeech") \
 CODEC_OP(CODEC_ID_RA_144            ,143,"14_4") \
 CODEC_OP(CODEC_ID_RA_288            ,144,"28_8") \
 CODEC_OP(CODEC_ID_ATRAC3            ,145,"atrac 3") \
 CODEC_OP(CODEC_ID_NELLYMOSER        ,146,"nellymoser") \
 CODEC_OP(CODEC_ID_ADPCM_ADX         ,147,"adpcm adx") \
 CODEC_OP(CODEC_ID_MLP               ,148,"MLP") \
 CODEC_OP(CODEC_ID_EAC3              ,149,"EAC3") \
 CODEC_OP(CODEC_ID_WAVPACK           ,150,"Wavpack") \
 CODEC_OP(CODEC_ID_PCM_ZORK          ,151,"PCM Zork") \
 CODEC_OP(CODEC_ID_SHORTEN           ,152,"Shorten") \
\
 /* Raw formats */ \
 CODEC_OP(CODEC_ID_RAW           ,300,"raw") \
 CODEC_OP(CODEC_ID_YUY2          ,301,"raw") \
 CODEC_OP(CODEC_ID_RGB2          ,302,"raw") \
 CODEC_OP(CODEC_ID_RGB3          ,303,"raw") \
 CODEC_OP(CODEC_ID_RGB5          ,304,"raw") \
 CODEC_OP(CODEC_ID_RGB6          ,305,"raw") \
 CODEC_OP(CODEC_ID_BGR2          ,306,"raw") \
 CODEC_OP(CODEC_ID_BGR3          ,307,"raw") \
 CODEC_OP(CODEC_ID_BGR5          ,308,"raw") \
 CODEC_OP(CODEC_ID_BGR6          ,309,"raw") \
 CODEC_OP(CODEC_ID_YV12          ,310,"raw") \
 CODEC_OP(CODEC_ID_YVYU          ,311,"raw") \
 CODEC_OP(CODEC_ID_UYVY          ,312,"raw") \
 CODEC_OP(CODEC_ID_VYUY          ,313,"raw") \
 CODEC_OP(CODEC_ID_I420          ,314,"raw") \
 CODEC_OP(CODEC_ID_CLJR          ,315,"raw") \
 CODEC_OP(CODEC_ID_Y800          ,316,"raw") \
 CODEC_OP(CODEC_ID_444P          ,317,"raw") \
 CODEC_OP(CODEC_ID_422P          ,318,"raw") \
 CODEC_OP(CODEC_ID_411P          ,319,"raw") \
 CODEC_OP(CODEC_ID_410P          ,320,"raw") \
 CODEC_OP(CODEC_ID_NV12          ,321,"raw") \
 CODEC_OP(CODEC_ID_NV21          ,322,"raw") \
 CODEC_OP(CODEC_ID_PAL1          ,323,"raw") \
 CODEC_OP(CODEC_ID_PAL4          ,324,"raw") \
 CODEC_OP(CODEC_ID_PAL8          ,325,"raw") \
 CODEC_OP(CODEC_ID_LPCM          ,398,"raw") \
 CODEC_OP(CODEC_ID_PCM           ,399,"raw") \
 CODEC_OP(CODEC_ID_RAWVIDEO      ,400,"raw") \
 \
 CODEC_OP(CODEC_ID_PCM_S32LE           ,200,"raw") \
 CODEC_OP(CODEC_ID_PCM_S32BE           ,201,"raw") \
 CODEC_OP(CODEC_ID_PCM_U32LE           ,202,"raw") \
 CODEC_OP(CODEC_ID_PCM_U32BE           ,203,"raw") \
 CODEC_OP(CODEC_ID_PCM_S24LE           ,204,"raw") \
 CODEC_OP(CODEC_ID_PCM_S24BE           ,205,"raw") \
 CODEC_OP(CODEC_ID_PCM_U24LE           ,206,"raw") \
 CODEC_OP(CODEC_ID_PCM_U24BE           ,207,"raw") \
 CODEC_OP(CODEC_ID_PCM_S16LE           ,208,"raw") \
 CODEC_OP(CODEC_ID_PCM_S16BE           ,209,"raw") \
 CODEC_OP(CODEC_ID_PCM_S24DAUD         ,210,"raw") \
 \
 CODEC_OP(CODEC_ID_XVID4         ,400,"xvid") \
 \
 CODEC_OP(CODEC_ID_LIBMPEG2      ,500,"libmpeg2") \
 \
 CODEC_OP(CODEC_ID_THEORA_LIB    ,600,"libtheora") \
 \
 CODEC_OP(CODEC_ID_MP3LIB        ,700,"mp3lib") \
 \
 CODEC_OP(CODEC_ID_LIBMAD        ,800,"libmad") \
 \
 CODEC_OP(CODEC_ID_LIBFAAD       ,900,"faad2") \
 \
 CODEC_OP(CODEC_ID_WMV9_LIB      ,1000,"wmv9codec") \
 \
 CODEC_OP(CODEC_ID_AVISYNTH      ,1100,"avisynth") \
 \
 CODEC_OP(CODEC_ID_SKAL          ,1200,"skal's") \
 \
 CODEC_OP(CODEC_ID_LIBA52        ,1300,"liba52") \
 \
 CODEC_OP(CODEC_ID_SPDIF_AC3     ,1400,"s/pdif") \
 CODEC_OP(CODEC_ID_SPDIF_DTS     ,1401,"s/pdif") \
 \
 CODEC_OP(CODEC_ID_LIBDTS        ,1500,"libdts") \
 \
 CODEC_OP(CODEC_ID_TREMOR        ,1600,"tremor") \
 \
 CODEC_OP(CODEC_ID_REALAAC       ,1700,"realaac") \
 \
 CODEC_OP(CODEC_ID_AUDX          ,1800,"Aud-X") \
 \
 CODEC_OP(CODEC_ID_X264          ,1900,"x264") \
 CODEC_OP(CODEC_ID_X264_LOSSLESS ,1901,"x264 lossless") \
 \
 /* Subtitles formats */ \
 CODEC_OP(CODEC_ID_DVD_SUBTITLE  ,2000,"DVD Subs") \
 CODEC_OP(CODEC_ID_DVB_SUBTITLE  ,2001,"DVB Subs") \
 CODEC_OP(CODEC_ID_SSA           ,2002,"SSA") \
 CODEC_OP(CODEC_ID_TEXT          ,2003,"Text") \
 CODEC_OP(CODEC_ID_XSUB          ,2004,"XSUB") \
 /* Other formats */ \
 CODEC_OP(CODEC_TYPE_ATTACHMENT  ,2100,"Matroska Attachement") \
 CODEC_OP(CODEC_ID_GSM           ,2101,"GSM") \
 CODEC_OP(CODEC_ID_GIF           ,2102,"Gif") \
 CODEC_OP(CODEC_ID_TIFF          ,2103,"Tiff") \
 CODEC_OP(CODEC_ID_TTF           ,2104,"TTF")
enum CodecID
{
 #define CODEC_OP(codecEnum,codecId,codecName) codecEnum=codecId,
 FFDSHOW_CODECS
 #undef CODEC_OP
};

#ifdef __cplusplus

template<> struct isPOD<CodecID> {enum {is=true};};

const FOURCC* getCodecFOURCCs(CodecID codecId);
//const char_t* getCodecName(CodecID codecId);

static __inline bool lavc_codec(int x)     {return x>0 && x<200;}
static __inline bool raw_codec(int x)      {return x>=300 && x<400;}
static __inline bool xvid_codec(int x)     {return x==CODEC_ID_XVID4;}
static __inline bool theora_codec(int x)   {return x==CODEC_ID_THEORA_LIB;}
static __inline bool mplayer_codec(int x)  {return x==CODEC_ID_MP3LIB;}
static __inline bool wmv9_codec(int x)     {return x>=1000 && x<1100;}
static __inline bool mpeg12_codec(int x)   {return x==CODEC_ID_MPEG1VIDEO || x==CODEC_ID_MPEG2VIDEO || x==CODEC_ID_LIBMPEG2;}
static __inline bool mpeg1_codec(int x)    {return x==CODEC_ID_MPEG1VIDEO || x==CODEC_ID_LIBMPEG2;}
static __inline bool mpeg2_codec(int x)    {return x==CODEC_ID_MPEG2VIDEO || x==CODEC_ID_LIBMPEG2;}
static __inline bool mpeg4_codec(int x)    {return x==CODEC_ID_MPEG4 || xvid_codec(x) || x==CODEC_ID_SKAL;}
static __inline bool spdif_codec(int x)    {return x==CODEC_ID_SPDIF_AC3 || x==CODEC_ID_SPDIF_DTS;}
static __inline bool huffyuv_codec(int x)  {return x==CODEC_ID_HUFFYUV || x==CODEC_ID_FFVHUFF;}
static __inline bool x264_codec(int x)     {return x==CODEC_ID_X264 || x==CODEC_ID_X264_LOSSLESS;}
static __inline bool lossless_codec(int x) {return huffyuv_codec(x) || x==CODEC_ID_LJPEG || x==CODEC_ID_FFV1 || x==CODEC_ID_DVVIDEO || x==CODEC_ID_X264_LOSSLESS;}

//I'm not sure of all these
static __inline bool sup_CBR(int x)           {return !lossless_codec(x) && !raw_codec(x);}
static __inline bool sup_VBR_QUAL(int x)      {return !lossless_codec(x) && !raw_codec(x) && x!=CODEC_ID_SKAL;}
static __inline bool sup_VBR_QUANT(int x)     {return (lavc_codec(x) || xvid_codec(x) || theora_codec(x) || x==CODEC_ID_SKAL || x==CODEC_ID_X264) && !lossless_codec(x) && x!=CODEC_ID_SNOW;}
static __inline bool sup_XVID2PASS(int x)     {return sup_VBR_QUANT(x) && x!=CODEC_ID_X264 && x!=CODEC_ID_SNOW;}
static __inline bool sup_LAVC2PASS(int x)     {return (lavc_codec(x) && !lossless_codec(x) && x!=CODEC_ID_MJPEG && !raw_codec(x)) || x==CODEC_ID_X264;}

static __inline bool sup_interlace(int x)         {return x==CODEC_ID_MPEG4 || x==CODEC_ID_MPEG2VIDEO || xvid_codec(x) || x==CODEC_ID_SKAL;}
static __inline bool sup_gray(int x)              {return x!=CODEC_ID_LJPEG && x!=CODEC_ID_FFV1 && x!=CODEC_ID_SNOW && !theora_codec(x) && !wmv9_codec(x) && !raw_codec(x) && x!=CODEC_ID_SKAL && x!=CODEC_ID_DVVIDEO && !x264_codec(x);}
static __inline bool sup_globalheader(int x)      {return x==CODEC_ID_MPEG4;}
static __inline bool sup_part(int x)              {return x==CODEC_ID_MPEG4;}
static __inline bool sup_packedBitstream(int x)   {return xvid_codec(x);}
static __inline bool sup_minKeySet(int x)         {return x!=CODEC_ID_MJPEG && x!=CODEC_ID_SNOW && (!lossless_codec(x) || x==CODEC_ID_X264_LOSSLESS) && !wmv9_codec(x) && !raw_codec(x);}
static __inline bool sup_maxKeySet(int x)         {return x!=CODEC_ID_MJPEG && (!lossless_codec(x) || x==CODEC_ID_X264_LOSSLESS) && !raw_codec(x);}
static __inline bool sup_bframes(int x)           {return x==CODEC_ID_MPEG4 || x==CODEC_ID_MPEG1VIDEO || x==CODEC_ID_MPEG2VIDEO || xvid_codec(x) || x264_codec(x);}
static __inline bool sup_adaptiveBframes(int x)   {return lavc_codec(x) || x==CODEC_ID_X264;}
static __inline bool sup_closedGop(int x)         {return sup_bframes(x) && !x264_codec(x);}
static __inline bool sup_lavcme(int x)            {return lavc_codec(x) && x!=CODEC_ID_MJPEG && !lossless_codec(x);}
static __inline bool sup_quantProps(int x)        {return !lossless_codec(x) && !theora_codec(x) && !wmv9_codec(x) && !raw_codec(x) && x!=CODEC_ID_SNOW;}
static __inline bool sup_trellisQuant(int x)      {return x==CODEC_ID_MPEG4 || x==CODEC_ID_MPEG1VIDEO || x==CODEC_ID_MPEG2VIDEO || x==CODEC_ID_XVID4 || x==CODEC_ID_H263 || x==CODEC_ID_H263P || x==CODEC_ID_SKAL || x==CODEC_ID_X264;}
static __inline bool sup_masking(int x)           {return x==CODEC_ID_MPEG4 || x==CODEC_ID_H263 || x==CODEC_ID_H263P || x==CODEC_ID_MPEG1VIDEO || x==CODEC_ID_MPEG2VIDEO || xvid_codec(x) || x==CODEC_ID_SKAL || x==CODEC_ID_X264;}
static __inline bool sup_lavcOnePass(int x)       {return (lavc_codec(x) && !lossless_codec(x)) || x==CODEC_ID_X264;}
static __inline bool sup_perFrameQuant(int x)     {return !lossless_codec(x) && !wmv9_codec(x) && !raw_codec(x) && !x264_codec(x) && x!=CODEC_ID_SNOW;}
static __inline bool sup_4mv(int x)               {return x==CODEC_ID_MPEG4 || x==CODEC_ID_H263 || x==CODEC_ID_H263P || x==CODEC_ID_SNOW || x==CODEC_ID_SKAL;}
static __inline bool sup_aspect(int x)            {return x==CODEC_ID_MPEG4 || x==CODEC_ID_MPEG1VIDEO || x==CODEC_ID_MPEG2VIDEO || x==CODEC_ID_XVID4 || x==CODEC_ID_THEORA_LIB || x264_codec(x);}
static __inline bool sup_PSNR(int x)              {return (lavc_codec(x) && !lossless_codec(x)) || xvid_codec(x) || x==CODEC_ID_SKAL || x264_codec(x);}
static __inline bool sup_quantBias(int x)         {return lavc_codec(x) && !lossless_codec(x);}
static __inline bool sup_MPEGquant(int x)         {return x==CODEC_ID_MPEG4 || x==CODEC_ID_MSMPEG4V3 || x==CODEC_ID_MPEG2VIDEO || xvid_codec(x) || x==CODEC_ID_SKAL;}
static __inline bool sup_lavcQuant(int x)         {return lavc_codec(x) && sup_quantProps(x);}
static __inline bool sup_customQuantTables(int x) {return x==CODEC_ID_MPEG4 || xvid_codec(x) || x==CODEC_ID_MPEG1VIDEO || x==CODEC_ID_MPEG2VIDEO || x==CODEC_ID_SKAL || x==CODEC_ID_X264;}
static __inline bool sup_qpel(int x)              {return x==CODEC_ID_MPEG4 || x==CODEC_ID_SNOW || xvid_codec(x) || x==CODEC_ID_SKAL;}
static __inline bool sup_gmc(int x)               {return xvid_codec(x) || x==CODEC_ID_SKAL;}
static __inline bool sup_me_mv0(int x)            {return sup_lavcme(x) && x!=CODEC_ID_SNOW;}
static __inline bool sup_cbp_rd(int x)            {return x==CODEC_ID_MPEG4;}
static __inline bool sup_qns(int x)               {return lavc_codec(x) && sup_quantProps(x) && x!=CODEC_ID_MSMPEG4V3 && x!=CODEC_ID_MSMPEG4V2 && x!=CODEC_ID_MSMPEG4V1 && x!=CODEC_ID_WMV1 && x!=CODEC_ID_WMV2 && x!=CODEC_ID_MJPEG && x!=CODEC_ID_SNOW;}
static __inline bool sup_threads(int x)           {return x==CODEC_ID_MPEG1VIDEO || x==CODEC_ID_MPEG2VIDEO || x==CODEC_ID_MPEG4 || x==CODEC_ID_XVID4 || x264_codec(x);}
static __inline bool sup_threads_dec(int x)       {return x==CODEC_ID_MPEG1VIDEO || x==CODEC_ID_MPEG2VIDEO || x==CODEC_ID_H264;}
static __inline bool sup_palette(int x)           {return x==CODEC_ID_MSVIDEO1 || x==CODEC_ID_8BPS || x==CODEC_ID_QTRLE || x==CODEC_ID_TSCC || x==CODEC_ID_QPEG || x==CODEC_ID_PNG;}

#endif

#endif
