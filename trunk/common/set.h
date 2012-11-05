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
���в�����
�Ϻ���飬160ҳ���壬148ҳ�����в������﷨
*/
typedef struct//profile:����,����,�ſ�,����
{
    int i_id;	//?????

    int i_profile_idc;	//ָ������profile
    int i_level_idc;	//ָ������level

    int b_constraint_set0;//[�Ϻ���� 160 ҳ,constraint_set0_flag]������1ʱ����ʱ�������A.2.1 ...��ָ����������Լ����������0ʱ��ʾ���������������
    int b_constraint_set1;//[�Ϻ���� 160 ҳ,constraint_set0_flag]������1ʱ����ʱ�������A.2.2 ...��ָ����������Լ����������0ʱ��ʾ���������������
    int b_constraint_set2;//[�Ϻ���� 160 ҳ,constraint_set0_flag]������1ʱ����ʱ�������A.2.3 ...��ָ����������Լ����������0ʱ��ʾ���������������//constraint:Լ��;����;ǿ��;ǿ��
							//ע�⣺��constraint_set0_flag�������������ϵ���1ʱ��A.2�е�������Լ������Ҫ�����
    int i_log2_max_frame_num;//��ʾͼ�����˳������ȡֵ
							//[�Ϻ���� 160 ҳ,log2_max_frame_num_minus4]������䷨Ԫ����Ҫ��Ϊ��ȡ��һ���䷨Ԫ��frame_num����ģ�frame_num������Ҫ�ľ䷨Ԫ��֮һ������ʶ����ͼ��Ľ���˳�򡣿����ھ䷨���п�����frame_num�Ľ��뺯����ue(v)�������е�v������ָ����v=log2_max_frame_num_minus + 4
    int i_poc_type;			//[�Ϻ���� 160 ҳ,pic_order_cnt_type]��ָ����POC(Picture Order Count)�ı��뷽����POC��ʶͼ��Ĳ���˳������H.264ʹ����B֡Ԥ�⣬ʹ��ͼ��Ľ���˳��һ�����ڲ���˳�򣬵�����֮�����һ����ӳ���ϵ��POC������frame_numͨ��ӳ���ϵ���������Ҳ���������ɱ�������ʽ�ش��͡�H.264һ��������3��POC�ı��뷽��������䷨Ԫ�ؾ�������֪ͨ�������������ַ���������POC��pic_order_cnt_tye��ȡֵ��Χ��[0,2]...
    /* poc 0 */
    int i_log2_max_poc_lsb;	//[�Ϻ���� 160 ҳ,log2_max_pic_order_cnt_lsb_minus4]��ָ���˱���MaxPicOrderCntLsb��ֵ��
    /* poc 1 */
    int b_delta_pic_order_always_zero;//[�Ϻ���� 161 ҳ,delta_pic_order_always_zero_flag]����ֵ����1ʱ�䷨Ԫ��delta_pic_order_cnt[0]��delta_pic_order_cnt[1]����Ƭͷ���֣������ǵ�Ĭ��ֵ��Ϊ0��Ϊ0ʱ��������֡�
    int i_offset_for_non_ref_pic;//[�Ϻ���� 161 ҳ,offset_for_non_ref_pic]����������ǲο�֡�򳡵�picture order count ,��ֵӦ��[-2e31,2e31-1]
    int i_offset_for_top_to_bottom_field;//[�Ϻ���� 161 ҳ,offset_for_top_to_bottom_field]����������֡�ĵ׳���picture order count ��ֵӦ��[-2e31,2e31-1]
    int i_num_ref_frames_in_poc_cycle;//[�Ϻ���� 161 ҳ,num_ref_frames_in_pic_order_cnt_cycle]����������picture order count ȡֵӦ��[0��255]֮��
    int i_offset_for_ref_frame[256];//[�Ϻ���� 161 ҳ,offset_for_ref_frame[i]]����picture order count type=1ʱ��������poc������﷨��ѭ��num_ref_frames_in_poc_cycle�е�ÿһ��Ԫ��ָ����һ��ƫ��

    int i_num_ref_frames;//[�Ϻ���� 161 ҳ,num_ref_frames]��ָ���ο�֡���е���󳤶ȣ�h264�涨����Ϊ16���ο�֡�����䷨Ԫ�ص�ֵ���Ϊ16��ֵ��ע��������������֡Ϊ��λ������ڳ�ģʽ�£�Ӧ����Ӧ����չһ����
    int b_gaps_in_frame_num_value_allowed;//[�Ϻ���� 161 ҳ,gaps_in_frame_num_value_allowed_flag]��Ϊ1ʱ��ʾ����䷨frame_num���Բ��������������ŵ���������ʱ����������������������ͼ��ȫ����������ʱ����������֡ͼ�������������ÿһ֡ͼ��������������frame_numֵ����������鵽���frame_num������������ȷ����ͼ�񱻱�������������ʱ���������������������ڲػ��������Ƶػָ���Щͼ����Ϊ��Щͼ���п��ܱ�����ͼ�������ο�֡��
									//������䷨Ԫ��=0ʱ����ʾ������frame_num�������������������κ�����¶����ܶ���ͼ����ʱ��H.264������������Բ�ȥ���frame_num���������Լ��ټ���������������������Ȼ����frame_num����������ʾ�ڴ����з�����������������ͨ���������Ƽ�⵽�����ķ�����Ȼ�����������ڲصĻָ�ͼ��
    int i_mb_width;//[�Ϻ���� 161 ҳ,pic_width_in_mbs_minus1]�����䷨Ԫ�ؼ�1��ָ��ͼ���ȣ��Ժ��Ϊ��λ��
    int i_mb_height;//[�Ϻ���� 161 ҳ,pic_height_in_map_units_minus1]�����䷨Ԫ�ؼ�1��ָ��ͼ��ĸ߶ȡ�
    int b_frame_mbs_only;//[�Ϻ���� 161 ҳ,frame_mbs_only_flag]�����䷨Ԫ��=1ʱ����ʾ������������ͼ��ı���ģʽ����֡��û����������ģʽ���ڣ����䷨Ԫ��=0ʱ����ʾ��������ͼ��ı���ģʽ������֡��Ҳ�����ǳ���֡������Ӧ��ĳ��ͼ���������һ��Ҫ�������䷨Ԫ�ؾ�����
    int b_mb_adaptive_frame_field;//[�Ϻ���� 161 ҳ,mb_adaptive_frame_field_flag]��ָ���������Ƿ�����֡������Ӧģʽ��=1ʱ�������ڱ�����
    int b_direct8x8_inference;//ָ��bƬ��ֱ�Ӻ�skipģʽ���˶�ʸ����Ԥ�ⷽ��

    int b_crop;//crop:���� [�Ϻ���� 162 ҳ,frame_cropping_flag]������ָ���������Ƿ�Ҫ��ͼ��ü������������ǵĻ�����������ŵ��ĸ��䷨Ԫ�طֱ�ָ�����ҡ��ϡ��²ü��Ŀ�ȡ�
    struct
    {
        int i_left;		//[�Ϻ���� 162 ҳ,frame_crop_left_offset]����
        int i_right;	//[�Ϻ���� 162 ҳ,frame_crop_left_offset]����
        int i_top;		//[�Ϻ���� 162 ҳ,frame_crop_left_offset]����
        int i_bottom;	//[�Ϻ���� 162 ҳ,frame_crop_left_offset]����
    } crop;//ͼ����ú�����Ĳ���;crop:����,����,����

    int b_vui;	////[�Ϻ���� 162 ҳ,vui_parameters_present_flag]��ָ��vui�ӽṹ�Ƿ�����������У�vui�������ṹ��...��¼ָ�������Ա�����Ƶ��ʽ�ȶ�����Ϣ
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

} x264_sps_t;//x264_sps_t�������вο����еĲ����Լ���ʼ��

typedef struct
{
    int i_id;//[�Ϻ�ܣ�P146 pic_parameter_set_id] ������������ţ��ڸ�Ƭ��Ƭͷ������
    int i_sps_id;//[�Ϻ�ܣ�P146 seq_parameter_set_id] ָ����ͼ������������õ����в����������

    int b_cabac;//[�Ϻ�ܣ�P146 entropy_coding_mode_flag] ָ���ر����ѡ��0ʱʹ��cavlc��1ʱʹ��cabac

    int b_pic_order;//[�Ϻ�ܣ�P147 pic_order_present_flag] poc�����ּ��㷽����Ƭ�㻹����Ҫ��һЩ�䷨Ԫ����Ϊ������������ʱ����ʾ��Ƭͷ���о�䷨Ԫ��ָ����Щ����������Ϊʱ����ʾƬͷ���������Щ����
    int i_num_slice_groups;//[�Ϻ�ܣ�P147 num_slice_groups_minus1] ��һ��ʾͼ����Ƭ��ĸ���

    int i_num_ref_idx_l0_active;//[�Ϻ�ܣ�P147 num_ref_idx_10_active_minus1] ָ��Ŀǰ�ο�֡���еĳ��ȣ����ж��ٸ��ο�֡�����ںͳ��ڣ�������list0
    int i_num_ref_idx_l1_active;//[�Ϻ�ܣ�P147 num_ref_idx_11_active_minus1] ָ��Ŀǰ�ο�֡���еĳ��ȣ����ж��ٸ��ο�֡�����ںͳ��ڣ�������list1

    int b_weighted_pred;//[�Ϻ�ܣ�P147 weighted_pred_flag] ָ���Ƿ�����p��spƬ�ļ�ȨԤ�⣬���������Ƭͷ��������Լ����ȨԤ��ľ䷨Ԫ��
    int b_weighted_bipred;//[�Ϻ�ܣ�P147 weighted_bipred_idc] ָ���Ƿ�����bƬ�ļ�ȨԤ��

    int i_pic_init_qp;//[�Ϻ�ܣ�P147 pic_init_qp_minus26] ���ȷ��������������ĳ�ʼֵ
    int i_pic_init_qs;//[�Ϻ�ܣ�P147 pic_init_qs_minus26] ���ȷ��������������ĳ�ʼֵ������SP��SI

    int i_chroma_qp_index_offset;//[�Ϻ�ܣ�P147 chroma_qp_index_offset] ɫ�ȷ��������������Ǹ������ȷ���������������������ģ����䷨Ԫ������ָ������ʱ�õ��Ĳ�����ʾΪ�� QPC ֵ�ı����Ѱ�� Cbɫ�ȷ�����Ӧ�ӵ����� QPY �� QSY �ϵ�ƫ��,chroma_qp_index_offset ��ֵӦ��-12 �� +12��Χ�ڣ������߽�ֵ��


    int b_deblocking_filter_control;//[�Ϻ�ܣ�P147 deblocking_filter_control_present_flag] ����������ͨ���䷨Ԫ����ʽ�ؿ���ȥ���˲���ǿ��
    int b_constrained_intra_pred;//[�Ϻ�ܣ�P147 constrained_intra_pred_flag] ��p��bƬ�У�֡�ڱ���ĺ����ڽ��������ǲ��õ�֡�����
    int b_redundant_pic_cnt;//[�Ϻ�ܣ�P147 redundant_pic_cnt_present_flag] redundant_pic_cnt ������Щ���ڻ�������ͼ����������������ݷָ��Ӧ����0�����������ͼ���еı��������ͱ����������ݷָ��� redundant_pic_cnt ��ֵӦ���� 0����redundant_pic_cnt ������ʱ��Ĭ����ֵΪ 0��redundant_pic_cnt��ֵӦ���� 0�� 127��Χ�ڣ����� 0��127��

    int b_transform_8x8_mode;//?????????????

    int i_cqm_preset;//cqm:�ⲿ�������������
    const uint8_t *scaling_list[6]; /* ���ű����б�������8��could be 8, but we don't allow separate(�ָ�) Cb/Cr lists */

} x264_pps_t;//ͼ�������[�Ϻ�ܣ�P146 ]

