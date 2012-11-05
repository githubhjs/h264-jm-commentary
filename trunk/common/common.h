/*****************************************************************************
 * common.h: h264 encoder
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: common.h,v 1.1 2004/06/03 19:27:06 fenrir Exp $
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

#ifndef _COMMON_H
#define _COMMON_H 1

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#include <inttypes.h>
#endif
#include <stdarg.h>
#include <stdlib.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#define X264_VERSION "" // no configure script for msvc
#endif

/* threads */
#ifdef __WIN32__
#include <windows.h>
#define pthread_t               HANDLE
#define pthread_create(t,u,f,d) *(t)=CreateThread(NULL,0,f,d,0,NULL)
#define pthread_join(t,s)       { WaitForSingleObject(t,INFINITE); \
                                  CloseHandle(t); } 
#define HAVE_PTHREAD 1

#elif defined(SYS_BEOS)
#include <kernel/OS.h>
#define pthread_t               thread_id
#define pthread_create(t,u,f,d) { *(t)=spawn_thread(f,"",10,d); \
                                  resume_thread(*(t)); }
#define pthread_join(t,s)       { long tmp; \
                                  wait_for_thread(t,(s)?(long*)(s):&tmp); }
#define HAVE_PTHREAD 1

#elif defined(HAVE_PTHREAD)
#include <pthread.h>
#endif

/****************************************************************************
 * Macros
 ****************************************************************************/
#define X264_MIN(a,b) ( (a)<(b) ? (a) : (b) )
#define X264_MAX(a,b) ( (a)>(b) ? (a) : (b) )
#define X264_MIN3(a,b,c) X264_MIN((a),X264_MIN((b),(c)))
#define X264_MAX3(a,b,c) X264_MAX((a),X264_MAX((b),(c)))
#define X264_MIN4(a,b,c,d) X264_MIN((a),X264_MIN3((b),(c),(d)))
#define X264_MAX4(a,b,c,d) X264_MAX((a),X264_MAX3((b),(c),(d)))
#define XCHG(type,a,b) { type t = a; a = b; b = t; }
#define FIX8(f) ((int)(f*(1<<8)+.5))//fix:取整数，修理

#if defined(__GNUC__) && (__GNUC__ > 3 || __GNUC__ == 3 && __GNUC_MINOR__ > 0)
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

#define CHECKED_MALLOC( var, size )\
{\
    var = x264_malloc( size );\
    if( !var )\
    {\
        x264_log( h, X264_LOG_ERROR, "malloc failed\n" );\
        goto fail;\
    }\
}

#define X264_BFRAME_MAX 16
#define X264_SLICE_MAX 4
#define X264_NAL_MAX (4 + X264_SLICE_MAX)

/****************************************************************************
 * Includes
 ****************************************************************************/
#include "x264.h"
#include "bs.h"
#include "set.h"
#include "predict.h"
#include "pixel.h"
#include "mc.h"
#include "frame.h"
#include "dct.h"
#include "cabac.h"
#include "csp.h"
#include "quant.h"

/****************************************************************************
 * Generals functions
 ****************************************************************************/
/* x264_malloc : will do or emulate(模仿) a memalign(内存对齐?)
 * XXX you HAVE TO use x264_free for buffer allocated
 * with x264_malloc
 */
void *x264_malloc( int );
void *x264_realloc( void *p, int i_size );
void  x264_free( void * );

/* x264_slurp_file: malloc space for the whole file and read it */
char *x264_slurp_file( const char *filename );

/* mdate: return the current date in microsecond( 一百万分之一秒,微秒) */
int64_t x264_mdate( void );

/* x264_param2string: return a (malloced) string containing most of
 * the encoding options */
char *x264_param2string( x264_param_t *p, int b_res );

/* log */
void x264_log( x264_t *h, int i_level, const char *psz_fmt, ... );

void x264_reduce_fraction( int *n, int *d );

static inline int x264_clip3( int v, int i_min, int i_max )
{
    return ( (v < i_min) ? i_min : (v > i_max) ? i_max : v );
}

static inline float x264_clip3f( float v, float f_min, float f_max )
{
    return ( (v < f_min) ? f_min : (v > f_max) ? f_max : v );
}

static inline int x264_median( int a, int b, int c )
{
    int min = a, max =a;
    if( b < min )
        min = b;
    else
        max = b;    /* no need to do 'b > max' (more consuming than always doing affectation) */

    if( c < min )
        min = c;
    else if( c > max )
        max = c;

    return a + b + c - min - max;
}


