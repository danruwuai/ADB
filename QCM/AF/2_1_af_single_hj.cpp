////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file af_single_hj.cpp
/// @brief
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "af_single_hj.h"
#include "af_caf_search.h"
#include "af_debug_data.h"

// NOWHINE ENTIRE FILE - legacy code ported temporarily

#define AF_LSP_FINE_SEARCH_RANGE_RATIO 0.2

//************************************
// Bilateral filter Threshold for flat curve detection by fv sampling
//#define IS_SUPPORT_FLAT_DECTION TRUE
//#define IS_SUPPORT_FLAT_DECTION FALSE
#define BILATERAL_THR_LOW 0.03f
#define BILATERAL_THR_HIGH 0.10f

#define SLOPE_HIGH    0.4
#define SLOPE_LOW    -0.8
#define ACCEL_HIGH  0.8
#define ACCEL_LOW   -0.8
//************************************

// TODO : need moving to chromatix header
#define SET_INVALID_FRAME TRUE //BOOL

#define STEP_BACK_START_INF 2  //UINT32
#define STEP_BACK_START_MAC 0 //UINT32
#define STEP_BACK_REVERSE_INF 2 //UINT32
#define STEP_BACK_REVERSE_MAC 2 //UINT32

#define Set_fine_fRatio_min     1.0f //FLOAT
#define Set_fine_fRatio_max     4.0f //FLOAT

#define Set_fine_1st_prev_y1    0.8f //FLOAT
#define Set_fine_1st_next_y1    0.3f //FLOAT
#define Set_fine_1st_prev_y2    1.3f //FLOAT
#define Set_fine_1st_next_y2   -0.3f //FLOAT

#define Set_fine_2nd_prev_y1       0.8f //FLOAT
#define Set_fine_2nd_next_y1       0.3f //FLOAT
#define Set_fine_2nd_prev_y2       0.3f //FLOAT
#define Set_fine_2nd_next_y2       0.8f //FLOAT

#define BV_TH_flat_detect_bilateral 6.0f /*FLOAT*/ //default : outdoor BV 6.0

#define Wait_frames_after_goto_peak 0 //UINT32

#define Offset_from_final_peak 0 //UINT32

#define FV_INC_FLAT_RATIO_TH  1.02f //FLOAT
#define FV_DEC_FLAT_RATIO_TH 0.98f //FLOAT

#define FV_INC_FLAT_RATIO_SPOT_LIGHT_TH 1.01f //FLOAT
#define FV_DEC_FLAT_RATIO_SPOT_LIGHT_TH 0.99f //FLOAT

#define IS_SPOTLIGHT_SCENE FALSE /*BOOL*/ //p_af_internal->spd.spotlight_detect_final;
#define AF_SPOT_LIGHT_INF_FLAT_THRS  0.0f /*FLOAT*/ //p_af_internal->haf.p_haf_tuning->reserve[11];
#define AF_SPOT_LIGHT_MARCO_FLAT_THRS 0.0f /*FLOAT*/ //p_af_internal->haf.p_haf_tuning->reserve[12];
// End of TODO : need moving to chromatix header

#ifdef VENDOR_EDIT
#define EXTRA_LOW_LIGHT_BV_IDX_THR 0
#define PEAK_PATTERN_FLAT_DEC_THR 0.989f
#define PEAK_PATTERN_FLAT_INC_THR 1.015f

#undef FV_INC_FLAT_RATIO_TH
#define FV_INC_FLAT_RATIO_TH 1.005f

#undef FV_DEC_FLAT_RATIO_TH
#define FV_DEC_FLAT_RATIO_TH 0.995f

VOID af_single_hj_set_optics(af_caf_search_internal_t* p_caf_instance);
#endif

//************************
/*Enable/disable features*/

#define AF_MAX_BAD_STATS_COUNT 15

/*Enaable/disable for flat_curve_detection_by_bilateral*/
#if 0
#define feature_flat_curve_detection_by_bilateral
#endif

/*Enagle/disable for optics setting for pdaf*/
#if 0
#define feature_optics_setting_for_pdaf
#endif

/*Enagle/disable for force fine search eventhough fine search is disabled by
 chromatix header parameters*/
#if 0
#define feature_force_enable_fine_search
#endif

/*End of Enable/disable features*/

/* -----------------------------------------------------------------------
 ** Type Declarations
 ** ----------------------------------------------------------------------- */
UINT32 LSP_Corse_Peak_Pos;

/* -----------------------------------------------------------------------
 ** Forward Declarations
 ** ----------------------------------------------------------------------- */
/* Call this function to update the AF parameters before runing the algorithm during
 search only for single AF. */
//VOID update_af_debug_data_before_frame_single_hj_af(af_single_hj_t* const af);
/* Call this function to update the AF parameters after runing the algorithm during search
 only for single af. */
//VOID update_af_debug_data_after_frame_single_hj_af(af_single_hj_t* const af);
/* -----------------------------------------------------------------------
 ** Function DefINTions
 ** ----------------------------------------------------------------------- */
double af_single_hj_fpow(
    double val,
    INT n)
{
    INT k;
    double s = 1;

    for (k = 1; k <= n; k++)
        s = s * val;

    return s;
}

INT af_single_hj_roundNo(
    FLOAT num)
{
    return (INT)((num < 0.0f) ? (num - 0.5f) : (num + 0.5f));
}

static BOOL af_single_is_pos_inside_range(
    af_caf_search_internal_t* p_caf_instance,
    INT pos)
{
    if (pos > p_caf_instance->af_range.far_end)
    {
        return FALSE;
    }
    else if (pos < p_caf_instance->af_range.near_end)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


static INT32 af_single_hj_cap_lens_sag(
    INT32 lens_pos,
    INT32 near_end,
    INT32 far_end)
{
    if (lens_pos < near_end)
    {
        lens_pos = near_end;
    }

    if (lens_pos > far_end)
    {
        lens_pos = far_end;
    }

    return lens_pos;
}

VOID af_single_hj_orientation_tunning(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    chromatixAFAlgoTuningType* p_algo_tuning =
        p_caf_instance->af_tuning->pAlgoTuningData;
    chromatixContrastAFType* pContrastTuningData =
        p_caf_instance->af_tuning->pContrastTuningData;
    INT i = 0;
    INT applied_comp = 0;
    INT gravity_comp = 0;    //TODO:  check this logic requires lens sag, if we dont include then open loop actuator wont work
    AF_MSG_LOW("Previous Comp: %d New Comp: %d",
               saf->gravity_comp,
               gravity_comp);
    applied_comp = gravity_comp - saf->gravity_comp;
    saf->gravity_comp = gravity_comp;

    for (i = 0; i < AF_DISTANCE_ENTRY_MAX; i++)
    {
        pContrastTuningData->singleSearch.index[i] =
            (UINT16) af_single_hj_cap_lens_sag(pContrastTuningData->singleSearch.index[i]
                                               - applied_comp,
                                               p_caf_instance->af_range.near_end,
                                               p_caf_instance->af_range.far_end);
    }

    AF_MSG_LOW(" mode->default_pos = %d  mode->hyp_pos =%d "
               "gravity_comp =%d",
               p_algo_tuning->commonElements.positionDefaultInNormal,
               p_algo_tuning->commonElements.positionNormalHyperfocal,
               applied_comp);
    p_algo_tuning->commonElements.positionNormalHyperfocal =
        (UINT16) af_single_hj_cap_lens_sag
        (p_algo_tuning->commonElements.positionNormalHyperfocal
         - applied_comp,
         p_caf_instance->af_range.near_end,
         p_caf_instance->af_range.far_end);
}

//Start of Hand Jitter related functions
/** af_single_hj_LSP_fv_update_sample: update the incoming sample
 *  with index af_index in fv_info for least square parabolic fitting
 *  @fv_info: pointer to fv array
 *  @af_index: the index to the array.
 *
 *  Return none
 **/
VOID af_single_hj_LSP_fv_update_sample(
    af_caf_search_internal_t* p_caf_instance,
    af_single_hj_fv_info_t* fv_info,
    INT af_idx)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    LSP_Process* LSP_AF_sample = &saf->LSP_AF_sample;
    FLOAT gyro_sqr = saf->gyro_stats.sqr;

    // cyclic buffer for two searching direction
    if (saf->direction == DIR_TO_NEAR)
    {
        LSP_AF_sample->ToN_sample[LSP_AF_sample->ToN_sampleCount %
                                  AF_COLLECTION_POINTS].hw_fv_info[0] = fv_info->fv_val[af_idx];
        LSP_AF_sample->ToN_sample[LSP_AF_sample->ToN_sampleCount %
                                  AF_COLLECTION_POINTS].pos = saf->af_index[af_idx];
        LSP_AF_sample->ToN_sample[LSP_AF_sample->ToN_sampleCount %
                                                                 AF_COLLECTION_POINTS].gyro_sqr = gyro_sqr;
        LSP_AF_sample->ToN_sample[LSP_AF_sample->ToN_sampleCount %
                                                                 AF_COLLECTION_POINTS].is_HJ_stats = fv_info->is_HJ_stats;

        if (TRUE == fv_info->is_HJ_stats)
            LSP_AF_sample->ToN_HJ_sampleCount++;

        AF_MSG_LOW("LSP_DBG LSP_fv_update 2Near Dir%d"
                   "(Ori LP,FV,Gyro,isHJ_stats,,NHJ_cnt)= %d %f %f %d %d",
                   saf->direction,
                   saf->af_index[af_idx],
                   fv_info->fv_val[af_idx],
                   gyro_sqr,
                   fv_info->is_HJ_stats,
                   LSP_AF_sample->ToN_HJ_sampleCount);
        AF_MSG_HIGH("%d is the af_idx %f is the fv",
                    af_idx,
                    LSP_AF_sample->ToN_sample[LSP_AF_sample->ToN_sampleCount %
                                              AF_COLLECTION_POINTS].hw_fv_info[0]);
        LSP_AF_sample->ToN_sampleCount++;
    }
    else
    {
        LSP_AF_sample->ToF_sample[LSP_AF_sample->ToF_sampleCount %
                                  AF_COLLECTION_POINTS].hw_fv_info[0] = fv_info->fv_val[af_idx];
        LSP_AF_sample->ToF_sample[LSP_AF_sample->ToF_sampleCount %
                                  AF_COLLECTION_POINTS].pos = saf->af_index[af_idx];
        LSP_AF_sample->ToF_sample[LSP_AF_sample->ToF_sampleCount %
                                                                 AF_COLLECTION_POINTS].gyro_sqr = gyro_sqr;
        LSP_AF_sample->ToF_sample[LSP_AF_sample->ToF_sampleCount %
                                                                 AF_COLLECTION_POINTS].is_HJ_stats = fv_info->is_HJ_stats;

        if (TRUE == fv_info->is_HJ_stats)
            LSP_AF_sample->ToF_HJ_sampleCount++;

        AF_MSG_LOW("LSP_DBG LSP_fv_update 2Far  Dir%d"
                   "(Ori LP,FV,Gyro,isHJ_stats,FHJ_cnt)= %d %f %f %d %d",
                   saf->direction,
                   saf->af_index[af_idx],
                   fv_info->fv_val[af_idx],
                   gyro_sqr,
                   fv_info->is_HJ_stats,
                   LSP_AF_sample->ToF_HJ_sampleCount);
        AF_MSG_HIGH("%d is the af_idx %f is the fv",
                    af_idx,
                    LSP_AF_sample->ToN_sample[LSP_AF_sample->ToF_HJ_sampleCount %
                                              AF_COLLECTION_POINTS].hw_fv_info[0]);
        LSP_AF_sample->ToF_sampleCount++;
    }
}

VOID af_single_hj_LSParabolicFit(
    Parabolic* para,
    LSP_AFSweepSample* in,
    INT sampleCount)
{
    double temp;
    INT i, j, k;
    double a[3][4];
    double cof[3];
    INT length = sampleCount;

    for (i = 0; i < 3; i++)
        for (j = 0; j <= 3; j++)
            a[i][j] = 0.0;

    for (k = 0; k < 3; k++)
        cof[k] = 0.0;

    //--- M11==c0+c1*Wp+c2*Wp^2
    for (i = 0; i < length; i++)    // assign curve fitting matrix
    {
        a[0][1] = (double) a[0][1] + (double) in[i].pos;
        a[0][2] = (double) a[0][2]
                  + (double) af_single_hj_fpow((double) in[i].pos,
                                               2);
        a[1][2] = (double) a[1][2]
                  + (double) af_single_hj_fpow((double) in[i].pos,
                                               3);
        a[2][2] = (double) a[2][2]
                  + (double) af_single_hj_fpow((double) in[i].pos,
                                               4);
        a[0][3] = (double) a[0][3] + (double) (in[i].hw_fv_info[0]);
        a[1][3] = (double) a[1][3] +
                  (double) (in[i].hw_fv_info[0]) * (double) in[i].pos;
        a[2][3] = (double) a[2][3] +
                  (double) (in[i].hw_fv_info[0]) *
                  (double) af_single_hj_fpow((double) in[i].pos,
                                             2);
    }

    a[0][0] = (double) (length);    // skip sample 0 data
    a[1][0] = a[0][1];
    a[1][1] = a[2][0] = a[0][2];
    a[2][1] = a[1][2];

    //--- using Gauss direct elimination method
    //---  a is symmetric matrix and a[:][:]!=0
    for (i = 0; i < 3; i++)
        for (j = i + 1; j < 3; j++)
            for (k = 3; k >= i; k--)
                a[j][k] = a[j][k] - a[i][k] * a[j][i] / a[i][i];

    for (j = 2; j >= 0; j--)
    {
        temp = 0.0;

        for (k = j + 1; k < 3; k++)
            temp = (double) temp + (double) (a[j][k] * cof[k]);

        cof[j] = (double) (a[j][3] - (double) temp) / a[j][j];    //  cof=[c0,c1,c2]
    }

    para->a = (FLOAT) cof[2];
    para->b = (FLOAT) cof[1];
    para->c = (FLOAT) cof[0];
}


VOID af_single_hj_HJ_LSParasolve_weight(
    LSP_AFSweepSample* in_sort,
    INT numSample,
    FLOAT* HJ_weight,
    chromatixAFAlgoTuningType* p_algo_tuning)
{
    INT i;

    for (i = 0; i < numSample; i++)
    {
        if (in_sort[i].gyro_sqr > p_algo_tuning->handJitter.AFSingle.fineSearch.FVNormalized.highGyroscopeDifferenceThreshold)
            HJ_weight[i] = p_algo_tuning->handJitter.AFSingle.fineSearch.FVNormalized.highGryoscopeFVWeight;
        else if (in_sort[i].gyro_sqr < p_algo_tuning->handJitter.AFSingle.fineSearch.FVNormalized.lowGyroscopeDifferenceThreshold)
            HJ_weight[i] = 1.0;
        else
            HJ_weight[i] =
                1.0f + ((p_algo_tuning->handJitter.AFSingle.fineSearch.FVNormalized.highGryoscopeFVWeight - 1.0f)
                        / (p_algo_tuning->handJitter.AFSingle.fineSearch.FVNormalized.highGyroscopeDifferenceThreshold -
                           p_algo_tuning->handJitter.AFSingle.fineSearch.FVNormalized.lowGyroscopeDifferenceThreshold))
                * (in_sort[i].gyro_sqr - p_algo_tuning->handJitter.AFSingle.fineSearch.FVNormalized.lowGyroscopeDifferenceThreshold);

        if (HJ_weight[i] > 1.0)
            HJ_weight[i] = 1.0;

        if (HJ_weight[i] < 0.0)
            HJ_weight[i] = 0.0;

        AF_MSG_LOW("HJ_CAF: HJ_weight: in_sort_Gyro: index %d, pos %d, gyro %f, hj_weight %f",
                   i, in_sort[i].pos, in_sort[i].gyro_sqr, HJ_weight[i]);
    }
}

VOID af_single_hj_HJ_LSParasolve_weight_sad(
    LSP_AFSweepSample* in_sort,
    INT numSample,
    FLOAT* HJ_weight,
    chromatixAFAlgoTuningType* p_algo_tuning)
{
    INT i;
    FLOAT HJ_Gyro_SAD[AF_COLLECTION_POINTS];

    for (i = 0; i < numSample; i++)
    {
        if (i == 0)
        {
            HJ_Gyro_SAD[i] = ABS(in_sort[i].gyro_sqr - in_sort[i + 1].gyro_sqr);
        }
        else if (i == numSample - 1)
        {
            HJ_Gyro_SAD[i] = ABS(in_sort[i].gyro_sqr - in_sort[i - 1].gyro_sqr);
        }
        else
        {
            HJ_Gyro_SAD[i] = (ABS(in_sort[i].gyro_sqr - in_sort[i - 1].gyro_sqr) +
                              ABS(in_sort[i].gyro_sqr - in_sort[i + 1].gyro_sqr)) / 2.0f;
        }

        if (HJ_Gyro_SAD[i] > p_algo_tuning->handJitter.AFSingle.fineSearch.FVNormalized.highGyroscopeDifferenceThreshold)
            HJ_weight[i] = p_algo_tuning->handJitter.AFSingle.fineSearch.FVNormalized.highGryoscopeFVWeight;
        else if (HJ_Gyro_SAD[i] < p_algo_tuning->handJitter.AFSingle.fineSearch.FVNormalized.lowGyroscopeDifferenceThreshold)
            HJ_weight[i] = 1.0;
        else
            HJ_weight[i] =
                1.0f + ((p_algo_tuning->handJitter.AFSingle.fineSearch.FVNormalized.highGryoscopeFVWeight - 1.0f)
                        / (p_algo_tuning->handJitter.AFSingle.fineSearch.FVNormalized.highGyroscopeDifferenceThreshold -
                           p_algo_tuning->handJitter.AFSingle.fineSearch.FVNormalized.lowGyroscopeDifferenceThreshold))
                * (HJ_Gyro_SAD[i] - p_algo_tuning->handJitter.AFSingle.fineSearch.FVNormalized.lowGyroscopeDifferenceThreshold);

        if (HJ_weight[i] > 1.0)
            HJ_weight[i] = 1.0;

        if (HJ_weight[i] < 0.0)
            HJ_weight[i] = 0.0;

        AF_MSG_LOW("HJ_CAF: HJ_weight: in_sort_Gyro: index %d, pos %d, gyro %f, gyro_sad %f, hj_weight %f",
                   i, in_sort[i].pos, in_sort[i].gyro_sqr, HJ_Gyro_SAD[i], HJ_weight[i]);
    }
}


