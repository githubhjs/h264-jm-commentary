/*****************************************************************************
 * x264: h264 encoder/decoder testing program.
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: x264.c,v 1.1 2004/06/03 19:24:12 fenrir Exp $
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <signal.h>
#define _GNU_SOURCE
#include <getopt.h>

#ifdef _MSC_VER
#include <io.h>     /* _setmode() */
#include <fcntl.h>  /* _O_BINARY */
#endif

#ifndef _MSC_VER
#include "config.h"
#endif

#include "common/common.h"
#include "x264.h"
#include "muxers.h"

#define DATA_MAX 3000000//数据的最大长度
uint8_t data[DATA_MAX];//实际存放数据的数组

/* Ctrl-C handler */
static int     b_ctrl_c = 0;
static int     b_exit_on_ctrl_c = 0;
static void    SigIntHandler( int a )
{
    if( b_exit_on_ctrl_c )
        exit(0);
    b_ctrl_c = 1;
}

typedef struct {
    int b_progress;		//用来控制是否显示编码进度的一个东西。取值为0,1
    int i_seek;			//表示开始从哪一帧编码
    hnd_t hin;			//Hin 指向输入yuv文件的指针	//void *在C语言里空指针是有几个特性的，他是一个一般化指针，可以指向任何一种类型，但却不能解引用，需要解引用的时候，需要进行强制转换。采用空指针的策略，应该是为了声明变量的简便和统一。
    hnd_t hout;			//Hout 指向编码过后生成的文件的指针
    FILE *qpfile;		//Qpfile 是一个指向文件类型的指针，他是文本文件，其每一行的格式是framenum frametype QP,用于强制指定某些帧或者全部帧的帧类型和QP(quant param量化参数)的值

} cli_opt_t;	/* 此结构体是记录一些与编码关系较小的设置信息的 */

/* input(输入) file(文件) operation(操作) function(函数) pointers(指针) */
int (*p_open_infile)( char *psz_filename, hnd_t *p_handle, x264_param_t *p_param );
int (*p_get_frame_total)( hnd_t handle );
int (*p_read_frame)( x264_picture_t *p_pic, hnd_t handle, int i_frame );
int (*p_close_infile)( hnd_t handle );

/* 输出文件操作的函数指针 output file operation function pointers */
static int (*p_open_outfile)( char *psz_filename, hnd_t *p_handle );
static int (*p_set_outfile_param)( hnd_t handle, x264_param_t *p_param );
static int (*p_write_nalu)( hnd_t handle, uint8_t *p_nal, int i_size );
static int (*p_set_eop)( hnd_t handle, x264_picture_t *p_picture );
static int (*p_close_outfile)( hnd_t handle );

static void Help( x264_param_t *defaults, int b_longhelp );
static int  Parse( int argc, char **argv, x264_param_t *param, cli_opt_t *opt );
static int  Encode( x264_param_t *param, cli_opt_t *opt );


/****************************************************************************
 * main: 
 ****************************************************************************/
int main( int argc, char **argv )
{
	//定义两个结构体
    x264_param_t param;
    cli_opt_t opt;	/*一点设置*/

	#ifdef _MSC_VER//stdin在STDIO.H，是系统定义的
    _setmode(_fileno(stdin), _O_BINARY);//_setmode(_fileno(stdin), _O_BINARY)功能是将stdin流(或其他文件流）从文本模式   <--切换-->   二进制模式 就是stdin流(或其他文件流）从文本模式   <--切换-->   二进制模式
    _setmode(_fileno(stdout), _O_BINARY);
	#endif

	//对编码器参数进行设定，初始化结构体对象
    x264_param_default( &param );	//(common/common.c)

    /* Parse command line (解析命令行,完成文件打开)*/
    if( Parse( argc, argv, &param, &opt ) < 0 )	/* 就是把用户通过命令行提供的参数保存到两个结构体中，未提供的参数还以x264_param_default函数设置的值为准 */
        return -1;

    /* 用函数signal注册一个信号捕捉函数 */
    signal( SIGINT/*要捕捉的信号*/, SigIntHandler/*信号捕捉函数*/ );//用函数signal注册一个信号捕捉函数，第1个参数signum表示要捕捉的信号，第2个参数是个函数指针，表示要对该信号进行捕捉的函数，该参数也可以是SIG_DEF(表示交由系统缺省处理，相当于白注册了)或SIG_IGN(表示忽略掉该信号而不做任何处理)。signal如果调用成功，返回以前该信号的处理函数的地址，否则返回 SIG_ERR。
									//sighandler_t是信号捕捉函数，由signal函数注册，注册以后，在整个进程运行过程中均有效，并且对不同的信号可以注册同一个信号捕捉函数。该函数只有一个参数，表示信号值。
	/* 开始编码*/
    return Encode( &param, &opt );//把两个参数提供给Encode，而它们已经保存上了命令行的参数,此函数在 x264.c 中定义 
									//Encode内部循环调用Encode_frame对帧编码
}

/* 取字符串的第i个字符 */
static char const *strtable_lookup( const char * const table[], int index )//static静态函数，只在定义它的文件内有效
{
    int i = 0; while( table[i] ) i++;
    return ( ( index >= 0 && index < i ) ? table[ index ] : "???" );
}

/*****************************************************************************
 * Help:
 *****************************************************************************/