/****************************************************************************
 *片_类型//毕厚杰书，第164页
 ****************************************************************************/
enum slice_type_e
{
    SLICE_TYPE_P  = 0,//P片
    SLICE_TYPE_B  = 1,//B片
    SLICE_TYPE_I  = 2,//I片
    SLICE_TYPE_SP = 3,//SP片
    SLICE_TYPE_SI = 4//SI片
};

static const char slice_type_to_char[] = { 'P', 'B', 'I', 'S', 'S' };

typedef struct
{
    x264_sps_t *sps;
    x264_pps_t *pps;

    int i_type;		//[毕厚杰：Page 164] 相当于slice_type，指明片的类型；0123456789对应P、B、I、SP、SI 
    int i_first_mb;	//[毕厚杰：Page 164] 相当于 first_mb_in_slice 片中的第一个宏块的地址，片通过这个句法元素来标定它自己的地址。要注意的是，在帧场自适应模式下，宏块都是成对出现的，这时本句法元素表示的是第几个宏块对，对应的第一个宏块的真实地址应该是2*---
    int i_last_mb;	//最后一个宏块

    int i_pps_id;	//[毕厚杰：Page 164] 相当于 pic_parameter_set_id ; 图像参数集的索引号，取值范围[0,255]

    int i_frame_num;	//[毕厚杰：Page 164] 相当于 frame_num ; 只有当这个图像是参考帧时，它所携带的这个句法元素在解码时才有意义，
						//[毕厚杰：Page 164] 相当于 frame_num ; 每个参考帧都有一个依次连续的frame_num作为它们的标识，这指明了各图像的解码顺序。frame_num所能达到的最大值由前文序列参数集中的句法元素推出。毕P164
    int b_field_pic;	//[毕厚杰：Page 165] 相当于 field_pic_flag; 这是在片层标识图像编码模式的唯一一个句法元素。所谓的编码模式是指的帧编码、场编码、帧场自适应编码。当这个句法元素取值为1时属于场编码，0时为非场编码。
    int b_bottom_field;	//[毕厚杰：Page 166] 相当于 bottom_field_flag ;  =1时表示当前的图像是属于底场，=0时表示当前图像属于顶场

    int i_idr_pic_id;   /* -1 if nal_type != 5 */ 
						//[毕厚杰：Page 164] 相当于idr_pic_id ;IDR图像的标识。不同的IDR图像有不同的idr_pic_id值。值得注意的是，IDR图像不等价于I图像，只有在作为IDR图像的I帧才有这个句法元素。在场模式下，IDR帧的两个场有相同的idr_pic_id值。idr_pic_id的取值范围是[0,65536]，和frame_num类似，当它的值超过这个范围时，它会以循环的方式重新开始计数。

    int i_poc_lsb;		//[毕厚杰：Page 166] 相当于 pic_order_cnt_lsb，在POC的第一种算法中本句法元素来计算POC值，在POC的第一种算法中是显式地传递POC的值，而其他两种算法是通过frame_num来映射POC的值，注意这个句法元素的读取函数是u(v)，这个v的值是序列参数集的句法元素--+4个比特得到的。取值范围是...
    int i_delta_poc_bottom;	//[毕厚杰：Page 166] 相当于 delta_pic_order_cnt_bottom，如果是在场模式下，场对中的两个场各自被构造为一个图像，它们有各自的POC算法来分别计算两个场的POC值，也就是一个场对拥有一对POC值；而在帧模式或帧场自适应模式下，一个图像只能根据片头的句法元素计算出一个POC值。根据H.264的规定，在序列中可能出现场的情况，即frame_mbs_only_flag不为1时，每个帧或帧场自适应的图像在解码完后必须分解为两个场，以供后续图像中的场作为参考图像。所以当时frame_mb_only_flag不为1时，帧或帧场自适应

    int i_delta_poc[2]; //[毕厚杰：Page 167] 相当于 delta_pic_order_cnt[0] , delta_pic_order_cnt[1]
    int i_redundant_pic_cnt;	//[毕厚杰：Page 167] 相当于 redundant_pic_cnt;冗余片的id号，取值范围[0,127]

    int b_direct_spatial_mv_pred;	//[毕厚杰：Page 167] 相当于 direct_spatial_mv_pred；指出在B图像的直接预测模式下，用时间预测还是用空间预测，1表示空间预测，0表示时间预测。直接预测在书上94页

    int b_num_ref_idx_override;		//[毕厚杰：Page 167] 相当于 num_ref_idx_active_override_flag，在图像参数集中，有两个句法元素指定当前参考帧队列中实际可用的参考帧的数目。在片头可以重载这对句法元素，以给某特定图像更大的灵活度。这个句法元素就是指明片头是否会重载，如果该句法元素等于1，下面会出现新的num_ref_idx_10_active_minus1和...11...
    int i_num_ref_idx_l0_active;	//[毕厚杰：Page 167] 相当于 num_ref_idx_10_active_minus1
    int i_num_ref_idx_l1_active;	//[毕厚杰：Page 167] 相当于 num_ref_idx_11_active_minus1

    int b_ref_pic_list_reordering_l0;//短期参考图像列表重排序?
    int b_ref_pic_list_reordering_l1;//长期参考图像列表重排序?
    struct {
        int idc;
        int arg;
    } ref_pic_list_order[2][16];	 

    int i_cabac_init_idc;//[毕厚杰：Page 167] 相当于 cabac_init_idc，给出cabac初始化时表格的选择，取值范围为[0,2]

    int i_qp;//
    int i_qp_delta;//[毕厚杰：Page 167] 相当于 slice_qp_delta，指出用于当前片的所有宏块的量化参数的初始值
    int b_sp_for_swidth;////[毕厚杰：Page 167] 相当于 sp_for_switch_flag
    int i_qs_delta;//[毕厚杰：Page 167] 相当于 slice_qs_delta，与slice_qp_delta语义相似，用在SI和SP中，由下式计算：...

    /* deblocking filter */
    int i_disable_deblocking_filter_idc;//[毕厚杰：Page 167] 相当于 disable_deblocking_filter_idc，H.264规定一套可以在解码器端独立地计算图像中各边界的滤波强度进行
    int i_alpha_c0_offset;
    int i_beta_offset;

} x264_slice_header_t;	//x264_片_头_
//x264这块的变量命名，与毕厚杰书上说的变量有一些出入，x264加了类型符号，比如i_、b_，另外省掉了_flag，进行了缩写，如_poc_，实际是pic_order_cnt_lsb，_pps_，实际为pic_parameter_set_id
/* From ffmpeg
 */