INT af_single_hj_HJ_LSParasolve(
    LSP_AFSweepSample* in,
    LSP_AFSweepSample* in_sort,
    INT count,
    INT* ValidSampleCount,
    BOOL is_fine_search,
    af_single_hj_LSP_info_t* lsp_info,
    chromatixAFAlgoTuningType* p_algo_tuning)
{
    INT i, idx, start_idx, total_idx;
    INT j, insert, sort_idx;
    INT bestPos = 0, numSample = 0;
    INT firstIndex = 0, lastIndex = 0, peakIndex = 0;
    FLOAT min = (FLOAT)(1.0e+32);
    FLOAT max = 0;
    FLOAT scale;
    INT soln;
    Parabolic para;
    FLOAT HJ_weight[AF_COLLECTION_POINTS];
    FLOAT in_sort_normlized[AF_COLLECTION_POINTS];
    BOOL No_ckeck_HJ_stats = FALSE;
    /* Sorting algorithm
     1. Tighten out unqualified stats e.g. HJ stats , zero stats
     2. use recent samples (only newest one is kept to calculate).
     3. in_sort[] store qualified stats in order
     4. pick up newest stats for same position
     */
    // start_idx (oldest sample)
    start_idx = (count > AF_COLLECTION_POINTS) ?
                count % AF_COLLECTION_POINTS :
                0;
    // total_idx (numbers of samples in the input cyclic buffer)
    total_idx = MIN(count,
                    AF_COLLECTION_POINTS);

    for (idx = start_idx, sort_idx = 0; total_idx > 0; idx++, total_idx--)
    {
        i = idx % AF_COLLECTION_POINTS;
        AF_MSG_LOW("HJ_CAF: in[%d] = [%d %f %f %d]",
                   i,
                   in[i].pos,
                   in[i].hw_fv_info[0],
                   in[i].gyro_sqr,
                   in[i].is_HJ_stats);

        /* Remove zero stats only, when HJ_normalized_Fv_enable
         is on && in fine_search case*/
        if ((TRUE
                == p_algo_tuning->handJitter.AFSingle.fineSearch.FVNormalized.enable)
                && (TRUE == is_fine_search))
            No_ckeck_HJ_stats = TRUE;

        // Remove HJ stats and zero stats in general case
        if ((in[i].hw_fv_info[0] > 0.0f) &&
                ((in[i].is_HJ_stats != TRUE) || (No_ckeck_HJ_stats == TRUE)))
        {
            if (sort_idx == 0)    // first one
            {
                in_sort[0] = in[i];
                sort_idx++;
            }
            else
            {
                insert = LSP_SAMPLE_IDX(sort_idx - 1);

                // skip from bigger elements in output:
                while (insert >= 0 && in_sort[insert].pos > in[i].pos)
                {
                    insert--;
                }

                //  in[i] is the smallest.
                if ((insert < 0) || (in_sort[insert].pos < in[i].pos))
                {
                    for (j = sort_idx - 1; j > insert; j--)
                        //shift [(insert + 1) .. (sort_idx-1)] elements
                    {
                        in_sort[j + 1] = in_sort[j];
                    }

                    in_sort[insert + 1] = in[i];    //pull in insert+1;
                    sort_idx++;
                }
                else
                {
                    // if sample positions. Replace by new one:
                    in_sort[insert] = in[i];    //pull in insert;
                }
            }
        }
    }

    numSample = sort_idx;
    *ValidSampleCount = numSample;

    /*
     HJ normalized algorithm :
     Fv normalized with Gyro info
     */
    //calculate HJ_weight of Fv according to Gyro
    if ((TRUE
            == p_algo_tuning->handJitter.AFSingle.fineSearch.FVNormalized.enable)
            && (TRUE == is_fine_search))
    {
        for (i = 0; i < numSample; i++)
        {
            AF_MSG_LOW("HJ_CAF Before normalized: in_sort[%d] = (%d, %f)",
                       i,
                       in_sort[i].pos,
                       in_sort[i].hw_fv_info[0]);
        }

        switch (p_algo_tuning->handJitter.AFSingle.fineSearch.FVNormalized.weightType)
        {
            case 0:
                {
                    af_single_hj_HJ_LSParasolve_weight(in_sort, numSample, HJ_weight, p_algo_tuning);
                    break;
                }

            case 1:
                {
                    af_single_hj_HJ_LSParasolve_weight_sad(in_sort, numSample, HJ_weight, p_algo_tuning);
                    break;
                }

            default:
                {
                    af_single_hj_HJ_LSParasolve_weight(in_sort, numSample, HJ_weight, p_algo_tuning);
                    break;
                }
        }

        //use HJ_weight to nomalize HJ influece to FV
        for (i = 0; i < numSample; i++)
        {
            if (i == 0 )
            {
                if ((0.5f * HJ_weight[i + 1] + 1.5f * HJ_weight[i]) > 0)
                {
                    in_sort_normlized[i] =
                        ( 0.5f * HJ_weight[i + 1] * in_sort[i + 1].hw_fv_info[0]
                          + 1.5f * HJ_weight[i] * in_sort[i].hw_fv_info[0] )
                        / (0.5f * HJ_weight[i + 1] + 1.5f * HJ_weight[i]);
                }
                else
                {
                    in_sort_normlized[i] = in_sort[i].hw_fv_info[0];
                }
            }
            else if ( i == (numSample - 1) )
            {
                if ((1.5f * HJ_weight[i] + 0.5f * HJ_weight[i - 1]) > 0)
                {
                    in_sort_normlized[i] =
                        ( 1.5f * HJ_weight[i] * in_sort[i].hw_fv_info[0]
                          + 0.5f * HJ_weight[i - 1] * in_sort[i - 1].hw_fv_info[0] )
                        / (1.5f * HJ_weight[i] + 0.5f * HJ_weight[i - 1]);
                }
                else
                {
                    in_sort_normlized[i] = in_sort[i].hw_fv_info[0];
                }
            }
            else
            {
                if ((0.5f * HJ_weight[i + 1] + 1.0f * HJ_weight[i] + 0.5f * HJ_weight[i - 1]) > 0)
                {
                    in_sort_normlized[i] =
                        ( 0.5f * HJ_weight[i + 1] * in_sort[i + 1].hw_fv_info[0]
                          + 1.0f * HJ_weight[i] * in_sort[i].hw_fv_info[0]
                          + 0.5f * HJ_weight[i - 1] * in_sort[i - 1].hw_fv_info[0] )
                        / (0.5f * HJ_weight[i + 1] + 1.0f * HJ_weight[i] + 0.5f * HJ_weight[i - 1]);
                }
                else
                {
                    in_sort_normlized[i] = in_sort[i].hw_fv_info[0];
                }
            }
        }
    }

    //Final stats after Sort process
    for (i = 0; i < numSample; i++)
    {
        if ((TRUE
                == p_algo_tuning->handJitter.AFSingle.fineSearch.FVNormalized.enable)
                &&
                (TRUE == is_fine_search))
            in_sort[i].hw_fv_info[0] = in_sort_normlized[i];

        AF_MSG_LOW("HJ_CAF: Final: in_sort[%d] = (%d, %f)",
                   i,
                   in_sort[i].pos,
                   in_sort[i].hw_fv_info[0]);

        if (max < in_sort[i].hw_fv_info[0])
        {
            max = in_sort[i].hw_fv_info[0];
            bestPos = in_sort[i].pos;
            peakIndex = i;
        }

        if (min > in_sort[i].hw_fv_info[0])
        {
            min = in_sort[i].hw_fv_info[0];
        }
    }

    AF_MSG_LOW("HJ_CAF: numSample = %d, min %f, peakIndex %d, "
               "bestPos %d, max %f",
               numSample,
               min,
               peakIndex,
               bestPos,
               max);

    if (min >= max)
    {
        // if FV all the same, pick smallest position:
        return bestPos;
    }

    //  if no valid stats:
    if (*ValidSampleCount < 1)
    {
        return 0;
    }
    else if (*ValidSampleCount < 5)
    {
        if (peakIndex <= numSample / 2)
        {
            firstIndex = MAX(0,
                             (peakIndex - 1));
            lastIndex = MIN((firstIndex + 2),
                            (numSample - 1));
        }
        else
        {
            lastIndex = MIN((peakIndex + 1),
                            (numSample - 1));
            firstIndex = MAX(0,
                             (lastIndex - 2));
        }
    }
    else
    {
        if (peakIndex <= 1)
        {
            firstIndex = MAX(0,
                             (peakIndex - 1));
            lastIndex = MIN((firstIndex + 2),
                            (numSample - 1));
        }
        else if (peakIndex >= numSample - 2)
        {
            lastIndex = MIN((peakIndex + 1),
                            (numSample - 1));
            firstIndex = MAX(0,
                             (lastIndex - 2));
        }
        else if (peakIndex <= numSample / 2)
        {
            firstIndex = MAX(0,
                             (peakIndex - 2));
            lastIndex = MIN((firstIndex + 4),
                            (numSample - 1));
        }
        else
        {
            lastIndex = MIN((peakIndex + 2),
                            (numSample - 1));
            firstIndex = MAX(0,
                             (lastIndex - 4));
        }
    }

    // scale the Fv to range [0, 1.0] for fitting
    scale = 1.0f / (max - min);

    for (i = 0; i < numSample; i++)
    {
        in_sort[i].hw_fv_info[0] = scale * (in_sort[i].hw_fv_info[0] - min);
    }

    af_single_hj_LSParabolicFit(&para,
                                &in_sort[firstIndex],
                                (lastIndex - firstIndex + 1));

    if (para.a >= 0.0f)
    {
        soln = -1;
    }
    else
    {
        soln = (INT) (-para.b / (2 * para.a));
    }

    // if no good fit with 5 sample, redo with 3 sample
    if(((soln <= in_sort[0].pos) ||
            (soln >= in_sort[LSP_SAMPLE_IDX(*ValidSampleCount - 1)].pos))
            && ((lastIndex - firstIndex + 1) > 3))
    {
        AF_MSG_LOW("no good fit soln %d with first_index %d, last_index %d, samples %d, a %f, b %f, c %f, redo with 3 index",
                   soln, firstIndex, lastIndex, *ValidSampleCount, para.a, para.b, para.c);

        if (peakIndex <= numSample / 2)
        {
            firstIndex = GETMAX(0, (peakIndex - 1));
            lastIndex = GETMIN((firstIndex + 2), (numSample - 1));
        }
        else
        {
            lastIndex = GETMIN((peakIndex + 1), (numSample - 1));
            firstIndex = GETMAX(0, (lastIndex - 2));
        }

        af_single_hj_LSParabolicFit(&para, &in_sort[firstIndex], (lastIndex - firstIndex + 1));

        if (para.a >= 0.0f)
        {
            soln = -1;
        }
        else
        {
            soln = (int) (-para.b / (2 * para.a));
        }
    }

    AF_MSG_LOW("HJ_CAF: ValidSample %d, firstIndex %d, lastIndex %d, a %f, b %f, c %f",
               *ValidSampleCount, firstIndex, lastIndex, para.a, para.b, para.c);
    // update LSP info debug data
    lsp_info->first_index = (UINT8) firstIndex;
    lsp_info->last_index = (UINT8) lastIndex;
    lsp_info->start_pos = (UINT16) in_sort[0].pos;
    lsp_info->end_pos = (UINT16) in_sort[*ValidSampleCount - 1].pos;
    lsp_info->coeff_A = (FLOAT) para.a;
    lsp_info->coeff_B = (FLOAT) para.b;
    lsp_info->coeff_C = (FLOAT) para.c;
    return soln;
}

VOID af_single_hj_reset_LSPAFsweepSample(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    LSP_Process* LSP_AF_sample = &saf->LSP_AF_sample;
    Q3A_MEMSET(LSP_AF_sample->ToN_sample,
               0,
               AF_COLLECTION_POINTS * sizeof(LSP_AFSweepSample));
    Q3A_MEMSET(LSP_AF_sample->ToF_sample,
               0,
               AF_COLLECTION_POINTS * sizeof(LSP_AFSweepSample));
    Q3A_MEMSET(LSP_AF_sample->sampleSort,
               0,
               AF_COLLECTION_POINTS * sizeof(LSP_AFSweepSample));
    LSP_AF_sample->ToN_sampleCount = 0;
    LSP_AF_sample->ToF_sampleCount = 0;
    LSP_AF_sample->ToN_HJ_sampleCount = 0;
    LSP_AF_sample->ToF_HJ_sampleCount = 0;
    LSP_AF_sample->LSP_Max_Pos = 0;
    LSP_AF_sample->Continous_HJ_Cnt = 0;
    LSP_AF_sample->is_good_fit = FALSE;
    //LSP_AF_sample->is_need_HJ_Process = FALSE;
}

BOOL af_single_hj_get_LSParaPeak(
    LSP_AFSweepSample* in,
    LSP_AFSweepSample* in_sort,
    INT sampleSize,
    INT min_validSize,
    INT* ValidSampleCnt,
    LSP_AFSweepSample* Fitout,
    BOOL is_fine_search,
    af_single_hj_LSP_info_t* lsp_info,
    chromatixAFAlgoTuningType* p_algo_tuning)
{
    BOOL Fit_is_found = TRUE;
    Fitout->pos = 0;
    Fitout->pos = af_single_hj_HJ_LSParasolve(in,
                  in_sort,
                  sampleSize,
                  ValidSampleCnt,
                  is_fine_search,
                  lsp_info,
                  p_algo_tuning);
    AF_MSG_LOW("LSP_DBG HJLSP_fv_Get FV=%f,LP=%d sample_cnt=%d Fit_out= %d",
               in[sampleSize - 1].hw_fv_info[0],
               in[sampleSize - 1].pos,
               sampleSize,
               Fitout->pos
              );

    if (*ValidSampleCnt >= min_validSize)
    {
        Fit_is_found =
            (Fitout->pos > in_sort[0].pos)
            &&
            (Fitout->pos
             < in_sort[LSP_SAMPLE_IDX(*ValidSampleCnt - 1)].pos) ?
            TRUE :
            FALSE;
    }
    else
    {
        // Too few samples. (< min_validSize)
        Fit_is_found = FALSE;
    }

    // Update LSP info debug info
    lsp_info->fitting_peak = (UINT16) Fitout->pos;
    lsp_info->fit_is_found = (BOOL) Fit_is_found;
    AF_MSG_LOW("LSP_DBG Final_LSP_fv_Get Fit_is_found=%d,validcnt=%d,"
               "[%d]S_pos=%d,[%d]E_pos=%d",
               Fit_is_found,
               *ValidSampleCnt,
               0,
               in_sort[0].pos,
               LSP_SAMPLE_IDX(*ValidSampleCnt - 1),
               in_sort[LSP_SAMPLE_IDX(*ValidSampleCnt - 1)].pos);
    return Fit_is_found;
}

/** af_single_hj_LSPAF_fv_update_sample: update the incoming sample with gyro
 information
 * p_tmp_cur : current fv
 * p_tmp_pre : previous fv
 *  Return none
 **/
VOID af_single_hj_LSPAF_fv_update_sample(
    af_caf_search_internal_t* p_caf_instance,
    af_single_hj_fv_info_t* fv_info,
    FLOAT* p_fv_cur,
    FLOAT* p_fv_pre)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    chromatixAFAlgoTuningType* p_algo_tuning =
        p_caf_instance->af_tuning->pAlgoTuningData;
    af_caf_gyro_stats_t* gyro_info = &saf->gyro_stats;
    FLOAT gyro_sqr = gyro_info->sqr;
    af_input_from_aec_t* aec_input = &p_caf_instance->aec_input;
    LSP_Process* LSP_AF_sample = &saf->LSP_AF_sample;
    UINT8 criteria_1, criteria_2, criteria_3, is_need_HJ_check;
    FLOAT tmp_cur_gyro, tmp_pre_gyro, tmp_gyro_avgstd;
    FLOAT tmp_gyro_diff_ratio, change_ratio_fv, fv_cur, fv_pre;
    tmp_pre_gyro = fv_info->pre_gyro;
    tmp_cur_gyro = gyro_sqr;
    fv_cur = *p_fv_cur;
    fv_pre = *p_fv_pre;

    if (saf->saf_scan_state == HJ_AF_FINE_SCAN)
    {
        tmp_gyro_diff_ratio =
            ((tmp_cur_gyro > 0)
             &&
             (tmp_pre_gyro
              > p_algo_tuning->handJitter.AFSingle.fineSearch.gyroscopeThresholdForDifferenceRatio)) ?
            (FLOAT)(tmp_cur_gyro / tmp_pre_gyro) : 0;
    }
    else
    {
        tmp_gyro_diff_ratio =
            ((tmp_cur_gyro > 0)
             &&
             (tmp_pre_gyro
              > p_algo_tuning->handJitter.AFSingle.coarseSearch.gyroscopeThresholdForDifferenceRatio)) ?
            (FLOAT)(tmp_cur_gyro / tmp_pre_gyro) : 0;
    }

    tmp_gyro_avgstd = gyro_info->avg_std;
    is_need_HJ_check = FALSE;
    fv_info->is_HJ_stats = FALSE;
    change_ratio_fv = fv_cur / fv_pre;    //gyro Fv sync well
    criteria_1 =
        (saf->saf_scan_state == HJ_AF_FINE_SCAN) ?
        (tmp_cur_gyro
         > p_algo_tuning->handJitter.AFSingle.fineSearch.currentGyroscopeThreshold) :
        (tmp_cur_gyro
         > p_algo_tuning->handJitter.AFSingle.coarseSearch.currentGyroscopeThreshold);
    criteria_2 =
        (saf->saf_scan_state == HJ_AF_FINE_SCAN) ?
        (tmp_gyro_avgstd
         > p_algo_tuning->handJitter.AFSingle.fineSearch.currentGyroscopeSTDThreshold) :
        (tmp_gyro_avgstd
         > p_algo_tuning->handJitter.AFSingle.coarseSearch.currentGyroscopeThreshold);
    criteria_3 =
        (saf->saf_scan_state == HJ_AF_FINE_SCAN) ?
        (tmp_gyro_diff_ratio
         > p_algo_tuning->handJitter.AFSingle.fineSearch.gyroscodeDifferenceRatioThreshold) :
        (tmp_gyro_diff_ratio
         > p_algo_tuning->handJitter.AFSingle.coarseSearch.gyroscodeDifferenceRatioThreshold);

    if ((criteria_1 || criteria_2 || criteria_3) &&
            (aec_input->exp_index > p_algo_tuning->handJitter.AFSingle.workingExposureIndex))
    {
        is_need_HJ_check = TRUE;
    }

    AF_MSG_LOW("LSP_DBG fv_update Criteria_1 %d Criteria_2 %d Criteria_3 %d "
               "is_need_HJ_check %d ",
               criteria_1,
               criteria_2,
               criteria_3,
               is_need_HJ_check);
    AF_MSG_LOW("LSP_DBG fv_update (Pre_Gyro,Cur_Gyro),avg_Gyro_std = %f %f %f",
               tmp_pre_gyro,
               tmp_cur_gyro,
               tmp_gyro_avgstd);
    AF_MSG_LOW("LSP_DBG fv_update  saf->scan_cnt[%d]FV(cur,pre)= %f %f",
               saf->scan_cnt,
               *p_fv_cur,
               *p_fv_pre);

    if ((fv_info->fv_pre != 0) && (TRUE == is_need_HJ_check)
            && (saf->scan_cnt >= 1))
    {
        // need get INTeval update when reverse
        if (change_ratio_fv < 0.995f)
        {
            *p_fv_cur = *p_fv_pre;
            fv_info->is_HJ_stats = TRUE;
        }
    }

    if (TRUE == fv_info->is_HJ_stats)
        LSP_AF_sample->Continous_HJ_Cnt++;
    else
        LSP_AF_sample->Continous_HJ_Cnt = 0;

    if ((LSP_AF_sample->Continous_HJ_Cnt
            >= p_algo_tuning->handJitter.AFSingle.handleNeedCount)
            &&
            (aec_input->exp_index
             > p_algo_tuning->handJitter.AFSingle.workingExposureIndex))
        LSP_AF_sample->is_need_HJ_Process = TRUE;

    AF_MSG_LOW("LSP_DBG fv_update_HJ_Process Gyro_diff_ratio %f "
               "fv change ratio %f Fv(cur,pre)=%f,%f,exp_idx=%d,isHJ_stats=%d",
               tmp_gyro_diff_ratio,
               change_ratio_fv,
               *p_fv_cur,
               *p_fv_pre,
               aec_input->exp_index,
               fv_info->is_HJ_stats);
    AF_MSG_LOW("LSP_DBG fv_update_HJ_Process2 Continous_HJ_Cnt=%d,"
               "HJ_enable %d,is_need_HJ=%d",
               LSP_AF_sample->Continous_HJ_Cnt,
               p_algo_tuning->handJitter.AFSingle.enable,
               LSP_AF_sample->is_need_HJ_Process);
    fv_info->pre_gyro = tmp_cur_gyro;
    fv_info->gyro_diff_ratio = tmp_gyro_diff_ratio;
    fv_info->change_ratio_fv = change_ratio_fv;
    fv_info->exp_index = (UINT16) aec_input->exp_index;
}
//End of Hand Jitter related functions

/** af_single_hj_get_bv_index: Get BV level from the current gain to
 *  tuning gain threshold
 *
 *  @af: internal AF data structure
 *  @bv: input BV value
 **/
UINT8 af_single_hj_get_bv_index(
    af_caf_search_internal_t* p_caf_instance)
{
    chromatixContrastAFType* pContrastTuningData =
        p_caf_instance->af_tuning->pContrastTuningData;
    FLOAT bv = p_caf_instance->aec_input.Bv_af;
    UINT8 bv_index;

    if (bv >= pContrastTuningData->singleSearch.BVGain[_SKY_LX])
        bv_index = _SKY_LX;
#ifdef VENDOR_EDIT
    else if (bv <= pContrastTuningData->singleSearch.BVGain[_0LX])
        bv_index = _0LX;
#endif
    else if (bv <= pContrastTuningData->singleSearch.BVGain[_20LX])
        bv_index = _20LX;
    else if (bv <= pContrastTuningData->singleSearch.BVGain[_50LX])
        bv_index = _50LX;
    else if (bv <= pContrastTuningData->singleSearch.BVGain[_100LX])
        bv_index = _100LX;
    else if (bv <= pContrastTuningData->singleSearch.BVGain[_400LX])
        bv_index = _400LX;
    else if (bv <= pContrastTuningData->singleSearch.BVGain[_700LX])
        bv_index = _700LX;
    else
        bv_index = _OUTDOOR_LX;

    AF_MSG_LOW("apex bv %f, BV_index = %d",
               bv,
               bv_index);
    return bv_index;
} // af_single_hj_get_bv_index

VOID af_single_hj_return_result(
    af_caf_search_internal_t* p_caf_instance,
    af_single_hj_result result)
{
    af_single_hj_t* af = p_caf_instance->af_single;
    af_single_hj_lib_info_t* saf = &af->af_hj_lib_info;
    AF_MSG_LOW("Arriving Final position, ALGO Complete");
    // Reset First Run Flag
    saf->af_run_first_time = FALSE;
    AF_MSG_LOW("%s, final_pos %d, MaxPos %d ",
               (result == HJ_AF_SINGLE_SUCCESS ? "AF_SUCCESS" : "AF_FAILED"),
               saf->final_pos,
               saf->af_index[saf->fv_info.max_index]);
#ifdef VENDOR_EDIT
    ALOGI("[AFDBG] %s %s, final_pos %d, MaxPos %d", __func__,
               (result == HJ_AF_SINGLE_SUCCESS ? "AF_SUCCESS" : "AF_FAILED"),
               saf->final_pos,
               saf->af_index[saf->fv_info.max_index]);
#endif
    af->output.progress = AF_PROGRESS_100_DONE;
    af->output.confidence =
        result == HJ_AF_SINGLE_SUCCESS ?
        AF_CONFIDENCE_HIGH :
        AF_CONFIDENCE_LOW;
    return;
} // af_single_hj_return_result

/** af_single_hj_get_scanstep: Get scan-step in reference position.
 *
 *  @ref_pos: Reference Focus Position
 *  @af: internal AF data structure
 *
 **/
VOID af_single_hj_get_scan_step(
    INT32 ref_pos,
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    LSP_Process* LSP_AF_sample = &saf->LSP_AF_sample;
    chromatixAFTuningSingle* p_tuning =
        &p_caf_instance->af_tuning->pContrastTuningData->singleSearch;
    INT32 hyp_pos, pre_scan_step, fine_scan_step, step_per_um;
    FLOAT bv;
    BOOL is_beyond_end_pre = FALSE;
    BOOL is_beyond_end_fine = FALSE;
    // choose table by search type CAF : TAF
    BOOL is_af_caf = saf->is_af_caf;
    chromatixStepSizeTable* step_table =
        (is_af_caf == TRUE) ?
        &p_tuning->CAFStepTable :
        &p_tuning->TAFStepTable;
    //Get bv & hyperfocal position
    bv = p_caf_instance->aec_input.Bv_af;
    hyp_pos = p_tuning->index[AF_HYP_F_IDX];

    if (bv > p_tuning->BVGain[_100LX])
    {
        //Scan step for normal lux
        if (ref_pos > hyp_pos)
        {
            pre_scan_step = step_table->prescanNormalLight.region0;
            fine_scan_step = step_table->finescanNormalLight.region0;
        }
        else if (ref_pos >= saf->reg1_thrd)    //50cm
        {
            pre_scan_step = step_table->prescanNormalLight.region1;
            fine_scan_step = step_table->finescanNormalLight.region1;
        }
        else if (ref_pos >= saf->reg2_thrd)
        {
            pre_scan_step = step_table->prescanNormalLight.region2;
            fine_scan_step = step_table->finescanNormalLight.region2;
        }
        else if (ref_pos >= saf->reg3_thrd)
        {
            pre_scan_step = step_table->prescanNormalLight.region3;
            fine_scan_step = step_table->finescanNormalLight.region3;
        }
        else
        {
            pre_scan_step = step_table->prescanNormalLight.region4;
            fine_scan_step = step_table->finescanNormalLight.region4;
        }
    }
    else
    {
        if (ref_pos > hyp_pos)
        {
            pre_scan_step = step_table->prescanLowLight.region0;
            fine_scan_step = step_table->finescanLowLight.region0;
        }
        else if (ref_pos >= saf->reg1_thrd)
        {
            pre_scan_step = step_table->prescanLowLight.region1;
            fine_scan_step = step_table->finescanLowLight.region1;
        }
        else if (ref_pos >= saf->reg2_thrd)
        {
            pre_scan_step = step_table->prescanLowLight.region2;
            fine_scan_step = step_table->finescanLowLight.region2;
        }
        else if (ref_pos >= saf->reg3_thrd)
        {
            pre_scan_step = step_table->prescanLowLight.region3;
            fine_scan_step = step_table->finescanLowLight.region3;
        }
        else
        {
            pre_scan_step = step_table->prescanLowLight.region4;
            fine_scan_step = step_table->finescanLowLight.region4;
        }
    }

    // Map step to step_index_per_um
    step_per_um = p_tuning->stepIndexPerMicroMeter;
    saf->pre_scan_step = _step_index_for_um(pre_scan_step,
                                            step_per_um);

    if (TRUE == LSP_AF_sample->is_need_HJ_Process &&
            TRUE == p_caf_instance->is_hj_enable)
    {
        saf->fine_scan_step = _step_index_for_um(pre_scan_step,
                              step_per_um)
                              / 2;
    }
    else
    {
        saf->fine_scan_step = _step_index_for_um(fine_scan_step,
                              step_per_um);
    }

    AF_MSG_LOW("LSP step size: isHJ=%d 1step pre-scan %d, "
               "finescan %d step_per_um %d",
               LSP_AF_sample->is_need_HJ_Process,
               saf->pre_scan_step,
               saf->fine_scan_step,
               step_per_um);
} // af_single_hj_get_scan_step

/** af_single_hj_util_move_lens: To issue a move lens command to
 *  af_port after house keeping
 *
 *  @af: internal AF data structure
 *  @dir: move direction
 *  @steps: move steps, Should ALWAYS BE Positive
 **/
VOID af_single_hj_util_move_lens(
    af_caf_search_internal_t* p_caf_instance,
    af_move_direction_type dir,
    INT steps)
{
    af_single_hj_t* af = p_caf_instance->af_single;
    af_single_hj_lib_info_t *saf = &af->af_hj_lib_info;
    chromatixAFTuningSingle* p_tuning =
        &p_caf_instance->af_tuning->pContrastTuningData->singleSearch;
    UINT16 full_range;
    INT range_small, range_large, cur_pos, next_pos;
    INT far_limit, near_limit;
    far_limit = saf->far_end_pos;
    near_limit = saf->near_end_pos;

    if (steps < 0)
        AF_MSG_LOW("Error! Move Step should not be negative!");

    full_range = (UINT16) abs(far_limit - near_limit);
    range_large = (INT)((full_range) / 3.0f * 2);
    range_small = (full_range) >> 1;

    /* If move range is too large, to ensure reliable FV,
     we will add frame delays */
    if (steps > range_large)
    {
        AF_MSG_LOW("Jumping pass 2/3 of the range, set %d frame Delay",
                   p_tuning->frameDelay.coarseFrameDelay);
        af->af_hj_lib_info.frame_skip = p_tuning->frameDelay.coarseFrameDelay;
    }
    else if (steps > range_small)
    {
        AF_MSG_LOW("Jumping pass 1/2 of the range, set %d frame Delay",
                   p_tuning->frameDelay.baseFrameDelay);
        af->af_hj_lib_info.frame_skip = p_tuning->frameDelay.baseFrameDelay;
    }
    else
    {
        AF_MSG_LOW("Base frame delay: %d",
                   p_tuning->frameDelay.baseFrameDelay);
        af->af_hj_lib_info.frame_skip = p_tuning->frameDelay.baseFrameDelay;
    }

    cur_pos = p_caf_instance->cur_pos;
    next_pos = (dir == AF_MOVE_FAR) ? cur_pos + steps : cur_pos - steps;
    next_pos = next_pos >= far_limit ? far_limit : next_pos;
    next_pos = next_pos <= near_limit ? near_limit : next_pos;
    af->output.search_dir = dir;
    af->output.next_pos = next_pos;
    af->output.fine_step_size = steps;
    AF_MSG_LOW("Moving to %d from %d frame_skip: %d",
               af->output.next_pos,
               cur_pos,
               af->af_hj_lib_info.frame_skip);
} // af_single_hj_util_move_lens