static void Help( x264_param_t *defaults, int b_longhelp )
{
#define H0 printf
#define H1 if(b_longhelp) printf
    H0( "x264 core:%d%s\n"
        "Syntax: x264 [options] -o outfile infile [widthxheight]\n"
        "\n"
        "Infile can be raw YUV 4:2:0 (in which case resolution is required),\n"
        "  or YUV4MPEG 4:2:0 (*.y4m),\n"
        "  or AVI or Avisynth if compiled with AVIS support (%s).\n"
        "Outfile type is selected by filename:\n"
        " .264 -> Raw bytestream\n"
        " .mkv -> Matroska\n"
        " .mp4 -> MP4 if compiled with GPAC support (%s)\n"
        "\n"
        "Options:\n"
        "\n"
        "  -h, --help                  List the more commonly used options\n"
        "      --longhelp              List all options\n"
        "\n",
        X264_BUILD, X264_VERSION,
#ifdef AVIS_INPUT
        "yes",
#else
        "no",
#endif
#ifdef MP4_OUTPUT
        "yes"
#else
        "no"
#endif
      );
    H0( "Frame-type options:\n" );
    H0( "\n" );
    H0( "  -I, --keyint <integer>      Maximum GOP size [%d]\n", defaults->i_keyint_max );
    H1( "  -i, --min-keyint <integer>  Minimum GOP size [%d]\n", defaults->i_keyint_min );
    H1( "      --scenecut <integer>    How aggressively to insert extra I-frames [%d]\n", defaults->i_scenecut_threshold );
    H0( "  -b, --bframes <integer>     Number of B-frames between I and P [%d]\n", defaults->i_bframe );
    H1( "      --no-b-adapt            Disable adaptive B-frame decision\n" );
    H1( "      --b-bias <integer>      Influences how often B-frames are used [%d]\n", defaults->i_bframe_bias );
    H0( "      --b-pyramid             Keep some B-frames as references\n" );
    H0( "      --no-cabac              Disable CABAC\n" );
    H0( "  -r, --ref <integer>         Number of reference frames [%d]\n", defaults->i_frame_reference );
    H1( "      --nf                    Disable loop filter\n" );
    H0( "  -f, --filter <alpha:beta>   Loop filter AlphaC0 and Beta parameters [%d:%d]\n",
                                       defaults->i_deblocking_filter_alphac0, defaults->i_deblocking_filter_beta );
    H0( "\n" );
    H0( "Ratecontrol:\n" );
    H0( "\n" );
    H0( "  -q, --qp <integer>          Set QP (0=lossless) [%d]\n", defaults->rc.i_qp_constant );
    H0( "  -B, --bitrate <integer>     Set bitrate (kbit/s)\n" );
    H0( "      --crf <integer>         Quality-based VBR (nominal QP)\n" );
    H1( "      --vbv-maxrate <integer> Max local bitrate (kbit/s) [%d]\n", defaults->rc.i_vbv_max_bitrate );
    H0( "      --vbv-bufsize <integer> Enable CBR and set size of the VBV buffer (kbit) [%d]\n", defaults->rc.i_vbv_buffer_size );
    H1( "      --vbv-init <float>      Initial VBV buffer occupancy [%.1f]\n", defaults->rc.f_vbv_buffer_init );
    H1( "      --qpmin <integer>       Set min QP [%d]\n", defaults->rc.i_qp_min );
    H1( "      --qpmax <integer>       Set max QP [%d]\n", defaults->rc.i_qp_max );
    H1( "      --qpstep <integer>      Set max QP step [%d]\n", defaults->rc.i_qp_step );
    H0( "      --ratetol <float>       Allowed variance of average bitrate [%.1f]\n", defaults->rc.f_rate_tolerance );
    H0( "      --ipratio <float>       QP factor between I and P [%.2f]\n", defaults->rc.f_ip_factor );
    H0( "      --pbratio <float>       QP factor between P and B [%.2f]\n", defaults->rc.f_pb_factor );
    H1( "      --chroma-qp-offset <integer>  QP difference between chroma and luma [%d]\n", defaults->analyse.i_chroma_qp_offset );
    H0( "\n" );
    H0( "  -p, --pass <1|2|3>          Enable multipass ratecontrol\n"
        "                                  - 1: First pass, creates stats file\n"
        "                                  - 2: Last pass, does not overwrite stats file\n"
        "                                  - 3: Nth pass, overwrites stats file\n" );
    H0( "      --stats <string>        Filename for 2 pass stats [\"%s\"]\n", defaults->rc.psz_stat_out );
    H1( "      --rceq <string>         Ratecontrol equation [\"%s\"]\n", defaults->rc.psz_rc_eq );
    H0( "      --qcomp <float>         QP curve compression: 0.0 => CBR, 1.0 => CQP [%.2f]\n", defaults->rc.f_qcompress );
    H1( "      --cplxblur <float>      Reduce fluctuations in QP (before curve compression) [%.1f]\n", defaults->rc.f_complexity_blur );
    H1( "      --qblur <float>         Reduce fluctuations in QP (after curve compression) [%.1f]\n", defaults->rc.f_qblur );
    H0( "      --zones <zone0>/<zone1>/...  Tweak the bitrate of some regions of the video\n" );
    H1( "                              Each zone is of the form\n"
        "                                  <start frame>,<end frame>,<option>\n"
        "                                  where <option> is either\n"
        "                                      q=<integer> (force QP)\n"
        "                                  or  b=<float> (bitrate multiplier)\n" );
    H1( "      --qpfile <string>       Force frametypes and QPs\n" );
    H0( "\n" );
    H0( "Analysis:\n" );
    H0( "\n" );
    H0( "  -A, --analyse <string>      Partitions to consider [\"p8x8,b8x8,i8x8,i4x4\"]\n"
        "                                  - p8x8, p4x4, b8x8, i8x8, i4x4\n"
        "                                  - none, all\n"
        "                                  (p4x4 requires p8x8. i8x8 requires --8x8dct.)\n" );
    H0( "      --direct <string>       Direct MV prediction mode [\"%s\"]\n"
        "                                  - none, spatial, temporal, auto\n",
                                       strtable_lookup( x264_direct_pred_names, defaults->analyse.i_direct_mv_pred ) );
    H0( "  -w, --weightb               Weighted prediction for B-frames\n" );
    H0( "      --me <string>           Integer pixel motion estimation method [\"%s\"]\n",
                                       strtable_lookup( x264_motion_est_names, defaults->analyse.i_me_method ) );
    H1( "                                  - dia: diamond search, radius 1 (fast)\n"
        "                                  - hex: hexagonal search, radius 2\n"
        "                                  - umh: uneven multi-hexagon search\n"
        "                                  - esa: exhaustive search (slow)\n" );
    else H0( "                                  - dia, hex, umh\n" );
    H0( "      --merange <integer>     Maximum motion vector search range [%d]\n", defaults->analyse.i_me_range );
    H0( "  -m, --subme <integer>       Subpixel motion estimation and partition\n"
        "                                  decision quality: 1=fast, 7=best. [%d]\n", defaults->analyse.i_subpel_refine );
    H0( "      --b-rdo                 RD based mode decision for B-frames. Requires subme 6.\n" );
    H0( "      --mixed-refs            Decide references on a per partition basis\n" );
    H1( "      --no-chroma-me          Ignore chroma in motion estimation\n" );
    H1( "      --bime                  Jointly optimize both MVs in B-frames\n" );
    H0( "  -8, --8x8dct                Adaptive spatial transform size\n" );
    H0( "  -t, --trellis <integer>     Trellis RD quantization. Requires CABAC. [%d]\n"
        "                                  - 0: disabled\n"
        "                                  - 1: enabled only on the final encode of a MB\n"
        "                                  - 2: enabled on all mode decisions\n", defaults->analyse.i_trellis );
    H0( "      --no-fast-pskip         Disables early SKIP detection on P-frames\n" );
    H0( "      --no-dct-decimate       Disables coefficient thresholding on P-frames\n" );
    H0( "      --nr <integer>          Noise reduction [%d]\n", defaults->analyse.i_noise_reduction );
    H1( "\n" );
    H1( "      --cqm <string>          Preset quant matrices [\"flat\"]\n"
        "                                  - jvt, flat\n" );
    H0( "      --cqmfile <string>      Read custom quant matrices from a JM-compatible file\n" );
    H1( "                                  Overrides any other --cqm* options.\n" );
    H1( "      --cqm4 <list>           Set all 4x4 quant matrices\n"
        "                                  Takes a comma-separated list of 16 integers.\n" );
    H1( "      --cqm8 <list>           Set all 8x8 quant matrices\n"
        "                                  Takes a comma-separated list of 64 integers.\n" );
    H1( "      --cqm4i, --cqm4p, --cqm8i, --cqm8p\n"
        "                              Set both luma and chroma quant matrices\n" );
    H1( "      --cqm4iy, --cqm4ic, --cqm4py, --cqm4pc\n"
        "                              Set individual quant matrices\n" );
    H1( "\n" );
    H1( "Video Usability Info (Annex E):\n" );
    H1( "The VUI settings are not used by the encoder but are merely suggestions to\n" );
    H1( "the playback equipment. See doc/vui.txt for details. Use at your own risk.\n" );
    H1( "\n" );
    H1( "      --overscan <string>     Specify crop overscan setting [\"%s\"]\n"
        "                                  - undef, show, crop\n",
                                       strtable_lookup( x264_overscan_names, defaults->vui.i_overscan ) );
    H1( "      --videoformat <string>  Specify video format [\"%s\"]\n"
        "                                  - component, pal, ntsc, secam, mac, undef\n",
                                       strtable_lookup( x264_vidformat_names, defaults->vui.i_vidformat ) );
    H1( "      --fullrange <string>    Specify full range samples setting [\"%s\"]\n"
        "                                  - off, on\n",
                                       strtable_lookup( x264_fullrange_names, defaults->vui.b_fullrange ) );
    H1( "      --colorprim <string>    Specify color primaries [\"%s\"]\n"
        "                                  - undef, bt709, bt470m, bt470bg\n"
        "                                    smpte170m, smpte240m, film\n",
                                       strtable_lookup( x264_colorprim_names, defaults->vui.i_colorprim ) );
    H1( "      --transfer <string>     Specify transfer characteristics [\"%s\"]\n"
        "                                  - undef, bt709, bt470m, bt470bg, linear,\n"
        "                                    log100, log316, smpte170m, smpte240m\n",
                                       strtable_lookup( x264_transfer_names, defaults->vui.i_transfer ) );
    H1( "      --colormatrix <string>  Specify color matrix setting [\"%s\"]\n"
        "                                  - undef, bt709, fcc, bt470bg\n"
        "                                    smpte170m, smpte240m, GBR, YCgCo\n",
                                       strtable_lookup( x264_colmatrix_names, defaults->vui.i_colmatrix ) );
    H1( "      --chromaloc <integer>   Specify chroma sample location (0 to 5) [%d]\n",
                                       defaults->vui.i_chroma_loc );
    H0( "\n" );
    H0( "Input/Output:\n" );
    H0( "\n" );
    H0( "  -o, --output                Specify output file\n" );
    H0( "      --sar width:height      Specify Sample Aspect Ratio\n" );
    H0( "      --fps <float|rational>  Specify framerate\n" );
    H0( "      --seek <integer>        First frame to encode\n" );
    H0( "      --frames <integer>      Maximum number of frames to encode\n" );
    H0( "      --level <string>        Specify level (as defined by Annex A)\n" );
    H0( "\n" );
    H0( "  -v, --verbose               Print stats for each frame\n" );
    H0( "      --progress              Show a progress indicator while encoding\n" );
    H0( "      --quiet                 Quiet Mode\n" );
    H0( "      --no-psnr               Disable PSNR computation\n" );
    H0( "      --threads <integer>     Parallel encoding (uses slices)\n" );
    H0( "      --thread-input          Run Avisynth in its own thread\n" );
    H1( "      --no-asm                Disable all CPU optimizations\n" );
    H1( "      --visualize             Show MB types overlayed on the encoded video\n" );
    H1( "      --sps-id <integer>      Set SPS and PPS id numbers [%d]\n", defaults->i_sps_id );
    H1( "      --aud                   Use access unit delimiters\n" );
    H0( "\n" );
}