#define X264_SCAN8_SIZE (6*8)	//
#define X264_SCAN8_0 (4+1*8)	//

/*
这是一个坐标变换用的查找表,是个数组,将0--23变换为一个8x8矩阵中的4x4 和2个2x2 的块扫描
*/
static const int x264_scan8[16+2*4] =	//
{
    /* Luma亮度 */
    4+1*8, 5+1*8, 4+2*8, 5+2*8,
    6+1*8, 7+1*8, 6+2*8, 7+2*8,
    4+3*8, 5+3*8, 4+4*8, 5+4*8,
    6+3*8, 7+3*8, 6+4*8, 7+4*8,

    /* Cb */
    1+1*8, 2+1*8,
    1+2*8, 2+2*8,

    /* Cr */
    1+4*8, 2+4*8,
    1+5*8, 2+5*8,
};
/*
变换矩阵如下,先luma亮度,然后chroma(色度) b chroma(色度) r,都是从一个2x2小块开始,raster(光栅) scan
其中L块先第一44块raster scane 再第二44块 scane
每一个块的左边和上边是空出来的,用来存放left和top mb的 4x4小块的intra pre mode
*/

/*
   0 1 2 3 4 5 6 7
 0
 1   B B   L L L L
 2   B B   L L L L
 3         L L L L
 4   R R   L L L L
 5   R R
*/

typedef struct x264_ratecontrol_t   x264_ratecontrol_t;//ratecontrol.c(85):struct x264_ratecontrol_t
typedef struct x264_vlc_table_t     x264_vlc_table_t;

//x264_t结构体维护着CODEC的诸多重要信息
struct x264_t
{
    /* encoder parameters ( 编码器参数 )*/
    x264_param_t    param;

    x264_t *thread[X264_SLICE_MAX];

    /* bitstream output ( 字节流输出 ) */
    struct
    {
        int         i_nal;
        x264_nal_t  nal[X264_NAL_MAX];
        int         i_bitstream;    /* size of p_bitstream */
        uint8_t     *p_bitstream;   /* will hold data for all nal */
        bs_t        bs;
    } out;