/** af_single_hj_get_scan_step_INTerpolate: Get scan-step in
 *  reference position.
 *
 *  @ref_pos: Reference Focus Position
 *  @af: internal AF data structure
 *  @ref_pos: number of steps to take from reference position
 *  @ref_pos: direction from the Reference position input
 *
 **/
VOID af_single_hj_get_scan_step_INTerpolate(
    INT ref_pos,
    af_caf_search_internal_t* p_caf_instance,
    INT num_step,
    UINT8 direction_from_ref_pos)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    INT32 temp_pre, temp_fine, scan_steps, dest_pos;
    INT32 INTerpolated_pre, INTerpolated_fine;
    AF_MSG_LOW("INPUT: num_step %d ref_pos %d",
               num_step,
               ref_pos);
    af_single_hj_get_scan_step(ref_pos,
                               p_caf_instance);
    temp_pre = saf->pre_scan_step;
    temp_fine = saf->fine_scan_step;
    scan_steps = temp_pre * num_step;
    // Get step from region in destination position
    dest_pos = (direction_from_ref_pos == DIR_TO_FAR) ?
               ref_pos + scan_steps :
               ref_pos - scan_steps;

    if (direction_from_ref_pos == DIR_TO_FAR)
    {
        if (dest_pos > saf->far_end_pos)
        {
            dest_pos = saf->far_end_pos;
        }
    }
    else
    {
        if (dest_pos < saf->near_end_pos)
        {
            dest_pos = saf->near_end_pos;
        }
    }

    af_single_hj_get_scan_step(dest_pos,
                               p_caf_instance);
    temp_pre += saf->pre_scan_step;
    temp_fine += saf->fine_scan_step;
    INTerpolated_pre = temp_pre >> 1;
    INTerpolated_fine = temp_fine >> 1;

    if (num_step > 0)
    {
        INTerpolated_pre *= num_step;
        INTerpolated_fine *= num_step;
    }

    saf->fine_scan_step = INTerpolated_fine;
    saf->pre_scan_step = INTerpolated_pre;
    AF_MSG_LOW("6. pre_scan_step %d, fine_scan_step %d",
               saf->pre_scan_step,
               saf->fine_scan_step);
}

//*******************************
//Flat curve detection functions
FLOAT GetBilateralG(
    unsigned long c,
    unsigned long nei)
{
    if (nei == 0)
        return 0.0;

    INT diff = (INT)(nei - c);
    INT diffAbs = diff;
    FLOAT g;

    if (diff >= 0)
        diffAbs = diff;
    else
        diffAbs = -diff;

    FLOAT diffratio = (FLOAT) diffAbs / (FLOAT) nei;
    FLOAT thr_low = BILATERAL_THR_LOW;
    FLOAT thr_hi = BILATERAL_THR_HIGH;

    if (diffratio < thr_low)
        g = 1.0;
    else if (diffratio > thr_hi)
        g = 0.0;
    else
    {
        if ((thr_hi - thr_low) == 0)
            g = 0.0;
        else
            g = (thr_hi - diffratio) / (thr_hi - thr_low);
    }

    return g;
}

FLOAT GetBilateral3(
    FLOAT left,
    FLOAT c,
    FLOAT right)
{
    FLOAT left_G, right_G, denom, filterOut;
    left_G = GetBilateralG((unsigned long) c,
                           (unsigned long) left);
    right_G = GetBilateralG((unsigned long) c,
                            (unsigned long) right);
    denom = 3.0;

    if (left == 0)
        denom -= 1.0;

    if (right == 0)
        denom -= 1.0;

    filterOut = c + left_G * (left - c) / denom + right_G * (right - c) / denom;
    return filterOut;
}

FLOAT GetFilterOutput(
    INT n,
    INT FilterStart,
    INT FilterEnd,
    af_single_hj_filter_info* filterIn)
{
    FLOAT filterOut;

    if (n == 0)
        return filterIn[0].fv_val;

    if (filterIn[n].fv_val == 0)
        return 0;

    if (filterIn[n - 1].fv_val == 0)
        return filterIn[n].fv_val;

    if (n == FilterStart)
        filterOut = GetBilateral3(0,
                                  filterIn[n].fv_val,
                                  filterIn[n + 1].fv_val);
    else if (n == FilterEnd)
        filterOut = GetBilateral3(filterIn[n - 1].fv_val,
                                  filterIn[n].fv_val,
                                  0);
    else
        filterOut = GetBilateral3(filterIn[n - 1].fv_val,
                                  filterIn[n].fv_val,
                                  filterIn[n + 1].fv_val);

    return filterOut;
}

BOOL GetNormalizedAB(
    FLOAT refx,
    FLOAT refy,
    FLOAT sx,
    FLOAT sy,
    FLOAT ex,
    FLOAT ey,
    FLOAT* line_a,
    FLOAT* line_b)
{
    if (ex == sx)
    {
        *line_a = 1.0;
        *line_b = 0.0;
        return FALSE;
    }

    if (refx <= 0.0)
        refx = 1.0;

    if (refy <= 0.0)
        refy = 1.0;

    sx = sx / refx;
    sy = sy / refy;
    ex = ex / refx;
    ey = ey / refy;
    FLOAT a = (ey - sy) / (ex - sx);
    FLOAT b = sy - a * sx;
    *line_a = a;
    *line_b = b;
    return TRUE;
}

BOOL af_single_hj_flat_detect(
    af_single_hj_filter_info* filterOut,
    INT FilterStart,
    INT FilterEnd)
{
    INT x1, x2, k;
    FLOAT refx = (FLOAT) filterOut[FilterStart].pos;
    FLOAT refy = filterOut[FilterStart].fv_val;
    FLOAT sx = (FLOAT) filterOut[FilterStart].pos;
    FLOAT sy = filterOut[FilterStart].fv_val;
    FLOAT ex = (FLOAT) filterOut[FilterEnd].pos;
    FLOAT ey = filterOut[FilterEnd].fv_val;

    if (refx == 0.0)
        return FALSE;

    if (refy == 0.0)
        return FALSE;

    if (sx == ex)
        return FALSE;

    FLOAT a1, b1, a2, b2, a3, b3;
    INT x1_point = 0;
    INT x2_point = 0;
    FLOAT diff_sum_backup = 10000;
    FLOAT diff_sum = 0.0;
    FLOAT mx, my, hx, hy, Yorg, diff;

    for (x1 = FilterStart + 1; x1 <= FilterEnd - 2; x1++)
    {
        mx = (FLOAT) filterOut[x1].pos;
        my = filterOut[x1].fv_val;

        if (GetNormalizedAB(refx,
                            refy,
                            sx,
                            sy,
                            mx,
                            my,
                            &a1,
                            &b1) == 0)
            return FALSE;

        for (x2 = x1 + 1; x2 <= FilterEnd - 1; x2++)
        {
            hx = (FLOAT) filterOut[x2].pos;
            hy = filterOut[x2].fv_val;

            if (GetNormalizedAB(refx,
                                refy,
                                mx,
                                my,
                                hx,
                                hy,
                                &a2,
                                &b2) == 0)
                return FALSE;

            if (GetNormalizedAB(refx,
                                refy,
                                hx,
                                hy,
                                ex,
                                ey,
                                &a3,
                                &b3) == 0)
                return FALSE;

            diff_sum = 0.0;

            for (k = FilterStart; k <= FilterEnd; k++)
            {
                FLOAT X = (FLOAT) filterOut[k].pos / refx;
                FLOAT Y;
                // INT eq = 0;

                if (k <= x1)
                {
                    // eq = 1;
                    Y = a1 * X + b1;
                }
                else if (k <= x2)
                {
                    // eq = 2;
                    Y = a2 * X + b2;
                }
                else
                {
                    // eq = 3;
                    Y = a3 * X + b3;
                }

                Yorg = (FLOAT) filterOut[k].fv_val / refy;
                diff = (FLOAT)(Yorg - Y);

                if (diff < 0)
                    diff = -diff;

                diff_sum += diff;
            }

            if (diff_sum_backup > diff_sum)
            {
                x1_point = x1;
                x2_point = x2;
                diff_sum_backup = diff_sum;
            }
        }
    }

    mx = (FLOAT) filterOut[x1_point].pos;
    my = (FLOAT) filterOut[x1_point].fv_val;
    hx = (FLOAT) filterOut[x2_point].pos;
    hy = (FLOAT) filterOut[x2_point].fv_val;

    if (GetNormalizedAB(refx,
                        refy,
                        sx,
                        sy,
                        mx,
                        my,
                        &a1,
                        &b1) == 0)
        return FALSE;

    if (GetNormalizedAB(refx,
                        refy,
                        mx,
                        my,
                        hx,
                        hy,
                        &a2,
                        &b2) == 0)
        return FALSE;

    if (GetNormalizedAB(refx,
                        refy,
                        hx,
                        hy,
                        ex,
                        ey,
                        &a3,
                        &b3) == 0)
        return FALSE;

    FLOAT a21 = a2 - a1;
    FLOAT a32 = a3 - a2;
    FLOAT a31 = a3 - a1;
    INT slope, accel;
    slope = ((a1 < SLOPE_HIGH) && (a2 < SLOPE_HIGH) && (a3 < SLOPE_HIGH) &&
             (a1 > SLOPE_LOW) && (a2 > SLOPE_LOW) && (a3 > SLOPE_LOW)) ?
            TRUE :
            FALSE;
    AF_MSG_LOW("slope %s : a1 %f, a2 %f, a3 %f",
               slope ? "TRUE" : "FALSE",
               a1,
               a2,
               a3);
    accel =
        ((a21 < ACCEL_HIGH) && (a32 < ACCEL_HIGH) && (a31 < ACCEL_HIGH) &&
         (a21 > ACCEL_LOW) && (a32 > ACCEL_LOW) && (a31 > ACCEL_LOW)) ?
        TRUE :
        FALSE;
    AF_MSG_LOW("accel %s : a21 %f, a32 %f, a31 %f",
               accel ? "TRUE" : "FALSE",
               a21,
               a32,
               a31);

    if (slope && accel)
        return TRUE;
    else
        return FALSE;
}

BOOL af_single_hj_flat_curve_detection_by_bilateral
(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    af_single_hj_fv_info_t* fv_info = &saf->fv_info;
    //AF_COLLECTION_POINTS+10 : to prevent overflow by FilterEnd+3
    af_single_hj_filter_info filter_in[AF_COLLECTION_POINTS + 10] =
    {
        { 0, 0 }
    };
    af_single_hj_filter_info filter_out[AF_COLLECTION_POINTS + 10];
    INT i, filter_start, filter_end;
    FLOAT filter;
    BOOL detect;
    filter_start = saf->start_index;
    filter_end = saf->stats_index;

    if (filter_end >= AF_COLLECTION_POINTS)
        return FALSE;

    for (i = 0; i <= filter_end; i++)
    {
        filter_in[i].pos = saf->af_index[i];
        filter_in[i].fv_val = fv_info->fv_val[i];
        AF_MSG_LOW("1 pos %d, fv %f",
                   filter_in[i].pos,
                   filter_in[i].fv_val);
    }

    for (i = 0; i <= filter_end + 3; i++)
    {
        filter = GetFilterOutput(i,
                                 filter_start,
                                 filter_end,
                                 filter_in);

        if (i <= filter_end)
        {
            filter_out[i].pos = saf->af_index[i];
            filter_out[i].fv_val = filter;
        }
        else
        {
            filter_out[i].pos = 0;
            filter_out[i].fv_val = 0;
        }

        AF_MSG_LOW("2 pos %d, filter %f",
                   filter_out[i].pos,
                   filter_out[i].fv_val);
    }

    detect = af_single_hj_flat_detect(filter_out,
                                      filter_start,
                                      filter_end);
    return detect;
}

/** af_single_hj_get_flat_ratio_threshold: Get Flat Ratio from BV
 *  level
 *
 *  @ref_pos: Reference Focus Position
 *  @af: internal AF data structure
 *
 **/
FLOAT af_single_hj_get_flat_ratio_threshold(
    af_caf_search_internal_t* p_caf_instance,
    UINT8 bv_index)
{
    FLOAT flat_threshold = 0.0;

    if (p_caf_instance != NULL)
    {
        af_single_hj_lib_info_t* saf =
            &p_caf_instance->af_single->af_hj_lib_info;

        if (saf != NULL)
        {
            chromatixSingleThreshold* thres = saf->p_thres;

            if (thres != NULL)
            {
                if (bv_index >= 0 && bv_index < AF_BV_COUNT)
                {
                    flat_threshold = thres->flatThreshold.Threshold[bv_index];
                }
            }
        }
    }

    AF_MSG_LOW("flat_threshold %f",
               flat_threshold);
    return flat_threshold;
} // af_single_hj_get_flat_ratio_threshold

/** af_single_hj_get_spotlight_flat_threshold: get spotlight flat threshuld
 *
 *  @af: internal AF data structure
 **/
FLOAT af_single_hj_get_spotlight_flat_threshold(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    chromatixAFTuningSingle* p_tuning =
        &p_caf_instance->af_tuning->pContrastTuningData->singleSearch;
    INT max_index, mid_zone_pos;
    INT spotlight_far_zone_pos, spotlight_near_zone_pos;
    FLOAT spot_light_flat_threshold;
    max_index = saf->fv_info.max_index;
    mid_zone_pos = p_tuning->index[p_tuning->optics.midZone];
    //default step response far,near zone = mid_zone_pos
    spotlight_far_zone_pos = mid_zone_pos;
    spotlight_near_zone_pos = mid_zone_pos;

    if (saf->af_index[max_index] < spotlight_near_zone_pos)
    {
        spot_light_flat_threshold = AF_SPOT_LIGHT_MARCO_FLAT_THRS;
    }
    else if (saf->af_index[max_index] >= spotlight_far_zone_pos)
    {
        spot_light_flat_threshold = AF_SPOT_LIGHT_INF_FLAT_THRS;
    }
    else
    {
        spot_light_flat_threshold = (FLOAT)((AF_SPOT_LIGHT_INF_FLAT_THRS -
                                             AF_SPOT_LIGHT_MARCO_FLAT_THRS) * (saf->af_index[max_index] -
                                                     spotlight_near_zone_pos) / (spotlight_far_zone_pos -
                                                             spotlight_near_zone_pos) + AF_SPOT_LIGHT_MARCO_FLAT_THRS);
    }

    return spot_light_flat_threshold;
}

/** af_single_hj_is_scan_end_pre: Determin if scan is reaching end
 *  from search range given from tuning data
 *
 *  @af: internal AF data structure
 **/
UINT8 af_single_hj_is_scan_end_pre(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    INT stats_pos = saf->af_index[saf->stats_index];
    UINT8 Rtn;
    Rtn = (((saf->direction == DIR_TO_NEAR) &&
            (stats_pos <= saf->near_end_pos)) ||
           ((saf->direction == DIR_TO_FAR) &&
            (stats_pos >= saf->far_end_pos))) ?
          TRUE :
          FALSE;
    AF_MSG_LOW("stats_pos %d, Far_Pos %d, Near_pos %d, direction ? %s, "
               "Prescan end ? %s",
               stats_pos,
               saf->far_end_pos,
               saf->near_end_pos,
               (saf->direction ? "Far" : "Near"),
               (Rtn ? "Yes" : "No"));
    return Rtn;
} // af_single_hj_is_scan_end_pre

/** af_single_hj_flat_curve_detection_by_fratio: detecting if it is flat curve
 *
 *  @af: internal AF data structure
 **/
BOOL af_single_hj_flat_curve_detection_by_fratio(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    af_single_hj_fv_info_t* fv_info = &saf->fv_info;
    chromatixAFTuningSingle* p_tuning =
        &p_caf_instance->af_tuning->pContrastTuningData->singleSearch;
    UINT8 bv_index;
    FLOAT min_max_ratio, flat_threshold;
    BOOL is_flat_curve;
    UINT8 is_scanned_e2e_far, is_scanned_e2e_near, is_started_in_mid_zone;
    UINT8 criteria_1, criteria_2;
    INT32 stats_pos;
    INT32 far_zone_pos, near_zone_pos, mid_zone_pos;
    stats_pos = saf->af_index[saf->stats_index];
    far_zone_pos = p_tuning->index[p_tuning->optics.farZone];
    near_zone_pos = p_tuning->index[p_tuning->optics.nearZone];
    mid_zone_pos = p_tuning->index[p_tuning->optics.midZone];
    BOOL is_spotlight_scene = IS_SPOTLIGHT_SCENE;

    if (is_spotlight_scene)
    {
        flat_threshold =
            af_single_hj_get_spotlight_flat_threshold(p_caf_instance);
    }
    else
    {
        bv_index = af_single_hj_get_bv_index(p_caf_instance);
        flat_threshold =
            af_single_hj_get_flat_ratio_threshold(p_caf_instance,
                    bv_index);
    }

    min_max_ratio = fv_info->fv_min / fv_info->fv_max;
    // Check if Start and currently in both either Far or Near Zone
    is_started_in_mid_zone = (saf->start_pos > near_zone_pos &&
                              saf->start_pos < far_zone_pos);
    is_scanned_e2e_far = ((saf->start_pos
                           <= (INT32) p_tuning->index[AF_20CM_IDX]) &&
                          (stats_pos >= far_zone_pos) &&
                          (saf->direction == DIR_TO_FAR));
    is_scanned_e2e_near = ((saf->start_pos >= far_zone_pos) &&
                           (stats_pos <= (INT32) p_tuning->index[AF_10CM_IDX]) &&
                           (saf->direction == DIR_TO_NEAR));
    criteria_1 = (min_max_ratio > flat_threshold) ? TRUE : FALSE;
    criteria_2 = ((is_scanned_e2e_far || is_scanned_e2e_near) ||
                  (is_started_in_mid_zone &&
                   af_single_hj_is_scan_end_pre(p_caf_instance) == TRUE));
    is_flat_curve = criteria_1 && criteria_2;
    AF_MSG_LOW("is_started_in_mid_zone %d start_pos %d stats_pos %d",
               is_started_in_mid_zone,
               saf->start_pos,
               stats_pos);
    AF_MSG_LOW("far_zone_pos %d near_zone_pos %d mid_zone_pos %d",
               far_zone_pos,
               near_zone_pos,
               mid_zone_pos);
    AF_MSG_LOW("is_spotlight_scene %d flat_threshold %f",
               is_spotlight_scene,
               flat_threshold);
    AF_MSG_LOW("fv_min %f fv_max %f min_max_ratio %f",
               fv_info->fv_min,
               fv_info->fv_max,
               min_max_ratio);
#ifdef VENDOR_EDIT
    AF_MSG_LOW("criteria_1 %d criteria_2 %d is_flat_curve %d",
               criteria_1,
               criteria_2,
               is_flat_curve);
#else
    AF_MSG_LOW("criteria_1 %d criteria_2 %d is_flat_curve %d",
               is_flat_curve,
               criteria_1,
               criteria_2);
#endif
    return is_flat_curve;
}

//End of flat curve detection functions

/** af_single_hj_util_move_to_dest: to move lens to destination position
 *  depending on movement and mode.
 *
 *  @af: internal AF data structure
 *  @dest: destination position
 **/
UINT8 af_single_hj_util_move_to_dest(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    INT move_steps, next_pos, cur_pos, dest;
    UINT8 dir, is_last_move;
    cur_pos = p_caf_instance->cur_pos;
    dest = saf->dest_pos;
    is_last_move = FALSE;
    BOOL is_af_caf = saf->is_af_caf;

    // Save current state if state is not go to destination
    if (saf->saf_scan_state != HJ_AF_GOTO_DEST)
    {
        saf->saf_scan_state_prev = saf->saf_scan_state;
    }

    // Move to Lens to destination
    move_steps = abs((INT)(dest - cur_pos));
    dir = (UINT8) ((dest > cur_pos) ? AF_MOVE_FAR : AF_MOVE_NEAR);

#ifdef VENDOR_EDIT
    //Prevent cur_pos in the limit end, direction not change
        if (cur_pos == p_caf_instance->af_range.near_end){
            dir = AF_MOVE_FAR;
        } else if (cur_pos == p_caf_instance->af_range.far_end){
            dir = AF_MOVE_NEAR;
        }
#endif

    next_pos = (dir == AF_MOVE_FAR) ?
               cur_pos + move_steps :
               cur_pos - move_steps;

    if (move_steps != 0)
    {
        AF_MSG_LOW("moving lens from %d to %d, Destination %d",
                   cur_pos,
                   next_pos,
                   dest);
        af_single_hj_util_move_lens(p_caf_instance,
                                    (af_move_direction_type) dir,
                                    move_steps);
    }

    if (saf->saf_scan_state_prev == HJ_AF_GOTO_PEAK)
    {
        // Check if Move is completed this Run
        if (move_steps == 0)
        {
            AF_MSG_LOW("Move complete");
            is_last_move = TRUE;

            if (saf->saf_scan_state == HJ_AF_GOTO_DEST)
            {
                AF_MSG_LOW("Recover state! state : %d",
                           saf->saf_scan_state_prev);
                saf->saf_scan_state = saf->saf_scan_state_prev;
                af_single_hj_return_result(p_caf_instance,
                                           saf->af_status);
                return FALSE;
            }
        }
        else if (move_steps == abs((INT)(dest - cur_pos)))
        {
            AF_MSG_LOW("Moving to Dest!!");
            // we can add delay if required.
            is_last_move = FALSE;
        }
    }
    else
    {
        // Check if Move is completed this Run
        if (move_steps == abs((INT)(dest - cur_pos)))
        {
            AF_MSG_LOW("Move complete");
            is_last_move = TRUE;

            if (saf->saf_scan_state == HJ_AF_GOTO_DEST)
            {
                AF_MSG_LOW("Recover state! state : %d",
                           saf->saf_scan_state_prev);
                saf->saf_scan_state = saf->saf_scan_state_prev;
            }
        }
    }

    AF_MSG_LOW("DEBUG check! cur_pos %d, next_pos %d, move_steps %d, dest %d",
               cur_pos,
               next_pos,
               move_steps,
               dest);
    AF_MSG_LOW("DEBUG check! saf scan state prev %d, saf_scan state %d",
               saf->saf_scan_state_prev,
               saf->saf_scan_state);
    return is_last_move;
} // af_single_hj_util_move_to_dest

/** af_single_hj_wait_valid_frame: Wait for Valid frame
 *
 *  @af: internal AF data structure
 **/