/*****************************************************************************
 * Parse:
 *****************************************************************************/
static int  Parse( int argc, char **argv,
                   x264_param_t *param, cli_opt_t *opt )
{
    char *psz_filename = NULL;
    x264_param_t defaults = *param;
    char *psz;
    int b_avis = 0;
    int b_y4m = 0;
    int b_thread_input = 0;

    memset( opt, 0, sizeof(cli_opt_t) );/* 内存块设置00000 */

    /* Default input file driver */
    p_open_infile = open_file_yuv;/*函数指针赋值*/
    p_get_frame_total = get_frame_total_yuv;//p_get_frame_total是一个函数指针，右侧的get_frame_total_yuv是一个函数名称；//muxers.c(82):int get_frame_total_yuv( hnd_t handle )
    p_read_frame = read_frame_yuv;/*函数指针赋值*/
    p_close_infile = close_file_yuv;/*函数指针赋值*/

    /* Default output file driver */
    p_open_outfile = open_file_bsf;/*函数指针赋值*/
    p_set_outfile_param = set_param_bsf;/*函数指针赋值*/
    p_write_nalu = write_nalu_bsf;/*函数指针赋值*/
    p_set_eop = set_eop_bsf;/*函数指针赋值*/
    p_close_outfile = close_file_bsf;/*函数指针赋值*/

    /* Parse command line options */
    for( ;; )//for循环中的"初始化"、"条件表达式"和"增量"都是选择项, 即可以缺省, 但";"不能缺省。省略了初始化, 表示不对循环控制变量赋初值。 省略了条件表达式, 则不做其它处理时便成为死循环。省略了增量, 则不对循环控制变量进行操作, 这时可在语句体中加入修改循环控制变量的语句。
    {
        int b_error = 0;
        int long_options_index = -1;

		#define OPT_FRAMES 256
		#define OPT_SEEK 257
		#define OPT_QPFILE 258
		#define OPT_THREAD_INPUT 259
		#define OPT_QUIET 260
		#define OPT_PROGRESS 261
		#define OPT_VISUALIZE 262
		#define OPT_LONGHELP 263

		/*option类型的结构体数组*/ /*struct定义的是一种类型，并不分配内存空间*/
        static struct option long_options[] =
        {
            { "help",    no_argument,       NULL, 'h' },
            { "longhelp",no_argument,       NULL, OPT_LONGHELP },
            { "version", no_argument,       NULL, 'V' },
            { "bitrate", required_argument, NULL, 'B' },
            { "bframes", required_argument, NULL, 'b' },
            { "no-b-adapt", no_argument,    NULL, 0 },
            { "b-bias",  required_argument, NULL, 0 },
            { "b-pyramid", no_argument,     NULL, 0 },
            { "min-keyint",required_argument,NULL,'i' },
            { "keyint",  required_argument, NULL, 'I' },
            { "scenecut",required_argument, NULL, 0 },
            { "nf",      no_argument,       NULL, 0 },
            { "filter",  required_argument, NULL, 'f' },
            { "no-cabac",no_argument,       NULL, 0 },
            { "qp",      required_argument, NULL, 'q' },
            { "qpmin",   required_argument, NULL, 0 },
            { "qpmax",   required_argument, NULL, 0 },
            { "qpstep",  required_argument, NULL, 0 },//argument:参数
            { "crf",     required_argument, NULL, 0 },
            { "ref",     required_argument, NULL, 'r' },
            { "no-asm",  no_argument,       NULL, 0 },
            { "sar",     required_argument, NULL, 0 },
            { "fps",     required_argument, NULL, 0 },
            { "frames",  required_argument, NULL, OPT_FRAMES },
            { "seek",    required_argument, NULL, OPT_SEEK },
            { "output",  required_argument, NULL, 'o' },
            { "analyse", required_argument, NULL, 'A' },
            { "direct",  required_argument, NULL, 0 },
            { "weightb", no_argument,       NULL, 'w' },
            { "me",      required_argument, NULL, 0 },
            { "merange", required_argument, NULL, 0 },
            { "subme",   required_argument, NULL, 'm' },
            { "b-rdo",   no_argument,       NULL, 0 },
            { "mixed-refs", no_argument,    NULL, 0 },
            { "no-chroma-me", no_argument,  NULL, 0 },
            { "bime",    no_argument,       NULL, 0 },
            { "8x8dct",  no_argument,       NULL, '8' },
            { "trellis", required_argument, NULL, 't' },
            { "no-fast-pskip", no_argument, NULL, 0 },
            { "no-dct-decimate", no_argument, NULL, 0 },
            { "level",   required_argument, NULL, 0 },
            { "ratetol", required_argument, NULL, 0 },
            { "vbv-maxrate", required_argument, NULL, 0 },
            { "vbv-bufsize", required_argument, NULL, 0 },
            { "vbv-init", required_argument,NULL,  0 },
            { "ipratio", required_argument, NULL, 0 },
            { "pbratio", required_argument, NULL, 0 },
            { "chroma-qp-offset", required_argument, NULL, 0 },
            { "pass",    required_argument, NULL, 'p' },
            { "stats",   required_argument, NULL, 0 },
            { "rceq",    required_argument, NULL, 0 },
            { "qcomp",   required_argument, NULL, 0 },
            { "qblur",   required_argument, NULL, 0 },
            { "cplxblur",required_argument, NULL, 0 },
            { "zones",   required_argument, NULL, 0 },
            { "qpfile",  required_argument, NULL, OPT_QPFILE },
            { "threads", required_argument, NULL, 0 },
            { "thread-input", no_argument,  NULL, OPT_THREAD_INPUT },
            { "no-psnr", no_argument,       NULL, 0 },
            { "quiet",   no_argument,       NULL, OPT_QUIET },
            { "verbose", no_argument,       NULL, 'v' },
            { "progress",no_argument,       NULL, OPT_PROGRESS },
            { "visualize",no_argument,      NULL, OPT_VISUALIZE },
            { "sps-id",  required_argument, NULL, 0 },
            { "aud",     no_argument,       NULL, 0 },
            { "nr",      required_argument, NULL, 0 },
            { "cqm",     required_argument, NULL, 0 },
            { "cqmfile", required_argument, NULL, 0 },
            { "cqm4",    required_argument, NULL, 0 },
            { "cqm4i",   required_argument, NULL, 0 },
            { "cqm4iy",  required_argument, NULL, 0 },
            { "cqm4ic",  required_argument, NULL, 0 },
            { "cqm4p",   required_argument, NULL, 0 },
            { "cqm4py",  required_argument, NULL, 0 },
            { "cqm4pc",  required_argument, NULL, 0 },
            { "cqm8",    required_argument, NULL, 0 },
            { "cqm8i",   required_argument, NULL, 0 },
            { "cqm8p",   required_argument, NULL, 0 },
            { "overscan", required_argument, NULL, 0 },
            { "videoformat", required_argument, NULL, 0 },
            { "fullrange", required_argument, NULL, 0 },
            { "colorprim", required_argument, NULL, 0 },
            { "transfer", required_argument, NULL, 0 },
            { "colormatrix", required_argument, NULL, 0 },
            { "chromaloc", required_argument, NULL, 0 },
            {0, 0, 0, 0}
        };

        int c = getopt_long( argc, argv, "8A:B:b:f:hI:i:m:o:p:q:r:t:Vvw",
                             long_options, &long_options_index);//解析入口地址的向量，最后c 得到的是 运行参数（“-o test.264 foreman.yuv  352x288”）中前面“-o”中“o”的ASCII值 即 c = 111 。可通过VC Debug查看。 getopt_long() 定义在getopt.c中。其中用到 getopt_internal(nargc, nargv, options)也定义在extras/getopt.c中，解析入口地址向量。


        if( c == -1 )//
        {
            break;//退出for
        }

        switch( c )
        {
            case 'h':
                Help( &defaults, 0 );//显示帮助信息
                exit(0);//exit函数中的实参是返回给操作系统，表示程序是成功运行结束还是失败运行结束。对于程序本身的使用没有什么太实际的差别。习惯上，一般使用正常结束程序exit(0)。exit(非0）：非正常结束程序运行
            case OPT_LONGHELP:
                Help( &defaults, 1 );//显示帮助信息
                exit(0);//STDLIB.H里声明
            case 'V':
				#ifdef X264_POINTVER
					printf( "x264 "X264_POINTVER"\n" );//产生格式化输出的函数(定义在 stdio.h 中)。
				#else
					printf( "x264 0.%d.X\n", X264_BUILD );
				#endif
                exit(0);
            case OPT_FRAMES://256	//--frames <整数> 最大编码帧数
                param->i_frame_total = atoi( optarg );//atoi(const char *);把字符串转换成整型数 函数说明: 参数nptr字符串，如果第一个非空格字符不存在或者不是数字也不是正负号则返回零，否则开始做类型转换，之后检测到非数字或结束符 \0 时停止转换，返回整型数。
                break;
            case OPT_SEEK://命令行：--seek <整数> 设定起始帧  //

                opt->i_seek = atoi( optarg );
                break;
            case 'o'://对运行参数（“-o test.264 foreman.yuv  352x288”）由c = 111 ,程序跳转到case 'o'，执行p_open_outfile ( optarg, &opt->hout )，即进入函数open_file_bsf（），功能为以二进制写的方式打开输出文件test.264，函数在nuxers.c中
				//mp4文件
                if( !strncasecmp(optarg + strlen(optarg) - 4, ".mp4", 4) )
                {
					#ifdef MP4_OUTPUT
						p_open_outfile = open_file_mp4;//muxers.c
						p_write_nalu = write_nalu_mp4;
						p_set_outfile_param = set_param_mp4;
						p_set_eop = set_eop_mp4;
						p_close_outfile = close_file_mp4;
					#else
						fprintf( stderr, "x264 [error]: not compiled with MP4 output support\n" );
						return -1;
					#endif
                }
				//MKV文件
                else if( !strncasecmp(optarg + strlen(optarg) - 4, ".mkv", 4) )//int strncasecmp(const char *s1, const char *s2, size_t n) 用来比较参数s1和s2字符串前n个字符，比较时会自动忽略大小写的差异, 若参数s1和s2字符串相同则返回0; s1若大于s2则返回大于0的值; s1若小于s2则返回小于0的值
                {	
                    p_open_outfile = open_file_mkv;//muxers.c
                    p_write_nalu = write_nalu_mkv;
                    p_set_outfile_param = set_param_mkv;
                    p_set_eop = set_eop_mkv;
                    p_close_outfile = close_file_mkv;
                }
                if( !strcmp(optarg, "-") )
                    opt->hout = stdout;
                else if( p_open_outfile( optarg, &opt->hout ) )
                {
                    fprintf( stderr, "x264 [error]: can't open output file `%s'\n", optarg );
                    return -1;
                }
                break;
            case OPT_QPFILE:
                opt->qpfile = fopen( optarg, "r" );
                if( !opt->qpfile )
                {
                    fprintf( stderr, "x264 [error]: can't open `%s'\n", optarg );
                    return -1;
                }
                param->i_scenecut_threshold = -1;
                param->b_bframe_adaptive = 0;
                break;
            case OPT_THREAD_INPUT:
                b_thread_input = 1;
                break;
            case OPT_QUIET:
                param->i_log_level = X264_LOG_NONE;
                param->analyse.b_psnr = 0;
                break;
            case 'v':
                param->i_log_level = X264_LOG_DEBUG;
                break;
            case OPT_PROGRESS:
                opt->b_progress = 1;
                break;
            case OPT_VISUALIZE:
				#ifdef VISUALIZE
					param->b_visualize = 1;
					b_exit_on_ctrl_c = 1;
				#else
					fprintf( stderr, "x264 [warning]: not compiled with visualization support\n" );
				#endif
                break;
            default:
            {
                int i;
                if( long_options_index < 0 )
                {
                    for( i = 0; long_options[i].name; i++ )
                        if( long_options[i].val == c )
                        {
                            long_options_index = i;
                            break;
                        }
                    if( long_options_index < 0 )
                    {
                        /* getopt_long already printed an error message */
                        return -1;
                    }
                }

                b_error |= x264_param_parse( param, long_options[long_options_index].name, optarg ? optarg : "true" );
            }
        }

        if( b_error )
        {
            const char *name = long_options_index > 0 ? long_options[long_options_index].name : argv[optind-2];
            fprintf( stderr, "x264 [error]: invalid argument: %s = %s\n", name, optarg );
            return -1;
        }
    }

    /* Get the file name */
    if( optind > argc - 1 || !opt->hout )
    {
        fprintf( stderr, "x264 [error]: No %s file. Run x264 --help for a list of options.\n",
                 optind > argc - 1 ? "input" : "output" );
        return -1;
    }
    psz_filename = argv[optind++];

    /* check demuxer type (muxer是合并将视频文件、音频文件和字幕文件合并为某一个视频格式。如，可将a.avi, a.mp3, a.srt用muxer合并为mkv格式的视频文件。demuxer是拆分这些文件的。)*/
    psz = psz_filename + strlen(psz_filename) - 1;
    while( psz > psz_filename && *psz != '.' )
        psz--;
    if( !strncasecmp( psz, ".avi", 4 ) || !strncasecmp( psz, ".avs", 4 ) )
        b_avis = 1;
    if( !strncasecmp( psz, ".y4m", 4 ) )
        b_y4m = 1;

    if( !(b_avis || b_y4m) ) // raw yuv
    {
        if( optind > argc - 1 )
        {
            /* try to parse the file name */
            for( psz = psz_filename; *psz; psz++ )
            {
                if( *psz >= '0' && *psz <= '9'
                    && sscanf( psz, "%ux%u", &param->i_width, &param->i_height ) == 2 )
                {
                    if( param->i_log_level >= X264_LOG_INFO )
                        fprintf( stderr, "x264 [info]: file name gives %dx%d\n", param->i_width, param->i_height );
                    break;
                }
            }
        }
        else
        {
            sscanf( argv[optind++], "%ux%u", &param->i_width, &param->i_height );
        }
    }
        
    if( !(b_avis || b_y4m) && ( !param->i_width || !param->i_height ) )
    {
        fprintf( stderr, "x264 [error]: Rawyuv input requires a resolution.\n" );
        return -1;
    }

    /* open the input */
    {
        if( b_avis )
        {
#ifdef AVIS_INPUT
            p_open_infile = open_file_avis;
            p_get_frame_total = get_frame_total_avis;
            p_read_frame = read_frame_avis;
            p_close_infile = close_file_avis;
#else
            fprintf( stderr, "x264 [error]: not compiled with AVIS input support\n" );
            return -1;
#endif
        }
        if ( b_y4m )
        {
            p_open_infile = open_file_y4m;
            p_get_frame_total = get_frame_total_y4m;
            p_read_frame = read_frame_y4m;
            p_close_infile = close_file_y4m;
        }

        if( p_open_infile( psz_filename, &opt->hin, param ) )
        {
            fprintf( stderr, "x264 [error]: could not open input file '%s'\n", psz_filename );
            return -1;
        }
    }

#ifdef HAVE_PTHREAD
    if( b_thread_input || param->i_threads > 1 )
    {
        if( open_file_thread( NULL, &opt->hin, param ) )
        {
            fprintf( stderr, "x264 [warning]: threaded input failed\n" );
        }
        else
        {
            p_open_infile = open_file_thread;
            p_get_frame_total = get_frame_total_thread;
            p_read_frame = read_frame_thread;
            p_close_infile = close_file_thread;
        }
    }
#endif

    return 0;
}

