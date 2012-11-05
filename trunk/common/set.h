/*****************************************************************************
 * set.h: h264 encoder
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: set.h,v 1.1 2004/06/03 19:27:07 fenrir Exp $
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

#ifndef _SET_H
#define _SET_H 1

enum profile_e
{
    PROFILE_BASELINE = 66,
    PROFILE_MAIN     = 77,
    PROFILE_EXTENTED = 88,
    PROFILE_HIGH    = 100,
    PROFILE_HIGH10  = 110,
    PROFILE_HIGH422 = 122,
    PROFILE_HIGH444 = 144
};

enum cqm4_e
{
    CQM_4IY = 0,
    CQM_4PY = 1,
    CQM_4IC = 2,
    CQM_4PC = 3
};
enum cqm8_e
{
    CQM_8IY = 0,
    CQM_8PY = 1
};



/*
序列参数集
毕厚杰书，160页语义，148页，序列参数集语法
*/
typedef struct//profile:外形,评测,概况,剖面
{
    int i_id;	//?????

    int i_profile_idc;	//指明所用profile
    int i_level_idc;	//指明所用level

    int b_constraint_set0;//[毕厚杰书 160 页,constraint_set0_flag]，等于1时，表时必须遵从A.2.1 ...所指明的所有制约条件，等于0时表示不必遵从所有条件
    int b_constraint_set1;//[毕厚杰书 160 页,constraint_set0_flag]，等于1时，表时必须遵从A.2.2 ...所指明的所有制约条件，等于0时表示不必遵从所有条件
    int b_constraint_set2;//[毕厚杰书 160 页,constraint_set0_flag]，等于1时，表时必须遵从A.2.3 ...所指明的所有制约条件，等于0时表示不必遵从所有条件//constraint:约束;限制;强迫;强制
							//注意：当constraint_set0_flag、中有两个以上等于1时，A.2中的所有制约条件都要被遵从
    int i_log2_max_frame_num;//表示图像解码顺序的最大取值
							//[毕厚杰书 160 页,log2_max_frame_num_minus4]，这个句法元素主要是为读取另一个句法元素frame_num服务的，frame_num是最重要的句法元素之一，它标识所属图像的解码顺序。可以在句法表中看到，frame_num的解码函数是ue(v)，函数中的v在这里指定，v=log2_max_frame_num_minus + 4
    int i_poc_type;			//[毕厚杰书 160 页,pic_order_cnt_type]，指明了POC(Picture Order Count)的编码方法、POC标识图像的播放顺序。由于H.264使用了B帧预测，使得图像的解码顺序不一定等于播放顺序，但它们之间存在一定的映射关系。POC可以由frame_num通过映射关系计算得来，也可以索性由编码器显式地传送。H.264一共定义了3种POC的编码方法，这个句法元素就是用来通知解码器该用哪种方法来计算POC。pic_order_cnt_tye的取值范围是[0,2]...
    /* poc 0 */
    int i_log2_max_poc_lsb;	//[毕厚杰书 160 页,log2_max_pic_order_cnt_lsb_minus4]，指明了变量MaxPicOrderCntLsb的值。
    /* poc 1 */
    int b_delta_pic_order_always_zero;//[毕厚杰书 161 页,delta_pic_order_always_zero_flag]，其值等于1时句法元素delta_pic_order_cnt[0]和delta_pic_order_cnt[1]不再片头出现，且他们的默认值都为0。为0时上述则出现。
    int i_offset_for_non_ref_pic;//[毕厚杰书 161 页,offset_for_non_ref_pic]，用来计算非参考帧或场的picture order count ,其值应在[-2e31,2e31-1]
    int i_offset_for_top_to_bottom_field;//[毕厚杰书 161 页,offset_for_top_to_bottom_field]，用来计算帧的底场的picture order count 其值应在[-2e31,2e31-1]
    int i_num_ref_frames_in_poc_cycle;//[毕厚杰书 161 页,num_ref_frames_in_pic_order_cnt_cycle]，用来解码picture order count 取值应在[0，255]之间
    int i_offset_for_ref_frame[256];//[毕厚杰书 161 页,offset_for_ref_frame[i]]，当picture order count type=1时用来解码poc，这句语法对循环num_ref_frames_in_poc_cycle中的每一个元素指定了一个偏移

    int i_num_ref_frames;//[毕厚杰书 161 页,num_ref_frames]，指定参考帧队列的最大长度，h264规定最多可为16个参考帧，本句法元素的值最大为16。值得注意的是这个长度以帧为单位，如果在场模式下，应该相应地扩展一倍。
    int b_gaps_in_frame_num_value_allowed;//[毕厚杰书 161 页,gaps_in_frame_num_value_allowed_flag]，为1时表示允许句法frame_num可以不连续。当传输信道堵塞严重时，编码器来不及将编码后的图像全部发出，这时允许丢弃若干帧图像。在正常情况下每一帧图像都有依次连续的frame_num值，解码器检查到如果frame_num不连续，便能确定有图像被编码器丢弃。这时，解码器必须启动错误掩藏机制来近似地恢复这些图像，因为这些图像有可能被后续图像用作参考帧。
									//当这个句法元素=0时，表示不允许frame_num不连续，即编码器在任何情况下都不能丢弃图像。这时，H.264允许解码器可以不去检查frame_num的连续性以减少计算量。这种情况下如果依然发生frame_num不连续，表示在传输中发生丢包，解码器会通过其他机制检测到丢包的发生，然后启动错误掩藏的恢复图像。
    int i_mb_width;//[毕厚杰书 161 页,pic_width_in_mbs_minus1]，本句法元素加1后指明图像宽度，以宏块为单位。
    int i_mb_height;//[毕厚杰书 161 页,pic_height_in_map_units_minus1]，本句法元素加1后指明图像的高度。
    int b_frame_mbs_only;//[毕厚杰书 161 页,frame_mbs_only_flag]，本句法元素=1时，表示本序列中所有图像的编码模式都是帧，没有其他编码模式存在；本句法元素=0时，表示本序列中图像的编码模式可能是帧，也可能是场或帧场自适应，某个图像具体是哪一种要由其他句法元素决定。
    int b_mb_adaptive_frame_field;//[毕厚杰书 161 页,mb_adaptive_frame_field_flag]，指明本序列是否属于帧场自适应模式。=1时，表明在本序列
    int b_direct8x8_inference;//指明b片的直接和skip模式下运动矢量的预测方法

    int b_crop;//crop:剪裁 [毕厚杰书 162 页,frame_cropping_flag]，用于指明解码器是否要将图像裁剪后输出，如果是的话，后面紧跟着的四个句法元素分别指出左、右、上、下裁剪的宽度。
    struct
    {
        int i_left;		//[毕厚杰书 162 页,frame_crop_left_offset]，左
        int i_right;	//[毕厚杰书 162 页,frame_crop_left_offset]，右
        int i_top;		//[毕厚杰书 162 页,frame_crop_left_offset]，上
        int i_bottom;	//[毕厚杰书 162 页,frame_crop_left_offset]，下
    } crop;//图像剪裁后输出的参数;crop:剪裁,剪切,剪辑

    int b_vui;	////[毕厚杰书 162 页,vui_parameters_present_flag]，指明vui子结构是否出现在码流中，vui的码流结构在...附录指明，用以表征视频格式等额外信息
    struct
    {
        int b_aspect_ratio_info_present;
        int i_sar_width;
        int i_sar_height;
        
        int b_overscan_info_present;
        int b_overscan_info;

        int b_signal_type_present;
        int i_vidformat;
        int b_fullrange;
        int b_color_description_present;
        int i_colorprim;
        int i_transfer;
        int i_colmatrix;

        int b_chroma_loc_info_present;
        int i_chroma_loc_top;
        int i_chroma_loc_bottom;

        int b_timing_info_present;
        int i_num_units_in_tick;
        int i_time_scale;
        int b_fixed_frame_rate;

        int b_bitstream_restriction;
        int b_motion_vectors_over_pic_boundaries;
        int i_max_bytes_per_pic_denom;
        int i_max_bits_per_mb_denom;
        int i_log2_max_mv_length_horizontal;
        int i_log2_max_mv_length_vertical;
        int i_num_reorder_frames;
        int i_max_dec_frame_buffering;

        /* FIXME to complete */
    } vui;

    int b_qpprime_y_zero_transform_bypass;

} x264_sps_t;//x264_sps_t定义序列参考队列的参数以及初始化