VOID af_single_hj_wait_valid_frame(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    af_single_hj_fv_info_t* fv_info = &saf->fv_info;
    INT cur_pos, scan_step;
    cur_pos = p_caf_instance->cur_pos;
    // Alway move lens
    af_single_hj_get_scan_step_INTerpolate(cur_pos,
                                           p_caf_instance,
                                           1,
                                           saf->direction);
    scan_step = (saf->saf_scan_state == HJ_AF_FINE_SCAN) ?
                saf->fine_scan_step :
                saf->pre_scan_step;
    af_single_hj_util_move_lens(p_caf_instance,
                                (af_move_direction_type) saf->direction,
                                scan_step);

    // if HW frame is valid, sw frame also invalid
    if (saf->is_valid_frame == FALSE)
        saf->is_valid_frame_sw = FALSE;

    AF_MSG_LOW("Frame Valid: HW %d , SW %d",
               saf->is_valid_frame,
               saf->is_valid_frame_sw);

    if ((!saf->use_paaf && saf->is_valid_frame) ||
            (saf->use_paaf && saf->is_valid_frame_sw))
    {
        saf->start_index = saf->stats_index;
        saf->scan_cnt++;
    }
    else
    {
        fv_info->fv_pre = 0;
        fv_info->pre_gyro = 0;
        fv_info->gyro_diff_ratio = 0.0f;
        fv_info->change_ratio_fv = 0.0f;
        fv_info->is_HJ_stats = FALSE;
        fv_info->peak_down = 0;

        if (saf->reverse_scan_cnt == 0 ||
                saf->saf_scan_state == HJ_AF_FINE_SCAN)
        {
            fv_info->fv_max = 0;
            fv_info->fv_min = Q3A_FLT_MAX;
        }

        if (saf->is_valid_frame == TRUE)
        {
            AF_MSG_LOW("Frame valid for next sw stats");
            saf->is_valid_frame_sw = TRUE;
        }

        saf->is_valid_frame = TRUE;
    }
} // af_single_hj_wait_valid_frame

/** af_single_hj_end_search: Function to end search and go to
 *  final_pos when the final_pos is determined. Also search result
 *  is passed in.
 *
 *  @af: internal AF data structure
 *  @result: search result
 **/
VOID af_single_hj_end_search(
    af_caf_search_internal_t* p_caf_instance,
    af_single_hj_result result)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    UINT8 move_complete;
    INT wait_frames = Wait_frames_after_goto_peak;
    AF_MSG_LOW("Search complete with %s, going to final position : %d",
               (result == HJ_AF_SINGLE_SUCCESS ? "AF_SUCCESS" : "AF_FAILED"),
               saf->final_pos);
    // Declare af_done_type
    saf->af_status = result;
    // Change Scan State
    saf->saf_scan_state = HJ_AF_GOTO_PEAK;
    // Assign Final position as destination
    saf->dest_pos = saf->final_pos;
    move_complete = af_single_hj_util_move_to_dest(p_caf_instance);

    if (!move_complete)
        saf->scan_cnt = 0;
    else
        saf->scan_cnt++;

    // End Algo if move is completed, otherwise this is called when arrived
    if (move_complete && (saf->scan_cnt > wait_frames))
    {
        af_single_hj_return_result(p_caf_instance,
                                   result);
    }

    return;
} // af_single_hj_end_search

/** af_single_hj_calc_curve_fitting: API for predicting the peak
 *  from the input 3 FV points using 2nd order polynomial.
 *  Assumption : suppose posp > posmax >posn
 *
 *  @fvprev: focus values of one before peak in coarse search
 *  @fvmax:  focus values at peak in coarse search
 *  @fvnext: focus values of one after peak in coarse search
 *  @posp:   Position of previous to peak
 *  @posmax: Position of max
 *  @posn:   Position of next to peak
 *  @ratiomax:
 *  @af: internal AF data structure
 *
 *  Return :  peak value > 1, Failed < 0
 **/

INT af_single_hj_calc_curve_fitting(FLOAT fvprev, FLOAT fvmax, FLOAT fvnext,
    INT posp, INT posmax, INT posn, INT far_end, INT near_end)
{
    /* @a & b: is the coefficient of the quadratic equation
     *         maxima of a prabola equation (ax^2 + bx + c)
     *         is given by - (b / 2a)
     * @fv_diff_mp: focus value diff between max point and previous
     * @fv_diff_np: focus value diff between next point and previous
     * @pos_diff_np: position diff between next point and previous
     * @pos_diff_nm: position diff between next point and max
     * @pos_diff_mp: position diff between max point and previous
     * @pos_sum_mp: position sum between max point and previous
     **/
    FLOAT a, b, peak_f;
    INT peak = -1;
    FLOAT fv_diff_mp, fv_diff_np;
    INT pos_diff_np, pos_diff_nm, pos_diff_mp;
    INT pos_sum_mp;
    const FLOAT peak_coeff = 2;
    const FLOAT peak_offset = 0.5;
    AF_MSG_HIGH(" curve fitting: fvprev, fvmax, fvnext, posp, posmax, posn, "
                "%f %f %f %d %d %d!",
                fvprev,
                fvmax,
                fvnext,
                posp,
                posmax,
                posn);

    if (posp == posmax || posn == posmax || posp == posn)
    {
        AF_MSG_ERROR("curve fitting failed, need fine search!");
        return -1;
    }

    // INTermediate values
    fv_diff_mp = fvmax - fvprev;
    fv_diff_np = fvnext - fvprev;
    pos_diff_np = posn - posp;
    pos_diff_nm = posn - posmax;
    pos_diff_mp = posmax - posp;
    pos_sum_mp = posmax + posp;
    a = ((FLOAT)((fv_diff_np * pos_diff_mp) - (fv_diff_mp * pos_diff_np))) /
        (pos_diff_np * pos_diff_mp * pos_diff_nm);

    if (a > 0)
    {
        AF_MSG_HIGH(" curve fitting failed, a>0");
        peak = -1;
    }
    else
    {
        b = ((FLOAT) fv_diff_mp / pos_diff_mp) - (a * pos_sum_mp);
        peak_f = -b / (a * peak_coeff);
        peak = (INT)(peak_f + peak_offset);
        AF_MSG_HIGH(" curve fitting original peak @ %d , a %f , b %f",
                    peak,
                    a,
                    b);

        if (peak > far_end)
        {
            peak = far_end;
            AF_MSG_HIGH("curve fitting peak adjusted @ %d ",
                        peak);
        }
        else if (peak < near_end)
        {
            peak = near_end;
            AF_MSG_HIGH(" curve fitting peak adjusted @ %d ",
                        peak);
        }
    }

    return peak;
} // af_single_hj_calc_curve_fitting

/** af_single_hj_calc_curve_fitting_peak: Calcuate the peak with curve
 *  fitting method
 *
 *  @af: pointer to internal AF data.
 *
 *  Return position of peak value
 **/
INT af_single_hj_calc_curve_fitting_peak(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    af_single_hj_fv_info_t* fv_info = &saf->fv_info;
    INT af_idx, max_index, offset_from_peak;
    INT peak = -1;
    INT far_end = saf->far_end_pos;
    INT near_end = saf->near_end_pos;
    offset_from_peak = Offset_from_final_peak;
    af_idx = saf->stats_index;
    max_index = fv_info->max_index;
    FLOAT value[] =
    {
        fv_info->fv_val[LSP_SAMPLE_IDX(max_index - 1)],
        fv_info->fv_val[max_index + 0],
        fv_info->fv_val[LSP_SAMPLE_IDX(max_index + 1)]
    };
    INT index[] =
    {
        saf->af_index[LSP_SAMPLE_IDX(max_index - 1)],
        saf->af_index[max_index + 0],
        saf->af_index[LSP_SAMPLE_IDX(max_index + 1)]
    };

    // Check whether eligible >= 3 points
    if (af_idx < 2)
    {
        AF_MSG_LOW("ERROR: 1 or 2 samples, no curve fitting");
    }
    else if (value[0] == 0 || value[1] == 0 || value[2] == 0)
    {
        // check of zero stats
        AF_MSG_LOW("1 or 2 samples, skip curve fitting");
    }
    else if (max_index == 0 || max_index == af_idx)
    {
        AF_MSG_LOW("Max at two ends, no curve fitting");
    }
    else
    {
        AF_MSG_LOW("Max at middle");
        peak = af_single_hj_calc_curve_fitting(value[0],
                                               value[1],
                                               value[2],
                                               index[0],
                                               index[1],
                                               index[2],
                                               far_end,
                                               near_end);
    }

    if (peak == -1)
    {
        AF_MSG_LOW("Invalid curve fitting, moveBackStep uses original");
        peak = saf->af_index[max_index];
    }

    if (offset_from_peak > 0)
    {
        // Manual shift for compensate hysteresis or slow lens movement
        if (saf->direction == DIR_TO_FAR)
        {
            AF_MSG_LOW("Manual shift from final peak, direction : to far, "
                       " shift code : %d",
                       -(INT )offset_from_peak);
            peak -= offset_from_peak;
        }
        else
        {
            AF_MSG_LOW("Manual shift from final peak, direction : to far, "
                       " shift code : %d",
                       offset_from_peak);
            peak += offset_from_peak;
        }
    }

    AF_MSG_LOW("END: orig %d => new %d",
               saf->af_index[max_index],
               peak);
    return peak;
} // af_single_hj_calc_curve_fitting_peak

/** af_single_hj_handle_state_goto_peak: Handle HJ_AF_GOTO_PEAK
 *  Scanstate
 *
 *  @af: internal AF data structure
 **/
VOID af_single_hj_handle_state_goto_peak(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    af_single_hj_end_search(p_caf_instance,
                            saf->af_status);
    return;
} // af_single_hj_handle_state_goto_peak

/** af_single_hj_is_scan_end_fine: Determin if fine scan is
 *  reaching end from fine search range
 *
 *  @af: internal AF data structure
 **/
UINT8 af_single_hj_is_scan_end_fine(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    af_single_hj_fv_info_t* fv_info = &saf->fv_info;
    LSP_Process* LSP_AF_sample = &saf->LSP_AF_sample;
    INT cur_pos, stats_pos, target_pos;
    UINT8 Rtn;
    cur_pos = p_caf_instance->cur_pos;
    stats_pos = saf->af_index[saf->stats_index];
    Rtn = FALSE;
    af_single_hj_get_scan_step_INTerpolate(cur_pos,
                                           p_caf_instance,
                                           1,
                                           saf->direction);

    //fine scan end check by fine scan range
    if (!saf->use_paaf)
    {
        if (saf->direction == DIR_TO_NEAR)
        {
            Rtn = stats_pos < saf->fine_end_pos ? TRUE : FALSE;
        }
        else if (saf->direction == DIR_TO_FAR)
        {
            Rtn = stats_pos > saf->fine_end_pos ? TRUE : FALSE;
        }
    }
    else
    {
        if (saf->direction == DIR_TO_NEAR)
        {
            target_pos = cur_pos - saf->fine_scan_step;
            Rtn = (stats_pos <= saf->fine_end_pos &&
                   target_pos < saf->fine_end_pos) ? TRUE : FALSE;
        }
        else if (saf->direction == DIR_TO_FAR)
        {
            target_pos = cur_pos + saf->fine_scan_step;
            Rtn = (stats_pos >= saf->fine_end_pos &&
                   target_pos > saf->fine_end_pos) ? TRUE : FALSE;
        }
    }

    //fine scan end check by peak down if HJ algorithm is enabled from chromatix or setprop
    if (TRUE == p_caf_instance->is_hj_enable)
    {
        Rtn =
            (TRUE == LSP_AF_sample->is_need_HJ_Process) ?
            Rtn :
            (fv_info->peak_down >= 1 ? TRUE : Rtn);
    }
    else
    {
#ifdef VENDOR_EDIT
        INT peak_down_thr = 1;
        UINT8 bv_index = af_single_hj_get_bv_index(p_caf_instance);

        if( bv_index <= EXTRA_LOW_LIGHT_BV_IDX_THR)
            peak_down_thr = 2;

        Rtn = fv_info->peak_down >= peak_down_thr ? TRUE : Rtn;
#else
        Rtn = fv_info->peak_down >= 1 ? TRUE : Rtn;
#endif
    }

    AF_MSG_LOW("LSP_DBG :is_HJ_LSP %d Rtn %d, peakdown %d stats_pos %d",
               LSP_AF_sample->is_need_HJ_Process,
               Rtn,
               fv_info->peak_down,
               stats_pos);
    return Rtn;
} // af_single_hj_is_scan_end_fine

/** af_single_hj_handle_state_fine_scan: Handle Finescan scan state
 *
 *  @af: internal AF data structure.*
 **/
VOID af_single_hj_handle_state_fine_scan(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    af_single_hj_fv_info_t* fv_info = &saf->fv_info;
    LSP_AFSweepSample Fitout;
    LSP_Process* LSP_AF_sample = &saf->LSP_AF_sample;
    chromatixAFAlgoTuningType* p_tuning =
        p_caf_instance->af_tuning->pAlgoTuningData;
    INT max_index, cur_pos, Valid_sample_cnt;
    BOOL is_fine_search;
    is_fine_search = (saf->saf_scan_state == HJ_AF_FINE_SCAN) ? TRUE : FALSE;
    cur_pos = p_caf_instance->cur_pos;
    max_index = fv_info->max_index;
    Valid_sample_cnt = 0;
    Q3A_MEMSET(&Fitout,
               0,
               sizeof(LSP_AFSweepSample));

    if (saf->scan_cnt == 0)
    {
        af_single_hj_wait_valid_frame(p_caf_instance);
        return;
    }

    if (af_single_hj_is_scan_end_fine(p_caf_instance) == TRUE)
    {
        Q3A_ATRACE_INT("CAF:FINE_SEARCH_TIME", 0);
        AF_MSG_LOW("Fine Search Complete, go to peak");

        if (TRUE == LSP_AF_sample->is_need_HJ_Process &&
                TRUE == p_caf_instance->is_hj_enable)
        {
            if (saf->direction == DIR_TO_NEAR)
            {
                LSP_AF_sample->is_good_fit =
                    af_single_hj_get_LSParaPeak(LSP_AF_sample->ToN_sample,
                                                LSP_AF_sample->sampleSort,
                                                LSP_AF_sample->ToN_sampleCount,
                                                p_tuning->handJitter.AFSingle.fineSearch.minSampleSize,
                                                &Valid_sample_cnt,
                                                &Fitout,
                                                is_fine_search,
                                                &saf->lsp_info,
                                                p_tuning);
            }
            else
            {
                LSP_AF_sample->is_good_fit =
                    af_single_hj_get_LSParaPeak(LSP_AF_sample->ToF_sample,
                                                LSP_AF_sample->sampleSort,
                                                LSP_AF_sample->ToF_sampleCount,
                                                p_tuning->handJitter.AFSingle.fineSearch.minSampleSize,
                                                &Valid_sample_cnt,
                                                &Fitout,
                                                is_fine_search,
                                                &saf->lsp_info,
                                                p_tuning);
            }

            saf->final_pos =
                (TRUE == LSP_AF_sample->is_good_fit) ?
                Fitout.pos :
                af_single_hj_calc_curve_fitting_peak(p_caf_instance);
            ;
        }
        else
        {
            saf->final_pos =
                af_single_hj_calc_curve_fitting_peak(p_caf_instance);
        }

        AF_MSG_LOW("%s LSP_DBG Fine Search Complete: isHJ=%d final_pos %d "
                   "Stats_MaxPos %d Cos_LSP_Max_Pos %d Fine_Max_Pos %d goodfit %d",
                   saf->direction == DIR_TO_NEAR ? "DIR_TO_NEAR" : "DIR_TO_FAR",
                   LSP_AF_sample->is_need_HJ_Process,
                   saf->final_pos,
                   saf->af_index[max_index],
                   LSP_Corse_Peak_Pos,
                   Fitout.pos,
                   LSP_AF_sample->is_good_fit);
        af_single_hj_end_search(p_caf_instance,
                                HJ_AF_SINGLE_SUCCESS);
        return;
    }
    else
    {
        af_single_hj_get_scan_step_INTerpolate(cur_pos,
                                               p_caf_instance,
                                               1,
                                               saf->direction);
        af_single_hj_util_move_lens(p_caf_instance,
                                    (af_move_direction_type) saf->direction,
                                    saf->fine_scan_step);
        saf->scan_cnt++;
    }

    // Update LSP info debug data
    saf->lsp_info.valid_sample_cnt = (UINT8) Valid_sample_cnt;
    return;
} // af_single_hj_handle_state_fine_scan

/** af_single_hj_is_peak_in_macro_area: Logic to Check if any peak
 *  presented in macro area.
 *
 *  @af: internal AF data structure
 **/
UINT8 af_single_hj_is_peak_in_macro_area(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    af_single_hj_fv_info_t* fv_info = &saf->fv_info;
    chromatixSingleThreshold* thres = saf->p_thres;
    INT i, max_index, down_cnt, end_index, macro_pos;
    BOOL Rtn;
    UINT8 bv_index;
    // FLOAT flat_threshold, min_max_ratio;
    Rtn = FALSE;
    max_index = fv_info->max_index;
    end_index = saf->stats_index;
    macro_pos =
        p_caf_instance->af_tuning->pContrastTuningData->singleSearch.index[AF_14CM_IDX];
    bv_index = af_single_hj_get_bv_index(p_caf_instance);
    af_single_hj_get_flat_ratio_threshold(p_caf_instance, bv_index);
    // min_max_ratio = fv_info->fv_min / fv_info->fv_max;

    // Only Run if Searching to Near
    if (saf->direction != DIR_TO_NEAR)
    {
        AF_MSG_LOW("FALSE because direction to Far");
        return FALSE;
    }

    //if focus value is decreased more than 2 times, it's fail
    down_cnt = 0;

    for (i = saf->start_index; i <= end_index; i++)
    {
        if ((saf->af_index[i] <= macro_pos) && (fv_info->fv_ratio[i] <= 1))
            down_cnt++;
    }

    if (down_cnt >= 2)
        return FALSE;

    //If scan frames are too small, then return False
    if (saf->scan_cnt < 4)
        return FALSE;

    //Check if peak is in macro area
    if (max_index == end_index)
    {
        AF_MSG_LOW("MaxPos is at the last index %d",
                   saf->af_index[max_index]);
        Rtn = TRUE;
    }
    else if (max_index == (end_index - 1))
    {
        AF_MSG_LOW("MaxPos is at the second last index %d",
                   saf->af_index[max_index]);

        if (fv_info->fv_ratio[end_index] <= thres->macroThres)
        {
            Rtn = TRUE;
            AF_MSG_LOW("fRatio[end_index] = %f thres->macro_thres= %f ",
                       fv_info->fv_ratio[end_index],
                       thres->macroThres);
        }
    }

    AF_MSG_LOW("Peak in macro ? %s",
               Rtn ? "Yes" : "No");
    return (UINT8) Rtn;
} // af_single_hj_is_peak_in_macro_area

/** af_single_hj_is_needed_rev_scan_at_fail: Determine if reverse
 *  search is still needed. Designed to address search fail when
 *  defocused at far/near zone with a quick reverse.
 *  assumption :
 *   1. IS_pre_scan returned TRUE
 *   2. SCAN is INAVLID(meaning fail)
 *
 *  @af: internal AF data structure
 **/
UINT8 af_single_hj_is_needed_rev_scan_at_fail(
    af_caf_search_internal_t* p_caf_instance,
    UINT8 is_flat_curve)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    chromatixAFTuningSingle* p_tuning =
        &p_caf_instance->af_tuning->pContrastTuningData->singleSearch;
    af_single_hj_fv_info_t* fv_info = &saf->fv_info;
    UINT8 is_peak_detected_in_search;
    UINT8 is_scanned_e2e_far, is_scanned_e2e_near, is_started_in_mid_zone;
    UINT8 criteria_1, criteria_2, criteria_3, criteria_4, is_need_rev_scan;
    INT32 stats_pos;
    INT32 far_zone_pos, near_zone_pos, mid_zone_pos;
    is_need_rev_scan = FALSE;
    stats_pos = saf->af_index[saf->stats_index];
    far_zone_pos = p_tuning->index[p_tuning->optics.farZone];
    near_zone_pos = p_tuning->index[p_tuning->optics.nearZone];
    mid_zone_pos = p_tuning->index[p_tuning->optics.midZone];
    // Check if Start and currently in both either Far or Near Zone
    is_started_in_mid_zone = (saf->start_pos > near_zone_pos &&
                              saf->start_pos < far_zone_pos);
    is_scanned_e2e_far = ((saf->start_pos <= (INT32) p_tuning->index[AF_20CM_IDX]) &&
                          (stats_pos >= far_zone_pos) && (saf->direction == DIR_TO_FAR));
    is_scanned_e2e_near = ((saf->start_pos >= far_zone_pos) &&
                           (stats_pos <= (INT32) p_tuning->index[AF_10CM_IDX]) &&
                           (saf->direction == DIR_TO_NEAR));
    is_peak_detected_in_search = fv_info->is_peak_detected_in_inc_dec == 0
                                 ? FALSE : TRUE;
    AF_MSG_LOW("is_scanned_e2e_far %d is_scanned_e2e_near: %d "
               "saf->start_pos %d midzone %d stats_pos %d Peak_detected %d "
               "is_started_in_mid_zone %d is_peak_detected_in_search %d",
               is_scanned_e2e_far,
               is_scanned_e2e_near,
               saf->start_pos,
               mid_zone_pos,
               stats_pos,
               fv_info->is_peak_detected_in_inc_dec,
               is_started_in_mid_zone,
               is_peak_detected_in_search);
    /* Check for these criteria :
     1. Already Reversed or never reversed
     2. start pos from midzone and reached end or scanned end to end
     3. if peak is not detected during search
     4. Flat enough */
    criteria_1 = saf->reverse_scan_cnt == 0;
    criteria_2 = is_scanned_e2e_far
        || is_scanned_e2e_near
        || (is_started_in_mid_zone && af_single_hj_is_scan_end_pre(p_caf_instance) == TRUE);
    criteria_3 = !is_peak_detected_in_search;
    criteria_4 = is_flat_curve;
    AF_MSG_LOW("Criteria_1 %d Criteria_2 %d Criteria_3 %d Criteria_4 %d ",
               criteria_1,
               criteria_2,
               criteria_3,
               criteria_4);

    if (criteria_1 && criteria_2 && (criteria_3 || criteria_4))
        is_need_rev_scan = TRUE;

    return is_need_rev_scan;
} // af_single_hj_is_needed_rev_scan_at_fail

/** af_single_hj_set_start_pos_reverse: Set the starting location
 *  to start pre-scan in reverse direction
 *
 *  @af: internal AF data structure
 *  @target_pos: target position for the first valid sample,
 *             DONOT Pass prescan offsetted position
 **/
INT af_single_hj_set_start_pos_reverse(
    af_caf_search_internal_t* p_caf_instance,
    INT target_pos)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    chromatixAFTuningSingle* p_tuning =
        &p_caf_instance->af_tuning->pContrastTuningData->singleSearch;
    BOOL step_back_enable =
        (BOOL)p_caf_instance->af_tuning->pContrastTuningData->stepBack.stepBackEnable;
    INT step_back_reverse_inf =
        p_caf_instance->af_tuning->pContrastTuningData->stepBack.stepBackReverseInfinity;
    INT step_back_reverse_mac =
        p_caf_instance->af_tuning->pContrastTuningData->stepBack.stepBackReverseMacro;
    INT sign, scan_steps, start_pos;
    INT mid_zone;
    // Load tuning paramters
    mid_zone = p_tuning->index[p_tuning->optics.midZone];

    // step backs from target_pos
    if (target_pos > mid_zone)
    {
        scan_steps = step_back_enable ?
                     step_back_reverse_inf :
                     (p_tuning->actuatorType == ACT_TYPE_CLOSELOOP) ? 1 : 2;
        AF_MSG_LOW("inf area, scan_steps =%d", scan_steps);
    }
    else
    {
        scan_steps = step_back_enable ?
                     step_back_reverse_mac :
                     (p_tuning->actuatorType == ACT_TYPE_CLOSELOOP) ? 1 : 2;
        AF_MSG_LOW("mac area, scan_steps =%d", scan_steps);
    }

    af_single_hj_get_scan_step_INTerpolate(target_pos,
                                           p_caf_instance,
                                           scan_steps,
                                           saf->direction);
    // Set Target position
    sign = (saf->direction == DIR_TO_FAR) ? 1 : -1;
    start_pos = target_pos + sign * saf->pre_scan_step;

    // Cap Start position to limit
    if (saf->direction == DIR_TO_FAR)
    {
        start_pos = (start_pos > saf->far_end_pos) ?
                    saf->far_end_pos :
                    start_pos;
    }
    else
    {
        start_pos = (start_pos < saf->near_end_pos) ?
                    saf->near_end_pos :
                    start_pos;
    }

    saf->rev_pos = start_pos;
    AF_MSG_LOW("Target %d Starting at %d",
               target_pos,
               start_pos);
    return start_pos;
}