static void parse_qpfile( cli_opt_t *opt, x264_picture_t *pic, int i_frame )
{
    int num = -1, qp;
    char type;
    while( num < i_frame )
    {
        int ret = fscanf( opt->qpfile, "%d %c %d\n", &num, &type, &qp );
        if( num < i_frame )
            continue;
        pic->i_qpplus1 = qp+1;
        if     ( type == 'I' ) pic->i_type = X264_TYPE_IDR;
        else if( type == 'i' ) pic->i_type = X264_TYPE_I;
        else if( type == 'P' ) pic->i_type = X264_TYPE_P;
        else if( type == 'B' ) pic->i_type = X264_TYPE_BREF;
        else if( type == 'b' ) pic->i_type = X264_TYPE_B;
        else ret = 0;
        if( ret != 3 || qp < 0 || qp > 51 || num > i_frame )
        {
            fprintf( stderr, "x264 [error]: can't parse qpfile for frame %d\n", i_frame );
            fclose( opt->qpfile );
            opt->qpfile = NULL;
            pic->i_type = X264_TYPE_AUTO;
            pic->i_qpplus1 = 0;
            break;
        }
    }
}

/*****************************************************************************
 * Decode:
 *****************************************************************************/

/*
Encode_frame
这个函数将输入每帧的YUV数据，然后输出nal包。编码码流的具体工作交由API x264_encoder_encode
*/