    /* frame number/poc ( 帧序号 )*/
    int             i_frame;

    int             i_frame_offset; /* decoding only */
    int             i_frame_num;    /* decoding only */
    int             i_poc_msb;      /* decoding only */
    int             i_poc_lsb;      /* decoding only */
    int             i_poc;          /* decoding only */

    int             i_thread_num;   /* threads only */
    int             i_nal_type;     /* threads only */
    int             i_nal_ref_idc;  /* threads only */

    /* We use only one SPS(序列参数集) and one PPS(图像参数集) */
    x264_sps_t      sps_array[1];
    x264_sps_t      *sps;
    x264_pps_t      pps_array[1];
    x264_pps_t      *pps;
    int             i_idr_pic_id;

    int             (*dequant4_mf[4])[4][4]; /* [4][6][4][4] */
    int             (*dequant8_mf[2])[8][8]; /* [2][6][8][8] */
    int             (*quant4_mf[4])[4][4];   /* [4][6][4][4] */
    int             (*quant8_mf[2])[8][8];   /* [2][6][8][8] */
    int             (*unquant4_mf[4])[16];   /* [4][52][16] */
    int             (*unquant8_mf[2])[64];   /* [2][52][64] */

    uint32_t        nr_residual_sum[2][64];
    uint32_t        nr_offset[2][64];
    uint32_t        nr_count[2];

    /* Slice header (片头部) */
    x264_slice_header_t sh;

    /* cabac(适应性二元算术编码) context */
    x264_cabac_t    cabac;

    struct
    {
        /* Frames to be encoded (whose types have been decided(明确的) ) */
        x264_frame_t *current[X264_BFRAME_MAX+3];//current是已经准备就绪可以编码的帧，其类型已经确定
        /* Temporary(临时的) buffer (frames types not yet decided) */
        x264_frame_t *next[X264_BFRAME_MAX+3];//next是尚未确定类型的帧
        /* 未使用的帧Unused frames */
        x264_frame_t *unused[X264_BFRAME_MAX+3];//unused用于回收不使用的frame结构体以备今后再次使用
        /* For adaptive(适应的) B decision(决策) */
        x264_frame_t *last_nonb;

        /* frames used for reference +1 for decoding + sentinels (发射器,标记) */
        x264_frame_t *reference[16+2+1+2];

        int i_last_idr; /* Frame number of the last IDR (立即刷新图像) */

        int i_input;    //frames结构体中i_input指示当前输入的帧的（播放顺序）序号。/* Number of input frames already accepted */

        int i_max_dpb;  /* Number of frames allocated (分配) in the decoded picture buffer */
        int i_max_ref0;
        int i_max_ref1;
        int i_delay;    //i_delay设置为由B帧个数（线程个数）确定的帧缓冲延迟，在多线程情况下为i_delay = i_bframe + i_threads - 1。而判断B帧缓冲填充是否足够则通过条件判断：h->frames.i_input <= h->frames.i_delay + 1 - h->param.i_threads。 /* Number of frames buffered for B reordering (重新排序) */
        int b_have_lowres;  /* Whether 1/2 resolution (分辨率) luma(亮度) planes(平面) are being used */
    } frames;//指示和控制帧编码过程的结构

    /* current frame being encoded */
    x264_frame_t    *fenc;

    /* frame being reconstructed(重建的) */
    x264_frame_t    *fdec;

    /* references(参考) lists */
    int             i_ref0;
    x264_frame_t    *fref0[16+3];     /* ref list 0 */
    int             i_ref1;
    x264_frame_t    *fref1[16+3];     /* ref list 1 */
    int             b_ref_reorder[2];



    /* Current MB DCT coeffs(估计系数) */
    struct
    {
        DECLARE_ALIGNED( int, luma16x16_dc[16], 16 );
        DECLARE_ALIGNED( int, chroma_dc[2][4], 16 );
        // FIXME merge(合并) with union(结合,并集)
        DECLARE_ALIGNED( int, luma8x8[4][64], 16 );
        union
        {
            DECLARE_ALIGNED( int, residual_ac[15], 16 );
            DECLARE_ALIGNED( int, luma4x4[16], 16 );
        } block[16+8];
    } dct;