/** af_single_hj_start_srch_reverse: Setup up PreScan search in the
 *  reverse direction
 *
 *  @af: internal AF data structure
 **/
VOID af_single_hj_start_srch_reverse(
    af_caf_search_internal_t* p_caf_instance,
    INT target_pos)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    BOOL ignore_first_invalid_frame = SET_INVALID_FRAME;
    // set the starting position
    saf->dest_pos =
        af_single_hj_set_start_pos_reverse(p_caf_instance,
                                           target_pos);
    // House Keeping
    saf->reverse_scan_cnt++;
    saf->saf_scan_state = HJ_AF_PRE_SCAN;
    saf->direction = (UINT8) (saf->direction == DIR_TO_FAR ?
                              DIR_TO_NEAR :
                              DIR_TO_FAR);
    saf->scan_cnt = 0;
    /* Determine the first invalid frame as valid or not,
     * according to chromatix*/
    saf->is_valid_frame = ignore_first_invalid_frame ? FALSE : TRUE;
    // move lens to destination
    af_single_hj_util_move_to_dest(p_caf_instance);
    return;
} // af_single_hj_start_srch_reverse

/** af_single_hj_set_start_pos_fine: Setup starting position for
 *  Fine Search
 *
 *  @af: internal AF data structure
 **/
VOID af_single_hj_set_start_pos_fine(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    af_single_hj_fv_info_t* fv_info = &saf->fv_info;
    LSP_Process* LSP_AF_sample = &saf->LSP_AF_sample;
    INT max_index = 0, stepprev = 0, stepnext = 0;
    INT max_pos = 0, ofs_prev = 0, ofs_next = 0;
    FLOAT fv_max = 0.0, fv_prev = 0.0, fv_next = 0.0;
    FLOAT Fv_Ratio_prev = 0.0, Fv_Ratio_next = 0.0, a = 0.0, b = 0.0;
    FLOAT fRatio_min = 0.0, fRatio_max = 0.0, fRatio = 0.0;
    FLOAT prev_y1 = 0.0, prev_y2 = 0.0, next_y1 = 0.0, next_y2 = 0.0;
    INT cur_pos = p_caf_instance->cur_pos;

    /* Use p_data2 MaxIndex which contains the maxindex for that particular curve */
    max_index = fv_info->max_index;

    if ((TRUE == LSP_AF_sample->is_good_fit) &&
            (TRUE == LSP_AF_sample->is_need_HJ_Process) &&
            (TRUE == p_caf_instance->is_hj_enable))
    {
        max_pos = LSP_AF_sample->LSP_Max_Pos;
    }
    else
    {
        max_pos = saf->af_index[max_index];
    }

    LSP_Corse_Peak_Pos = max_pos;
    fv_prev = fv_info->fv_val[LSP_SAMPLE_IDX(max_index - 1)];
    fv_max = fv_info->fv_val[max_index];
    fv_next = fv_info->fv_val[LSP_SAMPLE_IDX(max_index + 1)];

    if (max_index != saf->start_index)
    {
        stepprev = abs((INT)(saf->af_index[LSP_SAMPLE_IDX(max_index - 1)]
                             - saf->af_index[max_index]));
    }
    else
    {
        stepprev = 0;
    }

    if (max_index != saf->stats_index)
    {
        stepnext = abs((INT)(saf->af_index[LSP_SAMPLE_IDX(max_index + 1)]
                             - saf->af_index[max_index]));
    }
    else
    {
        stepnext = 0;
    }

    AF_MSG_LOW("%d saf_index %d - 1 and %d is +1 %d is max_index_location ",
               saf->index,
               saf->af_index[LSP_SAMPLE_IDX(max_index - 1)],
               saf->af_index[LSP_SAMPLE_IDX(max_index + 1)],
               saf->af_index[max_index]);
    Fv_Ratio_prev = fv_prev / fv_max;
    Fv_Ratio_next = fv_next / fv_max;
    AF_MSG_LOW(
        "Fv_Ratio_prev %f, Fv_Ratio_next %f stepNext(%d):%d stepprev(%d):%d",
        Fv_Ratio_prev,
        Fv_Ratio_next,
        LSP_SAMPLE_IDX(max_index + 1),
        stepnext,
        LSP_SAMPLE_IDX(max_index - 1),
        stepprev);
    fRatio_min = Set_fine_fRatio_min;
    fRatio_max = Set_fine_fRatio_max;

    if (Fv_Ratio_prev > Fv_Ratio_next)
    {
        prev_y1 = Set_fine_1st_prev_y1;
        next_y1 = Set_fine_1st_next_y1;
        prev_y2 = Set_fine_1st_prev_y2;
        next_y2 = Set_fine_1st_next_y2;
        fRatio = (FLOAT)(fv_max - fv_next) / (FLOAT)(fv_max - fv_prev);

        if (fRatio <= fRatio_min)
        {
            ofs_prev = (INT)((FLOAT) stepprev * prev_y1);
            ofs_next = (INT)((FLOAT) stepnext * next_y1);
        }
        else if (fRatio >= fRatio_max)
        {
            ofs_prev = (INT)((FLOAT) stepprev * prev_y2);
            ofs_next = (INT)((FLOAT) stepnext * next_y2);
        }
        else
        {
            // y = ax + b, y1 = ax1 + b, y2 = ax2 + b
            /* a = (y1 - y2)/(x1 - x2), b = y1 - ax1 */
            // For ofs_left
            /* x1 = fRatio_min, x2 = fRatio_max
             y1 = prev_y1, y2 = prev_y2 */
            a = (prev_y1 - prev_y2) / (fRatio_min - fRatio_max);
            b = prev_y1 - a * fRatio_min;
            ofs_prev = (INT)((FLOAT) stepprev * (a * fRatio + b));
            AF_MSG_LOW("fRatio %f, ofs_prev %d: a %f, b %f",
                       fRatio,
                       ofs_prev,
                       a,
                       b);
            // For ofs_next
            /* x1 = fRatio_min, x2 = fRatio_max
             y1 = next_y1, y2 = next_y2 */
            a = (next_y1 - next_y2) / (fRatio_min - fRatio_max);
            b = next_y1 - a * fRatio_min;
            ofs_next = (INT)((FLOAT) stepnext * (a * fRatio + b));
            AF_MSG_LOW("fRatio %f, ofs_next %d: a %f, b %f",
                       fRatio,
                       ofs_next,
                       a,
                       b);
        }
    }
    else
    {
        prev_y1 = Set_fine_2nd_prev_y1;
        next_y1 = Set_fine_2nd_next_y1;
        prev_y2 = Set_fine_2nd_prev_y2;
        next_y2 = Set_fine_2nd_next_y2;
        fRatio = (FLOAT)(fv_max - fv_prev) / (FLOAT)(fv_max - fv_next);

        if (fRatio <= fRatio_min)
        {
            ofs_prev = (INT)((FLOAT) stepprev * prev_y1);
            ofs_next = (INT)((FLOAT) stepnext * next_y1);
        }
        else if (fRatio >= fRatio_max)
        {
            ofs_prev = (INT)((FLOAT) stepprev * prev_y2);
            ofs_next = (INT)((FLOAT) stepnext * next_y2);
        }
        else
        {
            // y = ax + b, y1 = ax1 + b, y2 = ax2 + b
            /* a = (y1 - y2)/(x1 - x2), b = y1 - x1*[(y1-y2)/(x1-x2)] */
            // For ofs_prev
            /* x1 = fRatio_min, x2 = fRatio_max
             y1 = prev_y1, y2 = prev_y2 */
            a = (prev_y1 - prev_y2) / (fRatio_min - fRatio_max);
            b = prev_y1 - a * fRatio_min;
            ofs_prev = (INT)((FLOAT) stepprev * (a * fRatio + b));
            AF_MSG_LOW("fRatio %f, ofs_prev %d: a %f, b %f",
                       fRatio,
                       ofs_prev,
                       a,
                       b);
            // For ofs_next
            /* x1 = fRatio_min, x2 = fRatio_max
             y1 = next_y1, y2 = next_y2 */
            a = (next_y1 - next_y2) / (fRatio_min - fRatio_max);
            b = next_y1 - a * fRatio_min;
            ofs_next = (INT)((FLOAT) stepnext * (a * fRatio + b));
            AF_MSG_LOW("fRatio %f, ofs_next %d: a %f, b %f",
                       fRatio,
                       ofs_next,
                       a,
                       b);
        }
    }

    if (TRUE == LSP_AF_sample->is_need_HJ_Process &&
            TRUE == p_caf_instance->is_hj_enable)
    {
        if (saf->direction == DIR_TO_NEAR)
        {
            saf->direction = DIR_TO_FAR;

            if (TRUE == LSP_AF_sample->is_good_fit)
            {
                saf->fine_start_pos =
                    (max_pos < saf->af_index[max_index] ?
                     max_pos :
                     saf->af_index[max_index])
                    - stepnext;
                saf->fine_end_pos =
                    (max_pos < saf->af_index[max_index] ?
                     saf->af_index[max_index] :
                     max_pos) + stepprev;
            }
            else
            {
                saf->fine_start_pos = max_pos - stepnext;
                saf->fine_end_pos = max_pos + stepprev;
            }

            if (saf->fine_start_pos < saf->near_end_pos)
                saf->fine_start_pos = saf->near_end_pos;

            if (saf->fine_end_pos > saf->far_end_pos)
                saf->fine_end_pos = saf->far_end_pos;
        }
        else
        {
            saf->direction = DIR_TO_NEAR;

            if (TRUE == LSP_AF_sample->is_good_fit)
            {
                saf->fine_start_pos =
                    (max_pos > saf->af_index[max_index] ?
                     max_pos :
                     saf->af_index[max_index])
                    + stepnext;
                saf->fine_end_pos =
                    (max_pos > saf->af_index[max_index] ?
                     saf->af_index[max_index] :
                     max_pos) - stepprev;
            }
            else
            {
                saf->fine_start_pos = max_pos + stepnext;
                saf->fine_end_pos = max_pos - stepprev;
            }

            if (saf->fine_start_pos > saf->far_end_pos)
                saf->fine_start_pos = saf->far_end_pos;

            if (saf->fine_end_pos < saf->near_end_pos)
                saf->fine_end_pos = saf->near_end_pos;
        }
    }
    else
    {
        af_single_hj_get_scan_step_INTerpolate(cur_pos, p_caf_instance, 1, saf->direction);
        if (saf->direction == DIR_TO_NEAR)
        {
            saf->direction = DIR_TO_FAR;
            saf->fine_start_pos = max_pos - stepnext - saf->fine_scan_step;
            saf->fine_end_pos = max_pos + stepprev;

            if (saf->fine_start_pos < saf->near_end_pos)
                saf->fine_start_pos = saf->near_end_pos;

            if (saf->fine_end_pos > saf->far_end_pos)
                saf->fine_end_pos = saf->far_end_pos;
        }
        else
        {
            saf->direction = DIR_TO_NEAR;
            saf->fine_start_pos = max_pos + stepnext + saf->fine_scan_step;
            saf->fine_end_pos = max_pos - stepprev;

            if (saf->fine_start_pos > saf->far_end_pos)
                saf->fine_start_pos = saf->far_end_pos;

            if (saf->fine_end_pos < saf->near_end_pos)
                saf->fine_end_pos = saf->near_end_pos;
        }
    }

    AF_MSG_LOW("%s LSP_DBG isHJ=%d fine_start_pos %d, fine_end_pos %d "
               "Fin_MaxPos %d MaxPos %d LSP_Max_Pos %d goodfit %d",
               saf->direction == DIR_TO_NEAR ? "DIR_TO_NEAR" : "DIR_TO_FAR",
               LSP_AF_sample->is_need_HJ_Process,
               saf->fine_start_pos,
               saf->fine_end_pos,
               max_pos,
               saf->af_index[max_index],
               LSP_AF_sample->LSP_Max_Pos,
               LSP_AF_sample->is_good_fit);
} // af_single_hj_set_start_pos_fine

/** af_single_hj_start_srch_fine: Setup Fine search position and
 *  move len to starting point
 *
 *  @af: internal AF data structure.*
 **/
VOID af_single_hj_start_srch_fine(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_t* af = p_caf_instance->af_single;
    af_single_hj_lib_info_t* saf = &af->af_hj_lib_info;
    af_single_hj_fv_info_t* fv_info = &saf->fv_info;
    chromatixAFAlgoTuningType* p_algo_tuning =
        p_caf_instance->af_tuning->pAlgoTuningData;
    INT temp_pos;
    saf->scan_cnt = 0;
    saf->saf_scan_state = HJ_AF_FINE_SCAN;
    saf->is_valid_frame = FALSE;
    Q3A_ATRACE_INT("CAF:COARSE_SEARCH_TIME", 0);
    Q3A_ATRACE_INT("CAF:FINE_SEARCH_TIME", 1);
    AF_MSG_LOW("Start Fine Search");
    /* Reset min/max */
    fv_info->fv_max = 0;
    fv_info->fv_min = Q3A_FLT_MAX;
    AF_MSG_HIGH("Moving to starting pos, Ready for fine search");
    // set up the search range
    af_single_hj_set_start_pos_fine(p_caf_instance);

    // Reset LSP stats before entering fine search
    if (TRUE == p_algo_tuning->handJitter.AFSingle.resetDataBeforeFineSearch &&
            TRUE == p_caf_instance->is_hj_enable)
        af_single_hj_reset_LSPAFsweepSample(p_caf_instance);

    //To start 2nd scan from 1 SndScanStep before SndStartPos
    af_single_hj_get_scan_step_INTerpolate(saf->fine_start_pos,
                                           p_caf_instance,
                                           1,
                                           saf->direction);
    // Target position to start fine search
    temp_pos =
        (saf->direction == DIR_TO_FAR) ?
        saf->fine_start_pos - saf->fine_scan_step :
        saf->fine_start_pos + saf->fine_scan_step;
    // Move to starting point
    saf->dest_pos = temp_pos;
    af_single_hj_util_move_to_dest(p_caf_instance);
    return;
} // af_single_hj_start_srch_fine

/** af_single_hj_is_needed_fine_scan: Determine if fine search is
 *  needed.
 *
 *  @af: internal AF data structure
 **/
BOOL af_single_hj_is_needed_fine_scan(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    chromatixAFTuningSingle* p_tuning =
        &p_caf_instance->af_tuning->pContrastTuningData->singleSearch;
    af_single_hj_fv_info_t* fv_info = &saf->fv_info;
    af_roi_info_t* af_roi = &p_caf_instance->roi;
    INT max_pos, fine_search_th;
    BOOL Rtn;
    BOOL is_af_caf = saf->is_af_caf;
    max_pos = saf->af_index[fv_info->max_index];
    fine_search_th = p_tuning->index[p_tuning->optics.fineSearchRegion];

    // for Video CAF, no need for fine search
    if (saf->af_mode == AF_RUN_MODE_VIDEO && is_af_caf == TRUE)
    {
        AF_MSG_LOW("Video CAF no need for Fine Search");
        return FALSE;
    }

    AF_MSG_LOW("MaxPos %d, fineSearchTh %d",
               max_pos,
               fine_search_th);

    if (saf->af_run_first_time == TRUE)
    {
        AF_MSG_LOW("First time search enable fine_search");
        return TRUE;
    }

#ifdef feature_force_enable_fine_search

    //if force_enable_fine_search is enabed, execute fine search for below cases
    if((fv_info->max_index < 5) ||
            (saf->reverse_scan_cnt > 0) ||
            ((is_af_caf == FALSE) && (max_pos > saf->reg1_thrd)))
    {
        AF_MSG_LOW("force enabled fine search : max_index %d, "
                   " reverse_scan_cnt %d, is_af_caf %d, max_pos > reg1_thrd ? %d",
                   fv_info->max_index, saf->reverse_scan_cnt, is_af_caf,
                   (max_pos > saf->reg1_thrd));
        return TRUE;
    }

#endif
    Rtn = FALSE;

    if (max_pos >= fine_search_th)
    {
        AF_MSG_LOW("Far object rough peak at %d",
                   max_pos);
        Rtn = TRUE;
    }
    else if (af_roi->type == AF_ROI_TYPE_FACE)
    {
        AF_MSG_LOW("Face detected, enable Fine search");
        Rtn = TRUE;
    }

    return Rtn;
} // af_single_hj_is_needed_fine_scan

/** af_single_hj_LSPAF_pre_scan_algo: determin prescan result for LSPAF
 * search range
 *
 *  @af: internal AF data structure
 **/
VOID af_single_hj_LSPAF_pre_scan_algo(
    af_caf_search_internal_t* p_caf_instance)
{
    //HJ process
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    af_single_hj_fv_info_t* fv_info = &saf->fv_info;
    LSP_Process* LSP_AF_sample = &saf->LSP_AF_sample;
    chromatixAFTuningSingle* p_tuning =
        &p_caf_instance->af_tuning->pContrastTuningData->singleSearch;
    chromatixAFAlgoTuningType* p_algo_tuning =
        p_caf_instance->af_tuning->pAlgoTuningData;
    INT af_idx, max_index, cur_pos, target_start_pos, hyp_pos;
    BOOL is_scene_change_gyro = FALSE;
    UINT8 prescan_result_LSP;
    LSP_AFSweepSample Fitout;
    af_idx = saf->stats_index;
    max_index = fv_info->max_index;
    hyp_pos = p_tuning->index[AF_HYP_F_IDX];
    prescan_result_LSP = (UINT8) fv_info->fv_status[af_idx];
    cur_pos = p_caf_instance->cur_pos;
    INT Valid_sample_cnt = 0;
    FLOAT HJ_Sample_rate = 0;
    BOOL is_fine_search =
        (saf->saf_scan_state == HJ_AF_FINE_SCAN) ? TRUE : FALSE;
    HJ_Sample_rate =
        (saf->direction == DIR_TO_NEAR) ?
        (FLOAT) LSP_AF_sample->ToN_HJ_sampleCount
        / LSP_AF_sample->ToN_sampleCount :
        (FLOAT) LSP_AF_sample->ToF_HJ_sampleCount
        / LSP_AF_sample->ToF_sampleCount;

    switch (prescan_result_LSP)
    {
        case inc:
            {
                AF_MSG_LOW("LSP_DBG: LSP_SCAN_INC");

                if (af_single_hj_is_scan_end_pre(p_caf_instance) == TRUE)
                {
                    //peak is at end
                    AF_MSG_LOW("LSP_DBG: LSP_SCAN_INC :HJ-HJ_rate(%f) during srch: "
                               "Check GYRO SC=%d",
                               HJ_Sample_rate,
                               is_scene_change_gyro);

                    if (HJ_Sample_rate
                            > p_algo_tuning->handJitter.AFSingle.redoRateThreshold)
                    {
                        AF_MSG_LOW("LSP_DBG: LSP_SCAN_INC:Too serious HJ");
                        saf->scene_chg_during_srch = TRUE;
                        saf->final_pos = hyp_pos;
                        af_single_hj_end_search(p_caf_instance,
                                                HJ_AF_SINGLE_FAILED);
                        return;
                    }
                    else
                    {
                        if (saf->direction == DIR_TO_NEAR)
                        {
                            LSP_AF_sample->is_good_fit =
                                af_single_hj_get_LSParaPeak(LSP_AF_sample->ToN_sample,
                                                            LSP_AF_sample->sampleSort,
                                                            LSP_AF_sample->ToN_sampleCount,
                                                            p_algo_tuning->handJitter.AFSingle.coarseSearch.minSampleSize,
                                                            &Valid_sample_cnt,
                                                            &Fitout,
                                                            is_fine_search,
                                                            &saf->lsp_info,
                                                            p_algo_tuning);
                        }
                        else
                        {
                            LSP_AF_sample->is_good_fit =
                                af_single_hj_get_LSParaPeak(LSP_AF_sample->ToF_sample,
                                                            LSP_AF_sample->sampleSort,
                                                            LSP_AF_sample->ToF_sampleCount,
                                                            p_algo_tuning->handJitter.AFSingle.coarseSearch.minSampleSize,
                                                            &Valid_sample_cnt,
                                                            &Fitout,
                                                            is_fine_search,
                                                            &saf->lsp_info,
                                                            p_algo_tuning);
                        }

                        AF_MSG_LOW("LSP_DBG: LSP_SCAN_INC Good_Fit%d Fit or Fine_search ",
                                   LSP_AF_sample->is_good_fit);

                        if (TRUE == LSP_AF_sample->is_good_fit)
                        {
                            LSP_AF_sample->LSP_Max_Pos = Fitout.pos;
                            saf->final_pos = LSP_AF_sample->LSP_Max_Pos;
                            af_single_hj_end_search(p_caf_instance,
                                                    HJ_AF_SINGLE_SUCCESS);
                            return;
                        }
                        else
                        {
                            af_single_hj_start_srch_fine(p_caf_instance);
                            return;
                        }
                    }
                }
                else
                {
                    saf->scan_cnt++;
                    af_single_hj_get_scan_step(cur_pos,
                                               p_caf_instance);
                    af_single_hj_util_move_lens(p_caf_instance,
                                                (af_move_direction_type) saf->direction,
                                                saf->pre_scan_step);
                    return;
                }

                break;
            }

        case dec:
            {
                AF_MSG_LOW("LSP_DBG: LSP_SCAN_DEC");

                if (saf->direction == DIR_TO_NEAR)
                {
                    LSP_AF_sample->is_good_fit =
                        af_single_hj_get_LSParaPeak(LSP_AF_sample->ToN_sample,
                                                    LSP_AF_sample->sampleSort,
                                                    LSP_AF_sample->ToN_sampleCount,
                                                    p_algo_tuning->handJitter.AFSingle.coarseSearch.minSampleSize,
                                                    &Valid_sample_cnt,
                                                    &Fitout,
                                                    is_fine_search,
                                                    &saf->lsp_info,
                                                    p_algo_tuning);
                }
                else
                {
                    LSP_AF_sample->is_good_fit =
                        af_single_hj_get_LSParaPeak(LSP_AF_sample->ToF_sample,
                                                    LSP_AF_sample->sampleSort,
                                                    LSP_AF_sample->ToF_sampleCount,
                                                    p_algo_tuning->handJitter.AFSingle.coarseSearch.minSampleSize,
                                                    &Valid_sample_cnt,
                                                    &Fitout,
                                                    is_fine_search,
                                                    &saf->lsp_info,
                                                    p_algo_tuning);
                }

                if (TRUE == LSP_AF_sample->is_good_fit)
                {
                    LSP_AF_sample->LSP_Max_Pos = Fitout.pos;

                    if (TRUE == af_single_hj_is_needed_fine_scan(p_caf_instance))
                    {
                        //2nd scan is needed
                        af_single_hj_start_srch_fine(p_caf_instance);
                        return;
                    }
                    else
                    {
                        // Use HJ coarse search parabolic curve fit peak
                        saf->final_pos = LSP_AF_sample->LSP_Max_Pos;
                        af_single_hj_end_search(p_caf_instance,
                                                HJ_AF_SINGLE_SUCCESS);
                        return;
                    }
                }
                else
                {
                    // Not good fit
                    if (max_index
                            == p_algo_tuning->handJitter.AFSingle.maxFVInBeginning ||
                            af_single_hj_is_scan_end_pre(p_caf_instance) == TRUE)
                    {
                        if (saf->reverse_scan_cnt == 0)
                        {
                            /*hw_fv_info array includes sw and hw fv stats, so max_index
                             could be sw or hw stats index. this change is to reduce reverse search range and
                             time */
                            target_start_pos = saf->af_index[fv_info->max_index];
                            af_single_hj_start_srch_reverse(p_caf_instance,
                                                            target_start_pos);
                        }
                        else
                        {
                            AF_MSG_LOW("LSP_DBG: LSP_SCAN_DEC:HJ-HJ_rate(%f) during srch:"
                                       "Check GYRO SC=%d",
                                       HJ_Sample_rate,
                                       is_scene_change_gyro);

                            if (HJ_Sample_rate
                                    > p_algo_tuning->handJitter.AFSingle.redoRateThreshold)
                            {
                                AF_MSG_LOW("LSP_DBG: LSP_SCAN_DEC:Too serious HJ");
                                saf->scene_chg_during_srch = TRUE;
                                saf->final_pos = hyp_pos;
                                af_single_hj_end_search(p_caf_instance,
                                                        HJ_AF_SINGLE_FAILED);
                                return;
                            }
                            else
                            {
                                af_single_hj_start_srch_fine(p_caf_instance);
                                return;
                            }
                        }
                    }
                    else
                    {
                        if ((Valid_sample_cnt
                                < (INT32) p_algo_tuning->handJitter.AFSingle.coarseSearch.minSampleSize)
                                ||
                                ((Fitout.pos
                                  > saf->af_index[LSP_SAMPLE_IDX(af_idx - 1)])
                                 &&
                                 (saf->direction == DIR_TO_FAR))
                                ||
                                ((Fitout.pos
                                  < saf->af_index[LSP_SAMPLE_IDX(af_idx - 1)])
                                 &&
                                 (saf->direction == DIR_TO_NEAR)))
                        {
                            AF_MSG_LOW("LSP_DBG: LSP_SCAN_DEC,Valid_sample_cnt=%d<5"
                                       "o LSPmaxpos%d not within last2postion %d direction %d,"
                                       "keep going",
                                       Valid_sample_cnt,
                                       Fitout.pos,
                                       saf->af_index[af_idx - 1],
                                       saf->direction);
                            saf->scan_cnt++;
                            af_single_hj_get_scan_step(cur_pos,
                                                       p_caf_instance);
                            af_single_hj_util_move_lens(p_caf_instance,
                                                        (af_move_direction_type) saf->direction,
                                                        saf->pre_scan_step);
                            return;
                        }
                        else
                        {
                            if (saf->reverse_scan_cnt == 0)
                            {
                                /*hw_fv_info array includes sw and hw fv stats, so max_index
                                 could be sw or hw stats index. this change is to reduce reverse
                                 search range and time */
                                target_start_pos =
                                    saf->af_index[fv_info->max_index];
                                af_single_hj_start_srch_reverse(p_caf_instance,
                                                                target_start_pos);
                            }
                            else
                            {
                                af_single_hj_start_srch_fine(p_caf_instance);
                                return;
                            }
                        }
                    }
                }

                break;
            }

        case flat:
            {
                AF_MSG_LOW("LSP_DBG: LSP_SCAN_FLAT");

                if (af_single_hj_is_scan_end_pre(p_caf_instance) == TRUE)
                {
                    if (saf->reverse_scan_cnt == 0)
                    {
                        /*hw_fv_info array includes sw and hw fv stats, so max_index could
                         be sw or hw
                         stats index. this change is to reduce reverse search range and time
                         */
                        target_start_pos = saf->af_index[fv_info->max_index];
                        af_single_hj_start_srch_reverse(p_caf_instance,
                                                        target_start_pos);
                    }
                    else
                    {
                        AF_MSG_LOW("LSP_DBG: LSP_SCAN_FLAT:HJ-HJ_rate(%f) during srch: "
                                   "Check GYRO SC=%d",
                                   HJ_Sample_rate,
                                   is_scene_change_gyro);

                        if (HJ_Sample_rate
                                > p_algo_tuning->handJitter.AFSingle.redoRateThreshold)
                        {
                            AF_MSG_LOW("LSP_DBG: LSP_SCAN_FLAT:Too serious HJ");
                            saf->scene_chg_during_srch = TRUE;
                            saf->final_pos = hyp_pos;
                            af_single_hj_end_search(p_caf_instance,
                                                    HJ_AF_SINGLE_FAILED);
                            return;
                        }
                        else
                        {
                            AF_MSG_LOW("LSP_DBG: LSP_SCAN_FLAT:"
                                       "Curve is flat Ending the search");
                            saf->final_pos = hyp_pos;
                            af_single_hj_end_search(p_caf_instance,
                                                    HJ_AF_SINGLE_FAILED);
                            return;
                        }
                    }
                }
                else
                {
                    saf->scan_cnt++;
                    af_single_hj_get_scan_step(cur_pos,
                                               p_caf_instance);
                    af_single_hj_util_move_lens(p_caf_instance,
                                                (af_move_direction_type) saf->direction,
                                                saf->pre_scan_step);
                    return;
                }

                break;
            }

        default:
            {
                AF_MSG_LOW("LSP_DBG: LSP_Invalid,so Scan more since it's not the end");
                saf->scan_cnt++;
                af_single_hj_get_scan_step_INTerpolate(cur_pos,
                                                       p_caf_instance,
                                                       1,
                                                       saf->direction);
                af_single_hj_util_move_lens(p_caf_instance,
                                            (af_move_direction_type) saf->direction,
                                            saf->pre_scan_step);
                return;
                break;
            }
    }

    // Update LSP info debug data
    saf->lsp_info.valid_sample_cnt = (UINT8) Valid_sample_cnt;
}