static int  Encode_frame( x264_t *h, hnd_t hout, x264_picture_t *pic )
{
    x264_picture_t pic_out;
    x264_nal_t *nal;
    int i_nal, i;
    int i_file = 0;

    if( x264_encoder_encode( h, &nal, &i_nal, pic, &pic_out ) < 0 )//264_encoder_encode每次会以参数送入一帧待编码的帧pic_in
    {
        fprintf( stderr, "x264 [error]: x264_encoder_encode failed\n" );
    }

    for( i = 0; i < i_nal; i++ )
    {
        int i_size;
        int i_data;

        i_data = DATA_MAX;//右边这个是3百万，本文件最上面给了
		//网络打包编码
        if( ( i_size = x264_nal_encode( data, &i_data, 1, &nal[i] ) ) > 0 )//第一个参数是一个全局变量，是个数组
        {
			//把网络包写入到输出文件中去//这儿换成通过套接字发送
            i_file += p_write_nalu( hout, data, i_size );//这个是函数指针的形式，//p_write_nalu = write_nalu_mp4;p_write_nalu = write_nalu_mkv;p_write_nalu = write_nalu_bsf;
        }
        else if( i_size < 0 )
        {
            fprintf( stderr, "x264 [error]: need to increase buffer size (size=%d)\n", -i_size );
        }
    }
    if (i_nal)
        p_set_eop( hout, &pic_out );

    return i_file;
}