/*
 default quant matrices (Ĭ��������)
 �⼸�����飬д������ķ�����ʽ������ȥ����һ���Խ��߶ԳƵ�
 firstime����̳�Ͻ��ͣ������Զ����������󰡡��� H.264 Э�� 200503 ��� 7-3��7-4 �����ֶ���һ�¾�������
 hai296 ����̳�Ͻ��� ���Ǹ��ݴ���ͼ�񾭹������ܽ���������ŵ��������ڱ�׼������ȷ�Ĺ涨��
*/
/*
��׼ ��7-3 82ҳ/342ҳ
*/
static const uint8_t x264_cqm_jvt4i[16] =
{
      6,13,20,28,
     13,20,28,32,
     20,28,32,37,
     28,32,37,42
};
/*
��׼ ��7-3 82ҳ/342ҳ
*/
static const uint8_t x264_cqm_jvt4p[16] =
{
    10,14,20,24,
    14,20,24,27,
    20,24,27,30,
    24,27,30,34
};
/*
��׼ ��7-4�ĵ�1�� 82ҳ/342ҳ �����̫�����Էֳ���4��������д�Ÿ���
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
��׼ ��7-4�ĵڶ��� 82ҳ/342ҳ �����̫�����Էֳ���4��������д�Ÿ���
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