/** af_single_hj_pre_scan_algo: determin prescan result for LSPAF
 * search range
 *
 *  @af: internal AF data structure
 **/
VOID af_single_hj_pre_scan_algo(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    af_single_hj_fv_info_t* fv_info = &saf->fv_info;
    chromatixAFTuningSingle* p_tuning =
        &p_caf_instance->af_tuning->pContrastTuningData->singleSearch;
    UINT8 prescan_result;
    INT cur_pos, target_start_pos, hyp_pos;
    cur_pos = p_caf_instance->cur_pos;
    hyp_pos = p_tuning->index[AF_HYP_F_IDX];
    prescan_result = (UINT8) fv_info->prescan_result[saf->stats_index];
    BOOL is_flat_curve =
        af_single_hj_flat_curve_detection_by_fratio(p_caf_instance);
#ifdef feature_flat_curve_detection_by_bilateral
    FLOAT bv_bilateral = BV_TH_flat_detect_bilateral;

    if((p_caf_instance->aec_input.Bv_af > bv_bilateral) &&
            af_single_hj_flat_curve_detection_by_bilateral(p_caf_instance))
    {
        prescan_result = HJ_SCAN_INVALID;
        AF_MSG_LOW("flat is detected");
    }

#endif

    //run algorithm with prescan_result
    switch (prescan_result)
    {
        case HJ_SCAN_PEAK:
            {
                AF_MSG_LOW("HJ_SCAN_PEAK");

                if (af_single_hj_is_needed_fine_scan(p_caf_instance) == TRUE)
                {
                    //2nd scan is needed
                    af_single_hj_start_srch_fine(p_caf_instance);
                    return;
                }
                else
                {
                    Q3A_ATRACE_INT("CAF:COARSE_SEARCH_TIME", 0);
                    // Add parabolic curve fit
                    saf->final_pos =
                        af_single_hj_calc_curve_fitting_peak(p_caf_instance);
                    af_single_hj_end_search(p_caf_instance,
                                            HJ_AF_SINGLE_SUCCESS);
                    return;
                }

                break;
            }

        case HJ_SCAN_DECREASE:
            {
                AF_MSG_LOW("HJ_SCAN_DECREASE");

                if (is_flat_curve)
                {
                    AF_MSG_LOW("flat curve detected, moving to hyperfocal %d",
                               hyp_pos);
                    saf->final_pos = hyp_pos;
                    af_single_hj_end_search(p_caf_instance,
                                            HJ_AF_SINGLE_FAILED);
                    return;
                }
                else if (saf->reverse_scan_cnt == 0)
                {
                    //Scan to Reverse direction
                    /*We don't use multi kernal. So don't need to check if sw stats or
                     hw stats*/
                    target_start_pos = saf->af_index[fv_info->max_index];
                    af_single_hj_start_srch_reverse(p_caf_instance,
                                                    target_start_pos);
                }
                else
                {
                    // Decrease and Reverse != 0
                    //feature eable for flat_curve_detection_by_fratio & spot light detection
                    AF_MSG_LOW("Decrease in Reversed direction!");

                    // if decrease in reverse, we can evaluate for fine search
                    if (af_single_hj_is_needed_fine_scan(p_caf_instance) == TRUE)
                    {
                        af_single_hj_start_srch_fine(p_caf_instance);
                        return;
                    }
                    else
                    {
                        Q3A_ATRACE_INT("CAF:COARSE_SEARCH_TIME", 0);

                        if (!is_flat_curve)
                        {
                            AF_MSG_LOW("Curve is not flat, go to peak");
                            saf->final_pos =
                                af_single_hj_calc_curve_fitting_peak(p_caf_instance);
                            af_single_hj_end_search(p_caf_instance,
                                                    HJ_AF_SINGLE_SUCCESS);
                            return;
                        }
                        else
                        {
                            AF_MSG_LOW("Curve is flat, go to hyper focal");
                            saf->final_pos = hyp_pos;
                            af_single_hj_end_search(p_caf_instance,
                                                    HJ_AF_SINGLE_FAILED);
                            return;
                        }
                    }
                }

                break;
            }

        default:
            {
                AF_MSG_LOW("HJ_SCAN_DEFAULT");

                //feature eable for flat_curve_detection_by_fratio & spot light detection
                // Increase or invalid Result
                if (prescan_result != HJ_SCAN_INCREASE)
                {
                    AF_MSG_LOW("HJ_SCAN_INVALID");

                    // Check to see if reverse is needed
                    if (saf->reverse_scan_cnt == 0)
                    {
                        // If Start in mid zone, and reached far or near zone
                        AF_MSG_LOW("scanned from mid to far/near zone. noise_cnt %d,"
                                   " peak %d, min %f, max %f",
                                   fv_info->noise_cnt,
                                   fv_info->is_peak_detected_in_inc_dec,
                                   fv_info->fv_min,
                                   fv_info->fv_max);

                        /*When we get HJ_SCAN_INVALID its too early to take decision either
                         to do curve fitting or AF_FAIL so best we can do take the decision
                         of reverse scan if there is too much of noise in the curve
                         Its an early call to reverse and we dont wait till we
                         reach the end of af_single_hj_is_scan_end_pre*/
                        if (af_single_hj_is_needed_rev_scan_at_fail(p_caf_instance, (UINT8) is_flat_curve)
                                && (fv_info->noise_cnt > 2))
                        {
                            AF_MSG_LOW("Too much noise, reverse now");
                            // Reverse to around starting position
                            target_start_pos = saf->af_index[fv_info->max_index];
                            af_single_hj_start_srch_reverse(p_caf_instance,
                                                            target_start_pos);
                            return;
                        }
                    }
                }

                /* When the lens_pos reaches either end (near or far) we need to do
                 following things
                 1. Check if the peak is in macro if the direction is towards macro
                 2. Check if we need a reverse scan -> This function has to be
                 very crucial, since if no reverse scan is needed then we are
                 about to make a decision i.e either HJ_AF_SINGLE_SUCCESS or FAIL
                 3. Check if curve is flat or not, if not flat then do curve_fitting
                 4 If the curve is flat then AF has to fail. */

                if (af_single_hj_is_scan_end_pre(p_caf_instance) == TRUE)
                {
                    //1. First check if the peak is in macro_region
                    if ((af_single_hj_is_peak_in_macro_area(p_caf_instance) == TRUE)
                            && saf->direction == DIR_TO_NEAR)
                    {
                        Q3A_ATRACE_INT("CAF:COARSE_SEARCH_TIME", 0);
                        saf->final_pos =
                            af_single_hj_calc_curve_fitting_peak(p_caf_instance);
                        af_single_hj_end_search(p_caf_instance,
                                                HJ_AF_SINGLE_SUCCESS);
                        return;
                    }

                    //feature eable for flat_curve_detection_by_fratio & spot light detection
                    //2. Check if reverse scan is required or not
                    if (af_single_hj_is_needed_rev_scan_at_fail(
                                p_caf_instance,
                                (UINT8) is_flat_curve))
                    {
                        AF_MSG_LOW("again check reverse direction at %d ",
                                   saf->rev_pos);
                        target_start_pos = saf->af_index[fv_info->max_index];
                        af_single_hj_start_srch_reverse(p_caf_instance,
                                                        target_start_pos);
                    }
                    else if (!is_flat_curve
                             || fv_info->is_peak_detected_in_inc_dec > 0)
                    {
                        /*3. If the curve is not flat or if there was a peak detected during
                         search then do a curve fitting*/
                        if (af_single_hj_is_needed_fine_scan(p_caf_instance) == TRUE)
                        {
                            af_single_hj_start_srch_fine(p_caf_instance);
                            return;
                        }
                        else
                        {
                            AF_MSG_LOW("Already reversed and not Flat. go to peak");
                            // Add parabolic curve fit
                            Q3A_ATRACE_INT("CAF:COARSE_SEARCH_TIME", 0);
                            saf->final_pos = af_single_hj_calc_curve_fitting_peak(
                                                 p_caf_instance);
                            af_single_hj_end_search(p_caf_instance,
                                                    HJ_AF_SINGLE_SUCCESS);
                            return;
                        }
                    }
                    else
                    {
                        /*4. All the above conditions fail then curve is either bad or flat,
                         so we fail here.*/
                        AF_MSG_LOW("Curve is flat Ending the search");
                        Q3A_ATRACE_INT("CAF:COARSE_SEARCH_TIME", 0);
                        saf->final_pos = hyp_pos;
                        af_single_hj_end_search(p_caf_instance,
                                                HJ_AF_SINGLE_FAILED);
                        return;
                    }
                }
                else
                {
                    AF_MSG_LOW("Scan more since its not the end");
                    saf->scan_cnt++;
                    af_single_hj_get_scan_step_INTerpolate(cur_pos,
                                                           p_caf_instance,
                                                           1,
                                                           saf->direction);
                    af_single_hj_util_move_lens(p_caf_instance,
                                                (af_move_direction_type) saf->direction,
                                                saf->pre_scan_step);
                    return;
                }

                break;
            }
    }
}

/** af_single_hj_util_get_current_max_val: get max value of current
 * search range
 *
 *  @af: internal AF data structure
 **/
FLOAT af_single_hj_util_get_current_max_val(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    af_single_hj_fv_info_t* fv_info = &saf->fv_info;
    INT pos_lo, pos_hi, cur_pos, max_index, i, stats_pos;
    FLOAT cur_max;
    cur_pos = p_caf_instance->cur_pos;
    stats_pos = saf->af_index[saf->stats_index];
    cur_max = 0;
    max_index = fv_info->max_index;

    // Error Checking for ensuring reversed.
    if (saf->reverse_scan_cnt == 0 || saf->rev_pos == -1 ||
            cur_pos == saf->rev_pos)
    {
        AF_MSG_LOW("Error! Should not be called! cnt %d, pos %d.",
                   saf->reverse_scan_cnt,
                   saf->rev_pos);
        return fv_info->fv_val[max_index];
    }

    // find position Range
    pos_lo = stats_pos < saf->rev_pos ? stats_pos : saf->rev_pos;
    pos_hi = stats_pos < saf->rev_pos ? saf->rev_pos : stats_pos;

    // search in the pos range in current direction
    for (i = saf->hw_stats_index; saf->af_index[i] != saf->rev_pos; i--)
    {
        if (pos_lo <= saf->af_index[i] && saf->af_index[i] <= pos_hi)
        {
            cur_max = (fv_info->fv_val[i] > cur_max) ?
                      fv_info->fv_val[i] :
                      cur_max;
        }
    }

    AF_MSG_LOW("max is %f",
               cur_max);
    return cur_max;
} // af_single_hj_util_get_current_max_val

/** af_single_hj_is_peak_in_decrease: From scanstatus, Analyze if
 *  the stats is indeed decreasing
 *
 *  @af: internal AF data structure
 **/
UINT8 af_single_hj_is_peak_in_decrease(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    af_single_hj_fv_info_t* fv_info = &saf->fv_info;
    chromatixSingleThreshold* thres = saf->p_thres;
    UINT8 bv_index, peak_down, Rtn;
    FLOAT fv_max, fv_ratio, fv_ratio_th;
    FLOAT fv_ratio_offset_by_peakdown, peak_adjust;

    if (saf->reverse_scan_cnt == 0)
    {
        fv_max = fv_info->fv_val[fv_info->max_index];
    }
    else
    {
        // use the Max in current search direction only
        fv_max = af_single_hj_util_get_current_max_val(p_caf_instance);
    }

    fv_ratio = fv_info->fv_cur / fv_max;
    peak_down = fv_info->peak_down;
    Rtn = FALSE;
    peak_adjust = (saf->af_mode == AF_RUN_MODE_VIDEO) ? 0.3f : 0.01f;
    fv_ratio_offset_by_peakdown =
        (((FLOAT) peak_down - 3.0f) * peak_adjust) / 100.0f;
    AF_MSG_LOW("PeakDown %d: fv_ratio_offset_by_peakdown %f",
               peak_down,
               fv_ratio_offset_by_peakdown);
    bv_index = af_single_hj_get_bv_index(p_caf_instance);

    if (fv_info->noise_cnt == 0)
    {
        fv_ratio_offset_by_peakdown =
            (bv_index == _SKY_LX) ? 0 : fv_ratio_offset_by_peakdown;
        fv_ratio_th =
            thres->decreaseDecreasePeakThreshold.Threshold[bv_index]
            - fv_ratio_offset_by_peakdown;
        AF_MSG_LOW("fv_ratio_th %f: fv_ratio %f",
                   fv_ratio_th,
                   fv_ratio);

        if ((fv_ratio < fv_ratio_th) ||
                ((saf->direction == DIR_TO_FAR) && (peak_down >= 3)))
            Rtn = TRUE;
    }
    else if (fv_info->noise_cnt <= 2)
    {
        /* sometime, FV is so strange, fist frame's FV is the highest value
         * for far area object */
        if (fv_ratio
                < thres->decreaseDecreasePeakWithNoiseThreshold.Threshold[bv_index])
        {
            fv_info->noise_cnt = 0;
            Rtn = TRUE;
        }
    }

#if 0 //Rethink about this logic of detecting possible peak

    if(fv_info->possible_peak == TRUE && peak_down >= 3 && fv_ratio < 0.96)
    {
        AF_MSG_LOW("PEAK found");
        Rtn = TRUE;
        fv_info->is_peak_detected_in_inc_dec = TRUE;
    }

#endif
    AF_MSG_LOW("BV %d, PeakDown %d: fv_ratio %f",
               bv_index,
               peak_down,
               fv_ratio);
    return Rtn;
} // af_single_hj_is_peak_in_decrease

/** af_single_hj_is_peak_pattern_detected_in_inc_dec: From scanstatus,
 Analyze if
 *  the stats is considered as a peak peak pattern.
 *  This stage, not determine it's real peak or not, just check it is peak
 *  pattern
 *  @af: internal AF data structure
 **/
BOOL af_single_hj_is_peak_pattern_detected_in_inc_dec(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    af_single_hj_fv_info_t* fv_info = &saf->fv_info;
    chromatixSingleThreshold* thres = saf->p_thres;
    INT af_idx, max_index, tmp_max_index, i, max_i;
    FLOAT temp_max_fv, fv_ratio[3], fv_val[4];
#ifdef VENDOR_EDIT
    FLOAT flatDecThres, flatIncThres;
    UINT8 bv_index;
#endif
    UINT8 Rtn, fstatus[3];
    max_i = 0, tmp_max_index = 0;
    af_idx = saf->stats_index;
    max_index = fv_info->max_index;

#ifdef VENDOR_EDIT
    bv_index = af_single_hj_get_bv_index(p_caf_instance);
    if( bv_index <= EXTRA_LOW_LIGHT_BV_IDX_THR )
    {
        flatDecThres = PEAK_PATTERN_FLAT_DEC_THR;
        flatIncThres = PEAK_PATTERN_FLAT_INC_THR;
    }
    else
    {
        flatDecThres = thres->flatDecThres;
        flatIncThres = thres->flatIncThres;
    }
#endif
    //Save fstatus, fv_ratio, fv_val to temp variables
    for (i = 0; i <= 3; i++)
    {
        if (i < 3)
        {
            fstatus[i] = (UINT8) fv_info->fv_status[LSP_SAMPLE_IDX(af_idx - i)];
            fv_ratio[i] = fv_info->fv_ratio[LSP_SAMPLE_IDX(af_idx - i)];
            fv_val[i] = fv_info->fv_val[LSP_SAMPLE_IDX(af_idx - i)];
        }
        else
        {
            fv_val[i] = fv_info->fv_val[LSP_SAMPLE_IDX(af_idx - i)];
        }
    }

    //get the max index from past 4 points(including current point)
    temp_max_fv = 0;

    for (i = 0; i <= 3; i++)
    {
        if (fv_val[i] > temp_max_fv)
        {
            temp_max_fv = fv_val[i];
            tmp_max_index = af_idx - i;
            max_i = i;
            AF_MSG_LOW("fv_val[%d] %f, temp_max_f %f, temp_max_i %d,"
                       " max_i %d",
                       LSP_SAMPLE_IDX(af_idx - i),
                       fv_info->fv_val[LSP_SAMPLE_IDX(af_idx - i)],
                       temp_max_fv,
                       tmp_max_index,
                       max_i);
        }
    }

    //Check if it is fake peak pattern
    AF_MSG_LOW("tmp_max_index %d, max_index %d",
               tmp_max_index,
               max_index);

    if (tmp_max_index != max_index)
    {
        AF_MSG_LOW("It's fake pattern");
        return FALSE;
    }

    AF_MSG_LOW("max_i %d",
               max_i);

    //Check if peak is at the left end or right end
    if (max_i == 0 || max_i == 3)
    {
        AF_MSG_LOW("peak is at the left end or right end. Not peak pattern");
        return FALSE;
    }

    //Check if fv_ratio for each point exceeds the peak pattern threshold
    AF_MSG_LOW("thres->flat_dec_thres %f, thres->flat_inc_thres %f",
               thres->flatDecThres,
               thres->flatIncThres);
    Rtn = TRUE;

    for (i = 0; i <= 2; i++)
    {
        if ((fstatus[i] == HJ_FOCAL_STATUS_DECREASE) &&
#ifdef VENDOR_EDIT
                (fv_ratio[i] > flatDecThres))
#else
                (fv_ratio[i] > thres->flatDecThres))
#endif
        {
            AF_MSG_LOW("fv_ratio[%d] %f",
                       i,
                       fv_ratio[i]);
            Rtn = FALSE;
            break;
        }
        else if ((fstatus[i] == HJ_FOCAL_STATUS_INCREASE) &&
#ifdef VENDOR_EDIT
                (fv_ratio[i] < flatIncThres))
#else
                (fv_ratio[i] < thres->flatIncThres))
#endif
        {
            AF_MSG_LOW("fv_ratio[%d] %f",
                       i,
                       fv_ratio[i]);
            Rtn = FALSE;
            break;
        }
    }

    AF_MSG_LOW("Peak pattern is %s detected",
               Rtn == TRUE ? "" : "NOT");
    return Rtn;
}

/** af_single_hj_is_peak_in_inc_dec: From scanstatus, Analyze if
 *  the stats is considered as a peak.
 *
 *  @af: internal AF data structure
 **/
UINT8 af_single_hj_is_peak_in_inc_dec(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    af_single_hj_fv_info_t* fv_info = &saf->fv_info;
    chromatixSingleThreshold* thres = saf->p_thres;
    FLOAT fv_ratio = fv_info->fv_cur / fv_info->fv_max;
    ;
    BOOL Rtn = FALSE;
    UINT8 bv_index = af_single_hj_get_bv_index(p_caf_instance);
    fv_info->is_peak_detected_in_inc_dec =
        af_single_hj_is_peak_pattern_detected_in_inc_dec(p_caf_instance);

    if (fv_info->noise_cnt == 0)
    {
        if ((fv_ratio < thres->increaseDecreasePeakThreshold.Threshold[bv_index]))
        {
            AF_MSG_LOW("PEAK FOUND thres->inc_dec.thres[%d] %f!",
                       bv_index,
                       thres->increaseDecreasePeakThreshold.Threshold[bv_index]);
            Rtn = TRUE;
        }
    }

    AF_MSG_LOW("bv_index %d, fv_ratio %f, noise_cnt %d!!!",
               bv_index,
               fv_ratio,
               fv_info->noise_cnt);
    /*This is to check if reverse scan meets peak pattern with no noise count
     If yes, this is peak*/    // check with Justin on this, have a log to confirm if this is a good change
    //  if((saf->reverse_scan_cnt > 0) && (fv_info->noise_cnt == 0)) {
    //    AF_MSG_LOW("Reverse scan with Peak pattern with no noise!");
    //    Rtn = TRUE;
    //  }
    return (UINT8) Rtn;
} // af_single_hj_is_peak_in_inc_dec