/*****************************************************************************
 * Encode: 编码
 * 这个函数执行完，全部编码工作就结束了
 *****************************************************************************/
static int  Encode( x264_param_t *param, cli_opt_t *opt )
{
    x264_t *h;	//定义一个结构体的指针
    x264_picture_t pic;//这是一个结构体的对象，所以已经分配了内存空间，不用再动态分配了，本结构体里有一个数组，是用来存图像的，但它只是存的指针，所以后面还得分配实际存图像的内存，然后用这个数组记下这块内存的地址

    int     i_frame, i_frame_total;//x264_param_t结构的字段里也有个i_frame_total字段，默认值为0
    int64_t i_start, i_end;	//从i_start开始编码，直到i_end结束
    int64_t i_file;//好像是计数编码过的长度的
    int     i_frame_size;//后面有一句，是用编码一帧的返回值赋值的
    int     i_progress;//记录进度

    i_frame_total = p_get_frame_total( opt->hin );	/* 得到输入文件的总帧数 由于p_get_frame_total = get_frame_total_yuv（见Parse（）函数），所以调用函数int get_frame_total_yuv( hnd_t handle )，在文件muxers.c中*/
    i_frame_total -= opt->i_seek;	/* 要编码的总帧数= 源文件总帧数-开始帧(不一定从第1帧开始编码) */
    if( ( i_frame_total == 0 || param->i_frame_total < i_frame_total ) && param->i_frame_total > 0 )	/* 对这个待编码的总帧数进行一些判断 */
        i_frame_total = param->i_frame_total;	/* 保证i_frame_total是有效的 */
    param->i_frame_total = i_frame_total;	/* 获取要求编码的帧数param->i_frame_total 根据命令行输入参数计算得到的总帧数，存进结构体的字段*/

    if( ( h = x264_encoder_open( param ) ) == NULL )	/* 在文件encoder.c中，创建编码器，并对不正确的264_t结构体(h的类型是264_t * )参数进行修改，并对各结构体参数、编码、预测等需要的参数进行初始化 */
    {
        fprintf( stderr, "x264 [error]: x264_encoder_open failed\n" );	/* 打印错误信息 */
        p_close_infile( opt->hin );	/* 关闭输入文件 */
        p_close_outfile( opt->hout );	/* 关闭输出文件 */
        return -1;//程序结束
    }

    if( p_set_outfile_param( opt->hout, param ) )	/* 设置输出文件格式 */
    {
        fprintf( stderr, "x264 [error]: can't set outfile param\n" );//输出：不能设置输出文件参数
        p_close_infile( opt->hin );	/* 关闭YUV文件 关闭输入文件 */
        p_close_outfile( opt->hout );	/* 关闭码流文件 关闭输出文件 */
        return -1;
    }

    /* 创建一帧图像空间 Create a new pic (这个图像分配好的内存首地址等填充到了pic结构)*/
    x264_picture_alloc( &pic, X264_CSP_I420, param->i_width, param->i_height );	/* #define X264_CSP_I420  0x0001  yuv 4:2:0 planar */ 

    i_start = x264_mdate();	/* 返回当前日期，开始和结束时间求差，可以计算总的耗时和每秒的效率 return the current date in microsecond */

    /* (循环编码每一帧) Encode frames */
    for( i_frame = 0, i_file = 0, i_progress = 0; b_ctrl_c == 0 && (i_frame < i_frame_total || i_frame_total == 0); )
    {
        if( p_read_frame( &pic, opt->hin, i_frame + opt->i_seek ) )/*读取一帧，并把这帧设为prev; 读入第i_frame帧，从起始帧算，而不是视频的第0帧 */
            break;

        pic.i_pts = (int64_t)i_frame * param->i_fps_den; //字段：I_pts ：program time stamp 程序时间戳，指示这幅画面编码的时间戳
														//帧率*帧数，好像也能算出时间戳来
        if( opt->qpfile )//这儿可能是一个什么设置，或许是存在一个配置文件
            parse_qpfile( opt, &pic, i_frame + opt->i_seek );//parse_qpfile() 为从指定的文件中读入qp的值留下的接口，qpfile为文件的首地址
        else//没有配置的话，就全自动了，呵呵
        {
            /* 未强制任何编码参数Do not force any parameters */
            pic.i_type = X264_TYPE_AUTO;
            pic.i_qpplus1 = 0;//I_qpplus1 ：此参数减1代表当前画面的量化参数值
        }

		/*编码一帧图像，h编码器句柄，hout码流文件，pic预编码帧图像，实际就是上面从视频文件中读到的东西*/
        i_file += Encode_frame( h, opt->hout, &pic );//进入核心编码层

        i_frame++;//已编码的帧数统计(递增1)

        /* 更新数据行，用于显示整个编码过程的进度 update status line (up to 1000 times per input file) */
        if( opt->b_progress && param->i_log_level < X264_LOG_DEBUG && 
            ( i_frame_total ? i_frame * 1000 / i_frame_total > i_progress
                            : i_frame % 10 == 0 ) )
        {
            int64_t i_elapsed = x264_mdate() - i_start;//elapsed:(时间)过去;开始至现在的时间差
            double fps = i_elapsed > 0 ? i_frame * 1000000. / i_elapsed : 0;
            if( i_frame_total )
            {
                int eta = i_elapsed * (i_frame_total - i_frame) / ((int64_t)i_frame * 1000000);//已用的时间*(总共要编码的帧数-已编码的帧数=待编码的帧数)/(已编码的帧数*1000000)，看上去就象是预计剩余时间，也就是估计的倒计时，根据过去的效率动态的估计这个时间
                i_progress = i_frame * 1000 / i_frame_total;
                fprintf( stderr, "已编码的帧数:: %d/%d (%.1f%%), %.2f fps, eta %d:%02d:%02d  \r",	/* 状态提示 FPS（Frames Per Second）：每秒传输帧数,每秒钟填充图像的帧数（帧/秒)*/
                         i_frame, i_frame_total, (float)i_progress / 10, fps,
                         eta/3600, (eta/60)%60, eta%60 );//ETA是Estimated Time of Arrival的英文缩写，指 预计到达时间
            }
            else
                fprintf( stderr, "已编码的帧数: %d, %.2f fps   \r", i_frame, fps );	/* 状态提示,共编码了... */
            fflush( stderr ); // needed in windows
       }
    }

    /* Flush delayed(延迟) B-frames */
    do {
        i_file +=
        i_frame_size = Encode_frame( h, opt->hout, NULL );//上面调用的是i_file += Encode_frame( h, opt->hout, &pic );
    } while( i_frame_size );

    i_end = x264_mdate();		//返回当前时间return the current date in microsecond//前面有句： i_start = x264_mdate();
    x264_picture_clean( &pic );	//
    x264_encoder_close( h );
    fprintf( stderr, "\n" );

    if( b_ctrl_c )
        fprintf( stderr, "aborted at input frame %d\n", opt->i_seek + i_frame );

    p_close_infile( opt->hin );		//关闭输入文件
    p_close_outfile( opt->hout );	//关闭输出文件

    if( i_frame > 0 )
    {
        double fps = (double)i_frame * (double)1000000 /
                     (double)( i_end - i_start );/* 每秒多少帧 */

        fprintf( stderr, "encoded %d frames, %.2f fps, %.2f kb/s\n", i_frame, fps,
                 (double) i_file * 8 * param->i_fps_num /
                 ( (double) param->i_fps_den * i_frame * 1000 ) );	/* 在窗口显示encoded 26 frames等进度提示 */
    }

    return 0;
}
/*直接传输像素值是IPCM模式
*/