typedef struct
{
    int i_id;//[毕厚杰：P146 pic_parameter_set_id] 本参数集的序号，在各片的片头被引用
    int i_sps_id;//[毕厚杰：P146 seq_parameter_set_id] 指明本图像参数集所引用的序列参数集的序号

    int b_cabac;//[毕厚杰：P146 entropy_coding_mode_flag] 指明熵编码的选择：0时使用cavlc，1时使用cabac

    int b_pic_order;//[毕厚杰：P147 pic_order_present_flag] poc的三种计算方法在片层还各需要用一些句法元素作为参数；当等于时，表示在片头会有句句法元素指明这些参数；当不为时，表示片头不会给出这些参数
    int i_num_slice_groups;//[毕厚杰：P147 num_slice_groups_minus1] 加一表示图像中片组的个数

    int i_num_ref_idx_l0_active;//[毕厚杰：P147 num_ref_idx_10_active_minus1] 指明目前参考帧队列的长度，即有多少个参考帧（短期和长期），用于list0
    int i_num_ref_idx_l1_active;//[毕厚杰：P147 num_ref_idx_11_active_minus1] 指明目前参考帧队列的长度，即有多少个参考帧（短期和长期），用于list1

    int b_weighted_pred;//[毕厚杰：P147 weighted_pred_flag] 指明是否允许p和sp片的加权预测，如果允许，在片头会出现用以计算加权预测的句法元素
    int b_weighted_bipred;//[毕厚杰：P147 weighted_bipred_idc] 指明是否允许b片的加权预测

    int i_pic_init_qp;//[毕厚杰：P147 pic_init_qp_minus26] 亮度分量的量化参数的初始值
    int i_pic_init_qs;//[毕厚杰：P147 pic_init_qs_minus26] 亮度分量的量化参数的初始值，用于SP和SI

    int i_chroma_qp_index_offset;//[毕厚杰：P147 chroma_qp_index_offset] 色度分量的量化参数是根据亮度分量的量化参数计算出来的，本句法元素用以指明计算时用到的参数表示为在 QPC 值的表格中寻找 Cb色度分量而应加到参数 QPY 和 QSY 上的偏移,chroma_qp_index_offset 的值应在-12 到 +12范围内（包括边界值）


    int b_deblocking_filter_control;//[毕厚杰：P147 deblocking_filter_control_present_flag] 编码器可以通过句法元素显式地控制去块滤波的强度
    int b_constrained_intra_pred;//[毕厚杰：P147 constrained_intra_pred_flag] 在p和b片中，帧内编码的宏块的邻近宏块可能是采用的帧间编码
    int b_redundant_pic_cnt;//[毕厚杰：P147 redundant_pic_cnt_present_flag] redundant_pic_cnt 对于那些属于基本编码图像的条带和条带数据分割块应等于0。在冗余编码图像中的编码条带和编码条带数据分割块的 redundant_pic_cnt 的值应大于 0。当redundant_pic_cnt 不存在时，默认其值为 0。redundant_pic_cnt的值应该在 0到 127范围内（包括 0和127）

    int b_transform_8x8_mode;//?????????????

    int i_cqm_preset;//cqm:外部量化矩阵的设置
    const uint8_t *scaling_list[6]; /* 缩放比例列表，不能是8，could be 8, but we don't allow separate(分割) Cb/Cr lists */

} x264_pps_t;//图像参数集[毕厚杰，P146 ]