/** af_single_hj_get_pre_scan_result_with_3frames: Analyze
 *  scanStatus of 3 frames to determine Scan result
 *
 *  @af: internal AF data structure
 **/
UINT8 af_single_hj_get_pre_scan_result_with_3frames(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    af_single_hj_fv_info_t* fv_info = &saf->fv_info;
    chromatixSingleThreshold* thres = saf->p_thres;
    UINT8 _1st, _2nd, bv_index, search_result;
    INT af_idx, max_index, pre_index;
    FLOAT fv_cur, fv_max, fv_ratio;
    af_idx = saf->stats_index;
    pre_index = LSP_SAMPLE_IDX(saf->stats_index - 1);
    max_index = fv_info->max_index;
    search_result = HJ_SCAN_INVALID;
    fv_cur = fv_info->fv_cur;

    if (saf->reverse_scan_cnt == 0)
    {
        fv_max = fv_info->fv_val[max_index];
    }
    else
    {
        // use the Max in current search direction only
        fv_max = af_single_hj_util_get_current_max_val(p_caf_instance);
    }

    fv_ratio = fv_cur / fv_max;
    _1st = (UINT8) fv_info->fv_status[pre_index];
    _2nd = (UINT8) fv_info->fv_status[af_idx];
    bv_index = af_single_hj_get_bv_index(p_caf_instance);
    AF_MSG_LOW("_1st %d, _2nd %d, thres->dec_dec_3frame.thres[bv_index] %f!!!",
               _1st,
               _2nd,
               thres->threeFrameDecreaseDecreasePeakThreshold.Threshold[bv_index]);
    AF_MSG_LOW("af_idx %d, max_index %d, bv_index %d",
               af_idx,
               max_index,
               bv_index);

    if ((_1st == HJ_FOCAL_STATUS_DECREASE) &&
            (_2nd == HJ_FOCAL_STATUS_DECREASE))
    {
        if (fv_ratio
                < thres->threeFrameDecreaseDecreasePeakThreshold.Threshold[bv_index])
        {
            search_result = HJ_SCAN_DECREASE;
            AF_MSG_LOW("Decrease pattern in 3 frames");
        }
    }
    else if ((_1st == HJ_FOCAL_STATUS_INCREASE) &&
             (_2nd == HJ_FOCAL_STATUS_DECREASE))
    {
        if ((pre_index == max_index)
                &&
                (fv_ratio
                 < thres->threeFrameIncreaseDecreasePeakThreshold.Threshold[bv_index]))
        {
            search_result = HJ_SCAN_PEAK;
            AF_MSG_LOW("peak pattern in 3 frames");
        }
    }

    if (search_result == HJ_SCAN_INVALID)
        AF_MSG_LOW("Invalid pattern in 3 frames");

    AF_MSG_LOW("fv_ratio %f, BV %d",
               fv_ratio,
               bv_index);
    return search_result;
} // af_single_hj_get_pre_scan_result_with_3frames

/** af_single_hj_get_pre_scan_result: Analyze
 *  scanStatus of 4 consecutive stats to determine Scan result.
 *
 *  @af: internal AF data structure
 **/
VOID af_single_hj_get_pre_scan_result(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    af_single_hj_fv_info_t* fv_info = &saf->fv_info;
    af_single_hj_scan_result search_result = HJ_SCAN_INVALID;
    UINT8 prescan_result, _1st, _2nd, _3rd;
    INT af_idx;
    af_idx = saf->stats_index;

    if (saf->scan_cnt == 2)
    {
        search_result = (af_single_hj_scan_result)
                        af_single_hj_get_pre_scan_result_with_3frames(p_caf_instance);
        fv_info->prescan_result[af_idx] = search_result;
        return;
    }

    AF_MSG_LOW("Pre_scan_result: scan_cnt %d search_result %d",
               saf->scan_cnt,
               search_result);
    _1st = (UINT8) fv_info->fv_status[LSP_SAMPLE_IDX(af_idx - 2)];
    _2nd = (UINT8) fv_info->fv_status[LSP_SAMPLE_IDX(af_idx - 1)];
    _3rd = (UINT8) fv_info->fv_status[af_idx];
    prescan_result = FOCAL_TREND(_1st,
                                 _2nd,
                                 _3rd);
    AF_MSG_LOW("[index-2]%d = %d, [index-1]%d = %d, [index]%d = %d,"
               "prescan_result=%d",
               af_idx - 2,
               _1st,
               af_idx - 1,
               _2nd,
               af_idx,
               _3rd,
               prescan_result);

    if ((_1st == HJ_FOCAL_STATUS_INVALID) ||
            (_2nd == HJ_FOCAL_STATUS_INVALID) ||
            (_3rd == HJ_FOCAL_STATUS_INVALID))
    {
        AF_MSG_LOW("Invalid Trend, exit.");
        fv_info->prescan_result[af_idx] = HJ_SCAN_INVALID;
        return;
    }

    switch (prescan_result)
    {
        case PRESCAN_INC_DEC1: // 37
        case PRESCAN_INC_DEC2: // 41
        case PRESCAN_INC_DEC3: // 45
        case PRESCAN_INC_DEC4: // 57
        case PRESCAN_INC_DEC5: // 39
            if (af_single_hj_is_peak_in_inc_dec(p_caf_instance) == TRUE)
            {
                AF_MSG_LOW("HJ_SCAN_PEAK");
                search_result = HJ_SCAN_PEAK;
            }

            break;

        case PRESCAN_DECREASE1: // 42
        case PRESCAN_DECREASE2: // 43
        case PRESCAN_DECREASE3: // 46
        case PRESCAN_DECREASE4: // 58
            if (af_single_hj_is_peak_in_decrease(p_caf_instance) == TRUE)
            {
                if (fv_info->is_peak_detected_in_inc_dec == TRUE)
                {
                    AF_MSG_LOW("HJ_SCAN_PEAK");
                    search_result = HJ_SCAN_PEAK;
                }
                else
                {
                    AF_MSG_LOW("HJ_SCAN_DECREASE");
                    search_result = HJ_SCAN_DECREASE;
                }
            }

            break;

        case PRESCAN_FLAT1: // 42
        case PRESCAN_FLAT2: // 43
        case PRESCAN_FLAT3: // 46
        case PRESCAN_FLAT4: // 58
        case PRESCAN_FLAT5: // 42
        case PRESCAN_FLAT6: // 43
        case PRESCAN_FLAT7: // 46
            search_result = HJ_SCAN_INVALID;
            AF_MSG_LOW("HJ_SCAN_FLAT")
            ;
            break;

        case PRESCAN_INCREASE1: // 21
        case PRESCAN_INCREASE2: // 23
        case PRESCAN_INCREASE3: // 29
        case PRESCAN_INCREASE4: // 53
            search_result = HJ_SCAN_INCREASE;
            AF_MSG_LOW("HJ_SCAN_INCREASE")
            ;
            break;

        case PRESCAN_NOISE1:
        case PRESCAN_NOISE2:
        case PRESCAN_NOISE3:
        case PRESCAN_NOISE4:
        default:
            fv_info->is_peak_detected_in_inc_dec = FALSE;
            fv_info->noise_cnt++;
            search_result = HJ_SCAN_INVALID;
            AF_MSG_LOW("PRESCAN_NOISE")
            ;
            break;
    }

    fv_info->prescan_result[af_idx] = search_result;
} // af_single_hj_get_pre_scan_result

/** af_single_hj_handle_state_pre_scan: Handle Prescan State Which
 *  is change dynamically in the algorithm
 *
 *  @af: internal AF data structure
 **/
VOID af_single_hj_handle_state_pre_scan(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    LSP_Process* LSP_AF_sample = &saf->LSP_AF_sample;
    INT cur_pos = p_caf_instance->cur_pos;

    //loop for prescan
    if (saf->scan_cnt == 0)
    {
        af_single_hj_wait_valid_frame(p_caf_instance);
        return;
    }
    else if (saf->scan_cnt < 2)
    {
        af_single_hj_get_scan_step_INTerpolate(cur_pos,
                                               p_caf_instance,
                                               1,
                                               saf->direction);
        af_single_hj_util_move_lens(p_caf_instance,
                                    (af_move_direction_type) saf->direction,
                                    saf->pre_scan_step);
        saf->scan_cnt++;
        return;
    }
    else
    {
        af_single_hj_get_pre_scan_result(p_caf_instance);

        if (TRUE == LSP_AF_sample->is_need_HJ_Process &&
                TRUE == p_caf_instance->is_hj_enable)
        {
            af_single_hj_LSPAF_pre_scan_algo(p_caf_instance);
        }
        else
        {
            af_single_hj_pre_scan_algo(p_caf_instance);
        }
    } // af_single_hj_handle_state_pre_scan
}

/** af_single_hj_handle_state_wait_aec_stable: Handle
 *  HJ_AF_WAIT_AEC_STABLE state before continuing with search
 *
 *  @af: internal AF data structure
 **/
VOID af_single_hj_handle_state_wait_aec_stable(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    af_input_from_aec_t* aec_input = &p_caf_instance->aec_input;
    BOOL is_aec_wait_end = FALSE;
    AF_MSG_LOW("exp_index %d aec_settle %d!",
               aec_input->exp_index,
               aec_input->aec_settled);

    // If luma target is reaching, We can Exit
    if (aec_input->aec_settled)
    {
        AF_MSG_LOW("AEC Settled!");
        saf->luma_stable_cnt++;
    }
    else
    {
        saf->luma_stable_cnt = 0;
    }

    if (saf->luma_stable_cnt > 1)
    {
        is_aec_wait_end = TRUE;
    }

    if (is_aec_wait_end == TRUE)
    {
#ifdef VENDOR_EDIT
        ALOGI("[AFDBG] %s AEC Stabled, proceed with search start_pos:%d scene:%d", __func__, saf->start_pos, aec_input->AISceneID);
        if ((aec_input->AISceneID == 42 || aec_input->AISceneID == 60) &&
            aec_input->AISceneID != p_caf_instance->AI_scene_id_single_start) {
            /* scene changed during waiting aec settle, re-adjust af range for sky and building scenes. */
            af_single_hj_set_optics(p_caf_instance);
        }
#endif
        AF_MSG_HIGH("AEC Stabled, proceed with search");
        saf->saf_scan_state = HJ_AF_PRE_SCAN;
        saf->scan_cnt = 0;
        saf->luma_stable_cnt = 0;
        // Set the destination to Start position
        saf->dest_pos = saf->start_pos;
        af_single_hj_util_move_to_dest(p_caf_instance);
    }
} // af_single_hj_handle_scanstate_wait_aec_stable

/** af_single_hj_handle_state: Handle internal stats of the
 *  algorithm upon new stats.
 *
 *  @af: internal AF data structure.*
 *  return: 0 for sucess
 **/
static INT af_single_hj_handle_state(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;

    // Update debug data before frame.
    if (HJ_AF_WAIT_AEC_STABLE != saf->saf_scan_state)
    {
        //        update_af_debug_data_before_frame_single_hj_af(p_caf_instance->af_single);
    }

    switch (saf->saf_scan_state)
    {
        case HJ_AF_WAIT_AEC_STABLE:
            {
                AF_MSG_LOW("HJ_AF_WAIT_AEC_STABLE");
                af_single_hj_handle_state_wait_aec_stable(p_caf_instance);
            }
            break;

        case HJ_AF_PRE_SCAN:
            {
                AF_MSG_LOW("HJ_AF_PRE_SCAN");
                af_single_hj_handle_state_pre_scan(p_caf_instance);
            }
            break;

        case HJ_AF_FINE_SCAN:
            {
                AF_MSG_LOW("HJ_AF_FINE_SCAN");
                af_single_hj_handle_state_fine_scan(p_caf_instance);
            }
            break;

        case HJ_AF_GOTO_PEAK:
            {
                AF_MSG_LOW("HJ_AF_GOTO_PEAK");
                af_single_hj_handle_state_goto_peak(p_caf_instance);
                break;
            }

        case HJ_AF_GOTO_DEST:
            {
                AF_MSG_LOW("HJ_AF_GOTO_DEST");
                af_single_hj_util_move_to_dest(p_caf_instance);
            }
            break;

        default:
            break;
    }

    // Update debug data after frame.
    if (HJ_AF_WAIT_AEC_STABLE != saf->saf_scan_state)
    {
        //        update_af_debug_data_after_frame_single_hj_af(p_caf_instance->af_single);
    }

    return 0;
} // af_single_hj_handle_state

/** af_single_hj_fv_update_sample: update the incoming sample with index
 af_index in fv_info
 *  @fv_info: pointer to fv array
 *   @af_index: the index to the array.
 *
 *  Return none
 **/

VOID af_single_hj_fv_update_sample(
    af_caf_search_internal_t* p_caf_instance,
    af_single_hj_fv_info_t* fv_info,
    INT af_idx)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    UINT8 fv_status;
    BOOL is_confident_trend = false;
    FLOAT tmp_fv, tmp_cur, fv_ratio;
    FLOAT fv_inc_flat_ratio_th, fv_dec_flat_ratio_th;
    fv_inc_flat_ratio_th = FV_INC_FLAT_RATIO_TH;
    fv_dec_flat_ratio_th = FV_DEC_FLAT_RATIO_TH;
    BOOL is_spotlight_scene = IS_SPOTLIGHT_SCENE;    //p_af_INTernel->spd.spotlight_detect_final;

    if (is_spotlight_scene)
    {
        fv_inc_flat_ratio_th = FV_INC_FLAT_RATIO_SPOT_LIGHT_TH;
        fv_dec_flat_ratio_th = FV_DEC_FLAT_RATIO_SPOT_LIGHT_TH;
    }

    // Value may be used uninitialized
    tmp_fv = fv_info->fv_val[af_idx];
    tmp_cur = fv_info->fv_cur;

    if (TRUE == p_caf_instance->is_hj_enable)
    {
        // if HJ algorithm is enabled then affect gyro input to FV sample
        af_single_hj_LSPAF_fv_update_sample(p_caf_instance,
                                            fv_info,
                                            &tmp_fv,
                                            &tmp_cur);
    }

    // Check max value and peak down count from Max position
    if (tmp_fv > fv_info->fv_max)
    {
        fv_info->fv_max = tmp_fv;
        fv_info->max_index = af_idx;

        if (saf->af_mode == AF_RUN_MODE_VIDEO)
            fv_info->peak_down = 0;
    }
    else
    {
        if (saf->af_mode == AF_RUN_MODE_VIDEO)
            fv_info->peak_down++;
    }

    /* Update uiTempMin only if uiTempVal is smaller than uiTempMin, and
     it is a valid value greater than 0 */
    if ((tmp_fv < fv_info->fv_min) && (tmp_fv > 0))
        fv_info->fv_min = tmp_fv;

    fv_info->fv_pre = tmp_cur;
    fv_info->fv_cur = tmp_fv;

    // Check increasement or decreasement
    if (fv_info->fv_pre == 0)
    {
        fv_status = HJ_FOCAL_STATUS_INVALID;
        fv_info->fv_ratio[af_idx] = 0;
    }
    else
    {
        fv_ratio = fv_info->fv_cur / fv_info->fv_pre;

        // fv_status : Status of increasement or decreasement of FV value
        if (fv_info->fv_cur >= fv_info->fv_pre)
        {
            is_confident_trend = (fv_ratio > fv_inc_flat_ratio_th);

            if (is_confident_trend)
            {
                fv_status = HJ_FOCAL_STATUS_INCREASE;
                AF_MSG_LOW("inc fv thr %f, change ratio %f",
                           fv_inc_flat_ratio_th, fv_ratio);
            }
            else
            {
                fv_status = HJ_FOCAL_STATUS_FLAT;
                AF_MSG_LOW("flat fv thr %f, @ inc, change ratio %f",
                           fv_inc_flat_ratio_th, fv_ratio);
            }
        }
        else
        {
            is_confident_trend = (fv_ratio < fv_dec_flat_ratio_th);

            if (is_confident_trend)
            {
                fv_status = HJ_FOCAL_STATUS_DECREASE;
                AF_MSG_LOW("dec fv ratio %f, change ratio %f ",
                           fv_dec_flat_ratio_th,
                           fv_ratio);
            }
            else
            {
                fv_status = HJ_FOCAL_STATUS_FLAT;
                AF_MSG_LOW("flat fv ratio @ dec %f, change ratio %f",
                           fv_dec_flat_ratio_th,
                           fv_ratio);
            }
        }

        if (saf->af_mode != AF_RUN_MODE_VIDEO)
        {
            if (fv_status == HJ_FOCAL_STATUS_INCREASE)
            {
                fv_info->peak_down = 0;
            }
            else if (fv_status == HJ_FOCAL_STATUS_DECREASE)
            {
                fv_info->peak_down++;
            }
        }

        // fv_ratio : 1st-order differentiation
        fv_info->fv_ratio[af_idx] = fv_info->fv_cur / fv_info->fv_pre;
    }

    // Save Status
    fv_info->fv_status[af_idx] = (af_single_hj_focal_status) fv_status;
    AF_MSG_HIGH("Af_getvalue is_confident_trend %d fv_inc_flat_ratio_th %f"
                "fv_dec_flat_ratio_th %f",
                is_confident_trend,
                fv_inc_flat_ratio_th,
                fv_dec_flat_ratio_th);
}

/** af_single_hj_reset_fv_single:
 *
 *
 **/
VOID af_single_hj_reset_fv_single(
    af_caf_search_internal_t* p_caf_instance,
    INT af_idx)
{
    af_single_hj_fv_info_t* fv =
        &p_caf_instance->af_single->af_hj_lib_info.fv_info;
    AF_MSG_LOW("Reset single af histories");
    fv->max_index = -1;
    fv->fv_max = 0;
    fv->fv_min = Q3A_FLT_MAX;
    fv->fv_pre = 0;
    fv->fv_cur = 0;
    fv->peak_down = 0;
    fv->noise_cnt = 0;
    fv->possible_peak = FALSE;
    fv->is_peak_detected_in_inc_dec = FALSE;
    fv->fv_status[af_idx] = HJ_FOCAL_STATUS_INVALID;
    fv->fv_val[af_idx] = 0;
    fv->fv_ratio[af_idx] = 0;
    fv->prescan_result[af_idx] = HJ_SCAN_INVALID;
    fv->pre_gyro = 0;
    fv->gyro_diff_ratio = 0.0f;
    fv->change_ratio_fv = 0.0f;
    fv->is_HJ_stats = FALSE;
    // Reset LSP info
    Q3A_MEMSET(&p_caf_instance->af_single->af_hj_lib_info.lsp_info,
               0,
               sizeof(p_caf_instance->af_single->af_hj_lib_info.lsp_info));
}

/** af_single_hj_calculate_gyro_metrics:
 * update gyro metrics
 *
 *  @gyro_info: data structure required to handle gyro assisted
 *            continuous search
 *
 *  @gyro_info: input data from gyro
 *
 *  Return 0: Success  -1: Gyro data not ready
 **/
// TODO: make this a common function
// that can be called from both single and haf fine search
BOOL af_single_hj_calculate_gyro_metrics(
    af_caf_search_internal_t* p_caf_instance)
{
    INT ret = 0;
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    af_caf_gyro_stats_t* gyro_info = &saf->gyro_stats;
    af_core_input_gyro_type* gyro_input = &p_caf_instance->gyro_input;
    FLOAT gyro_average[3] =
    { 0.0f, 0.0f, 0.0f };
    FLOAT sq_gyro_average[3] =
    { 0.0f, 0.0f, 0.0f };
    FLOAT samples_sum[3] =
    { 0.0f, 0.0f, 0.0f };
    FLOAT samples_std[3] =
    { 0.0f, 0.0f, 0.0f };
    FLOAT gyro_x = 0.0f;
    FLOAT gyro_y = 0.0f;
    FLOAT gyro_z = 0.0f;
    INT j = 0;
    AF_MSG_LOW("HJ_CAF sample_count %d",
               gyro_input->sample_count);

    if (gyro_input->enabled && gyro_input->sample_count > 0)
    {
        // For all the samples received
        for (j = 0; j < gyro_input->sample_count; j++)
        {
            gyro_x = gyro_input->angular_velocity_x[j];
            gyro_y = gyro_input->angular_velocity_y[j];
            gyro_z = gyro_input->angular_velocity_z[j];
            AF_MSG_LOW("HJ gyro sample[%d], %f, %f, %f",
                       j,
                       gyro_x,
                       gyro_y,
                       gyro_z);
            // Sum up all the values for each axis
            samples_sum[0] += gyro_x;
            samples_sum[1] += gyro_y;
            samples_sum[2] += gyro_z;
            // get sum of squares for each axis
            sq_gyro_average[0] += gyro_x * gyro_x;
            sq_gyro_average[1] += gyro_y * gyro_y;
            sq_gyro_average[2] += gyro_z * gyro_z;
        }

        // For all 3 axis x, y, z
        for (j = 0; j < 3; j++)
        {
            //Get average
            gyro_average[j] = samples_sum[j] / gyro_input->sample_count;
            //Get average of square values
            sq_gyro_average[j] /= gyro_input->sample_count;
            // calculating standard deviation -> rootof (average of squares - square of averages)
            samples_std[j] = (FLOAT) sqrt(sq_gyro_average[j]
                                          - (gyro_average[j] * gyro_average[j]));
            AF_MSG_LOW("gyro_average %f sq_gyro_average %f",
                       gyro_average[j],
                       sq_gyro_average[j]);
        }

        gyro_info->sqr = gyro_average[0] * gyro_average[0] +
                         gyro_average[1] * gyro_average[1] +
                         gyro_average[2] * gyro_average[2];
        gyro_info->sqr = (FLOAT) sqrt(gyro_info->sqr);
        gyro_info->x_sum = samples_sum[0];
        gyro_info->y_sum = samples_sum[1];
        gyro_info->z_sum = samples_sum[2];
        gyro_info->x_std = samples_std[0];
        gyro_info->y_std = samples_std[1];
        gyro_info->z_std = samples_std[2];
        gyro_info->avg_std = (FLOAT)(gyro_info->x_std + gyro_info->y_std
                                     + gyro_info->z_std) / 3;
        AF_MSG_LOW("Gyro metrics - gyro_sqr: %f gyro_avg_std %f",
                   gyro_info->sqr,
                   gyro_info->avg_std);
        ret = TRUE;
    }
    else
    {
        AF_MSG_LOW("Gyro data not ready yet. Will try later!!");
        ret = FALSE;
    }

    return (BOOL) ret;
} // af_single_hj_calculate_gyro_metrics

/** af_single_hj_get_value: Entry point to process the stats, and
 *  compute all the related params for the current stats sample
 *  and fill out Stats entry object
 *
 *  @af: internal AF data structure
 **/