    /* MB table and cache(高速缓冲存储器) for current frame/mb */
    struct
    {
        int     i_mb_count;                 /* number of mbs in a frame */ /* 在一帧中的宏块中的序号 */

        /* Strides (跨,越) */
        int     i_mb_stride;
        int     i_b8_stride;
        int     i_b4_stride;

        /* Current index */
        int     i_mb_x;
        int     i_mb_y;
        int     i_mb_xy;
        int     i_b8_xy;
        int     i_b4_xy;
        
        /* Search parameters (搜索参数) */
        int     i_me_method;
        int     i_subpel_refine;
        int     b_chroma_me;
        int     b_trellis;
        int     b_noise_reduction;

        /* Allowed qpel(四分之一映像点) MV range to stay (继续,停留) within the picture + emulated(模拟) edge (边) pixels */
        int     mv_min[2];
        int     mv_max[2];
        /* Subpel MV range for motion search.
         * same mv_min/max but includes levels' i_mv_range. */
        int     mv_min_spel[2];
        int     mv_max_spel[2];
        /* Fullpel MV range for motion search */
        int     mv_min_fpel[2];
        int     mv_max_fpel[2];

        /* neighboring MBs */
        unsigned int i_neighbour;
        unsigned int i_neighbour8[4];       /* neighbours of each 8x8 or 4x4 block that are available */
        unsigned int i_neighbour4[16];      /* at the time the block is coded */
        int     i_mb_type_top; 
        int     i_mb_type_left; 
        int     i_mb_type_topleft; 
        int     i_mb_type_topright; 

        /* mb table */
        int8_t  *type;                      /* mb type */
        int8_t  *qp;                        /* mb qp */
        int16_t *cbp;                       /* mb cbp: 0x0?: luma, 0x?0: chroma, 0x100: luma dc, 0x0200 and 0x0400: chroma dc  (all set for PCM)*/
        int8_t  (*intra4x4_pred_mode)[7];   /* intra4x4 pred mode. for non I4x4 set to I_PRED_4x4_DC(2) */
        uint8_t (*non_zero_count)[16+4+4];  /* nzc. for I_PCM set to 16 */
        int8_t  *chroma_pred_mode;          /* chroma_pred_mode. cabac only. for non intra I_PRED_CHROMA_DC(0) */
        int16_t (*mv[2])[2];                /* mb mv. set to 0 for intra mb */
        int16_t (*mvd[2])[2];               /* mb mv difference with predict. set to 0 if intra. cabac only */
        int8_t   *ref[2];                   /* mb ref. set to -1 if non used (intra or Lx only) */
        int16_t (*mvr[2][16])[2];           /* 16x16 mv for each possible ref */
        int8_t  *skipbp;                    /* block pattern for SKIP or DIRECT (sub)mbs. B-frames + cabac only */
        int8_t  *mb_transform_size;         /* transform_size_8x8_flag of each mb */

        /* current value */
        int     i_type;
        int     i_partition;
        int     i_sub_partition[4];
        int     b_transform_8x8;

        int     i_cbp_luma;
        int     i_cbp_chroma;

        int     i_intra16x16_pred_mode;
        int     i_chroma_pred_mode;

        struct
        {
            /* space for p_fenc and p_fdec */
#define FENC_STRIDE 16
#define FDEC_STRIDE 32
            DECLARE_ALIGNED( uint8_t, fenc_buf[24*FENC_STRIDE], 16 );
            DECLARE_ALIGNED( uint8_t, fdec_buf[27*FDEC_STRIDE], 16 );

            /* pointer over mb of the frame to be compressed */
            uint8_t *p_fenc[3];

            /* pointer over mb of the frame to be reconstrucated  */
            uint8_t *p_fdec[3];

            /* pointer over mb of the references */
            uint8_t *p_fref[2][16][4+2]; /* last: lN, lH, lV, lHV, cU, cV */
            uint16_t *p_integral[2][16];

            /* fref stride */
            int     i_stride[3];
        } pic;

        /* cache */
        struct
        {
            /* real intra4x4_pred_mode if I_4X4 or I_8X8, I_PRED_4x4_DC if mb available, -1 if not */
            int     intra4x4_pred_mode[X264_SCAN8_SIZE];

            /* i_non_zero_count if availble else 0x80 */
            int     non_zero_count[X264_SCAN8_SIZE];

            /* -1 if unused, -2 if unavaible */
            int8_t  ref[2][X264_SCAN8_SIZE];