/*
 default quant matrices (默认量化？)
 这几个数组，写成下面的方块形式，看上去是以一个对角线对称的
 firstime在论坛上解释：就是自定义量化矩阵啊。跟 H.264 协议 200503 版表 7-3、7-4 的数字对照一下就明白了
 hai296 在论坛上解释 这是根据大量图像经过概率总结出来的最优的量化表，在标准中有明确的规定的
*/
/*
标准 表7-3 82页/342页
*/
static const uint8_t x264_cqm_jvt4i[16] =
{
      6,13,20,28,
     13,20,28,32,
     20,28,32,37,
     28,32,37,42
};
/*
标准 表7-3 82页/342页
*/
static const uint8_t x264_cqm_jvt4p[16] =
{
    10,14,20,24,
    14,20,24,27,
    20,24,27,30,
    24,27,30,34
};
/*
标准 表7-4的第1行 82页/342页 这个表太大，所以分成了4个表，上面写着个续
*/
static const uint8_t x264_cqm_jvt8i[64] =
{
     6,10,13,16,18,23,25,27,
    10,11,16,18,23,25,27,29,
    13,16,18,23,25,27,29,31,
    16,18,23,25,27,29,31,33,
    18,23,25,27,29,31,33,36,
    23,25,27,29,31,33,36,38,
    25,27,29,31,33,36,38,40,
    27,29,31,33,36,38,40,42
};
/*
标准 表7-4的第二行 82页/342页 这个表太大，所以分成了4个表，上面写着个续
*/
static const uint8_t x264_cqm_jvt8p[64] =
{
     9,13,15,17,19,21,22,24,
    13,13,17,19,21,22,24,25,
    15,17,19,21,22,24,25,27,
    17,19,21,22,24,25,27,28,
    19,21,22,24,25,27,28,30,
    21,22,24,25,27,28,30,32,
    22,24,25,27,28,30,32,33,
    24,25,27,28,30,32,33,35
};


static const uint8_t x264_cqm_flat16[64] =
{
    16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16
};
static const uint8_t * const x264_cqm_jvt[6] =
{
    x264_cqm_jvt4i, x264_cqm_jvt4p,
    x264_cqm_jvt4i, x264_cqm_jvt4p,
    x264_cqm_jvt8i, x264_cqm_jvt8p
};

void x264_cqm_init( x264_t *h );
void x264_cqm_delete( x264_t *h );
int  x264_cqm_parse_file( x264_t *h, const char *filename );

#endif