VOID af_single_hj_get_value(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    af_stats_info_t* stats = &p_caf_instance->stats;
    af_single_hj_fv_info_t* fv_info = &saf->fv_info;
    INT af_idx, hw_index, is_invalid_frame, cur_pos;
    cur_pos = p_caf_instance->cur_pos;
    // Update Stats Index
    // depending on SW or hw Stats, current stats will have a lag
    saf->hw_stats_index = saf->index;
    saf->stats_index = (!saf->use_paaf) ? saf->hw_stats_index : LSP_SAMPLE_IDX(saf->hw_stats_index - 1);
    hw_index = saf->hw_stats_index;
    af_idx = saf->stats_index;
    saf->af_index[hw_index] = cur_pos;
    saf->index = LSP_SAMPLE_IDX(saf->index + 1);

    /* need to check this to by pass processing when going to peak
     incase stats changes max during going to peak*/
    if (saf->saf_scan_state == HJ_AF_GOTO_PEAK || saf->saf_scan_state == HJ_AF_GOTO_DEST)
    {
        AF_MSG_LOW("i %d, going to peak, no need to get value"
                   " stats_pos %d, cur_pos %d",
                   hw_index,
                   saf->af_index[af_idx],
                   cur_pos);
        return;
    }

    // determine if stats is invalid according to different factors
    /* 1. Frame is marked invalid
     2. SW stats is enabled and marked invalid */
    is_invalid_frame = (saf->use_paaf) ? (saf->is_valid_frame_sw ? FALSE : TRUE) : (saf->is_valid_frame ? FALSE : TRUE);


    // Invalidate Frame when aec not settled
    if (saf->saf_scan_state == HJ_AF_WAIT_AEC_STABLE)
    {
        is_invalid_frame = TRUE;
    }

    //if HJ algorithm is enabled from chromatix or setprop
    if (p_caf_instance->is_hj_enable)
    {
        af_single_hj_calculate_gyro_metrics(p_caf_instance);
    }

    if (is_invalid_frame)
    {
        AF_MSG_LOW("Frame invalid, reset params");
        af_single_hj_reset_fv_single(p_caf_instance, af_idx);
    }
    else
    {
        fv_info->fv_val[af_idx] = p_caf_instance->stats.fv_hv;
#if 0
        fv_info->fv_val[af_idx] = (saf->use_paaf) ? (FLOAT) stats->sw_stats[af_idx].fV : stats->cur_fv;

        if (saf->use_paaf)
        {
            // Proctection to take zero stats
            if (fv_info->fv_val[af_idx] == 0)
            {
                AF_MSG_LOW("ERROR! SW Stats is zero");

                if (af_idx > 0 && fv_info->fv_val[af_idx - 1] != 0)
                {
                    fv_info->fv_val[af_idx] = fv_info->fv_val[af_idx - 1];
                    stats->sw_stats[af_idx].fV = (UINT32) fv_info->fv_val[af_idx];
                    AF_MSG_LOW("Duplicated Last Stats value");
                }
                else
                {
                    AF_MSG_LOW("ERROR! Previous Stats also zero");
                }
            }
        }

#endif
        af_single_hj_fv_update_sample(p_caf_instance, fv_info, af_idx);
        af_single_hj_LSP_fv_update_sample(p_caf_instance, fv_info, af_idx);
    }

    AF_MSG_HIGH("i %d, cur_pos %d FV %.1f H1: %.1f H2: %.1f V: %.1f HV: %.1f"
                " H1_NORM: %.1f H2_NORM: %.1f V_NORM: %.1f HV_NORM: %.1f",
                hw_index,
                cur_pos,
                fv_info->fv_cur,
                stats->fv_h1,
                stats->fv_h2,
                stats->fv_v,
                stats->fv_hv,
                stats->fv_h1_norm,
                stats->fv_h2_norm,
                stats->fv_v_norm,
                stats->fv_hv_norm);
} // af_single_hj_get_value

/** af_single_hj_set_start_pos: Determine the starting location for
 *  starting position at the beginiing of the search
 *
 *  @af: internal AF data structure.*
 **/
VOID af_single_hj_set_start_pos(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    chromatixContrastAFType *pContrastTuning = p_caf_instance->af_tuning->pContrastTuningData;
    chromatixAFTuningSingle* p_tuning = &pContrastTuning->singleSearch;
    INT far_zone, near_zone, mid_zone, zone;
    UINT8 is_far_zone, is_near_zone, step_back_direction;
    INT sign, scan_steps, cur_pos;
    // Load tuning paramters
    far_zone = p_tuning->index[p_tuning->optics.farZone];
    near_zone = p_tuning->index[p_tuning->optics.nearZone];
    mid_zone = p_tuning->index[p_tuning->optics.midZone];
    saf->is_hys_comp_needed = p_tuning->isHysteresisCompensationNeeded;
    BOOL is_af_caf = saf->is_af_caf;
    //Get reference paramters
    cur_pos = p_caf_instance->cur_pos;

    if (p_caf_instance->run_mode == AF_RUN_MODE_VIDEO)
    {
        AF_MSG_LOW("AF_RUN_MODE_VIDEO & %s : NearEnd %d FarEnd %d",
                   is_af_caf == TRUE ? "CAF" : "TAF",
                   saf->near_end_pos,
                   saf->far_end_pos);
        saf->af_mode = AF_RUN_MODE_VIDEO;
    }
    else
    {
        AF_MSG_LOW("AF_RUN_MODE_PREVIEW & %s : NearEnd %d FarEnd %d",
                   is_af_caf == TRUE ? "CAF" : "TAF",
                   saf->near_end_pos,
                   saf->far_end_pos);
        saf->af_mode = AF_RUN_MODE_PREVIEW;
    }

    /* Determine the first invalid frame as valid or not,
     * according to chromatix*/
    saf->is_valid_frame = p_tuning->ignoreFirstInvalidFrame ? FALSE : TRUE;
    // Set up starting position
    // define zones
    is_far_zone = cur_pos > far_zone;
    is_near_zone = cur_pos < near_zone;

    // define step_backs for start position and sign(direction)
    if (cur_pos > mid_zone)
    {
        scan_steps = pContrastTuning->stepBack.stepBackEnable ?
            pContrastTuning->stepBack.stepBackStartInfinity
            : (p_tuning->actuatorType == ACT_TYPE_CLOSELOOP) ? 1 : 2;
        AF_MSG_LOW("inf area, scan_steps =%d", scan_steps);
    }
    else
    {
        scan_steps = pContrastTuning->stepBack.stepBackEnable ?
            pContrastTuning->stepBack.stepBackStartMacro
            : (p_tuning->actuatorType == ACT_TYPE_CLOSELOOP) ? 1 : 2;
        AF_MSG_LOW("mac area, scan_steps =%d", scan_steps);
    }

    sign = (cur_pos <= mid_zone) ? -1 : 1;

    if (saf->af_run_first_time == TRUE)
    {
        // first time start from location specify by Tuning Header
        saf->direction = DIR_TO_NEAR;
        // ensure init position is safe
        saf->start_pos = p_tuning->index[p_tuning->optics.initialPosition];
        AF_MSG_LOW("First Time start up");
    }
    else
    {
        if (is_far_zone)
        {
            // Far zone: start search at far zone
            saf->direction = DIR_TO_NEAR;
            zone = p_tuning->index[p_tuning->optics.farStartPosition];
            af_single_hj_get_scan_step_INTerpolate(zone,
                                                   p_caf_instance,
                                                   scan_steps,
                                                   DIR_TO_FAR);
            saf->start_pos = zone + sign * saf->pre_scan_step;
            saf->start_pos =
                (saf->start_pos > saf->far_end_pos) ?
                saf->far_end_pos :
                saf->start_pos;
        }
        else if (is_near_zone)
        {
            // Near zone: start search at near zone
            saf->direction = DIR_TO_FAR;
            zone = p_tuning->index[p_tuning->optics.nearStartPosition];
            af_single_hj_get_scan_step_INTerpolate(zone,
                                                   p_caf_instance,
                                                   scan_steps,
                                                   DIR_TO_NEAR);
            saf->start_pos = zone + sign * saf->pre_scan_step;
            saf->start_pos =
                (saf->start_pos < saf->near_end_pos) ?
                saf->near_end_pos :
                saf->start_pos;
        }
        else
        {
            // start with step back to optimize the speed
            saf->direction = (UINT8) ((cur_pos <= mid_zone) ?
                                      DIR_TO_FAR :
                                      DIR_TO_NEAR);
            step_back_direction = (UINT8) ((cur_pos <= mid_zone) ?
                                           DIR_TO_NEAR :
                                           DIR_TO_FAR);
            af_single_hj_get_scan_step_INTerpolate(cur_pos,
                                                   p_caf_instance,
                                                   scan_steps,
                                                   step_back_direction);
            // start two scans step back to optimize the speed
            saf->start_pos = cur_pos + sign * saf->pre_scan_step;
        }
    }

    AF_MSG_LOW("cur_pos %d, near_zone %d, far_zone %d, mid_zone %d firstrun %d",
               cur_pos,
               near_zone,
               far_zone,
               mid_zone,
               saf->af_run_first_time);

    // Error Check for Start Pos
    if (saf->start_pos < 0)
    {
        AF_MSG_LOW("Error! Neg start %d, capping",
                   saf->start_pos);
        saf->start_pos = p_tuning->index[p_tuning->optics.initialPosition];
    }

    // Final check must be to have start position within search range
    if (saf->start_pos > saf->far_end_pos)
        saf->start_pos = saf->far_end_pos;

    if (saf->start_pos < saf->near_end_pos)
        saf->start_pos = saf->near_end_pos;

    AF_MSG_LOW("Starting Position : %d",
               saf->start_pos);
} // af_single_hj_set_start_pos

/** af_single_hj_set_optics : Fill out Search
 *  criteria and optic information for the search depending on
 *  different modes and scenario.
 *
 *  @af: internal AF data structure.*
 **/
VOID af_single_hj_set_optics(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_t* af = p_caf_instance->af_single;
    af_single_hj_lib_info_t* saf = &af->af_hj_lib_info;
    chromatixAFTuningSingle* p_tuning =
        &p_caf_instance->af_tuning->pContrastTuningData->singleSearch;
    af_mode_type af_mode = p_caf_instance->mode;
    af_haf_focus_search_data_t* output = &p_caf_instance->af_single->output;
    BOOL is_af_caf;
    saf->is_af_caf = af_mode == AF_MODE_CAF ? TRUE : FALSE;
    is_af_caf = saf->is_af_caf;
    AF_MSG_LOW("output: next_pos %d dir %d near %d far %d",
               output->next_pos, output->search_dir,
               output->range_near, output->range_far);
    /* Set up Search Regions, each region have different settings */
    saf->reg1_thrd = p_tuning->index[p_tuning->optics.searchRegion1];
    saf->reg2_thrd = p_tuning->index[p_tuning->optics.searchRegion2];
    saf->reg3_thrd = p_tuning->index[p_tuning->optics.searchRegion3];

    // Seperate Search Regions
    if (is_af_caf == FALSE)
    {
        // TAF specific
        saf->far_end_pos = p_tuning->index[p_tuning->optics.TAFFarEnd];
        saf->near_end_pos = p_tuning->index[p_tuning->optics.TAFNearEnd];
    }
    else
    {
        // CAF Specific
#ifdef VENDOR_EDIT
        af_input_from_aec_t* aec_input = &p_caf_instance->aec_input;
        af_input_from_img_sensor_t *sensor_info = &p_caf_instance->sensor_info;
        if (p_caf_instance->roi.type == AF_ROI_TYPE_GENERAL &&
            (p_caf_instance->gravity_info.g_vector[2] < -3.0 || aec_input->AISceneID == 42 || aec_input->AISceneID == 60) &&
            sensor_info->driver_IC_ID != 0x61)
        {
            saf->far_end_pos = 370;
            saf->near_end_pos = p_caf_instance->af_range.near_end;
        }
        else
        {
            saf->far_end_pos = p_caf_instance->af_range.far_end;
            saf->near_end_pos = p_caf_instance->af_range.near_end;
        }
#else
        saf->far_end_pos = p_caf_instance->af_range.far_end;
        saf->near_end_pos = p_caf_instance->af_range.near_end;
#endif
    }

    af_single_hj_set_start_pos(p_caf_instance);
    AF_MSG_HIGH("DEFAULT: start_pos %d dir %d near %d far %d range limit %d - %d",
                saf->start_pos,
                saf->direction,
                saf->near_end_pos,
                saf->far_end_pos,
                p_caf_instance->af_range.near_end,
                p_caf_instance->af_range.far_end);

    if (af_single_is_pos_inside_range(p_caf_instance, output->range_near)
            && af_single_is_pos_inside_range(p_caf_instance, output->range_far)
            && af_single_is_pos_inside_range(p_caf_instance, output->next_pos)
            && (AF_MOVE_NEAR == output->search_dir || AF_MOVE_FAR == output->search_dir))
    {
        AF_MSG_HIGH("update af_haf_focus_search_data_t.");
        saf->near_end_pos = GETMAX(output->range_near, saf->near_end_pos);
        saf->far_end_pos = GETMIN(output->range_far, saf->far_end_pos);
        saf->start_pos = CLAMP(output->next_pos, saf->near_end_pos, saf->far_end_pos);
        if ((saf->start_pos == saf->far_end_pos) && (AF_MOVE_FAR == output->search_dir))
        {
            saf->direction = AF_MOVE_NEAR;
        } else if ((saf->start_pos == saf->near_end_pos) && (AF_MOVE_NEAR == output->search_dir))
        {
            saf->direction = AF_MOVE_FAR;
        } else
        {
            saf->direction = (uint8)output->search_dir;
        }
        AF_MSG_HIGH("Override: start_pos %d dir %d near %d far %d range limit %d - %d",
                    saf->start_pos,
                    saf->direction,
                    saf->near_end_pos,
                    saf->far_end_pos,
                    p_caf_instance->af_range.near_end,
                    p_caf_instance->af_range.far_end);
    }
} // af_single_hj_set_optics_for_normal_af_mode

/** af_single_hj_reset_search_data: Reset all search data upon
 *  start of search
 *
 *  @af: internal AF data structure.*
 **/
VOID af_single_hj_reset_search_data(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_lib_info_t* saf = &p_caf_instance->af_single->af_hj_lib_info;
    af_single_hj_fv_info_t* fv = &saf->fv_info;
    af_single_hj_t* g_af = (af_single_hj_t*) p_caf_instance->af_single;
    INT i;
    AF_MSG_LOW("Reset single af histories");
    // Reset the Stats index
    //  stats->index = 0; //check if this causes any side effect
    saf->scan_cnt = 0;
    saf->reverse_scan_cnt = 0;
    fv->max_index = -1;
    fv->peak_down = 0;
    fv->fv_max = 0;
    fv->fv_min = Q3A_FLT_MAX;
    fv->fv_cur = 0;
    fv->fv_pre = 0;
    fv->sw_bad_stats = 0;
    fv->is_peak_detected_in_inc_dec = FALSE;
    fv->noise_cnt = 0;
    fv->pre_gyro = 0;
    fv->gyro_diff_ratio = 0.0f;
    fv->change_ratio_fv = 0.0f;
    fv->is_HJ_stats = FALSE;

    for (i = 0; i < AF_COLLECTION_POINTS; i++)
    {
        fv->fv_status[i] = HJ_FOCAL_STATUS_INVALID;
        fv->fv_val[i] = 0;
        fv->fv_ratio[i] = 0;
        fv->prescan_result[i] = HJ_SCAN_INVALID;
    }

    saf->saf_scan_state = HJ_AF_SINGLE_START;
    saf->saf_scan_state_prev = HJ_AF_SINGLE_START;
    Q3A_MEMSET(saf->af_index,
               0,
               AF_COLLECTION_POINTS * sizeof(UINT32));
    g_af->output.progress = AF_PROGRESS_0_NONE;
    // Reset LSP info
    Q3A_MEMSET(&p_caf_instance->af_single->af_hj_lib_info.lsp_info,
               0,
               sizeof(p_caf_instance->af_single->af_hj_lib_info.lsp_info));
} // af_single_hj_reset_search_data

/** af_single_hj_start_srch: Starting point of the search, setup
 *  and initialize the search params
 *
 *  @af: internal AF data structure.*
 **/
static INT af_single_hj_start_srch(
    af_caf_search_internal_t* p_caf_instance)
{
    af_single_hj_t* af = p_caf_instance->af_single;
    af_single_hj_lib_info_t* saf = &af->af_hj_lib_info;
    af_input_from_aec_t* aec_input = &p_caf_instance->aec_input;
    af_roi_info_t* roi_info = &p_caf_instance->roi;
    af_mode_type af_mode = p_caf_instance->mode;
    INT far_limit, near_limit, search_range;
    chromatixContrastAFType* pContrastTuningData =
        p_caf_instance->af_tuning->pContrastTuningData;
    chromatixAFAlgoTuningType* pAlgoTuningData =
        p_caf_instance->af_tuning->pAlgoTuningData;
    INT cur_pos = p_caf_instance->cur_pos;

#ifdef VENDOR_EDIT
    ALOGI("[AFDBG] %s Start position check cur_pos = %d aec_settled:%d scene:%d", __func__,
               cur_pos, aec_input->aec_settled, aec_input->AISceneID);
    p_caf_instance->AI_scene_id_single_start = aec_input->AISceneID;
#endif

    AF_MSG_LOW("Start position check cur_pos =%d ",
               cur_pos);
    AF_MSG_HIGH("%d HJ_Algorithm_enable",
                pAlgoTuningData->handJitter.AFSingle.enable);
    AF_MSG_HIGH("%d HJ_Algorithm_enable_from Setprop",
                p_caf_instance->settingsInfo->enableHJAF);
    // If chromatix param and setprop is true run HJ AF
    /*
     If the feature is enabled in tuning, below setprop will give the
     flexibility to enable / disable the feature runtime
    */
    p_caf_instance->settingsInfo->enableHJAF =
        p_caf_instance->settingsInfo->enableHJAF
        & pAlgoTuningData->handJitter.AFSingle.enable;
    // Decision for Enabling SW Stats
    saf->use_paaf =
        (pContrastTuningData->fullSweep.previewAssistedAFEnable) ?
        TRUE :
        FALSE;
    AF_MSG_HIGH("%s using PAp_caf_instance",
                saf->use_paaf ? "" : "NOT");
    // Assign pointers for threshold to stats type
    saf->p_thres =
        (saf->use_paaf) ?
        &pContrastTuningData->singleSearch.software :
        &pContrastTuningData->singleSearch.hardware;
    // Initialize Data and states
    af_single_hj_reset_search_data(p_caf_instance);
    // Set AF Optics
    af_single_hj_set_optics(p_caf_instance);

    // Check if AEC wait is needed
    if (aec_input->aec_settled)
    {
        AF_MSG_LOW("AEC indicate settled, skip AEC check, start search");
        Q3A_ATRACE_INT("CAF:COARSE_SEARCH_TIME", 1);
        saf->saf_scan_state = HJ_AF_PRE_SCAN;
    }
    else
    {
        AF_MSG_LOW("AEC not settled, wait for AEC stable first");
        saf->saf_scan_state = HJ_AF_WAIT_AEC_STABLE;
    }

    if ((roi_info->type == AF_ROI_TYPE_TOUCH) ||
            (saf->af_run_first_time == TRUE && af_mode == AF_MODE_CAF))
    {
        saf->saf_scan_state = HJ_AF_WAIT_AEC_STABLE;
    }

    /* if the current lens position is within search range,
     we run Fine search  */
    if ((cur_pos > af->output.range_near &&
            cur_pos < af->output.range_far))
    {
        far_limit = pContrastTuningData->singleSearch.index[AF_INF_LIMIT_IDX];
        near_limit = pContrastTuningData->singleSearch.index[AF_NEAR_LIMIT_IDX];
        search_range = af->output.range_far - af->output.range_near;

        if (search_range
                < AF_LSP_FINE_SEARCH_RANGE_RATIO * (far_limit - near_limit))
        {
            AF_MSG_LOW("HJ_AF_FINE_SCAN ");
            saf->saf_scan_state = HJ_AF_FINE_SCAN;
        }
    }

    /* If Waiting for AEC to Settle first, we will only move to start pos once
     the AEC is settled. This is to enhance User Experience. */
    if (saf->saf_scan_state != HJ_AF_WAIT_AEC_STABLE)
    {
        //  Initialize AEC count
        saf->luma_stable_cnt = 0;
        // Move to Start position to start the search
        saf->dest_pos = saf->start_pos;
        af_single_hj_util_move_to_dest(p_caf_instance);
    }
    else
    {
        // didn't move lens when AEC is not stable
        af_single_hj_util_move_lens(p_caf_instance, DIR_TO_NEAR, 0);
    }

    return 0;
}

/** af_single_hj_search: entry Point of Algorithm
 *
 **/
INT af_single_hj_search(
    af_alg_interface_t* p_alg,
    af_haf_focus_search_data_t* output)
{
    INT rc = 0;
    af_caf_search_internal_t* p_caf_instance = (af_caf_search_internal_t*) p_alg;
    af_single_hj_t* g_af = p_caf_instance->af_single;
    af_single_hj_lib_info_t* saf = &g_af->af_hj_lib_info;
    LSP_Process* LSP_AF_sample = &saf->LSP_AF_sample;

    // If frame needs to be skipped, return
    if (g_af->af_hj_lib_info.frame_skip > 0)
    {
        AF_MSG_LOW("Skipping frame: frame_skip: %d",
                   g_af->af_hj_lib_info.frame_skip);
        g_af->af_hj_lib_info.frame_skip--;
        return TRUE;
    }

    Q3A_MEMCPY(&g_af->output,
               output,
               sizeof(af_haf_focus_search_data_t));
    g_af->output.progress = AF_PROGRESS_50_RUNNING;

    if (saf->saf_scan_state == HJ_AF_SINGLE_START)
    {
        af_single_hj_reset_LSPAFsweepSample(p_caf_instance);
        LSP_AF_sample->is_need_HJ_Process = FALSE;
        LSP_Corse_Peak_Pos = 0;
        //start from 1 because we use first frame (0) to start_srch
        saf->index = 1;
        saf->af_run_first_time = p_caf_instance->is_haf_first_run;
        saf->bad_stats_cnt = 0;
        rc = af_single_hj_start_srch(p_caf_instance);
    }
    else {
        /*Process stats only if they are valid otherwise timeout and end search*/
        if (p_caf_instance->stats.is_valid == FALSE)
        {
            saf->bad_stats_cnt++;
            g_af->output.confidence = AF_CONFIDENCE_HIGH;
            if (saf->bad_stats_cnt > AF_MAX_BAD_STATS_COUNT)
            {
                AF_MSG_HIGH("AF Failed Due to bad Stats cnt reached max");
                saf->final_pos = p_caf_instance->cur_pos;
                af_single_hj_end_search(p_caf_instance, HJ_AF_SINGLE_FAILED);
                saf->bad_stats_cnt = 0;
                g_af->output.confidence = AF_CONFIDENCE_LOW;
                rc = 0;
            }
        }
        else
        {
            saf->bad_stats_cnt = 0;
            af_single_hj_get_value(p_caf_instance);
            rc = af_single_hj_handle_state(p_caf_instance);
            g_af->output.confidence = AF_CONFIDENCE_HIGH;
        }
    }

    if (saf->saf_scan_state == HJ_AF_WAIT_AEC_STABLE)
    {
        g_af->output.progress = AF_PROGRESS_25_RUNNING;
    }

    //populate single af information
    g_af->output.target_pos = -1;
    g_af->output.range_near = saf->near_end_pos;
    g_af->output.range_far = saf->far_end_pos;
    Q3A_MEMCPY(output,
               &g_af->output,
               sizeof(af_haf_focus_search_data_t));
    return rc;
}

//* af_single_hj_destroy: Delete the single AF algorithm**
VOID af_single_hj_destroy(
    af_caf_search_internal_t* p_caf_instance)
{
    // Destroy SAF if it is valid
    if (p_caf_instance->af_single)
    {
        AF_FREE(p_caf_instance->af_single);
        p_caf_instance->af_single = NULL;
        AF_MSG_HIGH("af_single destroy success!\n");
    }
}

BOOL af_single_hj_if_link(
    af_caf_search_internal_t* p_caf_instance)
{
    BOOL rc = TRUE;
    p_caf_instance->af_single =
        (af_single_hj_t*) AF_MALLOC(sizeof(af_single_hj_t));
    af_single_hj_t* af_single = (af_single_hj_t*) p_caf_instance->af_single;

    if (af_single == NULL)
    {
        AF_MSG_ERROR("Memory allocation error!");
        return FALSE;
    }

    Q3A_MEMSET(p_caf_instance->af_single,
               1,
               sizeof(af_single_hj_t));
    af_single_hj_lib_info_t* saf = &af_single->af_hj_lib_info;
    saf->af_run_first_time = TRUE;
    saf->saf_scan_state = HJ_AF_SINGLE_START;
    saf->frame_skip = 0;
    saf->gravity_comp = 0;
    // Initialize Data and states
    af_single_hj_reset_search_data(p_caf_instance);
    return rc;
} // af_single_hj_if_link