            /* 0 if non avaible */
            int16_t mv[2][X264_SCAN8_SIZE][2];
            int16_t mvd[2][X264_SCAN8_SIZE][2];

            /* 1 if SKIP or DIRECT. set only for B-frames + CABAC */
            int8_t  skip[X264_SCAN8_SIZE];

            int16_t direct_mv[2][X264_SCAN8_SIZE][2];
            int8_t  direct_ref[2][X264_SCAN8_SIZE];

            /* number of neighbors (top and left) that used 8x8 dct */
            int     i_neighbour_transform_size;
            int     b_transform_8x8_allowed;
        } cache;

        /* */
        int     i_qp;       /* current qp */
        int     i_last_qp;  /* last qp */
        int     i_last_dqp; /* last delta qp */
        int     b_variable_qp; /* whether qp is allowed to vary per macroblock */
        int     b_lossless;
        int     b_direct_auto_read; /* take stats for --direct auto from the 2pass log */
        int     b_direct_auto_write; /* analyse direct modes, to use and/or save */

        /* B_direct and weighted prediction */
        int     dist_scale_factor[16][16];
        int     bipred_weight[16][16];
        /* maps fref1[0]'s ref indices into the current list0 */
        int     map_col_to_list0_buf[2]; // for negative indices
        int     map_col_to_list0[16];
    } mb;

    /* rate control encoding only */
    x264_ratecontrol_t *rc;//struct x264_ratecontrol_t,ratecontrol.c中定义

    /* stats */
    struct
    {
        /* Current frame stats */
        struct
        {
            /* Headers bits (MV+Ref+MB Block Type */
            int i_hdr_bits;
            /* Texture bits (Intra/Predicted) */
            int i_itex_bits;
            int i_ptex_bits;
            /* ? */
            int i_misc_bits;
            /* MB type counts */
            int i_mb_count[19];
            int i_mb_count_i;
            int i_mb_count_p;
            int i_mb_count_skip;
            int i_mb_count_8x8dct[2];//宏块数_8x8DCT
            int i_mb_count_size[7];
            int i_mb_count_ref[16];
            /* Estimated (SATD) cost as Intra/Predicted frame */
            /* XXX: both omit the cost of MBs coded as P_SKIP */
            int i_intra_cost;
            int i_inter_cost;
            /* Adaptive direct mv pred */
            int i_direct_score[2];
        } frame;

        /* Cummulated stats */

        /* per slice info */
        int     i_slice_count[5];
        int64_t i_slice_size[5];
        int     i_slice_qp[5];
        /* */
        int64_t i_sqe_global[5];
        float   f_psnr_average[5];
        float   f_psnr_mean_y[5];
        float   f_psnr_mean_u[5];
        float   f_psnr_mean_v[5];
        /* */
        int64_t i_mb_count[5][19];
        int64_t i_mb_count_8x8dct[2];
        int64_t i_mb_count_size[2][7];
        int64_t i_mb_count_ref[2][16];
        /* */
        int     i_direct_score[2];
        int     i_direct_frames[2];

    } stat;

    /* CPU functions dependants */
    x264_predict_t      predict_16x16[4+3];//predict:预测
    x264_predict_t      predict_8x8c[4+3];
    x264_predict8x8_t   predict_8x8[9+3];
    x264_predict_t      predict_4x4[9+3];

    x264_pixel_function_t pixf;
    x264_mc_functions_t   mc;
    x264_dct_function_t   dctf;
    x264_csp_function_t   csp;//x264_csp_function_t是一个结构体，在common/csp.h中定义
    x264_quant_function_t quantf;//在common/quant.h中定义
    x264_deblock_function_t loopf;//在common/frame.h中定义

    /* vlc table for decoding purpose only */
    x264_vlc_table_t *x264_coeff_token_lookup[5];
    x264_vlc_table_t *x264_level_prefix_lookup;
    x264_vlc_table_t *x264_total_zeros_lookup[15];
    x264_vlc_table_t *x264_total_zeros_dc_lookup[3];
    x264_vlc_table_t *x264_run_before_lookup[7];

#if VISUALIZE
    struct visualize_t *visualize;
#endif
};

void    x264_test_my1();
void    x264_test_my2(char * name);

//在最后包含它是因为它需要x264_t
// included at the end because it needs x264_t
#include "macroblock.h"

#endif

