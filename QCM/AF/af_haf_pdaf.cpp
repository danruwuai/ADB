////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file af_haf_pdaf.cpp
/// @brief
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "af_alg_interface.h"
#include "af_alg_util_interface.h"
#include "af_alg_factory.h"
#include "af_debugdatatypes.h"
#include "af_haf_pdaf.h"

#ifdef VENDOR_EDIT
#include <cutils/properties.h>

#define AF_PD_HUNTING_PREVENT_DEFOCUS_BASE 0x01
#define AF_PD_HUNTING_PREVENT_CHECK_DEVIATION 0x02
#define AF_PD_HUNTING_PREVENT_CHECK_CONF 0x04
#define AF_PD_HUNTING_PREVENT_CONTINUE_CNT 2

#define AF_PD_HUNTING_PREVENT_VIDEO_MODE_ENABLE 1

#define VIDEO_MOVE_PCT_DROP_PD_DIF_THR 50
#define VIDEO_MOVE_PCT_DROP_REV_CNT_TH 2
#define VIDEO_MOVE_PCT_DROP_RATIO 0.5
#endif

// TODO: move to tuning parameter
#define MINUS_ONE_FRAME_SKIP_VBT_THRESHOLD 15.0f
#ifdef VENDOR_EDIT
#define CONVERGE_DONE_DELAY_WHEN_ROI_TYPE_CHANGE 4
#endif

#ifdef VENDOR_EDIT
#define FALLBACK_REV_CNT_TH 3
#define FALLBACK_CONVERGE_REV_CNT_TH 4
#define HUNTING_LOW_LIGHT_GAIN_THR (8)
#endif

using namespace afpdlibelements;
using namespace afpdafelements;

static chromatixPDFocusTableEntryType lcrFocusEntryTable[] =
{
#ifdef VENDOR_EDIT
    {6,     0},
#endif
    {16,    1.0},
    {32,    0.9},//0.95
    {64,    0.8},//0.9
    {128,   0.7},//0.85
    {256,   0.6},//0.7
    {399,   0.5}
};

// Move to chromatix later
static const chromatixPDFocusTableType lcrFocusTable = {
    .focusEntriesCount = sizeof(lcrFocusEntryTable) / sizeof(chromatixPDFocusTableEntryType),
    .focusEntries = lcrFocusEntryTable,
};

// NOWHINE ENTIRE FILE - legacy code ported temporarily

/* -----------------------------------------------------------------------
 ** Forward Declarations
 ** ----------------------------------------------------------------------- */
static VOID af_pdaf_mw_massage_pd_data(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_roi_t* roi,
    INT size);
static BOOL af_pdaf_mw_proc_pd_windows(
    af_alg_PDAF_internal_t* pd,
    pdaf_defocus_t* defocus,
    af_pdaf_roi_t* roi_out,
    INT grid_count);
static VOID af_pdaf_mw_sanitize_windows(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_roi_t* data,
    INT grid_count,
    INT* size);
static VOID af_pdaf_mw_sort_windows(
    af_pdaf_roi_t* data,
    INT size);
static INT af_pdaf_mw_select_best_window(
    af_alg_PDAF_internal_t* af,
    af_pdaf_roi_t* data,
    INT sample_size);
static INT af_pdaf_mw_process_central(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_input_t* input);
static INT af_pdaf_mw_process_peripheral(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_input_t* input);
static VOID af_pdaf_mw_collect_similar_windows(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_roi_t* data,
    INT size,
    INT active_index);
static VOID af_pdaf_mw_update_active_roi(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_roi_t* active_roi);
VOID af_alg_init_interface(
    af_alg_interface_t* p_alg);

af_alg_interface_t* af_alg_PDAF_create(
    const af_alg_util_interface_t* p_alg_util_instance);
static VOID af_pdaf_destroy(
    af_alg_interface_t* af_alg_obj);

// PDAF af_alg_interface API related

//Get Debug Data:
static VOID af_pdaf_get_debug_data(
    af_alg_interface_t* p_alg,
    VOID* debug_data);

// Process_Monitor:
static BOOL af_pdaf_process_monitor(
    af_alg_interface_t* p_alg,
    af_haf_monitor_input_t* input,
    af_haf_monitor_data_t* output);
static BOOL af_pdaf_monitor(
    af_alg_PDAF_internal_t* pd,
    af_haf_monitor_data_t* output);
static VOID af_pdaf_update_defocus_monitor(
    af_alg_PDAF_internal_t* pd);
static VOID af_pdaf_populate_process_monitor_output(
    af_alg_PDAF_internal_t* pd,
    af_haf_monitor_data_t* output);

// Process_Search:
static BOOL af_pdaf_process_search(
    af_alg_interface_t* p_alg,
    af_haf_focus_search_input_t* input,
    af_haf_focus_search_data_t* output);

// Search 1.0 focus converge
static VOID af_pdaf_focus_converge(
    af_alg_PDAF_internal_t* pd);

// Search 2.0 LSP focus converge
static VOID af_pdaf_focus_converge_withFV(
    af_alg_PDAF_internal_t* pd);
static INT af_pdaf_get_high_conf(
    af_alg_PDAF_internal_t* pd);

static VOID af_pdaf_get_next_pos(
    af_alg_PDAF_internal_t* pd);
static int af_pdaf_get_fine_step(
    af_alg_PDAF_internal_t* pd);
static VOID af_pdaf_get_fine_range(
    af_alg_PDAF_internal_t* pd);
static BOOL af_pdaf_populate_process_search_output(
    af_alg_PDAF_internal_t* pd,
    BOOL is_frame_skip,
    af_haf_focus_search_data_t* output);

// SetParam / SetInputs / SetTuning:
static BOOL af_pdaf_set_parameter(
    af_alg_interface_t* p_alg,
    struct af_haf_set_parameter* param);
static BOOL af_PDAF_get_parameter(
    af_alg_interface_t* p_alg,
    af_haf_get_parameter_t* param);
static BOOL af_pdaf_set_inputs(
    af_alg_interface_t* p_alg,
    struct af_haf_set_parameter* param);
static BOOL af_pdaf_set_tuning(
    af_alg_interface_t* p_alg,
    af_haf_tune_params_t* haf_tuning);
static BOOL af_pdaf_set_sensitivity(
    af_alg_interface_t* p_alg,
    HAF_set_sensitivity_type sensitivity);
static VOID af_pdaf_update_stats(
    af_alg_interface_t* p_alg,
    af_pdaf_input_t* input);
static BOOL af_pdaf_clear_data(
    af_alg_interface_t* p_alg);

// Algo internal functions
static INT af_pdaf_is_depth_conf(
    af_alg_PDAF_internal_t* pd);
static INT af_pdaf_is_focus_converge_done(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_input_t* input,
    INT next_pos);
static VOID af_pdaf_face_pd_history_rebase(
    af_alg_PDAF_internal_t* pd);
static VOID af_pdaf_face_pd_history_update(
    af_alg_PDAF_internal_t* pd);
static INT af_pdaf_get_temporal_avg_target_pos(
    af_alg_PDAF_internal_t* pd,
    UINT32 cur_pos,
    INT* buffer,
    INT buffer_len);
static INT af_pdaf_get_face_temporal_next_position(
    af_alg_PDAF_internal_t* pd);
static INT af_pdaf_get_converge_done_temporal_pos(
    af_alg_PDAF_internal_t* pd,
    UINT32 cur_pos);
static INT af_pdaf_converge_done_temporal_process(
    af_alg_PDAF_internal_t* pd,
    INT cur_pos);
static INT af_pdaf_get_target_pos(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_input_t* input,
    INT lens_pos);
static FLOAT af_pdaf_get_focus_pcnt(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_input_t* input,
    INT defocus);
static INT af_pdaf_get_focus_move_steps(
    af_alg_PDAF_internal_t* pd,
    FLOAT focus_pcnt);
static FLOAT af_pdaf_get_noise_multiplier(
    af_alg_PDAF_internal_t* pd);
static INT af_pdaf_get_min_stable_cnt(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_input_t* input);
static INT af_pdaf_get_focus_roi_id(
    af_alg_PDAF_internal_t* pd);
static VOID af_pdaf_proc_pd(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_input_t* input);
static VOID af_pdaf_proc_noise(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_input_t* input);
static VOID af_pdaf_proc_depth_stable(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_input_t* input);
static VOID af_pdaf_proc_defocused(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_input_t* input);
static VOID af_pdaf_proc_rev_dir(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_input_t* input);
VOID af_pdaf_reset_data(
    af_alg_PDAF_internal_t* pd);
static VOID af_pdaf_set_mode(
    af_alg_interface_t* p_alg,
    af_mode_type mode);
static BOOL af_pdaf_rebase_monitor(
    af_alg_interface_t* p_alg);
static VOID af_pdaf_set_roi(
    af_alg_interface_t* p_alg,
    af_roi_info_t* roi);
static VOID af_pdaf_set_range(
    af_alg_interface_t* p_alg,
    INT near,
    INT far);
static VOID af_pdaf_calculate_fallback_opt(
    af_alg_PDAF_internal_t* pd);
static FLOAT af_pdaf_get_roi_to_center_distance(
    af_alg_PDAF_internal_t* pd);
static BOOL af_pdaf_face_change_refocus_trigger(
    af_alg_PDAF_internal_t* pd);
static BOOL af_pdaf_face_roi_is_stable(
    af_alg_PDAF_internal_t* pd);
static BOOL af_pdaf_face_tracking_move_suppression_check(
    af_alg_PDAF_internal_t* pd);
static BOOL af_pdaf_is_running_2PD(
    af_alg_PDAF_internal_t* pd);
static BOOL af_pdaf_is_running_2PD_face(
    af_alg_PDAF_internal_t* pd);
static BOOL af_pdaf_is_scene_focused(
    af_alg_PDAF_internal_t* pd);
static BOOL af_pdaf_is_valid_roi_to_run_pd(
    af_alg_PDAF_internal_t* pd);
static INT af_pdaf_check_sensitivity_range(
    INT sensitivity);
static VOID af_pdaf_proc_pd_single(
    af_alg_PDAF_internal_t* pd, af_pdaf_input_t* input);
#ifdef VENDOR_EDIT
static void af_pdaf_check_hunting_prevent(
    af_alg_PDAF_internal_t * pd, int hunting_final_pos);
#endif
/* -----------------------------------------------------------------------
 ** Function Definitions
 ** ----------------------------------------------------------------------- */

/** af_alg_PDAF_create: Create instance of PDAF. Mallocate PDAF internal structure
 *                        specifically and initializes each parameter.
 *
 *  @ p_alg_util_instance: AF algorithm util instance pointer
 *
 *  Return: af_alg_interface_t
 **/
af_alg_interface_t* AFAlgoFactory::af_alg_PDAF_create(
    const af_haf_create_params_t* p_create_params)
{
    (VOID) p_create_params;
    HAF_MSG_LOW("E");
    // Malloc for PDAF internal structure
    af_alg_PDAF_internal_t* p_PDAF_instance = (af_alg_PDAF_internal_t*) AF_MALLOC(sizeof(af_alg_PDAF_internal_t));

    if (NULL == p_PDAF_instance)
    {
        // Malloc failed
        AF_MSG_ERROR("Memory allocation failure");
        return NULL;
    }
    else
    {
        p_PDAF_instance->pTuningInfo = (af_tuning_info*)AF_MALLOC(sizeof(af_tuning_info));
        if (NULL == p_PDAF_instance->pTuningInfo)
        {
            // Malloc failed, Free allocated memory
            AF_FREE(p_PDAF_instance);
            p_PDAF_instance = NULL;
            AF_MSG_ERROR("Memory allocation failure");
            return NULL;
        }
        af_alg_init_interface(&p_PDAF_instance->af_alg);
        // Populate PDAF specific function pointers
        p_PDAF_instance->af_alg.set_parameters = af_pdaf_set_parameter;
        p_PDAF_instance->af_alg.get_parameters = af_PDAF_get_parameter;
        p_PDAF_instance->af_alg.set_tuning = af_pdaf_set_tuning;
        p_PDAF_instance->af_alg.get_debug_data = af_pdaf_get_debug_data;
        p_PDAF_instance->af_alg.process_monitor = af_pdaf_process_monitor;
        p_PDAF_instance->af_alg.process_search = af_pdaf_process_search;
        p_PDAF_instance->af_alg.clear_data = af_pdaf_clear_data;
        p_PDAF_instance->af_alg.set_sensitivity = af_pdaf_set_sensitivity;
        p_PDAF_instance->af_alg.rebase_monitor = af_pdaf_rebase_monitor;
        p_PDAF_instance->af_alg.destroy = af_pdaf_destroy;
        // Copy the ext func util instance pass during create time
        //    p_PDAF_instance->p_alg_util =
        //       (af_alg_util_interface_t *) p_alg_util_instance;
        // No matter what topology we pick, we always create value monitors
        // Create instances for SAD and gyro through Value monitor
        p_PDAF_instance->p_vm_sad_panning = af_value_monitor_create("PDAF_SAD_PANNING");
        p_PDAF_instance->p_vm_gyro_panning = af_value_monitor_create("PDAF_GRYO_PANNING");
        //Create instance for defocus and confidence through value monitor
        p_PDAF_instance->p_vm_defocus_low = af_value_monitor_create("PDAF_DEFOCUS_LOW");
        p_PDAF_instance->p_vm_defocus_high = af_value_monitor_create("PDAF_DEFOCUS_HIGH");
        AFUtil::AFGetTuningInfoFromCreateList(p_create_params, p_PDAF_instance->pTuningInfo);
        // Initialize internal fields
        af_pdaf_clear_data((af_alg_interface_t*) p_PDAF_instance);
        // Typecast to af_alg_interface_t defined in af_alg_interface.h
#ifdef VENDOR_EDIT
        char prop[PROPERTY_VALUE_MAX];
        memset(prop, 0, sizeof(prop));
        property_get ("persist.camera.disableVideoFinesearch", prop, "0");
        p_PDAF_instance->videoDisableFineSearch = atoi(prop);
#endif
        return (af_alg_interface_t*) p_PDAF_instance;
    }
}

/** af_pdaf_rebase_monitor: Call rebase reference of pdaf instance. Must be called
 *                        specifically.
 *
 *  Return: VOID
 **/
static BOOL af_pdaf_rebase_monitor(
    af_alg_interface_t* p_alg)
{
    // NULL pointer check
    if (NULL == p_alg)
    {
        AF_MSG_ERROR("ERROR! *af_alg is null pointer!! ");
        return FALSE;
    }

    af_alg_PDAF_internal_t* pd = (af_alg_PDAF_internal_t*) p_alg;

    // rebasing monitors w.r.t. the topology_index
    switch (pd->topology_index)
    {
        case chromatixTopoPDAFEnum::PDAFTopoProfileSparse:
        case chromatixTopoPDAFEnum::PDAFTopoProfileSparseAdvanced:
        case chromatixTopoPDAFEnum::PDAFTopoProfile2PD:
        case chromatixTopoPDAFEnum::PDAFTopoProfile2PDAdvanced:
        case chromatixTopoPDAFEnum::PDAFTopoProfileAdvanced:
            // Rebase Gyro
            pd->metric.gyro.is_panning = FALSE;
            // Rebase SAD
            pd->metric.sad.is_panning = FALSE;
            pd->p_vm_defocus_low->set_input(pd->p_vm_defocus_low, 0);
            pd->p_vm_defocus_high->set_input(pd->p_vm_defocus_high, 0);
            pd->p_vm_defocus_low->rebase_ref(pd->p_vm_defocus_low);
            pd->p_vm_defocus_high->rebase_ref(pd->p_vm_defocus_high);
            af_pdaf_reset_data(pd);
            break;

        default:
            AF_MSG_ERROR("No matched PDAF Topology Profile Index!");
            break;
    }

    return TRUE;
}

/** af_caf_set_roi: to set AF ROI configuration .
 *
 *  @p_alg : af_alg_interface pointer.
 *  @roi :  roi configuration to be set.
 *  Return: VOID
 **/
static VOID af_pdaf_set_roi(
    af_alg_interface_t* p_alg,
    af_roi_info_t* roi)
{
    // NULL pointer check
    if (NULL == p_alg || NULL == roi)
    {
        AF_MSG_ERROR("ERROR! *af_alg is null pointer!! ");
        return;
    }

    // Typecast from af_alg_interface_t to af_alg_caf_internal_t
    af_alg_PDAF_internal_t* pd = (af_alg_PDAF_internal_t*) p_alg;
    Q3A_MEMCPY_S(&(pd->set_parameters.roi), sizeof(af_roi_info_t), roi, sizeof(af_roi_info_t));
    return;
}

/** af_pdaf_set_range: to set AF search limit.
 *
 *  @p_alg : af_alg_interface pointer.
 *  @near : Near end limit of the AF search range to be set.
 *  @far : Far end limit of the AF search range to be set.
 *  Return: VOID
 **/
static VOID af_pdaf_set_range(
    af_alg_interface_t* p_alg,
    INT dist_near,
    INT dist_far)
{
    // NULL pointer check
    if (NULL == p_alg)
    {
        AF_MSG_ERROR("ERROR! p_alg is null pointer.");
        return;
    }

    // Typecast from af_alg_interface_t to af_alg_caf_internal_t
    af_alg_PDAF_internal_t* pd = (af_alg_PDAF_internal_t*) p_alg;
    af_tuning_info* p_tuning = pd->pTuningInfo;
    af_pdaf_params_t* p_params = &pd->params;

    if (dist_near < (INT)p_tuning->pAlgoTuningData->commonElements.positionNearEnd)
    {
        dist_near = (INT)p_tuning->pAlgoTuningData->commonElements.positionNearEnd;
    }
    else if (dist_near > (INT)p_tuning->pAlgoTuningData->commonElements.positionFarEnd)
    {
        dist_near = (INT)p_tuning->pAlgoTuningData->commonElements.positionFarEnd;
    }

    if (dist_far < (INT)p_tuning->pAlgoTuningData->commonElements.positionNearEnd)
    {
        dist_far = (INT)p_tuning->pAlgoTuningData->commonElements.positionNearEnd;
    }
    else if (dist_far > (INT)p_tuning->pAlgoTuningData->commonElements.positionFarEnd)
    {
        dist_far = (INT)p_tuning->pAlgoTuningData->commonElements.positionFarEnd;
    }

    p_params->near_limit = dist_near;
    p_params->far_limit = dist_far;
    AF_MSG_LOW("near %d far %d tuning near %d far %d",
               dist_near,
               dist_far,
               p_tuning->pAlgoTuningData->commonElements.positionNearEnd,
               p_tuning->pAlgoTuningData->commonElements.positionFarEnd);
}

static VOID af_pdaf_set_mode(
    af_alg_interface_t* p_alg,
    af_mode_type mode)
{
    // NULL pointer check
    if (NULL == p_alg)
    {
        AF_MSG_ERROR("ERROR! *af_alg is null pointer!! ");
        return;
    }

    af_alg_PDAF_internal_t* pd = (af_alg_PDAF_internal_t*) p_alg;
    pd->set_parameters.mode = mode;
    AF_MSG_LOW("Set mode %d", pd->set_parameters.mode);
    return;
}

/** af_PDAF_destroy: Destroy function to destroy the PDAF instance. Must be called
 *                        specifically.
 *
 *  Return: VOID
 **/
static VOID af_pdaf_destroy(
    af_alg_interface_t* af_alg_obj)
{
    HAF_MSG_LOW("E");
    af_alg_PDAF_internal_t* obj = (af_alg_PDAF_internal_t*) af_alg_obj;

    // Free PDAF internal type dynamically allocated memory
    if (obj)
    {
        // Free alg util dynamically allocated memory
        if (obj->pTuningInfo)
        {
            AF_FREE(obj->pTuningInfo);
            obj->pTuningInfo= NULL;
        }

        if (obj->p_alg_util)
        {
            obj->p_alg_util = NULL;
        }

        if (obj->p_vm_gyro_panning)
        {
            af_value_monitor_destroy(obj->p_vm_gyro_panning);
        }

        if (obj->p_vm_sad_panning)
        {
            af_value_monitor_destroy(obj->p_vm_sad_panning);
        }

        if (obj->p_vm_defocus_low)
        {
            af_value_monitor_destroy(obj->p_vm_defocus_low);
        }

        if (obj->p_vm_defocus_high)
        {
            af_value_monitor_destroy(obj->p_vm_defocus_high);
        }

        AF_FREE(obj);
    }
}

/** af_pdaf_is_running_2PD: Whether PDAF is running 2PD
 *
 *  Return: TRUE on success else FALSE
 **/
static BOOL af_pdaf_is_running_2PD(
    af_alg_PDAF_internal_t* pd)
{
    if (NULL == pd)
    {
        return FALSE;
    }

    return pd->params.is_2PD_enable;
}

/** is_running_2PD_face: Whether PDAF is running 2PD + face
 *
 *  Return: TRUE on success else FALSE
 **/
static BOOL af_pdaf_is_running_2PD_face(
    af_alg_PDAF_internal_t* pd)
{
    if (NULL == pd)
    {
        return FALSE;
    }

    return ((af_pdaf_is_running_2PD(pd)) & (pd->params.roi_info.type == AF_ROI_TYPE_FACE));
}

static BOOL af_pdaf_is_advanced_sparsePD_result(
    af_alg_PDAF_internal_t* pd)
{
    CAMX_ASSERT(NULL != pd);

    af_pdaf_params_t* p_params = &pd->params;
    INT focus_roi = p_params->focus_roi_id;

    return p_params->roi[focus_roi].conf > 1024 ? TRUE : FALSE;
}

/** af_PDAF_set_parameter: PDAF set parameter for caller to set PDAF configuration .
 *
 *  @p_alg : af_alg_interface pointer.
 *  @param : PDAF configuration to be set.
 *
 *  Return: VOID
 **/
static BOOL af_pdaf_set_parameter(
    af_alg_interface_t* p_alg,
    struct af_haf_set_parameter* param)
{
    BOOL result = TRUE;

    if (NULL == p_alg || NULL == param)
    {
        result = FALSE;
        return result;
    }

    // Typecast from af_alg_interface_t to af_alg_PDAF_internal_t
    af_alg_PDAF_internal_t* pd = (af_alg_PDAF_internal_t*) p_alg;
    af_pdaf_params_t* p_params = &pd->params;
    af_pdaf_input_t* pd_input = &pd->input;

    // Any algorithm implementation may route unsupported param to the Set_params method
    switch (param->type)
    {
        // Switch case and only set param if ID is recognized
        case AF_HAF_SET_PARAM_IS_FIRST_RUN:
            {
                // copy the is_first_run
                p_params->is_first_run = param->u.is_haf_first_run;
                HAF_MSG_LOW("is_first_run =%d", p_params->is_first_run);
            }
            break;

        case AF_HAF_SET_PARAM_ROI:
            Q3A_MEMCPY_S(&(p_params->roi_info), sizeof(af_roi_info_t), &param->u.af_roi_info, sizeof(af_roi_info_t));
#ifdef VENDOR_EDIT
            if (p_params->roi_info.type != p_params->prev_roi_info.type)
            {
                p_params->converge_done_delay = CONVERGE_DONE_DELAY_WHEN_ROI_TYPE_CHANGE;
            }
            HAF_MSG_LOW("cur type %d previous %d converge_done_dealy %d",
                p_params->roi_info.type, p_params->prev_roi_info.type, p_params->converge_done_delay);
#endif
            af_pdaf_set_roi(p_alg, &param->u.af_roi_info);
            break;

        case AF_HAF_SET_PARAM_IS_SPOTLIGHT_DETECTED:
            {
                // copy spotlight is detected
                p_params->is_spotlight_detected_prev = p_params->is_spotlight_detected;
                p_params->is_spotlight_detected = param->u.is_spotlight_detected;
                HAF_MSG_LOW("is_spotlight_detected = %d is_spotlight_detected_prev = %d",
                            p_params->is_spotlight_detected, p_params->is_spotlight_detected_prev);
            }
            break;

        case AF_HAF_SET_PARAM_LED_RUNNING:
            {
                // copy LED is running flag
                p_params->af_led_assist = param->u.af_led_assist;
                HAF_MSG_LOW("af_led_assist = %d", p_params->af_led_assist);
            }
            break;

        case AF_HAF_SET_PARAM_UPDATE_SENSOR_INFO:
        {
            // copy the sensor input to tof local struct
            Q3A_MEMCPY_S(&p_params->sensor_info,
                sizeof(af_input_from_img_sensor_t),
                &param->u.sensor_info,
                sizeof(af_input_from_img_sensor_t));
            pd_input->actuator_sensitivity = param->u.sensor_info.actuator_sensitivity;
        }
            break;

        case AF_HAF_SET_PARAM_GYRO_INFO:
            {
                FLOAT gyro_sqr = param->u.gyro_input.gyro_sqr;
                pd->p_vm_gyro_panning->set_input(pd->p_vm_gyro_panning, gyro_sqr);
                HAF_MSG_LOW("Received Gyro Sqr data %f", gyro_sqr);
            }
            break;
#ifdef VENDOR_EDIT
        case AF_HAF_SET_PARAM_UPDATE_GRAVITY_VECTOR:
            {
                pd->set_parameters.gravity_info = param->u.gravity_info;
            }
            break;
#endif

        case AF_HAF_SET_PARAM_UPDATE_SAD_INFO:
            {
                FLOAT sad = param->u.sad;
                HAF_MSG_LOW("Received SAD data %f, frame id %u", sad, param->current_frame_id);
                //Call the vm_sad_panning instance
                pd->p_vm_sad_panning->set_input(pd->p_vm_sad_panning, sad);
            }
            break;

        case AF_HAF_SET_PARAM_UPDATE_AEC_INFO:
            pd->set_parameters.aec_input = param->u.aec_info;
            pd_input->scene_bv = param->u.aec_info.Bv_af;
            pd_input->fps = static_cast<FLOAT>(param->u.aec_info.current_fps);
            pd_input->sensor_gain = param->u.aec_info.cur_real_gain;
            HAF_MSG_LOW("AEC gain = %f ",
                        pd->set_parameters.aec_input.cur_real_gain);
            break;

        case AF_HAF_SET_PARAM_DEPTH_SERVICE:
            result = af_pdaf_set_inputs(p_alg, param);
            break;

        case AF_HAF_SET_PARAM_FOCUS_MODE:
            af_pdaf_set_mode(p_alg, param->u.focus_mode);
            break;

        case AF_HAF_SET_PARAM_SEARCH_RANGE:
            af_pdaf_set_range(p_alg, param->u.search_range.near_range, param->u.search_range.far_range);
            break;

        case AF_HAF_SET_PARAM_SETTINGS:
            pd->settingsInfo = param->u.p_settingsInfo;
            break;

        case AF_HAF_SET_PARAM_RUN_TYPE:
            p_params->camera_run_mode = param->u.run_mode;
            break;
#ifdef VENDOR_EDIT
        case AF_HAF_SET_PARAM_CHECK_PDAF_HUNTING_PREVENT:
            af_pdaf_check_hunting_prevent(pd, param->u.hunting_final_pos);
            break;
#endif
        default:
            {
                // No action if set param ID is not recognized
                return result;
            }
    }

    return result;
}

/** af_PDAF_get_parameter: HAF mixer will call get parameter of the active algo when
 *   this algo have information which is needed by other algo or outsider.
 *
 *  @p_alg : af_alg_interface pointer.
 *  @param : PDAF configuration to be retrieved.
 *
 *  Return: TRUE in success and FALSE otherwise
 **/
static BOOL af_PDAF_get_parameter(
    af_alg_interface_t* p_alg,
    af_haf_get_parameter_t* param)
{
    BOOL result = TRUE;

    if (NULL == p_alg || NULL == param)
    {
        result = FALSE;
        return result;
    }

    return result;
}

/** af_pdaf_clear_data: API clear all PDAF internal parameters to INIT default state.
 *
 *  @p_alg : af_alg_interface pointer.
 *
 *  Return: VOID
 **/
static BOOL af_pdaf_clear_data(
    af_alg_interface_t* p_alg)
{
    HAF_MSG_LOW("E");
    BOOL result = TRUE;

    if (NULL == p_alg)
    {
        result = FALSE;
        return result;
    }

    // Typecast from af_alg_interface_t to af_alg_PDAF_internal_t
    af_alg_PDAF_internal_t* pd = (af_alg_PDAF_internal_t*) p_alg;
    af_pdaf_params_t* p_params = &pd->params;
    INT roi_index = 0, j = 0;

    for (roi_index = 0; roi_index < PDAF_MAX_NUM_PD_ROI; roi_index++)
    {
        // reset defocus history
        for (j = 0; j < PDAF_MAX_NUM_PD_HISTORY; j++)
        {
            p_params->roi[roi_index].defocus_history[j] = 0;
#ifdef VENDOR_EDIT
            if (TRUE == p_params->hunting_prevent.enable) {
                p_params->hunting_prevent.roi[roi_index].target_pos_hist[j].is_conf = FALSE;
                p_params->hunting_prevent.roi[roi_index].target_pos_hist[j].pos = pd->input.lens_pos;
            }
#endif
        }

        // reset pd scene monitor status
        p_params->roi[roi_index].is_defocused = FALSE;
        p_params->roi[roi_index].is_conf = FALSE;
        p_params->roi[roi_index].is_stable = FALSE;
        p_params->roi[roi_index].stable_cnt = 0;
        p_params->roi[roi_index].defocus_cnt = 0;
        p_params->roi[roi_index].rev_cnt = 0;
        p_params->roi[roi_index].converge_rev_cnt = 0;
        p_params->roi[roi_index].defocus = 0;
        p_params->roi[roi_index].not_conf_cnt = 0;
        p_params->roi[roi_index].is_soft_conf_rgn = 0;
    }

    // reset pd roi selection
    p_params->idx = 0;
    p_params->is_pd_updated = FALSE;
    p_params->focus_roi_id = 0;
    p_params->is_scene_changed = FALSE;
    p_params->is_prev_focus_conv_failed = FALSE;
    p_params->wait_conf_recover_cnt = 0;
    p_params->scene_change_cnt = 0;
    p_params->is_spotlight_detected = FALSE;
    p_params->af_led_assist = 0;
    //reset frame skip
    p_params->frame_skip = 0;
    p_params->focus_converge_cnt = 0;
    p_params->run_converge_done_temporal_filter = FALSE;
    p_params->converge_done_temporal_counter = 0;
    p_params->pdaf_convState = 0;
    p_params->conf_cnt = 0;
    p_params->dynamic_frame_cnt = 0;
    p_params->dynamic_frame_skip_running = 0;
    p_params->frame_skip_cnt = 0;
    p_params->fallback_single_during_search = 0;
    p_params->MW_grids_converge_used = 0;
    p_params->MW_switch_to_peripheral_during_search = 0;
    p_params->MW_in_lowlight = 0;
    p_params->MW_is_multi_depth = 0;
    p_params->wait_caf_stable_cnt = 0;
    // Clear memory of PD input and monitor data
    Q3A_MEMSET(&pd->input, 0, sizeof(af_pdaf_input_t));
    Q3A_MEMSET(&pd->monitor_data, 0, sizeof(af_pdaf_monitor_data_t));
    // Reset PD MW data
    Q3A_MEMSET(&pd->mw_roi_info, 0, sizeof(af_mw_selected_roi_t));
    pd->mw_roi_info.type = PDAF_MW_INVALID;
#ifdef VENDOR_EDIT
    pd->lcrUsed = FALSE;
    /* Reset pd hunting_prevent */
    p_params->hunting_prevent.defocus_base = 0;
    p_params->hunting_prevent.hunting_prevent_trigger = FALSE;
    p_params->hunting_prevent.trigger_cnt = 0;
    p_params->pre_pd_not_conf = 0;
    p_params->pre_rev_cnt_fallback = 0;
    p_params->rev_cnt_pct_drop = 1.0;
#endif
    return result;
}

#ifdef VENDOR_EDIT
float af_pdaf_get_pd_conf_multiplier(af_alg_PDAF_internal_t *pd)
{
     af_pdaf_params_t *p_params = &pd->params;
     int focus_roi = p_params->focus_roi_id;
     int confidence = p_params->roi[focus_roi].conf;
     int conf_low_thres = p_params->pd_conf_mutiplier_info.conf_low_thres;
     int conf_high_thres = p_params->pd_conf_mutiplier_info.conf_high_thres;
     float max_multiplier = p_params->pd_conf_mutiplier_info.max_multiplier;

     float multiplier = 1.0;

     if (conf_high_thres <= conf_low_thres || max_multiplier < 0) {
       HAF_MSG_LOW("disable pd_conf_multiplier: invalid conf_high_thres %d, conf_low_thres %d",
           conf_high_thres, conf_low_thres);
       return multiplier;
     }

     if (confidence > conf_high_thres)
       multiplier = 0.0f;
     else if (confidence < conf_low_thres)
       multiplier = max_multiplier;
     else {
       multiplier = max_multiplier * (float)(conf_high_thres - confidence)
           /(float)(conf_high_thres - conf_low_thres);
     }
     multiplier = multiplier + 1.0f;

     if (multiplier < 1.0) {
       multiplier = 1.0f;
     }

     HAF_MSG_LOW("multiplier %f, max_multiplier %f, conf_high_thres %d, conf_low_thres %d",
         multiplier, max_multiplier, conf_high_thres, conf_low_thres);
     return multiplier;
}
#endif
/** af_pdaf_set_inputs: PDAF set input parameter for caller to set PDAF algorithm inputs.
 *
 *  @p_alg : af_alg_interface pointer.
 *  @param : PDAF configuration to be set.
 *
 *  Return: VOID
 **/
static BOOL af_pdaf_set_inputs(
    af_alg_interface_t* p_alg,
    struct af_haf_set_parameter* param)
{
    BOOL result = TRUE;

    // NULL pointer check
    if (NULL == p_alg || NULL == param)
    {
        result = FALSE;
        return result;
    }

    // Typecast from af_alg_interface_t to af_alg_PDAF_internal_t
    af_alg_PDAF_internal_t* pd = (af_alg_PDAF_internal_t*) p_alg;
    af_pdaf_input_t* pd_input = &pd->input;

    // Any algorithm implementation may route unsupported param to the Set_params method
    switch (param->type)
    {
        // For PDAF, the only input that's been set is AF_SET_PARAM_DEPTH_SERVICE
        case AF_HAF_SET_PARAM_DEPTH_SERVICE:
        {
            if (param->u.depth_service.input.info.type == DEPTH_SERVICE_PDAF)
            {
                    // Copy all inputs from message to PD input
                    Q3A_MEMCPY_S(&pd_input->pdaf_info,
                                 sizeof(pdaf_update_t),
                                 &param->u.depth_service.input.u.pdaf_info,
                                 sizeof(pdaf_update_t));

                    pd_input->lens_pos = param->u.depth_service.input.lens_pos;
                    // Update stats for PDAF
                    af_pdaf_update_stats(p_alg, pd_input);
            }
        }
        break;

        default:
        {
            return p_alg->set_parameters(p_alg, param);
        }
    }

    return result;
}

/** af_pdaf_set_tuning: PDAF set Chromatix tuning data structure pointer internally.
 *
 *  @p_alg : af_alg_interface pointer.
 *  @p_tuning : af_algo_tune_parms_wrapper_t pointer.
 *  @enable : af_haf_enable_type pointer.
 *
 *  Return: VOID
 **/
static BOOL af_pdaf_set_tuning(
    af_alg_interface_t* p_alg,
    af_haf_tune_params_t* haf_tuning)
{
    BOOL result = TRUE;

    if (NULL == p_alg || NULL == haf_tuning)
    {
        result = FALSE;
        return result;
    }


    af_tuning_info* p_tuning = haf_tuning->p_tuning_info;

    if (!p_tuning)
    {
        HAF_MSG_LOW("Tuning is NULL");
        result = FALSE;
        return result;
    }

#ifdef VENDOR_EDIT
    AF_MSG_HIGH("video mode %d, maxstep %d, donethres %d",
        (((af_alg_PDAF_internal_t*) p_alg)->params.camera_run_mode == AF_RUN_MODE_VIDEO),
        (INT)(p_tuning->pHAFTuningData->maxMoveStep),(INT)(p_tuning->pPDAFTuningData->convergeDoneFilter.threshold) );
#endif
    // Typecast from af_alg_interface_t to af_alg_PDAF_internal_t
    af_alg_PDAF_internal_t* pd = (af_alg_PDAF_internal_t*) p_alg;
    af_pdaf_params_t* p_params = &pd->params;
    //copy tuning here
    Q3A_MEMCPY(pd->pTuningInfo, p_tuning, sizeof(af_tuning_info));

    pd->enable = p_tuning->pHAFTuningData->enable_section.PDAFEnable;
    // Copy chromatix data pointer INTo PDAF tuning
    pd->haf_roi_enable = (BOOL) p_tuning->pHAFTuningData->HAFROISettings.HAFROIEnable;
    //Reset the PD internal data
    af_pdaf_reset_data(pd);
    HAF_MSG_LOW("p_tuning->roi.num_entries =%d, noise_gain = %0.2f, wait_after_focus_cont = %d, wait_conf_recover_cnt = %d enable = %d",
                p_tuning->pPDAFTuningData->ROI.ConfigurationCount,
                p_tuning->pPDAFTuningData->noiseTable.noiseEntries[0].noiseGain,
                p_tuning->pPDAFTuningData->sceneMonitor.waitAfterFocusCount,
                p_tuning->pPDAFTuningData->sceneMonitor.waitConfidenceRecoverCount,
                p_tuning->pHAFTuningData->enable_section.PDAFEnable);
    p_params->defocus_stable_filter_len = p_tuning->pPDAFTuningData->sceneMonitor.defocusStableFilterLength;
    p_params->enable_trigger_suppr_by_scene_change =
        p_tuning->pPDAFTuningData->sceneMonitor.enableTriggerSuppressionBySceneChange;
    p_params->enable_fine_scan_extension = p_tuning->pPDAFTuningData->focusScan.enableFineScanExtension;
    p_params->enable_do_not_use_defocus_sample_by_pd =
        p_tuning->pPDAFTuningData->PDPreprocessing.enableDoNotUseDefocusSampleByPD;
    p_params->enable_inc_min_conf_thres_by_pd = p_tuning->pPDAFTuningData->PDPreprocessing.enableIncMinConfidenceThresholdByPD;
    p_params->pd_max_limit_do_not_use_defocus_sample =
        p_tuning->pPDAFTuningData->PDPreprocessing.PDMaxLimitDoNotUseDefocusSample;
    p_params->pd_max_limit_inc_min_conf_thres = p_tuning->pPDAFTuningData->PDPreprocessing.PDMaxLimitIncMinConfidenceThreshold;
    p_params->inc_min_conf_thres_by_pd_scaler = p_tuning->pPDAFTuningData->PDPreprocessing.incMinConfidenceThresholdByPDScaler;
    p_params->defocus_dof_multiplier = p_tuning->pPDAFTuningData->sceneMonitor.defocusDOFMultiplier;
    p_params->first_run_not_conf_cnt_add = p_tuning->pPDAFTuningData->focusScan.firstRunNotConfidenceAdd;
    p_params->inc_defocus_thres_prev_error = p_tuning->pPDAFTuningData->PDInternal.incDefocusThresholdPreviousError;
    p_params->soft_rgn_thres_multiplier = p_tuning->pPDAFTuningData->PDInternal.softRegionThresholdMultiplier;
    p_params->enable_soft_conf_thresh = p_tuning->pPDAFTuningData->PDInternal.enableSoftConfidenceThreshold;
    p_params->fine_scan_range_enhance = p_tuning->pPDAFTuningData->PDInternal.fineScanRangeEnhance;
    p_params->soft_conf_thresh_delta = p_tuning->pPDAFTuningData->PDInternal.softConfidenceThresholdDelta;
    p_params->soft_conf_focus_pnt = p_tuning->pPDAFTuningData->PDInternal.softConfidenceFocus_pnt;
    p_params->enable_soft_rgn_thres_high = p_tuning->pPDAFTuningData->PDInternal.enableSoftRegionThresholdHigh;
    p_params->soft_rgn_thres_high_multiplier = p_tuning->pPDAFTuningData->PDInternal.softRegionThresholdHighMultiplier;
    p_params->soft_scene_change_cnt = p_tuning->pPDAFTuningData->PDInternal.softSceneChangeCount;
    p_params->is_2PD_enable = p_tuning->pPDAFTuningData->is2PDEnable;
    p_params->face_2pd_min_move_thresh = p_tuning->pPDAFTuningData->face.minMoveThreshold;
    p_params->face_far_end_enabled = p_tuning->pAlgoTuningData->faceDetection.faceFarEnd.enable;
    p_params->face_far_end = p_tuning->pAlgoTuningData->faceDetection.faceFarEnd.farEnd;
#ifdef VENDOR_EDIT
    p_params->hunting_prevent.enable = 1;
    p_params->hunting_prevent.flag = AF_PD_HUNTING_PREVENT_DEFOCUS_BASE|AF_PD_HUNTING_PREVENT_CHECK_DEVIATION|AF_PD_HUNTING_PREVENT_CHECK_CONF;
    p_params->hunting_prevent.defocus_thres = 30;
    p_params->hunting_prevent.check_size = 3;
    p_params->hunting_prevent.defocus_deviation_thres = 0.05;
    p_params->hunting_prevent.defocus_deviation_face_thres = 0.025;
    p_params->hunting_prevent.search_range = (float)ABS(p_tuning->pAlgoTuningData->searchLimit[0].farEnd - p_tuning->pAlgoTuningData->searchLimit[0].nearEnd);
    p_params->pd_conf_mutiplier_info.conf_low_thres = 150;
    p_params->pd_conf_mutiplier_info.conf_high_thres = 400;
    p_params->pd_conf_mutiplier_info.max_multiplier = 0;/*tomorrow check 0307*/
    p_params->disable_fine_search_conf_thres = 800;
#endif
    HAF_MSG_LOW("PDAF tuning params");
    HAF_MSG_LOW("number of ROIs =%d", pd->pTuningInfo->pPDAFTuningData->ROI.ConfigurationCount);
    HAF_MSG_LOW("defocus_stable_filter_len=%d", p_params->defocus_stable_filter_len);
    HAF_MSG_LOW("enable_trigger_suppr_by_scene_change=%d", p_params->enable_trigger_suppr_by_scene_change);
    HAF_MSG_LOW("enable_fine_scan_extension=%d", p_params->enable_fine_scan_extension);
    HAF_MSG_LOW("enable_do_not_use_defocus_sample_by_pd=%d", p_params->enable_do_not_use_defocus_sample_by_pd);
    HAF_MSG_LOW("enable_inc_min_conf_thres_by_pd=%d", p_params->enable_inc_min_conf_thres_by_pd);
    HAF_MSG_LOW("pd_max_limit_do_not_use_defocus_sample=%1.1f", p_params->pd_max_limit_do_not_use_defocus_sample);
    HAF_MSG_LOW("pd_max_limit_inc_min_conf_thres=%1.1f", p_params->pd_max_limit_inc_min_conf_thres);
    HAF_MSG_LOW("inc_min_conf_thres_by_pd_scaler=%1.1f", p_params->inc_min_conf_thres_by_pd_scaler);
    HAF_MSG_LOW("defocus_dof_multiplier=%1.1f", p_params->defocus_dof_multiplier);
    HAF_MSG_LOW("first_run_not_conf_cnt_add=%d", p_params->first_run_not_conf_cnt_add);
    HAF_MSG_LOW("inc_defocus_thres_prev_error=%f", p_params->inc_defocus_thres_prev_error);
    HAF_MSG_LOW("soft_rgn_thres_multiplier=%f", p_params->soft_rgn_thres_multiplier);
    HAF_MSG_LOW("enable_soft_conf_thresh=%d", p_params->enable_soft_conf_thresh);
    HAF_MSG_LOW("soft_conf_thres_delta %d soft_conf_pnt %f", p_params->soft_conf_thresh_delta, p_params->soft_conf_focus_pnt);
    HAF_MSG_LOW("enable_soft_rgn_thres_high=%d", p_params->enable_soft_rgn_thres_high);
    HAF_MSG_LOW("soft_rgn_thres_high_multiplier=%f", p_params->soft_rgn_thres_high_multiplier);
    HAF_MSG_LOW("soft_scene_change_cnt=%f", p_params->soft_scene_change_cnt);
    HAF_MSG_LOW("is_2PD_enable=%d", p_params->is_2PD_enable);
    HAF_MSG_LOW("face_2pd_min_move_thresh=%d", p_params->face_2pd_min_move_thresh);
    pd->topology_index = p_tuning->pAFMonitorTuningData->monitorAlgoConfig.topoPDAF.profileIndex;
    INT sens_level = 0;
    chromatixValueMonitorType* p_vm_cfg_tuning;
    HAF_MSG_LOW("PDAF topology_index %d", pd->topology_index);

    // Switch topology based on the profile_index defined in chromatix
    switch (pd->topology_index)
    {
        case chromatixTopoPDAFEnum::PDAFTopoProfileSparse:
        case chromatixTopoPDAFEnum::PDAFTopoProfileSparseAdvanced:
        case chromatixTopoPDAFEnum::PDAFTopoProfile2PD:
        case chromatixTopoPDAFEnum::PDAFTopoProfile2PDAdvanced:
        case chromatixTopoPDAFEnum::PDAFTopoProfileAdvanced:
        case chromatixTopoPDAFEnum::PDAFTOPOProfileMAX:
            // For GYRO_PANNING
            // Obtain logical sensitivity level for GYRO_PANNING value monitor
            sens_level = p_tuning->pAFMonitorTuningData->monitorAlgoConfig.topoPDAF.gyroPanning;
            // Obtain GYRO_PANNING parameters from value monitor bank and pick the index
            p_vm_cfg_tuning = get_chromatix_vm_config(pd->pTuningInfo, Q3A_VM_SET_TYPE_GYRO_PANNING);
            // Send the selected Value Monitor Tuning to Value Monitor Object
            pd->p_vm_gyro_panning->set_config(pd->p_vm_gyro_panning, p_vm_cfg_tuning, sens_level);
            // For SAD_PANNING
            // Obtain logical sensitivity level for SAD_PANNING value monitor
            sens_level = p_tuning->pAFMonitorTuningData->monitorAlgoConfig.topoPDAF.SADPanning;
            // Obtain SAD_PANNING parameters from value monitor bank and pick the index
            p_vm_cfg_tuning = get_chromatix_vm_config(pd->pTuningInfo, Q3A_VM_SET_TYPE_SAD_PANNING);
            // Send the selected Value Monitor Tuning to Value Monitor Object
            pd->p_vm_sad_panning->set_config(pd->p_vm_sad_panning, p_vm_cfg_tuning, sens_level);
            // For DEFOCUS_LOW
            // Obtain Logical sensitivity Level from DEFOCUS_LOW value monitor
            sens_level = p_tuning->pAFMonitorTuningData->monitorAlgoConfig.topoPDAF.defocusLow;
            // Obtain Defocus_LOW parameters from value monitor bank and pick the index
            p_vm_cfg_tuning = get_chromatix_vm_config(pd->pTuningInfo, Q3A_VM_SET_TYPE_DEFOCUS_LOW);
            // Send the selected Value Monitor Tuning to Value Monitor Object
            pd->p_vm_defocus_low->set_config(pd->p_vm_defocus_low, p_vm_cfg_tuning, sens_level);
            // For DEFOCUS_HIGH
            // Obtain Logical sensitivity Level from DEFOCUS_HIGH value monitor
            sens_level = p_tuning->pAFMonitorTuningData->monitorAlgoConfig.topoPDAF.defocusHigh;
            // Obtain Defocus_HIGH parameters from value monitor bank and pick the index
            p_vm_cfg_tuning = get_chromatix_vm_config(pd->pTuningInfo, Q3A_VM_SET_TYPE_DEFOCUS_HIGH);
            // Send the selected Value Monitor Tuning to Value Monitor Object
            pd->p_vm_defocus_high->set_config(pd->p_vm_defocus_high, p_vm_cfg_tuning, sens_level);
            break;

        default: // if new topology is utilized, creating new case is required
            HAF_MSG_ERROR("No matched Topology Index!");
            break;
    }

    return result;
}

/* af_pdaf_reset_data:
 * Reset internal pdaf variables. Used when we
 * transition from FOCUS CONVERGE/FALLBACK to monitor
 */
VOID af_pdaf_reset_data(
    af_alg_PDAF_internal_t* pd)
{
    UINT32 i;
    af_pdaf_params_t* p_params = &pd->params;
    af_tuning_info* p_tuningInfo = pd->pTuningInfo;
    HAF_MSG_HIGH("PDAF Data Reseted");
    // reset scene monitor analytics
    p_params->is_scene_changed = FALSE;
    p_params->scene_change_cnt = 0;
    p_params->focus_converge_cnt = 0;
    p_params->frame_skip = 0;
    //reset wait conf recover cnt to normal from first search
    p_params->wait_conf_recover_cnt = p_tuningInfo->pPDAFTuningData->sceneMonitor.waitConfidenceRecoverCount;
    pd->last_search_roi_type = p_params->roi_info.type;

    for (i = 0; i < p_tuningInfo->pPDAFTuningData->ROI.ConfigurationCount/*p_tuning->roi.num_entries*/; i++)
    {
        p_params->roi[i].defocus_cnt = 0;
        p_params->roi[i].rev_cnt = 0;
        p_params->roi[i].converge_rev_cnt = 0;
        p_params->roi[i].stable_cnt = 0;
        p_params->roi[i].not_conf_cnt = 0;
    }

    p_params->run_converge_done_temporal_filter = FALSE;
    p_params->converge_done_temporal_counter = 0;
    p_params->conf_cnt = 0;
    p_params->dynamic_frame_cnt = 0;
    p_params->dynamic_frame_skip_running = 0;
    p_params->frame_skip_cnt = 0;
    p_params->fallback_single_during_search = 0;
    p_params->MW_grids_converge_used = 0;
    p_params->MW_switch_to_peripheral_during_search = 0;
    p_params->MW_in_lowlight = 0;
    p_params->MW_is_multi_depth = 0;
    p_params->wait_caf_stable_cnt = 0;
#ifdef VENDOR_EDIT
    p_params->rev_cnt_pct_drop = 1.0;
#endif
}

//Check if PD stats updated for current frame or not
static BOOL af_pdaf_check_if_pd_stats_updated(
    af_alg_PDAF_internal_t* pd)
{
    if (pd->params.is_pd_updated == TRUE)
    {
        pd->params.is_pd_updated = FALSE;
        pd->params.pd_no_update_cnt = 0;
        return TRUE;
    }
    else
    {
        // stats are not updated for this frame, increment cnt
        pd->params.pd_no_update_cnt++;
        return FALSE;
    }
}

/** af_pdaf_is_soft_threshold_triggered:
 *  If defocus value is less than defocus threshold and
 *  above soft threshold, then scene change determines if
 *  focus search needs to be triggered
 *
 * @pd: internal Haf data structure
 *
 *  Return: BOOL
 **/
INT af_pdaf_is_soft_threshold_triggered(
    af_alg_PDAF_internal_t* pd)
{
    INT rc = 0;
    BOOL is_within_soft_thres = FALSE;
    INT min_scene_chg_cnt, j;
    INT focus_roi_id;
    FLOAT dof_multiplier, min_defocus_thres, defocus_thres, soft_defocus_thres, soft_defocus_thres_high;
    FLOAT average_defocus = 0, average_defocus_high_thres = 0;
    af_pdaf_params_t* p_params = NULL;

    // Sanity check
    if (NULL == pd)
        return rc;

    p_params = &pd->params;
    min_scene_chg_cnt = (INT) p_params->soft_scene_change_cnt;
    // calculate soft threshold
    // adjust defocus threshold by noise estimate
    min_defocus_thres = pd->pTuningInfo->pPDAFTuningData->sceneMonitor.defocusedThreshold;
    dof_multiplier = af_pdaf_get_dof_multiplier(pd, pd->input.lens_pos);
    defocus_thres = min_defocus_thres * dof_multiplier;
    soft_defocus_thres = min_defocus_thres * p_params->soft_rgn_thres_multiplier;
    soft_defocus_thres_high = defocus_thres * p_params->soft_rgn_thres_high_multiplier;

    if (p_params->enable_soft_rgn_thres_high)
        average_defocus_high_thres = soft_defocus_thres_high;
    else
        average_defocus_high_thres = defocus_thres;

    /*use roi id, to calculate if scene is defocused,
     whose confidence and stability is used as decision factor */
    focus_roi_id = af_pdaf_get_focus_roi_id(pd);

    //Calculate average defocus
    for (j = 0; j < p_params->defocus_stable_filter_len; j++)
        average_defocus += (FLOAT) p_params->roi[focus_roi_id].defocus_history[PD_IDX(p_params->idx - j)];

    average_defocus = (FLOAT) fabs(average_defocus / (FLOAT) p_params->defocus_stable_filter_len);
    // is average defocus within soft threshold and defocus threshold
    is_within_soft_thres =
        (average_defocus > soft_defocus_thres && average_defocus < average_defocus_high_thres) ? TRUE : FALSE;

    // Condition to check if stable threshold trigger is set
    if (average_defocus > average_defocus_high_thres)
    {
        rc = 2;
    }
    else if (p_params->scene_change_cnt >= min_scene_chg_cnt && is_within_soft_thres)
    {
        rc = 1;
    }
    else
    {
        rc = 0;
    }

    HAF_MSG_HIGH("defocus_thres=%f soft_defocus_thres=%f average_defocus_high_thres= %f avg_defocus=%f"
                 " scene_change_cnt=%d, min_scene_chg_cnt %d is_within_soft_thres %d rc=%d multiplier %f",
                 defocus_thres,
                 soft_defocus_thres,
                 average_defocus_high_thres,
                 average_defocus,
                 p_params->scene_change_cnt,
                 min_scene_chg_cnt,
                 is_within_soft_thres,
                 rc,
                 p_params->soft_scene_change_cnt);
    return rc;
}

static VOID af_pdaf_get_debug_data(
    af_alg_interface_t* p_alg,
    VOID* debug_data)
{
    CAMX_ASSERT(NULL != p_alg);
    CAMX_ASSERT(NULL != debug_data);

    af_alg_PDAF_internal_t* pd = (af_alg_PDAF_internal_t*) p_alg;
    af_get_debug_data *pGetDebugData = (af_get_debug_data *)debug_data;

    AFDebugDataType* pDebugData = (AFDebugDataType*)pGetDebugData->payload;
    CAMX_ASSERT(NULL != pDebugData);

    switch (pGetDebugData->type)
    {
    case AF_GET_DEBUG_DATA_SEARCH:
    {
        AFDebugPhaseDifference *pConvergeData = (AFDebugPhaseDifference*)(&pDebugData->pdConvergeData);
        pConvergeData->isEarlyInterruptEnable = pd->pTuningInfo->pPDAFTuningData->enablePDEarlyInterrupt;
#ifdef VENDOR_EDIT
        /* Just a hack for lcr debug exif, before multiwindow is available. */
        pConvergeData->isMultiWindowEnable = pd->lcrUsed;
#else
        pConvergeData->isMultiWindowEnable = pd->settingsInfo->mwEnable;
#endif
        pConvergeData->xMultiWindowNumber = pd->input.pdaf_info.x_win_num;
        pConvergeData->yMultiWindowNumber = pd->input.pdaf_info.y_win_num;

        AFDebugPDHistoryType *pHistoryData = &pConvergeData->historyData[pDebugData->currentConvergeIndex];
        pHistoryData->sensorGain = pd->input.sensor_gain;
        pHistoryData->roiType = pd->mw_roi_info.type == PDAF_MW_PERIPHERAL ?
            AFDebugMultiWindowType::AF_DEBUG_PDAF_MW_PERIPHERAL :
            AFDebugMultiWindowType::AF_DEBUG_PDAF_MW_CENTRAL;
        if (pd->settingsInfo->mwEnable && AF_ROI_TYPE_GENERAL == pd->params.roi_info.type)
        {
            pHistoryData->selectedWindowIndex = pd->params.roi[0].id;
        }
        else
        {
            pHistoryData->selectedWindowIndex = af_pdaf_get_focus_roi_id(pd);
        }
        for (INT i = 0; i < pConvergeData->yMultiWindowNumber; i++)
        {
            for (INT j = 0; j < pConvergeData->xMultiWindowNumber; j++)
            {
                INT index = i * pConvergeData->xMultiWindowNumber + j;
                pHistoryData->centerWindowPD[index] = pd->input.pdaf_info.defocus[index].phase_diff;
                pHistoryData->centerWindowConf[index] = pd->input.pdaf_info.defocus[index].df_conf_level;
                pHistoryData->centerWindowDefocus[index] = pd->input.pdaf_info.defocus[index].defocus;
            }
        }

        for (INT i = 0; i < MAX_PERIPHERAL_WINDOWS; i++)
        {
            pHistoryData->peripheralWindowPD[i] = pd->input.pdaf_info.peripheral[i].phase_diff;
            pHistoryData->peripheralWindowConf[i] = pd->input.pdaf_info.peripheral[i].df_conf_level;
            pHistoryData->peripheralWindowDefocus[i] = pd->input.pdaf_info.peripheral[i].defocus;
        }
    }
        break;

    case AF_GET_DEBUG_DATA_MONITOR:
    {
        AFDebugMonitorPDAF* pMonitorPDAF = &pDebugData->pdafMonitorData[pDebugData->currentMonitorIndex];
        AFDebugMonitorCommon* pMonitorCommon = &pDebugData->monitorCommonData[pDebugData->currentMonitorIndex];
        pDebugData->activeMonMask = (AFDebugActiveMonitorType)((INT8)pDebugData->activeMonMask |
            (INT8)AFDebugActiveMonitorType::AF_DEBUG_ACTIVE_MON_PDAF);
        pMonitorPDAF->defocusLow.input        = pd->p_vm_defocus_low->input;
        pMonitorPDAF->defocusLow.sensitivity  = pd->p_vm_defocus_low->af_vm_intern_params.sensitivity;
        pMonitorPDAF->defocusLow.rebase       = pd->p_vm_defocus_low->af_vm_intern_params.rebasing;
        pMonitorPDAF->defocusLow.output       = pd->p_vm_defocus_low->decision;
        pMonitorPDAF->defocusHigh.input       = pd->p_vm_defocus_high->input;
        pMonitorPDAF->defocusHigh.sensitivity = pd->p_vm_defocus_high->af_vm_intern_params.sensitivity;
        pMonitorPDAF->defocusHigh.rebase      = pd->p_vm_defocus_high->af_vm_intern_params.rebasing;
        pMonitorPDAF->defocusHigh.output      = pd->p_vm_defocus_high->decision;
    }
        break;

    default:
        break;

    }

}

#ifdef VENDOR_EDIT
// set input for defocus VM
static VOID af_pdaf_defocus_set_input(af_alg_PDAF_internal_t* pd)
{
    /* select the focus_roi_id and send the defocus and confidence valuesto the value monitor */
    INT focus_roi_id = af_pdaf_get_focus_roi_id(pd);
    FLOAT dof_multiplier = af_pdaf_get_dof_multiplier(pd, pd->input.lens_pos);
    FLOAT noise_multiplier = af_pdaf_get_noise_multiplier(pd);

    FLOAT defocus_scaled = 0;
    FLOAT defocus = (FLOAT) pd->params.roi[focus_roi_id].defocus;

    FLOAT pd_conf_multipiler = 1;

     // set the multiplier to be 1 if it is smaller than 1
    dof_multiplier = MAX(dof_multiplier,1);
    noise_multiplier = 1;//MAX(noise_multiplier,1);


    if(pd->params.camera_run_mode == AF_RUN_MODE_VIDEO)
    {
        pd_conf_multipiler = af_pdaf_get_pd_conf_multiplier(pd);
    } else {
        pd_conf_multipiler = 1;
    }
    // reset defocus value w.r.t. multiplier
    defocus_scaled = defocus / (MAX(dof_multiplier, noise_multiplier)*pd_conf_multipiler);

    /* If someone puts an object very close like 3cm or 4cm,
       PDlib will give -ve defocus but the Monitor logic thinks
       there is a scene change even the lens is at the macro limit
       and monitor keeps retriggering. The below logic wouldnt
       retrigger un-neccesarily*/
    if ((pd->cur_pos == pd->params.near_limit && defocus_scaled < 0) ||
        (pd->cur_pos == pd->params.far_limit && defocus_scaled > 0))
    {
        defocus_scaled = 0;
    }

    /* input the defocus data to value monitor */
    pd->p_vm_defocus_low->set_input(pd->p_vm_defocus_low, defocus_scaled);
    pd->p_vm_defocus_high->set_input(pd->p_vm_defocus_high, defocus_scaled);
    HAF_MSG_HIGH("Received pd data, defocus %f, dof_multiplier %f, noise_multiplier %f, scaled defocus %f, video pd_conf_multipiler %f",
            defocus,
            dof_multiplier,
            noise_multiplier,
            defocus_scaled,
            pd_conf_multipiler);
}

static boolean af_pdaf_is_low_light(
    af_alg_PDAF_internal_t* pd)
{
    bool in_low_light =  0;
    chromatixAFCommonType* af_tuning = &(pd->pTuningInfo->pAlgoTuningData->commonElements);

    in_low_light = ( pd->set_parameters.aec_input.cur_real_gain >= HUNTING_LOW_LIGHT_GAIN_THR );

    HAF_MSG_HIGH(" AEC cur_real_gain: %f lowLightGainThreshold: %d, in_lowlight %d",
        pd->set_parameters.aec_input.cur_real_gain, HUNTING_LOW_LIGHT_GAIN_THR, in_low_light);
    return in_low_light;
}

#endif
/** af_pdaf_process_monitor:
 * PDAF process_monitor API. Handle monitor state for PDAF.
 *
 *
 *
 *  Return: VOID
 **/
static BOOL af_pdaf_process_monitor(
    af_alg_interface_t* p_alg,
    af_haf_monitor_input_t* input,
    af_haf_monitor_data_t* output)
{
    BOOL result = TRUE;

    // NULL pointer check
    if (NULL == p_alg || NULL == output)
    {
        result = FALSE;
        return result;
    }

    // Typecast from af_alg_interface_t to af_alg_PDAF_internal_t
    af_alg_PDAF_internal_t* pd = (af_alg_PDAF_internal_t*) p_alg;

    if ((!pd->enable) || (pd->settingsInfo->disablePDAF))
    {
        AF_MSG_HIGH("pdaf not enabled");
        Q3A_MEMSET(output, 0, sizeof(af_haf_monitor_data_t));
        result = FALSE;
        return result;
    }

    af_roi_type roi_type = pd->set_parameters.roi.type;
    pd->cur_pos = input->cur_pos;
    HAF_MSG_LOW("PD enable %d roi_type %d", pd->enable, roi_type);

    /**
     * here we only proc pd with the new pd_stats.
     * reference the af_pdaf_update_stats.
     * otherwise, it will make pd index wrong.
     */
    if (pd->params.frame_skip == 0)
    {
        /* Process depth information first before
           going to monitor, this is essential for
           early_interrupt, in the earlier implementation
           we used to process depth_info in last frame, here
           we need to process depth_info and monitor in the
           same frame but first process depth_info then do
           Monitor*/
        af_pdaf_proc_pd(pd, &pd->input);
        af_pdaf_proc_noise(pd, &pd->input);
        af_pdaf_proc_depth_stable(pd, &pd->input);
        af_pdaf_proc_rev_dir(pd, &pd->input);
        af_pdaf_proc_defocused(pd, &pd->input);
    }
    else
    {
        HAF_MSG_HIGH("skipped due to frame_skip: %d", pd->params.frame_skip);
    }

    result = af_pdaf_monitor(pd, output);
    return result;
}

/** af_pdaf_monitor:
 * Handle monitor state for PDAF.
 *
 * @haf:
 *
 *  Return: VOID
 **/
static BOOL af_pdaf_monitor(
    af_alg_PDAF_internal_t* pd,
    af_haf_monitor_data_t* output)
{
    BOOL result = TRUE;

    // NULL pointer check
    if (NULL == pd)
    {
        return FALSE;
    }

    // Always check for the validity of PD stats for current frame
    // Increment the counter if not updated for current frame
    af_pdaf_check_if_pd_stats_updated(pd);
    af_pdaf_params_t* p_params = &pd->params;
    af_pdaf_monitor_data_t* p_mon = &pd->monitor_data;
    af_pdaf_monitor_data_t mon_data = {FALSE, FALSE, FALSE, FALSE};
    BOOL defocus_low_is_change = FALSE;
    BOOL defocus_high_is_change = FALSE;
    BOOL sad_is_panning = FALSE;
    BOOL gyro_is_panning = FALSE;

    // switch different topology depends on the topology index
    switch (pd->topology_index)
    {
        case chromatixTopoPDAFEnum::PDAFTopoProfileSparse:
        case chromatixTopoPDAFEnum::PDAFTopoProfile2PD:
        case chromatixTopoPDAFEnum::PDAFTopoProfileAdvanced:
        case chromatixTopoPDAFEnum::PDAFTopoProfileSparseAdvanced:
        case chromatixTopoPDAFEnum::PDAFTopoProfile2PDAdvanced:
            {
                af_pdaf_update_defocus_monitor(pd);

                /* Call SAD_PANNING value monitor */
                sad_is_panning = pd->p_vm_sad_panning->get_trigger(pd->p_vm_sad_panning);
                /* Call GYRO_PANNING value monitor*/
                gyro_is_panning = pd->p_vm_gyro_panning->get_trigger(pd->p_vm_gyro_panning);
                /* Call DEFOCUS_LOW value monitor */
                defocus_low_is_change = pd->p_vm_defocus_low->get_trigger(pd->p_vm_defocus_low);
                /* Call DEFOCUS_HIGH value monitor */
                defocus_high_is_change = pd->p_vm_defocus_high->get_trigger(pd->p_vm_defocus_high);
                HAF_MSG_LOW("monitors: defocus_low %d, defocus_high %d sad_panning %d, gyro_panning %d aec_settled %d",
                    defocus_low_is_change,
                    defocus_high_is_change,
                    sad_is_panning,
                    gyro_is_panning,
                    pd->set_parameters.aec_input.aec_settled);
                /* Overwrite is_defocused by value monitor decision */
                mon_data.is_defocused = (defocus_low_is_change | defocus_high_is_change);
                /* Referring caf stability */
                p_params->is_caf_stable = (!(sad_is_panning | gyro_is_panning)) &&
                    (pd->set_parameters.aec_input.aec_settled|| af_pdaf_is_advanced_sparsePD_result(pd));
                /* Overwrite is_stable flag by checkingselected roi_id directly */
                int focus_roi_id = af_pdaf_get_focus_roi_id(pd);
                mon_data.is_stable = pd->params.roi[focus_roi_id].is_stable;
                /* Will overwrite the is_conf flag by value monitor decision */
                mon_data.is_conf = (boolean)af_pdaf_is_depth_conf(pd);
#ifdef VENDOR_EDIT
                p_params->sad_is_panning = sad_is_panning;
                p_params->gyro_is_panning = gyro_is_panning;
                if (!p_params->is_caf_stable)
                    p_params->wait_caf_stable_cnt++;
                else
                    p_params->wait_caf_stable_cnt = 0;
#endif
                break;
            }

        default:
            {
                HAF_MSG_LOW("No matched PDAF Topology Profile Index!");
                break;
            }
    }

    /* 2. Heuristic manipulations */
    /* 2.1 Spot light flag */
    /* Spotlight detected will override PDAF confidence level so CAF will be triggered instead */
    if(0)// (p_params->is_spotlight_detected)
    {
        mon_data.is_conf = FALSE;
    }

    HAF_MSG_LOW("Spotlight flag %d,conf %d", p_params->is_spotlight_detected,
                mon_data.is_conf);

    /* 2.2 Overwrite monitor outputs if pd is not confident */
    if(mon_data.is_conf == FALSE)
    {
        HAF_MSG_LOW("Defocus is not confident. Overwrite monitor stability/defocus");
        mon_data.is_defocused = FALSE;
        mon_data.is_stable = TRUE;
    }

    /* 2.3 set the pd as stable if is_defocus_high == 1*/
    if(defocus_high_is_change == TRUE)
    {
        HAF_MSG_LOW("Defocus high is changed, overwrite the monitor stability");
        mon_data.is_stable = TRUE;
    }

    /* 2.4 Determine to trigger refocusing based on the value monitor decision */
    mon_data.is_trig_refocus =
        mon_data.is_conf &&
        mon_data.is_defocused &&
        mon_data.is_stable;

    /* 2.5 Propagate Local mon_data to p_mon */

    /* switch different topology depends on the topology index */
    switch(pd->topology_index)
    {
        case chromatixTopoPDAFEnum::PDAFTopoProfileSparse:
        case chromatixTopoPDAFEnum::PDAFTopoProfileSparseAdvanced:

            /* 2.5.1 If face is detected, ignore suppress all pdaf monitor decision
                     Otherwise, send pd monitor result to p_mon */
#ifdef VENDOR_EDIT
            if(p_params->roi_info.type == AF_ROI_TYPE_FACE && af_pdaf_is_valid_roi_to_run_pd(pd) == FALSE)
#else
            if(p_params->roi_info.type == AF_ROI_TYPE_FACE)
#endif
            {
                if(mon_data.is_trig_refocus == TRUE)
                {
                    HAF_MSG_LOW("Sparse PD Face detected, PDAF trigger ignored");
                    p_mon->is_defocused = FALSE;
                    p_mon->is_stable = TRUE;
                    p_mon->is_trig_refocus = FALSE;
                    p_mon->is_conf = FALSE;
                }
            }
            else
            {
                p_mon->is_conf = mon_data.is_conf;
                p_mon->is_defocused = mon_data.is_defocused;
                p_mon->is_stable = mon_data.is_stable;
                p_mon->is_trig_refocus = mon_data.is_trig_refocus;
            }

            break;

        case chromatixTopoPDAFEnum::PDAFTopoProfile2PD:
        case chromatixTopoPDAFEnum::PDAFTopoProfileAdvanced:
        case chromatixTopoPDAFEnum::PDAFTopoProfile2PDAdvanced:
        case chromatixTopoPDAFEnum::PDAFTOPOProfileMAX:

            /* 2.5.1 If face is detected, ignore suppress all pdaf monitor decision
                      Otherwise, send pd monitor result to p_mon */
            if(p_params->roi_info.type == AF_ROI_TYPE_FACE)
            {
                HAF_MSG_LOW("Face tracking PD. Run HAF monitor, do not ignore PDAF trigger");

                if ( pd->conv_data.status != AF_PDAF_CONVERGE_FALLBACK)
                {
                    p_mon->is_conf = mon_data.is_conf;
                    p_mon->is_defocused = mon_data.is_defocused;
                    p_mon->is_stable = mon_data.is_stable;
                    p_mon->is_trig_refocus = mon_data.is_trig_refocus;
                }
                else
                {
                    HAF_MSG_LOW("Face tracking PD fallback suppress. No ROI change yet. PDAF trigger ignored");
                    p_mon->is_defocused = FALSE;
                    p_mon->is_stable = TRUE;
                    p_mon->is_trig_refocus = FALSE;
                    p_mon->is_conf = FALSE;
                }
            }
            else
            {
                p_mon->is_conf = mon_data.is_conf;
                p_mon->is_defocused = mon_data.is_defocused;
                p_mon->is_stable = mon_data.is_stable;
                p_mon->is_trig_refocus = mon_data.is_trig_refocus;
            }

            break;

        default:
            HAF_MSG_LOW("No matched PDAF Topology Profile Index!");
            break;
    }

    HAF_MSG_HIGH("is_conf %d, is_defocused %d, is_stable %d, is_trig_refocus %d",
                 mon_data.is_conf, mon_data.is_defocused, mon_data.is_stable, mon_data.is_trig_refocus);
    /* 3. Populate process_monitor output according to result of monitor */
    af_pdaf_populate_process_monitor_output(pd, output);

    /* 4. Overwrite the confidence if the defocus is in the uncertain zone */
    if ((defocus_low_is_change == 1)
            && (defocus_high_is_change == 0)
            && (output->confidence == AF_CONFIDENCE_HIGH))
    {
        output->confidence = AF_CONFIDENCE_MEDIUM;
        output->is_trig_refocus = FALSE;
    }
#ifdef VENDOR_EDIT
    if (TRUE == p_params->hunting_prevent.enable &&
        !af_pdaf_is_low_light(pd)&&
        (p_params->camera_run_mode != AF_RUN_MODE_VIDEO || AF_PD_HUNTING_PREVENT_VIDEO_MODE_ENABLE)) {
        p_params->focus_roi_id = af_pdaf_get_focus_roi_id(pd);
        if (TRUE == p_params->hunting_prevent.hunting_prevent_trigger/* ||
            p_params->pre_pd_not_conf == TRUE*/ ||
            p_params->pre_rev_cnt_fallback == TRUE) {
            if (output->is_trig_refocus == TRUE) {
                output->confidence = AF_CONFIDENCE_MEDIUM;
                output->is_trig_refocus = FALSE;
                HAF_MSG_HIGH("suppress focus trigger in absence of scene change by hunting prevent. pre_rev_cnt_fallback %d pre_pd_not_conf %d hunting_prevent_trigger %d",
                    p_params->pre_rev_cnt_fallback, p_params->pre_pd_not_conf,
                    p_params->hunting_prevent.hunting_prevent_trigger);
          }
       
        }
    }
#endif
    return result;
}

/** af_pdaf_update_defocus_monitor:
 * Update defocus monitor input value
 *
 * @haf:
 *
 *  Return: VOID
 **/
static VOID af_pdaf_update_defocus_monitor(
    af_alg_PDAF_internal_t* pd)
{
    /* select the focus_roi_id and send the defocus and confidence values
       to the value monitor */
    INT focus_roi_id = af_pdaf_get_focus_roi_id(pd);
    FLOAT multiplier = af_pdaf_get_dof_multiplier(pd, pd->input.lens_pos);
    FLOAT defocus_scaled = 0;
    FLOAT defocus = (FLOAT) pd->params.roi[focus_roi_id].defocus;
#ifdef VENDOR_EDIT
    FLOAT defocus_base_scaled = 0;
    FLOAT defocus_base = (FLOAT)(defocus - pd->params.hunting_prevent.defocus_base);
#endif

    // set the multiplier to be 1 if it is smaller than 1
    if (multiplier < 1)
    {
        multiplier = 1;
    }

    // reset defocus value w.r.t. dof multiplier
    defocus_scaled = defocus / multiplier;
#ifdef VENDOR_EDIT
    defocus_base_scaled = defocus_base / multiplier;
#endif
    /* If someone puts an object very close like 3cm or 4cm,
       PDlib will give -ve defocus but the Monitor logic thinks
       there is a scene change even the lens is at the macro limit
       and monitor keeps retriggering. The below logic wouldnt
       retrigger un-neccesarily*/
    if (pd->cur_pos == pd->params.near_limit  && defocus_scaled < 0)
    {
        defocus_scaled = 0;
    }

    if (pd->cur_pos == pd->params.far_limit  && defocus_scaled > 0)
    {
        defocus_scaled = 0;
    }

    /* input the defocus data to value monitor */
    pd->p_vm_defocus_low->set_input(pd->p_vm_defocus_low, defocus_scaled);

#ifdef VENDOR_EDIT
    if(TRUE == pd->params.hunting_prevent.enable){/*video mode should always enable this, as pd is about 30steps at fv peak in 20cm dxo face chart then retrigger af*/
        pd->p_vm_defocus_high->set_input(pd->p_vm_defocus_high, defocus_base_scaled);
    } else {
        pd->p_vm_defocus_high->set_input(pd->p_vm_defocus_high, defocus_scaled);
    }
#else
    pd->p_vm_defocus_high->set_input(pd->p_vm_defocus_high, defocus_scaled);
#endif

    HAF_MSG_HIGH("Received pd data, defocus %f, defocus_base %f, hunting_prevent->defocus_base %d mulltiplier %f, scaled defocus %f, defocus_base_scaled %f",
        defocus,
        defocus_base,
        pd->params.hunting_prevent.defocus_base,
        multiplier,
        defocus_scaled,
        defocus_base_scaled);
}

/** af_pdaf_populate_process_monitor_output:
 * Fill in monitor output result for PDAF.
 *
 * @haf:
 *
 *  Return: VOID
 **/
static VOID af_pdaf_populate_process_monitor_output(
    af_alg_PDAF_internal_t* pd,
    af_haf_monitor_data_t* output)
{

    // NULL pointer check
    if (NULL == pd || NULL == output)
    {
        return;
    }

    af_pdaf_monitor_data_t* p_mon = &pd->monitor_data;
    chromatixPDMultiwindowType* mw_tuning = &pd->pTuningInfo->pPDAFTuningData->PDMultiWindow;

    // The following output values can take INTermediate values. The values can have their own meaning.
    // Convert is_defocused to focus_level, note: the two variables have reverse meaning.
    if (FALSE == p_mon->is_defocused)
    {
        output->focus_level = AF_FOCUS_LEVEL_HIGH;
    }
    else
    {
        output->focus_level = AF_FOCUS_LEVEL_LOW;
    }

    // Convert is_stable to stability
    if (TRUE == p_mon->is_stable)
    {
        output->stability = AF_STABILITY_HIGH;
    }
    else
    {
        output->stability = AF_STABILITY_LOW;
    }

    switch (pd->topology_index)
    {
        case chromatixTopoPDAFEnum::PDAFTopoProfileSparse:
        case chromatixTopoPDAFEnum::PDAFTopoProfile2PD:
        case chromatixTopoPDAFEnum::PDAFTopoProfileAdvanced:
        case chromatixTopoPDAFEnum::PDAFTopoProfileSparseAdvanced:
        case chromatixTopoPDAFEnum::PDAFTopoProfile2PDAdvanced:
        case chromatixTopoPDAFEnum::PDAFTOPOProfileMAX:

            // Convert based on the monitor output
            if (p_mon->is_conf == TRUE)
            {
                output->confidence = AF_CONFIDENCE_HIGH;
            }
            else
            {
                output->confidence = AF_CONFIDENCE_LOW;
            }

            break;

        default:
            HAF_MSG_ERROR("No matched PDAF Topology Index!");
            break;
    }

    output->is_trig_refocus = p_mon->is_trig_refocus;
    output->target_pos = pd->params.estimated_target_pos;

    /* If multi-window is enabled, update mw output data as well
     *
     * This has been done assuming that Multi-Window chromatix/tuning parameter would always
     * be enabled from tuning, thus giving setprop the priority
     * By default setprop for Multi-Window is enabled
     */
    if (pd->settingsInfo->mwEnable && mw_tuning->enable)
    {
        output->mw_roi_info = pd->mw_roi_info;
    }

    // Save current face ROI info to prev face ROI info
#ifdef VENDOR_EDIT
    Q3A_MEMCPY_S(&pd->params.prev_roi_info, sizeof(pd->params.prev_roi_info), &pd->params.roi_info,
                 sizeof(pd->params.roi_info));
#else
    Q3A_MEMCPY_S(&pd->params.prev_roi_info, sizeof(pd->prev_roi_info), &pd->params.roi_info,
                 sizeof(pd->params.roi_info));
#endif
    HAF_MSG_LOW("focus_level %d ,stability %d,confidence %d is_trig_refocus %d",
                output->focus_level,
                output->stability,
                output->confidence,
                output->is_trig_refocus);
}

int af_pdaf_dynamic_frame_skip_early_interrupt(af_alg_PDAF_internal_t * pd)
{
    af_input_from_aec_t *aec_input;
    af_pdaf_focus_converge_data_t* p_conv = &pd->conv_data;
    chromatixPDAFType* pPDAFTuningData = pd->pTuningInfo->pPDAFTuningData;
    float min_frame_duration;
    float vb_time;
    int frame_skip;

    if (pd == NULL)
    {
        HAF_MSG_ERROR("NULL pointer");
        return 0;
    }

    // TODO: should calculated from sensor clock/max FPS
#ifdef VENDOR_EDIT
    min_frame_duration = 5.0f;
#else
    min_frame_duration = 33.0f;
#endif
    aec_input = &pd->set_parameters.aec_input;
    vb_time = min_frame_duration - aec_input->exp_time;
    frame_skip = pd->pTuningInfo->pPDAFTuningData->focusScan.focusConvergeFrameSkip;

    HAF_MSG_LOW("AEC_Info: exp_time %f preview_fps %d frame_interval %f vb_time %f lux_index %f",
        aec_input->exp_time,
        aec_input->current_fps,
        min_frame_duration,
        vb_time,
        aec_input->lux_idx);

    // If anytime vertical blanking time is less than 15 msec, then we will skip a frame to allow
    // enough lens move time
    if (vb_time > MINUS_ONE_FRAME_SKIP_VBT_THRESHOLD ||
        ABS(p_conv->next_pos - pd->cur_pos) < pPDAFTuningData->focusScan.focusDoneThreshold) {
        frame_skip = MAX(frame_skip - 1, 0);
        HAF_MSG_LOW("use frame_skip -1 (%d) frame skip due to long VBT %f",
            frame_skip, vb_time);
    }

    return frame_skip;
}

void af_pdaf_dynamic_frame_skip(af_alg_PDAF_internal_t* pd)
{
    af_pdaf_params_t* p_params = &pd->params;
    int cur_pos = pd->cur_pos;
    af_pdaf_focus_converge_data_t* p_conv = &pd->conv_data;
    int i = 0;
    int cur_defocus = 0;
    int diff_defocus = 0, diff_lens_pos = 0, diff = 0;

    /* During the first frame of the FOCUS_CONVERGE enable
       dynamic frame skip logic, later if the stats has problem
       disable dynamically*/
    if (p_params->dynamic_frame_cnt == 0)
    {
        p_params->dynamic_frame_skip_running = 1;
    }

    if (p_params->dynamic_frame_skip_running)
    {
        if (p_params->roi[i].conf == FALSE &&
                p_params->dynamic_frame_cnt < 3)
        {
            /* In early inttreupt or Zero Touch Lag, ISP takes
               1 or 2 frames to configure after the first jump,
               at that time, pdlib keep sending low confidence
               because Pdlib doesnt have enough lines to process
               hence we shouldnt fallback to contrast AF immediately
               keep a count of 3frames and then wait until we see
               a  high confidence. No need to put this to tuning param
               2 is more than enough but for safe approach made it 3*/
            if (p_params->conf_cnt >= 3)
            {
                p_conv->status = AF_PDAF_CONVERGE_FALLBACK;
                p_conv->next_pos = cur_pos;
            }
            else
            {
                p_conv->status = AF_PDAF_CONVERGE_RUNNING;
                p_conv->next_pos = cur_pos;
                p_params->frame_skip = 1;
                p_params->conf_cnt++;
            }
        }
        else if (p_params->dynamic_frame_cnt == 0)
        {
            /* For Zero Touch Lag, when its a first frame, we have to
               jump the very first frame to get the feel of instant ZTL
               hence dont add frame delay */
            p_params->prev_defocus =  p_params->roi[i].defocus;
            p_params->prev_lens_pos = pd->cur_pos;
            p_params->frame_skip = 0;
            p_params->frame_skip_cnt = 0;
            p_conv->next_pos = cur_pos;
            p_params->conf_cnt = 0;
            p_conv->status = AF_PDAF_CONVERGE_RUNNING;
        }
        else
        {
            /* This else case will dynamically decide if we need
               to add a frame delay or not based on the current defocus
               and prev_defocus and the lens moved from the prev frame
               if the defocus change is not significant then add a frame
               delay else dont add it. The reason why we need is even
               though we skip frame after big jump, ISP may not be in
               sync hence might need extra frame delay*/
            cur_defocus = (p_params->roi[i].defocus);
            cur_pos = pd->cur_pos;
            diff_defocus = abs(cur_defocus - p_params->prev_defocus);
            diff_lens_pos = abs(cur_pos - p_params->prev_lens_pos);
            diff = abs(diff_lens_pos - diff_defocus);

            if (diff > 10) //its fixed to avoid overshooting. let it be hardcode
            {
                p_params->frame_skip = 1;
                p_params->frame_skip_cnt++;
                p_conv->next_pos = cur_pos;
            }
            else
            {
                p_params->frame_skip = 0;
                p_params->frame_skip_cnt = 0;
            }

            p_params->conf_cnt = 0;
            p_params->prev_defocus = cur_defocus;
            p_params->prev_lens_pos = cur_pos;
            p_conv->status = AF_PDAF_CONVERGE_RUNNING;
        }
    }

    /* Logic to come out of dynamic frame skip
       if we keep adding frame skip for two consecutive
       frames after the third frame then come out of this
       logic and add frame delay for alternative frame same as
       old logic*/
    if (p_params->frame_skip_cnt > 1 &&
            p_params->dynamic_frame_cnt > 3)
    {
        p_params->dynamic_frame_skip_running = 0;
    }
    else
    {
        p_params->dynamic_frame_skip_running = 1;
    }

    HAF_MSG_HIGH("frame_skip %d diff %d frame_cnt %d, prev_defocu %d, cur_defocus %d, "
                 " diff_defocus %d cur_pos %d prev_lens_pos %d, diff_lens %d "
                 " next_pos %d status %d conf_cnt %d frame_skip_cnt %d dynamic_frame_skip_running %d",
                 p_params->frame_skip,
                 diff,
                 p_params->dynamic_frame_cnt,
                 p_params->prev_defocus,
                 cur_defocus,
                 diff_defocus,
                 cur_pos,
                 p_params->prev_lens_pos,
                 diff_lens_pos,
                 p_conv->next_pos,
                 p_conv->status,
                 p_params->conf_cnt,
                 p_params->frame_skip_cnt,
                 p_params->dynamic_frame_skip_running);
    p_params->dynamic_frame_cnt++;
}


/** af_pdaf_process_search:
 * PDAF process_search API. Handle search state for PDAF.
 *
 *
 *
 *  Return: VOID
 **/
static BOOL af_pdaf_process_search(
    af_alg_interface_t* p_alg,
    af_haf_focus_search_input_t* input,
    af_haf_focus_search_data_t* output)
{
    BOOL result = TRUE;
    BOOL is_frame_skip = FALSE;

    // NULL pointer check
    if (NULL == p_alg || NULL == output || NULL == input)
    {
        result = FALSE;
        return result;
    }

    // Typecast from af_alg_interface_t to af_alg_PDAF_internal_t
    af_alg_PDAF_internal_t* pd = (af_alg_PDAF_internal_t*) p_alg;
    af_pdaf_params_t* p_params = &pd->params;
    INT cur_pos = input->cur_pos;
    af_tuning_info* p_tuning = pd->pTuningInfo;

    /* **************************************************************************
     *  Process search will always run converge, get_next_pos and fine
     *  search all together. In the end the output will be populated, it is
     *  up to HAF framwork to determine which state to enter and use the output
     *  from process search accordingly.
     *****************************************************************************/
    if ((!pd->enable) || (pd->settingsInfo->disablePDAF))
    {
        HAF_MSG_HIGH("pd is not enabled, go to Contrast AF");
        output->progress = AF_PROGRESS_100_DONE;
        output->confidence = AF_CONFIDENCE_LOW;
        output->next_pos = cur_pos;
        return FALSE;
    }

    af_roi_type roi_type = pd->set_parameters.roi.type;
#ifdef VENDOR_EDIT
    pd->cur_pos = pd->input.lens_pos = cur_pos;
#else
    pd->cur_pos = cur_pos;
#endif
    BOOL roi_support = FALSE;

    //check if touch/face is enabled from chromatix
    if (pd->haf_roi_enable)
    {
        roi_support = TRUE;
    }
    else
    {
        roi_support = FALSE;
    }

    HAF_MSG_LOW("roi_type %d haf_roi_enable=%d roi_support=%d", roi_type, pd->haf_roi_enable, roi_support);

    /*Run frame skip logic only for ZTL*/
    if ( ENABLE_ZTL
            && p_tuning->pPDAFTuningData->enablePDEarlyInterrupt
            && AF_ROI_TYPE_TOUCH == pd->params.roi_info.type )
    {
        af_pdaf_dynamic_frame_skip(pd);
    }

    if ((roi_type == AF_ROI_TYPE_GENERAL || roi_support))
    {
        // Apply frame skip prior to focus convergence
        if (p_params->frame_skip > 0)
        {
            HAF_MSG_HIGH("frame skip: %d", p_params->frame_skip);
            is_frame_skip = TRUE;
            p_params->frame_skip--;
        }
        else
        {
            // ROI PDAF call flow
            if (AF_ROI_TYPE_TOUCH == pd->params.roi_info.type || AF_ROI_TYPE_FACE == pd->params.roi_info.type)
            {
                if (FALSE == af_pdaf_is_running_2PD(pd))
                {
                    BOOL is_valid_cond_to_run_pd = af_pdaf_is_valid_roi_to_run_pd(pd);
                    if (p_tuning->pPDAFTuningData->touch.skipSearchIfSceneInFocus &&
                            AF_ROI_TYPE_TOUCH == pd->params.roi_info.type &&
                            TRUE == af_pdaf_is_scene_focused(pd) &&
                            0 == p_params->focus_converge_cnt)
                    {
                        HAF_MSG_LOW("Touch sparse: Scene is focused don't run converge");
                        af_pdaf_fine_search_data_t* p_out = &pd->fine_search_data;
                        p_out->start_pos = pd->cur_pos;
                        pd->conv_data.status = AF_PDAF_CONVERGE_SUCCESS;
                        pd->conv_data.next_pos = af_pdaf_get_target_pos(pd, &pd->input, pd->cur_pos);
                        af_pdaf_get_fine_range(pd);
                    }
                    else if (is_valid_cond_to_run_pd)
                    {
                        HAF_MSG_LOW("Sparse ROI PDAF: Run converge");
                        // Process_Search begins:
                        // First run focus converge
                        af_pdaf_focus_converge(pd);
                        // Check for next position
                        af_pdaf_get_next_pos(pd);
                        // Get fine search range
                        af_pdaf_get_fine_range(pd);
                    }
                    else
                    {
#ifdef VENDOR_EDIT
                        if (p_params->converge_done_delay == 0 && p_params->is_caf_stable)
                        {
                            // Sparse Touch\Face PDAF flow
                            HAF_MSG_LOW("Sparse ROI PDAF: Run Search 1.0 PD Optimization");
                            pd->conv_data.status = AF_PDAF_CONVERGE_FALLBACK;
                        }
                        else
                        {
                            pd->conv_data.next_pos = pd->cur_pos;
                            pd->conv_data.status = AF_PDAF_CONVERGE_RUNNING;
                        }
#else
                        // Sparse Touch\Face PDAF flow
                        HAF_MSG_LOW("Sparse ROI PDAF: Run Search 1.0 PD Optimization");
                        pd->conv_data.status = AF_PDAF_CONVERGE_FALLBACK;
#endif
                    }
                }
                else if (TRUE == af_pdaf_is_running_2PD(pd) &&
                         AF_ROI_TYPE_TOUCH == pd->params.roi_info.type)
                {
                    /* Skip touch focus if scene already in focus has to met following criteria:
                       1. Tuning enable
                       2. Scene is already in focus
                       3. Only check on the first frame of search */
                    if (p_tuning->pPDAFTuningData->touch.skipSearchIfSceneInFocus &&
                        TRUE == af_pdaf_is_scene_focused(pd) &&
                        0 == p_params->focus_converge_cnt)
                    {
                        HAF_MSG_LOW("Touch 2PD: Scene is focused don't run converge");
                        af_pdaf_fine_search_data_t* p_out = &pd->fine_search_data;
                        p_out->start_pos = pd->cur_pos;
                        pd->conv_data.status = AF_PDAF_CONVERGE_SUCCESS;
                        pd->conv_data.next_pos = af_pdaf_get_target_pos(pd, &pd->input, pd->cur_pos);
                        af_pdaf_get_fine_range(pd);
                    }
                    else
                    {
                        // Touch 2PD flow
                        HAF_MSG_LOW("Touch 2PD: Running Search 2.0 convergeFV()");
                        af_pdaf_focus_converge_withFV(pd);
                        // Get fine search range
                        af_pdaf_get_fine_range(pd);
                    }
                }
                else if (TRUE == af_pdaf_is_running_2PD(pd) && AF_ROI_TYPE_FACE == pd->params.roi_info.type)
                {
                    // Face 2PD flow
                    if (FALSE == af_pdaf_face_tracking_move_suppression_check(pd))
                    {
                        HAF_MSG_LOW("Face 2PD: Running Search 2.0 convergeFV()");
                        af_pdaf_focus_converge_withFV(pd);
                        // Get fine search range
                        af_pdaf_get_fine_range(pd);
                    }
                }
            }
            else
            {
                /* **************************************************************************
                 *  Process search will always run converge, get_next_pos and fine
                 *  search all together. In the end the output will be populated, it is
                 *  up to HAF framwork to determine which state to enter and use the output
                 *  from process search accordingly.
                 *****************************************************************************/
                if (TRUE == af_pdaf_is_running_2PD(pd))
                {
                    // 2PD runs Search 2.0
                    HAF_MSG_LOW("2PD: Running Search 2.0 convergeFV()");
                    af_pdaf_focus_converge_withFV(pd);
                    // Get fine search range
                    af_pdaf_get_fine_range(pd);
                }
                else
                {
                    // Process_Search begins:
                    // First run focus converge
                    af_pdaf_focus_converge(pd);
                    // Get fine search range
                    af_pdaf_get_fine_range(pd);
                }
            }
        }

#ifdef VENDOR_EDIT
        if (p_params->converge_done_delay > 0)
        {
            p_params->converge_done_delay--;
        }
#endif

        if (TRUE == result)
        {
            // End of process_search, populate process_search output
            af_pdaf_populate_process_search_output(pd, is_frame_skip, output);
        }
    }
    else
    {
        HAF_MSG_LOW("Bypass PDAF go to Contrast AF");
        output->progress = AF_PROGRESS_100_DONE;
        output->confidence = AF_CONFIDENCE_LOW;
        output->next_pos = cur_pos;
    }

    return result;
}

/** af_pdaf_populate_process_search_output:
 * Populate focus search API for PDAF.
 *
 *
 *
 *  Return: VOID
 **/
static BOOL af_pdaf_populate_process_search_output(
    af_alg_PDAF_internal_t* pd,
    BOOL is_frame_skip,
    af_haf_focus_search_data_t* output)
{
    BOOL result = TRUE;
    BOOL is_within_range;

    // NULL pointer check
    if (NULL == pd || NULL == output)
    {
        result = FALSE;
        return result;
    }

    af_pdaf_params_t* p_params = &pd->params;
    chromatixPDAFType* p_tuning = pd->pTuningInfo->pPDAFTuningData;
    af_pdaf_focus_converge_data_t* p_conv = &pd->conv_data;
    output->is_frame_skip = is_frame_skip;
    Q3A_MEMSET(&p_params->fallback_opt_info, 0, sizeof(af_pdaf_fallback_opt_t));

    if (AF_PDAF_CONVERGE_RUNNING == p_conv->status)
    {
        // Case 1. AF_PDAF_CONVERGE_RUNNING:
        //         PROGRESS = INTermediate value
        output->progress = AF_PROGRESS_50_RUNNING;
        output->confidence = pd->monitor_data.is_conf ? AF_CONFIDENCE_HIGH : AF_CONFIDENCE_LOW;
        output->next_pos = p_conv->next_pos;
        output->fine_step_size = pd->fine_search_data.fine_step;
        output->range_near = pd->fine_search_data.range_near;
        output->range_far = pd->fine_search_data.range_far;
        output->search_dir = pd->fine_search_data.search_dir;
        // Check basic sanity of next pos and take decision of skip frame
        // If next pos crosses boundary HAF will terminate the search
        is_within_range = p_conv->next_pos > pd->params.far_limit ?
                          FALSE : (p_conv->next_pos < pd->params.near_limit ? FALSE : TRUE);

        if (is_within_range && (p_conv->next_pos != pd->cur_pos) &&
                p_params->dynamic_frame_skip_running == 0)
        {
            p_params->frame_skip = p_tuning->focusScan.focusConvergeFrameSkip;
            if(pd->pTuningInfo->pPDAFTuningData->enablePDEarlyInterrupt && p_params->camera_run_mode == AF_RUN_MODE_PREVIEW){
                p_params->frame_skip = af_pdaf_dynamic_frame_skip_early_interrupt(pd);
            }
            if (pd->set_parameters.roi.roi[0].dx == pd->params.sensor_info.sensor_res_width &&
                pd->set_parameters.roi.roi[0].dy == pd->params.sensor_info.sensor_res_height)
            {
                p_params->frame_skip = 2;
            }
            HAF_MSG_LOW("camif size: %dx%d, roi: %dx%d",
                pd->params.sensor_info.sensor_res_width, pd->params.sensor_info.sensor_res_height,
                pd->set_parameters.roi.roi[0].dx, pd->set_parameters.roi.roi[0].dy);
        }

        // Assign frame skip after lens move during focus converge
        HAF_MSG_LOW("Skipping %d frames for focus converge CUR_POS = %d, NEXT POS = %d"
                    "Dynamic frame skip running = %d",
                    p_params->frame_skip,
                    pd->cur_pos,
                    p_conv->next_pos,
                    p_params->dynamic_frame_skip_running);
        AF_MSG_HIGH("AF_PDAF_CONVERGE_RUNNING");
    }
    else if (AF_PDAF_CONVERGE_SUCCESS == p_conv->status)
    {
        // Case 2. AF_PDAF_CONVERGE_SUCCESS:
        //         PROGRESS = 100, CONFIDENCE = 100
        output->progress = AF_PROGRESS_100_DONE;
        output->confidence = AF_CONFIDENCE_HIGH;
        output->next_pos = p_conv->next_pos;
        output->fine_step_size = pd->fine_search_data.fine_step;
        output->range_near = pd->fine_search_data.range_near;
        output->range_far = pd->fine_search_data.range_far;
        output->search_dir = pd->fine_search_data.search_dir;
        AF_MSG_HIGH("AF_PDAF_CONVERGE_SUCCESS");
    }
    else if (AF_PDAF_CONVERGE_FALLBACK == p_conv->status)
    {
        // Case 3. AF_PDAF_CONVERGE_FALLBACK
        //         PROGRESS = 100, CONFIDENCE = 0, range_near = near end, range_far = far_end
        // Call fallback optimization
        af_pdaf_calculate_fallback_opt(pd);
        output->progress = AF_PROGRESS_100_DONE;
        output->confidence = AF_CONFIDENCE_LOW;
        output->next_pos = p_params->fallback_opt_info.start_pos;
        output->fine_step_size = af_pdaf_get_fine_step(pd);
        output->range_near = p_params->fallback_opt_info.search_range_near_pos;
        output->range_far = p_params->fallback_opt_info.search_range_far_pos;
        output->search_dir = p_params->fallback_opt_info.search_dir;
        AF_MSG_HIGH("AF_PDAF_CONVERGE_FALLBACK");
    }

    af_pdaf_get_target_pos(pd, &pd->input, pd->cur_pos);
    output->target_pos = pd->params.estimated_target_pos;

    // perform simple boundary check
    if (output->target_pos < p_params->near_limit)
    {
        output->target_pos = p_params->near_limit;
    }
    else if (output->target_pos > p_params->far_limit)
    {
        output->target_pos = p_params->far_limit;
    }

    AF_MSG_LOW("range_near=%d range_far=%d  search_dir=%d  fine_step_size=%d type:%d need_fine_search:%d status:%d",
               output->range_near,
               output->range_far,
               output->search_dir,
               output->fine_step_size,
               pd->params.roi_info.type,
               pd->fine_search_data.need_fine_search, p_conv->status);

    // If focus done or fine search is not needed,
    // then we set range_far and range_near equal to next_pos
    if (AF_PDAF_CONVERGE_FALLBACK != p_conv->status &&
            FALSE == pd->fine_search_data.need_fine_search && pd->params.roi_info.type != AF_ROI_TYPE_FACE)
    {
        AF_MSG_HIGH("Fine Search is not needed");
        output->range_near = output->next_pos;
        output->range_far = output->next_pos;
#ifdef VENDOR_EDIT
        pd->lcrUsed = TRUE;
#endif
    }
    else
    {
        // fine search is needed, populate the range according to internal fine search data structure
        AF_MSG_HIGH("Fine Search is needed");
#ifdef VENDOR_EDIT
        pd->lcrUsed = FALSE;
#endif
    }

    // Face PDAF ROI refocus trigger change: Save current face ROI as reference face ROI for face change monitor */
    if (output->progress == AF_PROGRESS_100_DONE)
    {
        Q3A_MEMCPY_S(&pd->params.ref_face_roi_info, sizeof(pd->params.ref_face_roi_info),
                     &pd->params.roi_info, sizeof(pd->params.roi_info));
        HAF_MSG_LOW("Face Sparse PD: Save Cur face ROI as Ref ROI. x,y,dx,dy = [%d, %d, %d, %d], ref = [%d, %d, %d ,%d]",
                    pd->params.roi_info.roi[p_params->focus_roi_id].x,
                    pd->params.roi_info.roi[p_params->focus_roi_id].y,
                    pd->params.roi_info.roi[p_params->focus_roi_id].dx,
                    pd->params.roi_info.roi[p_params->focus_roi_id].dy,
                    pd->params.ref_face_roi_info.roi[p_params->focus_roi_id].x,
                    pd->params.ref_face_roi_info.roi[p_params->focus_roi_id].y,
                    pd->params.ref_face_roi_info.roi[p_params->focus_roi_id].dx,
                    pd->params.ref_face_roi_info.roi[p_params->focus_roi_id].dy);
    }

    AF_MSG_HIGH("range_near %d range_far %d fine_step_size %d",
                output->range_near, output->range_far, output->fine_step_size);
    return result;
}

/** af_pdaf_focus_converge_withFV:
 * Handle Search 2.0 focus convergence for PDAF.
 * In converge_withFV, the lens is moving based on DOF to achieve smoother target reach.
 * In addition, LSP is able to predict the peak while collecting FV during PDAF convergence.
 *
 * @pd: internal phase detection(PD) data structure
 *
 *  Return: VOID
 **/
static VOID af_pdaf_focus_converge_withFV(
    af_alg_PDAF_internal_t* pd)
{
    af_pdaf_params_t* p_params = &pd->params;
    chromatixPDAFType* pPDAFTuningData = pd->pTuningInfo->pPDAFTuningData;
    chromatixHAFType* pHAFTuningData = pd->pTuningInfo->pHAFTuningData;
    af_pdaf_focus_converge_data_t* p_conv = &pd->conv_data;
    af_pdaf_focus_converge_data_t focus_data = {};
    af_pdaf_input_t* input = &pd->input;
    BOOL is_conf = FALSE;
    //status variable takes three (-1,0,1) possible values
    INT focus_converge_done_status = 0;
    INT lens_pos = pd->cur_pos;
    BOOL is_caf_stable = p_params->is_caf_stable;
    INT cur_pos = pd->cur_pos;
    INT target_pos = 0;
    INT min_fine_step = 0, DOF_fine_step = 0;
    FLOAT dof_multiplier = 0.0f;
    UINT16 dof_multiplier_4, dof_multiplier_2, dof_multiplier_1;
    focus_data.status = AF_PDAF_CONVERGE_RUNNING;

    // update roi when entering Focus converge
    if (0 == p_params->focus_converge_cnt)
    {
        p_params->focus_roi_id = af_pdaf_get_focus_roi_id(pd);
    }

    /* spotlight detected/Face detected will make focus converge inconfident
     and fallback to CAF */
    //is_conf = ((p_params->roi_info.type != AF_ROI_TYPE_FACE)
    //  )? af_pdaf_is_depth_conf(pd) : FALSE; //make it a tuning parameter
    //whether to use spotlight with PDAF or not. for 2PD we may not
    //require spotlight detection
    if (p_params->roi_info.type == AF_ROI_TYPE_GENERAL)
    {
        is_conf = (BOOL) af_pdaf_is_depth_conf(pd);
    }
    else
    {
        if (p_params->roi_info.type == AF_ROI_TYPE_FACE || p_params->roi_info.type == AF_ROI_TYPE_TOUCH)
        {
            if (pHAFTuningData->HAFROISettings.HAFROIEnable)
            {
                is_conf = (BOOL) af_pdaf_is_depth_conf(pd);
            }
            else
            {
                is_conf = FALSE;
            }
        }
    }

    // increase wait_conf_recover_cnt for first run of focus converge
    if (TRUE == p_params->is_first_run)
    {
        /* Focus roi computation is forced during first time run;
         Otherwise roi= 0 will always be picked */
        p_params->wait_conf_recover_cnt = pPDAFTuningData->sceneMonitor.waitConfidenceRecoverCount;
        p_params->wait_conf_recover_cnt += p_params->first_run_not_conf_cnt_add;
        HAF_MSG_LOW("Adjust wait_conf_recover_cnt "
                    "for focus convergence in first run = %d", p_params->wait_conf_recover_cnt);
        /* During first time launch, scene change logic will always detect
         caf not stable since the internal counters takes few frame to reach
         the threshold. This is not required for focus converge as it uses
         defocus value instead of FV. Settng is_caf_changes to FALSE;
         Similar logic from TOF */
    }

    /* If the selected Roi is not confident but overall pdaf is confident then
       Wait for selected ROI to recover confidence . If it doesnt recover within
       wait_conf_recover_cnt then repick new roi and then refocus */
    if (p_params->roi[p_params->focus_roi_id].is_conf == FALSE &&
            TRUE == af_pdaf_is_depth_conf(pd))
    {
        // Wait for the selected ROI to gain confidence
        if ((p_params->roi[p_params->focus_roi_id].not_conf_cnt <
                p_params->wait_conf_recover_cnt))
        {
            p_conv->status = focus_data.status;
            p_conv->next_pos = cur_pos;
            return;
        }
        // Previous selected ROI did not gain confidence in wait_conf_recover_cnt
        // number of frames. So, pick the new conf and stable ROI as PDAF reports
        // depth as confident
        INT newROIID = af_pdaf_get_focus_roi_id(pd);
        if (newROIID != p_params->focus_roi_id)
        {
            HAF_MSG_INFO("new stable and confident ROI found %d", newROIID);
            p_params->focus_roi_id = newROIID;
        }
        else
        {
            // New ROI is same as old ROI, which means the other ROIs
            // are not stable and confident.So, no need to wait further
            // fallback to contrast AF
            HAF_MSG_INFO("new stable and confident ROI NOT found, FALLBACK");
            is_conf = FALSE;
        }
    }

    // rebase for enter face pdaf, fill target pos array with cur_pos
    if (TRUE == af_pdaf_is_running_2PD_face(pd))
    {
        if (pd->conv_data.prev_roi_type != p_params->roi_info.type || p_params->focus_converge_cnt == 0)
        {
            af_pdaf_face_pd_history_rebase(pd);
        }

        af_pdaf_face_pd_history_update(pd);
    }

    p_params->focus_converge_cnt++;

    if (TRUE == is_conf)
    {
        /* Complete focus converge if defocus is within threshold and panning
         is done. This logic should be same as TOFAF */
        if ( ENABLE_ZTL && pPDAFTuningData->enablePDEarlyInterrupt )
        {
            /* For early interrupt use the target position by tuning
               the focus table*/
            target_pos = af_pdaf_get_target_pos(pd, input, lens_pos);
        }
        else
        {
            af_pdaf_get_target_pos(pd, input, lens_pos);
            target_pos = pd->params.estimated_target_pos;
        }

        // perform simple boundary check
        if (target_pos < p_params->near_limit)
        {
            target_pos = p_params->near_limit;
        }
        else if (target_pos > p_params->far_limit)
        {
            target_pos = p_params->far_limit;
        }

        // Typical PD & 2PD touch flow
        // get DOF step size
        min_fine_step = pd->pTuningInfo->pPDAFTuningData->focusScan.fineScanStepSize;
        dof_multiplier = af_pdaf_get_dof_multiplier(pd, target_pos);
        DOF_fine_step = (INT)((FLOAT) min_fine_step * dof_multiplier);

        if (DOF_fine_step < min_fine_step)
        {
            DOF_fine_step = min_fine_step;
        }

        /* Face 2PD: When confidence drops, we INTerpolate the move step to aVOID
         big sudden jump in on the preview. The scenario occurs when face is moving
         which causes motion blur and low confidence PD. */
        if (af_pdaf_is_running_2PD_face(pd) && TRUE == pPDAFTuningData->face.defocusFilter.enable)
        {
            focus_data.next_pos = af_pdaf_get_face_temporal_next_position(pd);
            p_params->pdaf_convState = PDAF_Conv_PD_Target;
            HAF_MSG_LOW("Face 2PD: current pos %d, temporal next pos %d", cur_pos, focus_data.next_pos);
        }

        HAF_MSG_LOW("Curent_state: %d target_pos %d DOF_fine_step %d--(min_step %d,dof_multiplier %f)",
                    p_params->pdaf_convState,
                    target_pos,
                    DOF_fine_step,
                    min_fine_step,
                    dof_multiplier);

        if ( ENABLE_ZTL && pPDAFTuningData->enablePDEarlyInterrupt )
        {
            /*For Early interrupt we dont need to take small DOF steps
               thats why make dof_multiplier_2 & dof_multiplier_1 = 0
               Only for big jump i.e dof_multiplier_4 make it 2
               Ideally this should have been a tuning param*/
            dof_multiplier_4 = 0;
            dof_multiplier_2 = 0;
            dof_multiplier_1 = 0;
        }
        else
        {
            /*This was earlier DOF numbers with SEARCH 2.0*/
            dof_multiplier_4 = 4;
            dof_multiplier_2 = 2;
            dof_multiplier_1 = 1;
        }

        switch (p_params->pdaf_convState)
        {
            case PDAF_Conv_Init:

                // 0x00; init.
                // get next lens position
                if (abs(target_pos - cur_pos) > dof_multiplier_4 * DOF_fine_step)
                {
                    /* Condition to enter half-target to aVOID overshoot:
                     1. If running touch PDAF, the ROI is at corner.
                     2. target position is more than 16 DOF, so half target is more than 8 DOF.
                     */
                    BOOL touch_roi_beyond_center = FALSE;

                    if (AF_ROI_TYPE_TOUCH == pd->params.roi_info.type
                            && af_pdaf_get_roi_to_center_distance(pd) > pPDAFTuningData->touch.offCenterThreshold)
                    {
                        touch_roi_beyond_center = TRUE;
                        HAF_MSG_LOW("Touch ROI off center = %d", touch_roi_beyond_center);
                    }

                    if (touch_roi_beyond_center == TRUE && (abs(target_pos - cur_pos) * 0.5 > 8 * DOF_fine_step))
                    {
                        // Corner ROI and target pos is greater than 16 DOF, enter half-target
                        focus_data.next_pos = target_pos > cur_pos ?
                                              target_pos - dof_multiplier_4 * DOF_fine_step :
                                              target_pos + dof_multiplier_4 * DOF_fine_step;
                        p_params->pdaf_convState = PDAF_Conv_Half_Target;
                    }
                    else
                    {
                        // Center ROI or target pos is less than 16 DOF
                        focus_data.next_pos = target_pos > cur_pos ?
                                              target_pos - dof_multiplier_4 * DOF_fine_step :
                                              target_pos + dof_multiplier_4 * DOF_fine_step;
                        p_params->pdaf_convState = PDAF_Conv_4DOF;
                    }
                }
                else if (abs(target_pos - cur_pos) > dof_multiplier_2 * DOF_fine_step)
                {
                    focus_data.next_pos = target_pos > cur_pos ?
                                          target_pos - dof_multiplier_2 * DOF_fine_step :
                                          target_pos + dof_multiplier_2 * DOF_fine_step;
                    p_params->pdaf_convState = PDAF_Conv_2DOF;
                }
                else if (abs(target_pos - cur_pos) > dof_multiplier_1 * DOF_fine_step)
                {
                    focus_data.next_pos = target_pos > cur_pos ?
                                          target_pos - dof_multiplier_1 * DOF_fine_step :
                                          target_pos + dof_multiplier_1 * DOF_fine_step;
                    p_params->pdaf_convState = PDAF_Conv_DOF;
                }
                else
                {
                    focus_data.next_pos = target_pos;
                    p_params->pdaf_convState = PDAF_Conv_PD_Target;
                }

                if ( ENABLE_ZTL
                        && pPDAFTuningData->enablePDEarlyInterrupt
                        && p_params->roi_info.type == AF_ROI_TYPE_TOUCH )
                {
                    p_params->pdaf_convState = PDAF_Conv_DOF;
                }

                break;

            case PDAF_Conv_Half_Target:

                // go to half target range
                if (abs(target_pos - cur_pos) > dof_multiplier_4 * DOF_fine_step)
                {
                    focus_data.next_pos = target_pos > cur_pos ?
                                          target_pos - dof_multiplier_4 * DOF_fine_step :
                                          target_pos + dof_multiplier_4 * DOF_fine_step;
                    p_params->pdaf_convState = PDAF_Conv_4DOF;
                }
                else if (abs(target_pos - cur_pos) > dof_multiplier_2 * DOF_fine_step)
                {
                    focus_data.next_pos = target_pos > cur_pos ?
                                          target_pos - dof_multiplier_2 * DOF_fine_step :
                                          target_pos + dof_multiplier_2 * DOF_fine_step;
                    p_params->pdaf_convState = PDAF_Conv_2DOF;
                }
                else if (abs(target_pos - cur_pos) > dof_multiplier_1 * DOF_fine_step)
                {
                    focus_data.next_pos = target_pos > cur_pos ?
                                          target_pos - dof_multiplier_1 * DOF_fine_step :
                                          target_pos + dof_multiplier_1 * DOF_fine_step;
                    p_params->pdaf_convState = PDAF_Conv_DOF;
                }
                else
                {
                    focus_data.next_pos = target_pos;
                    p_params->pdaf_convState = PDAF_Conv_PD_Target;
                }

                break;

            case PDAF_Conv_4DOF:

                // within 4DOF range
                if (abs(target_pos - cur_pos) > dof_multiplier_2 * DOF_fine_step)
                {
                    focus_data.next_pos = target_pos > cur_pos ?
                                          target_pos - dof_multiplier_2 * DOF_fine_step :
                                          target_pos + dof_multiplier_2 * DOF_fine_step;
                    p_params->pdaf_convState = PDAF_Conv_2DOF;
                }
                else if (abs(target_pos - cur_pos) > dof_multiplier_1 * DOF_fine_step)
                {
                    focus_data.next_pos = target_pos > cur_pos ?
                                          target_pos - dof_multiplier_1 * DOF_fine_step :
                                          target_pos + dof_multiplier_1 * DOF_fine_step;
                    p_params->pdaf_convState = PDAF_Conv_DOF;
                }
                else
                {
                    focus_data.next_pos = target_pos;
                    p_params->pdaf_convState = PDAF_Conv_PD_Target;
                }

                break;

            case PDAF_Conv_2DOF:

                // within 2DOF range
                if (abs(target_pos - cur_pos) > dof_multiplier_1 * DOF_fine_step)
                {
                    focus_data.next_pos = target_pos > cur_pos ?
                                          target_pos - dof_multiplier_1 * DOF_fine_step :
                                          target_pos + dof_multiplier_1 * DOF_fine_step;
                    p_params->pdaf_convState = PDAF_Conv_DOF;
                }
                else
                {
                    focus_data.next_pos = target_pos;
                    p_params->pdaf_convState = PDAF_Conv_PD_Target;
                }

                break;

            case PDAF_Conv_DOF:
                //within DOF range
                focus_data.next_pos = target_pos;
                p_params->pdaf_convState = PDAF_Conv_PD_Target;
                break;

            case PDAF_Conv_PD_Target:

                //Close loop PD converge
                if (af_pdaf_is_running_2PD_face(pd))
                {
                    focus_data.next_pos = lens_pos
                                          + (INT)((FLOAT)(focus_data.next_pos - lens_pos)
                                                  * af_pdaf_get_focus_pcnt(pd, input, focus_data.next_pos - lens_pos));
                }
                else
                {
                    focus_data.next_pos = target_pos;
                    focus_converge_done_status = af_pdaf_is_focus_converge_done(pd, input, focus_data.next_pos);
                }

                p_params->pdaf_convState = PDAF_Conv_PD_Target;
                break;
        }

        HAF_MSG_HIGH("gbPdaf_conv_next_State %d next_pos %d done status %d",
                     p_params->pdaf_convState,
                     focus_data.next_pos,
                     focus_converge_done_status);
    }

    /* First check if panning stable is reported before futher action. This
     is to aVOID fine search or fallback while camera is panning. */

    if (is_caf_stable && FALSE == af_pdaf_is_running_2PD_face(pd))
    {
        // reset wait caf stable cnt
        p_params->wait_caf_stable_cnt = 0;

        if (1 == focus_converge_done_status)
        {
            af_pdaf_reset_data(pd);
            HAF_MSG_HIGH("Converge done. Go to fine scan");
            focus_data.status = AF_PDAF_CONVERGE_SUCCESS;
            p_params->pdaf_convState = PDAF_Conv_Init;
        }
        else if (-1 == focus_converge_done_status)
        {
            HAF_MSG_HIGH("Convergence failed. Fall Back");
            af_pdaf_reset_data(pd);
            focus_data.status = AF_PDAF_CONVERGE_FALLBACK;
            p_params->pdaf_convState = PDAF_Conv_Init;
        }
        else if (TRUE == pPDAFTuningData->ledPdafMaxFrameAllow.enable &&
                 p_params->focus_converge_cnt > (INT)pPDAFTuningData->ledPdafMaxFrameAllow.maxFrameCnt &&
                 TRUE == p_params->af_led_assist)
        {
            // During LED mode, if focus converges longer than LED_CONVERGE_MAX_FRAME_ALLOW,
            // PDAF will directly report success to avoid power drain and APP timeout
            HAF_MSG_HIGH("LED converge more than max frames allow! Set converge done. Go to fine scan");
            af_pdaf_reset_data(pd);
            focus_data.status = AF_PDAF_CONVERGE_SUCCESS;
            p_params->pdaf_convState = PDAF_Conv_Init;
        }
        else
        {
            if (p_params->is_spotlight_detected_prev != p_params->is_spotlight_detected)
            {
                /* If spotlight decision switches during focus converge, stop PD and run CAF instead */
                af_pdaf_reset_data(pd);
                HAF_MSG_HIGH("Spotlight decision changed! Stop Focus Converge and Go to fine scan");
                focus_data.status = AF_PDAF_CONVERGE_SUCCESS;
                p_params->pdaf_convState = PDAF_Conv_Init;
            }
        }
    }
    else if (TRUE == af_pdaf_is_running_2PD_face(pd))
    {
        HAF_MSG_LOW("Face PDAF running. Stay in PD focus converge for face tracking.");
        HAF_MSG_LOW("Face PDAF LED = %d", p_params->af_led_assist);

        if (1 == p_params->af_led_assist)
        {
            /* Special case: LED face tracking PDAF
             in this case, we need to converge_done so app can take a picture */
            focus_converge_done_status = af_pdaf_is_focus_converge_done(pd, input, focus_data.next_pos);

            if (1 == focus_converge_done_status)
            {
                af_pdaf_reset_data(pd);
                HAF_MSG_HIGH("LED Face PDAF Converge done. Go to fine scan");
                focus_data.status = AF_PDAF_CONVERGE_SUCCESS;
                p_params->pdaf_convState = PDAF_Conv_Init;
            }
            else if (-1 == focus_converge_done_status)
            {
                HAF_MSG_HIGH("LED Face PDAF Convergence failed. Fall Back");
                af_pdaf_reset_data(pd);
                focus_data.status = AF_PDAF_CONVERGE_FALLBACK;
                p_params->pdaf_convState = PDAF_Conv_Init;
            }
        }
        else
        {
            // Normal face 2PD flow
            /* Customer wants below behavior for face tracking PD only:
             if movement is less or equal to min threshold, simply don't move */
            if (abs((INT)(focus_data.next_pos - cur_pos)) > 0
                    && abs((INT)(focus_data.next_pos - cur_pos)) <= p_params->face_2pd_min_move_thresh)
            {
                HAF_MSG_LOW("Face 2PD No move, move_step = %d < threshold %d",
                            abs(target_pos - cur_pos),
                            p_params->face_2pd_min_move_thresh);
                focus_data.next_pos = cur_pos;
            }

            HAF_MSG_LOW("(dx, dy) = (%d, %d) is_conf %d, is_caf_stable %d ",
                        p_params->roi_info.roi[p_params->focus_roi_id].dx,
                        p_params->roi_info.roi[p_params->focus_roi_id].dy,
                        is_conf,
                        is_caf_stable);

            if (TRUE == pd->pTuningInfo->pPDAFTuningData->face.fallback.enable
                    && (p_params->roi_info.roi[p_params->focus_roi_id].dx
                        < pd->pTuningInfo->pPDAFTuningData->face.fallback.fallbackROISizeThreshold
                        || p_params->roi_info.roi[p_params->focus_roi_id].dy
                        < pd->pTuningInfo->pPDAFTuningData->face.fallback.fallbackROISizeThreshold
                        ||
                        FALSE == is_conf) &&
                    TRUE == is_caf_stable)
            {
                // Add face 2PD fallback case here
                HAF_MSG_LOW("Face fallback case due to small ROI");
                af_pdaf_reset_data(pd);
                focus_data.status = AF_PDAF_CONVERGE_FALLBACK;
                p_params->pdaf_convState = PDAF_Conv_Init;
            }
        }
    }
    else
    {
        HAF_MSG_LOW("Scene is not stable yet.");
        if (TRUE == pPDAFTuningData->ledPdafMaxFrameAllow.enable &&
            p_params->focus_converge_cnt > (INT)pPDAFTuningData->ledPdafMaxFrameAllow.maxFrameCnt &&
            TRUE == p_params->af_led_assist)
        {
            // During LED mode, if focus converges longer than LED_CONVERGE_MAX_FRAME_ALLOW,
            // PDAF will directly report success to avoid power drain and APP timeout
            HAF_MSG_HIGH("LED converge more than max frames allow! Set converge done. Go to fine scan");
            af_pdaf_reset_data(pd);
            focus_data.status = AF_PDAF_CONVERGE_SUCCESS;
            p_params->pdaf_convState = PDAF_Conv_Init;
        }

        // CAMX_AF_TODO: not found in tuning header, set 0 now
        if (0 /* TRUE == pd->tuning->pd_extended.max_caf_stable_wait_cnt_enable */)
        {
            if (1 == focus_converge_done_status || -1 == focus_converge_done_status)
            {
                p_params->wait_caf_stable_cnt++;
                HAF_MSG_HIGH("check for caf stable max wait cnt, p_params->wait_caf_stable_cnt %d , max_caf_stable_wait_cnt %d ",
                             p_params->wait_caf_stable_cnt, 0/*pd->tuning->pd_extended.max_caf_stable_wait_cnt*/);

                if (0 /*p_params->wait_caf_stable_cnt >= pd->tuning->pd_extended.max_caf_stable_wait_cnt*/)
                {
                    af_pdaf_reset_data(pd);
                    focus_data.status = (1 == focus_converge_done_status) ? AF_PDAF_CONVERGE_SUCCESS : AF_PDAF_CONVERGE_FALLBACK;
                    p_params->pdaf_convState = PDAF_Conv_Init;
                    HAF_MSG_HIGH(" reach caf stable max wait cnt, focus_data.status %d", focus_data.status);
                }
            }
            else
            {
                p_params->wait_caf_stable_cnt = 0;
            }
        }
    }

    if (AF_ROI_TYPE_TOUCH == p_params->roi_info.type
        && pPDAFTuningData->ledPdafMaxFrameAllow.enable)
    {
        if (p_params->focus_converge_cnt > (INT)pPDAFTuningData->ledPdafMaxFrameAllow.maxFrameCnt &&
            focus_data.status != AF_PDAF_CONVERGE_SUCCESS &&
            focus_data.status != AF_PDAF_CONVERGE_FALLBACK)
        {
            HAF_MSG_HIGH("Touch Focus converge cnt more than max frames allow! Set converge done. Go to fine scan");
            af_pdaf_reset_data(pd);
            focus_data.status = AF_PDAF_CONVERGE_SUCCESS;
            p_params->pdaf_convState = PDAF_Conv_Init;
        }
    }

    // perform simple boundary check
    if (focus_data.next_pos < p_params->near_limit)
    {
        focus_data.next_pos = p_params->near_limit;
    }
    else if (focus_data.next_pos > p_params->far_limit)
    {
        focus_data.next_pos = p_params->far_limit;
    }

    // if estimation is not confident while camera is stable then
    // fall back to contrast AF. else, remain in focus converge
    // If spotlight reports not confidence, also fallback
    if (FALSE == is_conf)
    {
        HAF_MSG_HIGH("Not confident spot_light_flag %d caf_stable %d "
                     "Face detection: %s",
                     p_params->is_spotlight_detected,
                     is_caf_stable,
                     p_params->roi_info.type == AF_ROI_TYPE_FACE ? "TRUE" : "FALSE");

        if (is_caf_stable && p_params->roi_info.type != AF_ROI_TYPE_FACE)
        {
            HAF_MSG_HIGH("max not_conf_cnt reached during "
                         "focus converge. fallback to contrast AF");
            af_pdaf_reset_data(pd);
            focus_data.next_pos = cur_pos;
            focus_data.status = AF_PDAF_CONVERGE_FALLBACK;
        }
        else
        {
            focus_data.next_pos = cur_pos;
        }
    }

    // Put result in algo output
    p_conv->status = focus_data.status;
    p_conv->next_pos = focus_data.next_pos;
    pd->conv_data.prev_roi_type = p_params->roi_info.type;
    return;
}

/** af_pdaf_focus_converge:
 * Handle focus convergence for PDAF.
 *
 * @pd: internal phase detection(PD) data structure
 *
 *  Return: VOID
 **/
static VOID af_pdaf_focus_converge(
    af_alg_PDAF_internal_t* pd)
{
    af_pdaf_params_t* p_params = &pd->params;
    chromatixPDAFType* pPDAFTuningData = pd->pTuningInfo->pPDAFTuningData;
    af_pdaf_focus_converge_data_t* p_conv = &pd->conv_data;
    af_pdaf_focus_converge_data_t focus_data;
    af_pdaf_input_t* input = &pd->input;
    boolean is_conf = FALSE;
    //status variable takes three (-1,0,1) possible values
    int focus_converge_done_status = 0;
    int lens_pos = input->lens_pos;
    boolean is_caf_stable = p_params->is_caf_stable;
    int cur_pos = pd->cur_pos;
    focus_data.status = AF_PDAF_CONVERGE_RUNNING;

    /* Apply frame skip prior to focus convergence */
    if (p_params->frame_skip > 0)
    {
        HAF_MSG_LOW("frame skip: %d", p_params->frame_skip);
        p_conv->status = focus_data.status;
        p_conv->next_pos = cur_pos;
        return;
    }

    /* update roi when entering Focus converge */
    if (0 == p_params->focus_converge_cnt)
    {
        p_params->focus_roi_id = af_pdaf_get_focus_roi_id(pd);
    }

    /* if Sparse face change PDAF, upon start of search the need_for_refocus_flag
       has to be true to avoid early trigger face PDAF. When the flag is
       true, it means face is stable and ready to focus */
    if (AF_ROI_TYPE_FACE == p_params->roi_info.type &&
        0 == p_params->focus_converge_cnt &&
        pPDAFTuningData->face.faceChangeRefocusTrigger.enable)
    {
        boolean need_face_refocus = af_pdaf_face_change_refocus_trigger(pd);

        if (need_face_refocus == FALSE)
        {
            HAF_MSG_LOW("Sparse Face PDAF block searching, need_refocus = %d, face not stable yet.", need_face_refocus);
            p_conv->status = focus_data.status;
            p_conv->next_pos = cur_pos;
            return;
        }
    }

    /* spotlight detected/Face detected will make focus converge inconfident
       and fallback to CAF */
    if (AF_ROI_TYPE_GENERAL == p_params->roi_info.type)
    {
        is_conf = af_pdaf_is_depth_conf(pd);
    }
    else
    {
        if (AF_ROI_TYPE_FACE == p_params->roi_info.type ||
                AF_ROI_TYPE_TOUCH == p_params->roi_info.type)
        {
            if (pd->haf_roi_enable)
            {
                is_conf = af_pdaf_is_depth_conf(pd);
            }
            else
            {
                is_conf = FALSE;
            }
        }
    }

    /* increase wait_conf_recover_cnt for first run of focus converge */
    if (TRUE == p_params->is_first_run)
    {
        /* Focus roi computation is forced during first time run;
           Otherwise roi= 0 will always be picked */
        p_params->wait_conf_recover_cnt =
            pPDAFTuningData->sceneMonitor.waitConfidenceRecoverCount;
        p_params->wait_conf_recover_cnt +=
            p_params->first_run_not_conf_cnt_add;
        HAF_MSG_LOW("Adjust wait_conf_recover_cnt "
                    "for focus convergence in first run = %d",
                    p_params->wait_conf_recover_cnt);
        /* During first time launch, scene change logic will always detect
           caf not stable since the internal counters takes few frame to reach
           the threshold. This is not required for focus converge as it uses
           defocus value instead of FV. Settng is_caf_changes to FALSE;
           Similar logic from TOF */
    }

    /* If the selected Roi is not confident but overall pdaf is confident then
       Wait for selected ROI to recover confidence . If it doesnt recover within
       wait_conf_recover_cnt then repick new roi and then refocus */
    if (p_params->roi[p_params->focus_roi_id].is_conf == FALSE &&
            TRUE == af_pdaf_is_depth_conf(pd))
    {
        // Wait for the selected ROI to gain confidence
        if ((p_params->roi[p_params->focus_roi_id].not_conf_cnt <
                p_params->wait_conf_recover_cnt))
        {
            p_conv->status = focus_data.status;
            p_conv->next_pos = cur_pos;
            return;
        }
        // Previous selected ROI did not gain confidence in wait_conf_recover_cnt
        // number of frames. So, pick the new conf and stable ROI as PDAF reports
        // depth as confident
        INT newROIID = af_pdaf_get_focus_roi_id(pd);
        if (newROIID != p_params->focus_roi_id)
        {
            HAF_MSG_INFO("new stable and confident ROI found %d", newROIID);
            p_params->focus_roi_id = newROIID;
        }
        else
        {
            // New ROI is same as old ROI, which means the other ROIs
            // are not stable and confident.So, no need to wait further
            // fallback to contrast AF
            HAF_MSG_INFO("new stable and confident ROI NOT found, FALLBACK");
            is_conf = FALSE;
        }
    }

    p_params->focus_converge_cnt++;

#ifdef VENDOR_EDIT
    if ((p_params->roi[p_params->focus_roi_id].rev_cnt >=VIDEO_MOVE_PCT_DROP_REV_CNT_TH) &&
        (p_params->camera_run_mode == AF_RUN_MODE_VIDEO))
    {
        p_params->rev_cnt_pct_drop = VIDEO_MOVE_PCT_DROP_RATIO;
    }

    HAF_MSG_HIGH("rev_cnt_pct_drop %f" , p_params->rev_cnt_pct_drop);

    p_params->pre_rev_cnt_fallback = 0;
#endif

    /* get next lens position */
    focus_data.next_pos = af_pdaf_get_target_pos(pd, input, lens_pos);

    if (TRUE == is_conf)
    {
        /* Complete focus converge if defocus is within threshold and panning
           is done. This logic should be same as TOFAF */
        focus_converge_done_status =
            af_pdaf_is_focus_converge_done(pd, input, focus_data.next_pos);
    }

    /* First check if panning stable is reported before futher action. This
       is to avoid fine search or fallback while camera is panning. */
#ifdef VENDOR_EDIT
    if (is_caf_stable && p_params->converge_done_delay == 0)
#else
    if (is_caf_stable)
#endif
    {
        if (1 == focus_converge_done_status)
        {
            af_pdaf_reset_data(pd);
            HAF_MSG_HIGH("Converge done. Go to fine scan");
            focus_data.status = AF_PDAF_CONVERGE_SUCCESS;
        }
        else if (-1 == focus_converge_done_status)
        {

#ifdef VENDOR_EDIT
            if (!af_pdaf_is_low_light(pd)&&
                AF_ROI_TYPE_GENERAL == pd->params.roi_info.type &&
                ((p_params->roi[p_params->focus_roi_id].rev_cnt >= FALLBACK_REV_CNT_TH) ||
                (p_params->roi[p_params->focus_roi_id].converge_rev_cnt>= FALLBACK_CONVERGE_REV_CNT_TH)) )
            {
                p_params->pre_rev_cnt_fallback = 1;
            }
#endif
            HAF_MSG_HIGH("Convergence failed. Fall Back");
            af_pdaf_reset_data(pd);
            focus_data.status = AF_PDAF_CONVERGE_FALLBACK;
        }
    }
    else
    {
#ifdef VENDOR_EDIT
        if (-1 == focus_converge_done_status)
        {
            if (!af_pdaf_is_low_light(pd) &&
                AF_ROI_TYPE_GENERAL == pd->params.roi_info.type &&
                ((p_params->roi[p_params->focus_roi_id].rev_cnt >= FALLBACK_REV_CNT_TH) ||
                (p_params->roi[p_params->focus_roi_id].converge_rev_cnt>= FALLBACK_CONVERGE_REV_CNT_TH)))
            {
                p_params->pre_rev_cnt_fallback = 1;
            }

            HAF_MSG_HIGH("Convergence failed. Fall Back with converge_rev_cnt %d without caf stable ", p_params->roi[p_params->focus_roi_id].converge_rev_cnt);
            af_pdaf_reset_data(pd);
            focus_data.status = AF_PDAF_CONVERGE_FALLBACK;
        }
        else {
            HAF_MSG_LOW("Scene is panning. No action until stable.");
        }
#else
        HAF_MSG_LOW("Scene is panning. No action until stable.");
#endif
    }

    /* perform simple boundary check */
    if (focus_data.next_pos < p_params->near_limit)
    {
        focus_data.next_pos = p_params->near_limit;
    }
    else if (focus_data.next_pos > p_params->far_limit)
    {
        focus_data.next_pos = p_params->far_limit;
    }

    /* If estimation is not confident while camera is stable then
       fall back to contrast AF. else, remain in focus converge
       If spotlight reports not confidence, also fallback */
    if (FALSE == is_conf)
    {
        HAF_MSG_HIGH("Not confident spot_light_flag %d caf_stable %d"
                     "Face detection: %s",
                     p_params->is_spotlight_detected, is_caf_stable,
                     p_params->roi_info.type == AF_ROI_TYPE_FACE ? "TRUE" : "FALSE");

#ifdef VENDOR_EDIT
        if (is_caf_stable && p_params->converge_done_delay == 0)
#else
        if (is_caf_stable)
#endif
        {
            HAF_MSG_HIGH("max not_conf_cnt reached during "
                         "focus converge. fallback to contrast AF");
            af_pdaf_reset_data(pd);
            focus_data.next_pos = cur_pos;
            focus_data.status = AF_PDAF_CONVERGE_FALLBACK;
        }
        else
        {
            focus_data.next_pos = cur_pos;
        }
    }

#ifdef VENDOR_EDIT
    HAF_MSG_HIGH("is_caf_stable %d converge_done_delay %d pre_rev_cnt_fallback %d", is_caf_stable, p_params->converge_done_delay , p_params->pre_rev_cnt_fallback);
#endif

    /* Put result in algo output*/
    p_conv->status = focus_data.status;
    p_conv->next_pos = focus_data.next_pos;
}

/** af_pdaf_update_stats:
 * Update stats for PDAF.
 *
 * @pd: internal phase detection(PD) data structure
 *
 *  Return: VOID
 **/
static VOID af_pdaf_update_stats(
    af_alg_interface_t* p_alg,
    af_pdaf_input_t* input)
{
    af_alg_PDAF_internal_t* pd = (af_alg_PDAF_internal_t*) p_alg;
    af_pdaf_input_t* p_input = NULL;
    af_pdaf_params_t* p_params = &pd->params;

    if (!input)
    {
        HAF_MSG_LOW("Error in input pointer");
        return;
    }
    else
    {
        p_input = input;
    }

    pd->params.is_pd_updated = TRUE;

    // Apply frame skip prior to focus convergence
    if (p_params->frame_skip > 0)
    {
        HAF_MSG_LOW("ignore stats frameskip: %d", p_params->frame_skip);
        return;
    }

    return;
}

/** af_pdaf_get_next_pos:
 * Update the next lens position.
 *
 * @pd: internal phase detection(PD) data structure
 *
 *  Return: VOID
 **/
static VOID af_pdaf_get_next_pos(
    af_alg_PDAF_internal_t* pd)
{

    if (NULL == pd)
    {
        HAF_MSG_LOW("pd == NULL ERROR");
        return;
    }

    af_pdaf_focus_converge_data_t* p_out = &pd->conv_data;
    af_pdaf_input_t* input = &pd->input;
    af_pdaf_params_t* p_params = &pd->params;
    INT lens_stats_pos = input->lens_pos;
    INT cur_pos = pd->cur_pos;

    if (p_params->frame_skip > 0)
    {
        p_params->frame_skip--;
        p_out->next_pos = cur_pos;
    }
    else
    {
        p_out->next_pos = af_pdaf_get_target_pos(pd, input, lens_stats_pos);
    }

    // perform simple boundary check
    if (p_out->next_pos < p_params->near_limit)
    {
        p_out->next_pos = p_params->near_limit;
    }
    else if (p_out->next_pos > p_params->far_limit)
    {
        p_out->next_pos = p_params->far_limit;
    }

    return;
}

static int af_pdaf_get_fine_step(
    af_alg_PDAF_internal_t* pd)
{
    INT fine_step, min_fine_step;
    FLOAT dof_multiplier;
    /* get fine step size */
    min_fine_step = pd->pTuningInfo->pPDAFTuningData->focusScan.fineScanStepSize;
    dof_multiplier = af_pdaf_get_dof_multiplier(pd, pd->input.lens_pos);
    fine_step = (int)((float)min_fine_step * dof_multiplier);

    if (fine_step < min_fine_step)
    {
        fine_step = min_fine_step;
    }

    return fine_step;
}

static VOID af_pdaf_get_fine_range(
    af_alg_PDAF_internal_t* pd)
{
    af_pdaf_params_t* p_params = &pd->params;
    af_pdaf_fine_search_data_t* p_out = &pd->fine_search_data;
    af_pdaf_input_t* input = &pd->input;
    INT need_fine_search, range_near, range_far, fine_step;
    INT move_steps;
    FLOAT noise_multiplier, focus_pcnt;
    INT min_fine_range;
    INT focus_roi = 0, high_conf = 0, confidence = 0;
    INT stats_pos = input->lens_pos;
    INT cur_pos = pd->cur_pos;
    INT next_pos = pd->conv_data.next_pos;
    af_move_direction_type dir = (next_pos > cur_pos) ? AF_MOVE_FAR : AF_MOVE_NEAR;
    chromatixPDFocusScanType* focus_scan_tuning = &pd->pTuningInfo->pPDAFTuningData->focusScan;
    focus_roi = p_params->focus_roi_id;
    confidence = p_params->roi[focus_roi].conf;
    INT near_end, far_end;
    near_end = p_params->near_limit;
#ifdef VENDOR_EDIT
    int min_conf = af_pdaf_get_min_conf(pd);
#endif

    if (p_params->roi_info.type == AF_ROI_TYPE_FACE &&
            p_params->face_far_end_enabled)
    {
        far_end = p_params->face_far_end;
    }
#ifdef VENDOR_EDIT
    else if (p_params->roi_info.type == AF_ROI_TYPE_GENERAL &&
             (pd->set_parameters.gravity_info.g_vector[2] < -3.0 || pd->set_parameters.aec_input.AISceneID == 42 || pd->set_parameters.aec_input.AISceneID == 60) &&
             p_params->sensor_info.driver_IC_ID != 0x61)
    {
        far_end = 370;
    }
#endif
    else
    {
        far_end = p_params->far_limit;
    }

    // determine if fine search is needed by confidence or tuning
    if (TRUE == af_pdaf_is_running_2PD(pd))
    {
        high_conf = af_pdaf_get_high_conf(pd);
        need_fine_search = ((confidence < high_conf) &&
                            (p_params->MW_grids_converge_used == 0) &&
                            (p_params->MW_switch_to_peripheral_during_search == 0)) ? TRUE : FALSE;
    }
    else if (TRUE == af_pdaf_is_advanced_sparsePD_result(pd))
    {
        need_fine_search = FALSE;
    }
    else
    {
        need_fine_search = focus_scan_tuning->enableFineScan;
    }

    HAF_MSG_HIGH("Enable_high_conf_check: %d, need_fine_search: %d, high_conf %d, cur_conf %d",
                 af_pdaf_is_running_2PD(pd),
                 need_fine_search,
                 high_conf,
                 confidence);

    /* get fine step size */
    fine_step = af_pdaf_get_fine_step(pd);
    // get fine search range
    min_fine_range = focus_scan_tuning->minimumFineScanRange;
    focus_pcnt = 1.0f;
    move_steps = af_pdaf_get_focus_move_steps(pd, focus_pcnt);
    noise_multiplier = af_pdaf_get_noise_multiplier(pd);
    noise_multiplier = min_fine_range > noise_multiplier ? (FLOAT) min_fine_range : noise_multiplier;
    range_near = stats_pos + move_steps - (INT)((FLOAT) fine_step * noise_multiplier + 0.5f);
    range_far = stats_pos + move_steps + (INT)((FLOAT) fine_step * noise_multiplier + 0.5f);

    if (p_params->fine_scan_range_enhance)
    {
        if (stats_pos < range_near)
            range_near = stats_pos + (INT)((FLOAT)(range_near - stats_pos) * 0.75f);

        if (stats_pos > range_far)
            range_far = range_far + (INT)((FLOAT)(stats_pos - range_far) * 0.75f);
    }
#ifdef VENDOR_EDIT
    if(p_params->camera_run_mode == AF_RUN_MODE_VIDEO && pd->videoDisableFineSearch == TRUE){
        if( p_params->disable_fine_search_conf_thres > min_conf
            && confidence > p_params->disable_fine_search_conf_thres) {
            range_near = stats_pos + move_steps;
            range_far = stats_pos + move_steps;
            HAF_MSG_HIGH("video to disable fine search due to hight confidence %d, threshold %d",
                confidence, p_params->disable_fine_search_conf_thres);
        }else{
            HAF_MSG_HIGH("video not to disable fine search due to hight confidence %d, threshold %d",
                confidence, p_params->disable_fine_search_conf_thres);
        }
    }
#endif
    // Boundary Check and cap
    if (range_near < near_end)
    {
        range_far = near_end + (range_far - range_near);
        range_near = near_end;
    }

    if (range_far > far_end)
    {
        range_near = far_end - (range_far - range_near);
        range_far = far_end;
    }

    if (range_far == range_near)
    {
        HAF_MSG_LOW("fine range is zero! skip fine search!");
        need_fine_search = FALSE;
    }

    /* Special case: This is where the lens stats position is already inside the fine search range.
     In this case, we shrink fine search range to be starting at current stat position to shorten
     search time. Don't use this logic for fine scan extension as it causes reverse lens moving */
    if (stats_pos < range_far && stats_pos > range_near && p_params->enable_fine_scan_extension == false)
    {
        if (AF_MOVE_NEAR == dir)
        {
            // Set closer range_far
            range_far = stats_pos;
        }
        else
        {
            // Set closer range_near
            range_near = stats_pos;
        }

        HAF_MSG_LOW("Set closer range ( %d %d), dir %d, stats_pos %d", range_near, range_far, dir, stats_pos);
    }

    p_out->need_fine_search = need_fine_search;
    p_out->range_near = range_near;
    p_out->range_far = range_far;
    p_out->start_pos = pd->cur_pos;
    p_out->fine_step = fine_step;
    p_out->search_dir = dir;
    HAF_MSG_LOW("start_pos %d fine_step %d dir %d near %d far %d need_fine_search %d",
                p_out->start_pos,
                p_out->fine_step,
                p_out->search_dir,
                p_out->range_near,
                p_out->range_far,
                p_out->need_fine_search);
    return;
}

/** af_pdaf_is_depth_conf:
 * Compute if depth is confident.
 *
 * @pd: internal phase detection(PD) data structure
 *
 *  Return: VOID
 **/

static INT af_pdaf_is_depth_conf(
    af_alg_PDAF_internal_t* pd)
{
    af_pdaf_params_t* p_params = &pd->params;
    chromatixPDROIType* roi_tuning = &pd->pTuningInfo->pPDAFTuningData->ROI;
    INT i, is_conf = TRUE;
    HAF_MSG_LOW("Adjusting wait fr conf in case of lowlight = %d", p_params->wait_conf_recover_cnt);

    // ROI is confident only if PD confidence exceed min limit
    for (i = 0; i < (INT) roi_tuning->ConfigurationCount && p_params->roi[i].not_conf_cnt > p_params->wait_conf_recover_cnt;
            i++)
        ;

    if (i >= (INT) roi_tuning->ConfigurationCount)
    {
        is_conf = FALSE;
    }

    if (p_params->pd_no_update_cnt > 5)
    {
        is_conf = FALSE;
        HAF_MSG_HIGH("PD stats not updated, hence fallback");
    }

    HAF_MSG_HIGH("is_depth_conf= %s not_conf %d recover_cnt %d ",
                 is_conf ? "TRUE" : "FALSE",
                 p_params->roi[i].not_conf_cnt,
                 p_params->wait_conf_recover_cnt);
    return is_conf;
}

static INT af_pdaf_is_focus_converge_done(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_input_t* input,
    INT next_pos)
{
    af_pdaf_params_t* p_params = &pd->params;
    FLOAT noise_multiplier, dof_multiplier, focus_pcnt;
    INT min_focus_done_thres, focus_done_thresh;
    INT defocus, next_move, rev_cnt, converge_rev_cnt, focus_roi_id, confidence;
    INT stats_pos;
    BOOL is_roi_conf;
    INT focus_converge_done = 0;
    chromatixPDAFType* pPDAFTuningData = pd->pTuningInfo->pPDAFTuningData;
    stats_pos = pd->input.lens_pos;
    focus_roi_id = p_params->focus_roi_id;
    defocus = p_params->roi[focus_roi_id].defocus;
    rev_cnt = p_params->roi[focus_roi_id].rev_cnt;
    converge_rev_cnt = p_params->roi[focus_roi_id].converge_rev_cnt;
    is_roi_conf = (BOOL) p_params->roi[focus_roi_id].is_conf;
    confidence = p_params->roi[focus_roi_id].conf;
    min_focus_done_thres = pPDAFTuningData->focusScan.focusDoneThreshold;
    dof_multiplier = af_pdaf_get_dof_multiplier(pd, pd->input.lens_pos);
    noise_multiplier = af_pdaf_get_noise_multiplier(pd);
    focus_pcnt = af_pdaf_get_focus_pcnt(pd, input, pd->params.roi[pd->params.focus_roi_id].defocus);
    next_move = af_pdaf_get_focus_move_steps(pd, focus_pcnt);
    focus_done_thresh = (INT)((FLOAT) min_focus_done_thres * dof_multiplier * noise_multiplier + 0.5f);

    if (focus_done_thresh < min_focus_done_thres)
    {
        focus_done_thresh = min_focus_done_thres;
    }

    // This is to give one more chance to converge if
    // first set of stats is wrong and lens did not
    // move at all (focus_pcnt and steps = 0).
    if (p_params->focus_converge_cnt > 1)
    {
        if (stats_pos + next_move <= p_params->near_limit || stats_pos + next_move >= p_params->far_limit)
        {
            focus_converge_done = (is_roi_conf == TRUE) ? 1 : -1;
        }
    }

    if (is_roi_conf == TRUE)
    {
        if (!pPDAFTuningData->focusScan.enableFineScan || TRUE == af_pdaf_is_running_2PD(pd) ||
            af_pdaf_is_advanced_sparsePD_result(pd))
        {
            next_move = 0;
        }

#ifdef VENDOR_EDIT
        if ((abs((INT) defocus) - abs((INT) next_move)) < focus_done_thresh && p_params->focus_converge_cnt > 1)
        {
            if (p_params->camera_run_mode != AF_RUN_MODE_VIDEO  || (abs((INT) defocus) <= focus_done_thresh ))
            {
                focus_converge_done = 1;
            }
        }
        else if ((rev_cnt >= FALLBACK_REV_CNT_TH || converge_rev_cnt >= FALLBACK_CONVERGE_REV_CNT_TH) &&
                 AF_ROI_TYPE_GENERAL == pd->params.roi_info.type &&
                 !af_pdaf_is_low_light(pd))
#else
        if ((abs((INT) defocus) - abs((INT) next_move)) < focus_done_thresh && p_params->focus_converge_cnt > 1)
        {
            focus_converge_done = 1;
        }
        else if (rev_cnt >= 2)
#endif
        {
            focus_converge_done = (BOOL) - 1;
        }
    }

    /* 2PD converge done requires temporal result:
     When to run: 2PD, no ROI, no LED(to aVOID hanging LED),  not fallback case and
     is_conf(This is to let converge done asap so fine search run can right away, aVOID hunting)
     1. 2PD is not using fine search. Thus, the final converge done requires stricter condition
     to make sure defocus is indeed stabilizing at peak
     2. The implementation is, upon convergence done, start filling temporal buffer.
     if avg target position is within threshold, then report converge done. Otherwise, wait.
     3. Only for center ROI as Touch & Face we want to converge ASAP */
    INT temporal_converge_done = 0;
#ifndef VENDOR_EDIT
    if (((TRUE == af_pdaf_is_running_2PD(pd) || af_pdaf_is_advanced_sparsePD_result(pd)) &&
            pPDAFTuningData->convergeDoneFilter.enable) &&
            (AF_ROI_TYPE_GENERAL == pd->params.roi_info.type ||
             AF_ROI_TYPE_TOUCH == pd->params.roi_info.type) &&
            (FALSE == pd->params.af_led_assist) &&
            (focus_converge_done != -1 && TRUE == is_roi_conf))
#else
    if( pPDAFTuningData->convergeDoneFilter.enable &&
        (FALSE == pd->params.af_led_assist)&&
        (focus_converge_done != -1 && TRUE == is_roi_conf)&&
        ((TRUE == af_pdaf_is_running_2PD(pd)&& ((AF_ROI_TYPE_GENERAL == pd->params.roi_info.type || AF_ROI_TYPE_TOUCH == pd->params.roi_info.type)) )
            ||af_pdaf_is_advanced_sparsePD_result(pd)
            ||(p_params->camera_run_mode == AF_RUN_MODE_VIDEO)))
#endif
    {
        // Converge done condition triggered, begin target pos temporal filter
        if (TRUE == focus_converge_done && FALSE == pd->params.run_converge_done_temporal_filter)
        {
            pd->params.run_converge_done_temporal_filter = TRUE;
        }

        if (TRUE == pd->params.run_converge_done_temporal_filter)
        {
            temporal_converge_done = af_pdaf_converge_done_temporal_process(pd, next_pos);

            /* Only update converge done with temporal result if current converge done is TRUE
             The reason is, if current converge done is not TRUE, then there is a chance of faulty
             cur_pos which still satisfies the temporal done criteria and false reporting.
             So both converge done AND temporal done need to be satisfied simultaneously. */
            if (1 == focus_converge_done)
            {
                focus_converge_done = temporal_converge_done;
            }
        }
    }

    // Lower confidence, 2PD creverge cnt > 2 means huntting. We let it enter LSP
    if (pPDAFTuningData->enableEnterLSPLowConfidenceHunting && is_roi_conf == TRUE && confidence < af_pdaf_get_high_conf(pd)
            && af_pdaf_is_running_2PD(pd) && converge_rev_cnt >= pPDAFTuningData->LSPLowConfidenceHuntingThreshold)
    {
        HAF_MSG_LOW("converge done due to converge_rev_cnt large than threshold.");
        focus_converge_done = (BOOL) 1;
    }

#ifndef VENDOR_EDIT
    HAF_MSG_LOW("defocus=%d, focus_converge_done_thres=%d, "
                "focus_converge_done=%s", defocus, focus_done_thresh, focus_converge_done ? "TRUE" : "FALSE");
#else
    ALOGI("[AFDBG] %s defocus=%d, focus_converge_done_thres=%d, "
            "focus_converge_done=%s. sad panning=%d, gyro panning=%d, aec_settled=%d, wait_caf_cnt:%d, conf:%d, orig_defocus:%d, orig_conf:%d temporal_converge_done:%d next_move:%d focus_converge_cnt:%d is_roi_conf:%d",
            __func__, defocus, focus_done_thresh, focus_converge_done ? "TRUE" : "FALSE",
            p_params->sad_is_panning, p_params->gyro_is_panning,
            pd->set_parameters.aec_input.aec_settled, p_params->wait_caf_stable_cnt,
            p_params->roi[0].conf, p_params->roi[0].orig_defocus, p_params->roi[0].orig_conf, temporal_converge_done, next_move, p_params->focus_converge_cnt, is_roi_conf);
#endif
    return focus_converge_done;
}

static VOID af_pdaf_face_pd_history_rebase(
    af_alg_PDAF_internal_t* pd)
{
    INT* face_pd_target_pos_hist = pd->conv_data.face_pd_target_pos_hist;
    INT* face_pd_conf_hist = pd->conv_data.face_pd_conf_hist;
    af_pdaf_params_t* p_params = &pd->params;
    chromatixPDAFType* pPDAFTuningData = pd->pTuningInfo->pPDAFTuningData;
    INT focus_roi_id = p_params->focus_roi_id;
    INT cur_pos = pd->input.lens_pos;
    UINT32 i;

    // temporal filter implementation
    for (i = 0; i < pPDAFTuningData->face.defocusFilter.length; i++)
    {
        face_pd_target_pos_hist[i] = cur_pos + p_params->roi[focus_roi_id].defocus;
        face_pd_conf_hist[i] = p_params->roi[focus_roi_id].conf;
    }
}

static VOID af_pdaf_face_pd_history_update(
    af_alg_PDAF_internal_t* pd)
{
    INT* face_pd_target_pos_hist = pd->conv_data.face_pd_target_pos_hist;
    INT* face_pd_conf_hist = pd->conv_data.face_pd_conf_hist;
    af_pdaf_params_t* p_params = &pd->params;
    INT focus_roi_id = p_params->focus_roi_id;
    INT cur_pos = pd->input.lens_pos;
    chromatixPDAFType* pPDAFTuningData = pd->pTuningInfo->pPDAFTuningData;
    UINT32 i;

    // temporal filter implementation
    for (i = 1; i < pPDAFTuningData->face.defocusFilter.length; i++)
    {
        face_pd_target_pos_hist[i - 1] = face_pd_target_pos_hist[i];
        face_pd_conf_hist[i - 1] = face_pd_conf_hist[i];
    }

    face_pd_target_pos_hist[i - 1] = cur_pos + p_params->roi[focus_roi_id].defocus;
    face_pd_conf_hist[i - 1] = p_params->roi[focus_roi_id].conf;
    HAF_MSG_LOW("history: target pos (%d, %d, %d), conf (%d, %d, %d)",
                face_pd_target_pos_hist[0],
                face_pd_target_pos_hist[1],
                face_pd_target_pos_hist[2],
                face_pd_conf_hist[0],
                face_pd_conf_hist[1],
                face_pd_conf_hist[2]);
}

// temporal average target pos calculation
static INT af_pdaf_get_temporal_avg_target_pos(
    af_alg_PDAF_internal_t* pd,
    UINT32 cur_pos,
    INT* buffer,
    INT buffer_len)
{
    if (NULL == buffer || NULL == pd)
    {
        HAF_MSG_ERROR("ERROR!! Null pointers!!");
        return -9999;
    }

    // temporal filter
    INT* hist_target_pos = buffer;
    af_pdaf_params_t* p_params = &pd->params;
    INT focus_roi_id = 0;
    INT target_pos = cur_pos;
    INT avg_target_pos = -9999;
    INT i = 0;
    // temporal filter implementation
    focus_roi_id = p_params->focus_roi_id;

    if (p_params->roi[focus_roi_id].is_conf == TRUE)
    {
        target_pos = pd->params.estimated_target_pos;

        // perform simple boundary check
        if (target_pos < p_params->near_limit)
        {
            target_pos = p_params->near_limit;
        }
        else if (target_pos > p_params->far_limit)
        {
            target_pos = p_params->far_limit;
        }

        // update history, shift left
        for (i = 1; i < buffer_len; i++)
        {
            hist_target_pos[i - 1] = hist_target_pos[i];
        }

        hist_target_pos[i - 1] = target_pos;
        HAF_MSG_LOW("hist [%d, %d, %d] target %d", hist_target_pos[0], hist_target_pos[1], hist_target_pos[2], target_pos);
        // find average target pos
        target_pos = 0;

        for (i = 0; i < buffer_len; i++)
        {
            target_pos += hist_target_pos[i];
        }

        if (buffer_len > 0)
        {
            avg_target_pos = target_pos / buffer_len;
        }
    }
    else
    {
        HAF_MSG_LOW("Current frame not confident, do not save target_pos to temporal history.");
    }

    HAF_MSG_LOW("cur_pos %d , avg target_pos%d", cur_pos, avg_target_pos);
    return avg_target_pos;
}

// face pdaf: update temporal filter value and get next pos
static INT af_pdaf_get_face_temporal_next_position(
    af_alg_PDAF_internal_t* pd)
{
    INT* face_pd_target_pos_hist = pd->conv_data.face_pd_target_pos_hist;
    INT* face_pd_conf_hist = pd->conv_data.face_pd_conf_hist;
    INT cur_pos = pd->input.lens_pos;
    chromatixPDAFType* pPDAFTuningData = pd->pTuningInfo->pPDAFTuningData;
    INT conf_thres_high = (INT) pPDAFTuningData->face.defocusFilter.confidenceHighThreshold;
    INT conf_thres_low = af_pdaf_get_min_conf(pd);
    INT next_pos = cur_pos;
    FLOAT avg_defocus = 0.0f;
    FLOAT pcnt = 0;
    UINT32 i = 0;
    HAF_MSG_LOW("conf thes high, low (%d, %d)", conf_thres_high, conf_thres_low);

    for (i = 0; i < pPDAFTuningData->face.defocusFilter.length; i++)
    {
        if (face_pd_conf_hist[i] <= conf_thres_low)
            pcnt = 0.0;
        else if (face_pd_conf_hist[i] >= conf_thres_high)
            pcnt = 1.0;
        else
        {
            pcnt = ((FLOAT)(face_pd_conf_hist[i] - conf_thres_low)) / (FLOAT)(conf_thres_high - conf_thres_low);
        }

        avg_defocus += (face_pd_target_pos_hist[i] - cur_pos) * pcnt / (pPDAFTuningData->face.defocusFilter.length);
    }

    next_pos = cur_pos + (INT) avg_defocus;
    HAF_MSG_LOW("cur_pos %d, next pos %d", cur_pos, next_pos);
    return next_pos;
}

static INT af_pdaf_converge_done_temporal_process(
    af_alg_PDAF_internal_t* pd,
    INT next_pos)
{
    INT focus_converge_done = -9999;
    UINT32 cur_pos = pd->input.lens_pos;
    chromatixPDAFType* pPDAFTuningData = pd->pTuningInfo->pPDAFTuningData;
    // Get temporal filtered target pos
    INT avg_target_pos = af_pdaf_get_converge_done_temporal_pos(pd, cur_pos);
#ifdef VENDOR_EDIT
	INT doneThreshold = (INT) pPDAFTuningData->convergeDoneFilter.threshold * af_pdaf_get_noise_multiplier( pd );
#endif
    // First check if we have filled enough samples in the history
    if (pd->params.converge_done_temporal_counter >= pPDAFTuningData->convergeDoneFilter.length - 1)
    {
#ifndef VENDOR_EDIT
        if (ABS(avg_target_pos - next_pos) <= (INT) pPDAFTuningData->convergeDoneFilter.threshold)
#else
        if (ABS(avg_target_pos - next_pos) <= doneThreshold )
#endif
        {
            /* temporal avg_target_pos minus next destination is within threshold, report conv success
             Note: The reason why we use next_pos is because we want the logical lens location of next frame */
            focus_converge_done = 1;
            pd->conv_data.next_pos = avg_target_pos;
#ifdef VENDOR_EDIT
            HAF_MSG_INFO("temporal_conv_done: temporal delta = %d <= THRESH %d, tuning thres = %d, lgth = %d, focus success!",
                        ABS(avg_target_pos - next_pos),
                        doneThreshold,
                        (INT) pPDAFTuningData->convergeDoneFilter.threshold,
                        pPDAFTuningData->convergeDoneFilter.length );
#else
            HAF_MSG_INFO("temporal_conv_done: temporal delta = %d <= THRESH %d, focus success!",
                        ABS(avg_target_pos - next_pos),
                        pPDAFTuningData->convergeDoneFilter.threshold);
#endif
        }
        else
        {
            // Hold report converge done till we get a valid temporal target pos result
            focus_converge_done = 0;
#ifdef VENDOR_EDIT
            ALOGI("%s temporal_conv_done: temporal delta = %d > THRESH %d, tuning thres = %d, lgth = %d, wait!!", __func__,
                        ABS(avg_target_pos - next_pos),
                        doneThreshold,
                        (INT) pPDAFTuningData->convergeDoneFilter.threshold,
                        pPDAFTuningData->convergeDoneFilter.length );
#else
            HAF_MSG_LOW("temporal_conv_done: temporal delta = %d > THRESH %d, wait!!",
                        ABS(avg_target_pos - next_pos),
                        pPDAFTuningData->convergeDoneFilter.threshold);
#endif
        }
    }
    else
    {
        // Still filling the temporal buffer, wait till it is full, hold report converge
        HAF_MSG_LOW("temporal_conv_done: Still filling temporal target pos buffer, counter = %d!! length:%d",
                    pd->params.converge_done_temporal_counter, pPDAFTuningData->convergeDoneFilter.length);
        focus_converge_done = 0;
    }

    pd->params.converge_done_temporal_counter++;
    return focus_converge_done;
}

// 2pd convergence done temporal check: update temporal filter value and get next pos
static INT af_pdaf_get_converge_done_temporal_pos(
    af_alg_PDAF_internal_t* pd,
    UINT32 cur_pos)
{
    INT* hist_target_pos = pd->conv_data.conv_done_temporal_filter;
    INT avg_target_pos = -9999;
    UINT32 i = 0;
    chromatixPDAFType* pPDAFTuningData = pd->pTuningInfo->pPDAFTuningData;

    // First trigger, reset history
    if (0 == pd->params.converge_done_temporal_counter)
    {
        // Do not set to 0, because it is a valid target pos. Set to -9999
        for (i = 0; i < pPDAFTuningData->convergeDoneFilter.length; i++)
            hist_target_pos[i] = -9999;
    }

    // Get temporal avg target pos
    avg_target_pos = af_pdaf_get_temporal_avg_target_pos(pd,
                     cur_pos,
                     hist_target_pos,
                     pPDAFTuningData->convergeDoneFilter.length);
    HAF_MSG_LOW("temporal_conv_done: cur_pos %d , avg target_pos%d", cur_pos, avg_target_pos);
    return avg_target_pos;
}

static INT af_pdaf_get_target_pos(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_input_t* input,
    INT lens_pos)
{
    INT move_steps, target_pos;
    FLOAT focus_pcnt;
    focus_pcnt = af_pdaf_get_focus_pcnt(pd, input, pd->params.roi[pd->params.focus_roi_id].defocus);
#ifdef VENDOR_EDIT
    focus_pcnt = focus_pcnt *pd->params.rev_cnt_pct_drop;
#endif
    move_steps = af_pdaf_get_focus_move_steps(pd, focus_pcnt);
    target_pos = lens_pos + move_steps;
    pd->params.estimated_target_pos = lens_pos + af_pdaf_get_focus_move_steps(pd, 1.0f);
    HAF_MSG_HIGH("lens_pos %d , move_steps %d, target_pos%d", lens_pos, move_steps, target_pos);
    return target_pos;
}

/** af_pdaf_get_focus_pcnt:
 * Estimate the current focus percentage.
 *
 * @pd: internal pd data structure
 * @input: Pdaf input structure
 *
 *  Return: FLOAT
 **/
static FLOAT af_pdaf_get_focus_pcnt(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_input_t* input,
    INT defocus)
{

    if (NULL == pd || NULL == input)
    {
        return -1.0f;
    }

    FLOAT delta_x, delta_y, focus_pcnt;
    INT i, num_entries;
    const chromatixPDFocusTableType* focus_table_tuning = &pd->pTuningInfo->pPDAFTuningData->focusTable;
    if (af_pdaf_is_advanced_sparsePD_result(pd))
    {
        focus_table_tuning = &lcrFocusTable;
    }

    num_entries = focus_table_tuning->focusEntriesCount;
    defocus = abs(defocus);

    if (num_entries < 1)
    {
        focus_pcnt = 0;
    }
    else if (defocus <= focus_table_tuning->focusEntries[0].defocus)
    {
        focus_pcnt = focus_table_tuning->focusEntries[0].movePercent;
    }
    else if (defocus > focus_table_tuning->focusEntries[num_entries - 1].defocus)
    {
        focus_pcnt = focus_table_tuning->focusEntries[num_entries - 1].movePercent;
    }
    else
    {
        for (i = 1; defocus > focus_table_tuning->focusEntries[i].defocus; i++)
            ;

        delta_y = focus_table_tuning->focusEntries[i].movePercent - focus_table_tuning->focusEntries[i - 1].movePercent;
        delta_x = (FLOAT)(focus_table_tuning->focusEntries[i].defocus - focus_table_tuning->focusEntries[i - 1].defocus);
        focus_pcnt = delta_y / delta_x * (FLOAT)(defocus - focus_table_tuning->focusEntries[i - 1].defocus)
                     + focus_table_tuning->focusEntries[i - 1].movePercent;
    }

    if (pd->params.roi[pd->params.focus_roi_id].is_soft_conf_rgn == TRUE)
    {
        INT min_conf, cur_conf;
        FLOAT soft_conf_move_pcnt;
        min_conf = af_pdaf_get_min_conf(pd);

		cur_conf = pd->params.roi[pd->params.focus_roi_id].conf;
		cur_conf = CLIP(cur_conf, min_conf, min_conf + pd->params.soft_conf_thresh_delta);

        delta_y = 1.0 - pd->params.soft_conf_focus_pnt;
        delta_x = pd->params.soft_conf_thresh_delta;
        soft_conf_move_pcnt = delta_y / delta_x * (cur_conf - min_conf)
				  + pd->params.soft_conf_focus_pnt;
        focus_pcnt = (focus_pcnt * soft_conf_move_pcnt);
        HAF_MSG_HIGH("focus_pcnt=%f, soft_conf_move_pcnt=%f min_conf %d cur_conf %d soft_conf_thresh_delta %f soft_conf_focus_pnt %f",
			focus_pcnt, soft_conf_move_pcnt, min_conf, cur_conf,
			pd->params.soft_conf_thresh_delta, pd->params.soft_conf_focus_pnt );
    }

    HAF_MSG_LOW("defocus=%d, focus_pcnt=%0.2f, bv=%0.2f", defocus, focus_pcnt, input->scene_bv);
    return focus_pcnt;
}
/** af_pdaf_get_focus_move_steps:
 * Estimate the current focus percentage.
 *
 * @pd: internal pd data structure
 * @focus_pcnt: percentage of lens move to target pos
 *
 *  Return: INT
 **/
static INT af_pdaf_get_focus_move_steps(
    af_alg_PDAF_internal_t* pd,
    FLOAT focus_pcnt)
{
    af_pdaf_params_t* p_params = &pd->params;
    FLOAT defocus = 0;
    INT focus_roi_id, move_steps;
    move_steps = 0;
    focus_roi_id = p_params->focus_roi_id;

    if (p_params->roi[focus_roi_id].is_conf == TRUE)
    {
        defocus = (FLOAT) p_params->roi[focus_roi_id].defocus;
        move_steps = (INT)(defocus * focus_pcnt + 0.5f);
    }
    else
    {
        HAF_MSG_LOW("Not confident: is_conf = %d, conf value = %d",
                    p_params->roi[focus_roi_id].is_conf,
                    p_params->roi[focus_roi_id].conf);
    }

    HAF_MSG_LOW("defocus=%0.2f, focus_pcnt=%0.2f, move_steps=%d", defocus, focus_pcnt, move_steps);
    return move_steps;
}
/** af_pdaf_get_noise_multiplier:
 * Estimate the noise multiplier.
 *
 * @pd: internal pd data structure
 *
 *  Return: FLOAT
 **/
static FLOAT af_pdaf_get_noise_multiplier(
    af_alg_PDAF_internal_t* pd)
{
    FLOAT noise_gain, delta_x, delta_y, multiplier;
    INT i, num_entries;
    chromatixPDNoiseTableType* noise_table_tuning = &pd->pTuningInfo->pPDAFTuningData->noiseTable;
    num_entries = noise_table_tuning->noiseEntriesCount;
    noise_gain = pd->params.roi[pd->params.focus_roi_id].noise_gain;

    if (num_entries < 1)
    {
        multiplier = 1.0f;
    }
    else if (noise_gain <= noise_table_tuning->noiseEntries[0].noiseGain)
    {
        multiplier = noise_table_tuning->noiseEntries[0].noiseGainMultiplier;
    }
    else if (noise_gain > noise_table_tuning->noiseEntries[num_entries - 1].noiseGain)
    {
        multiplier = noise_table_tuning->noiseEntries[num_entries - 1].noiseGainMultiplier;
    }
    else
    {
        for (i = 1; noise_gain > noise_table_tuning->noiseEntries[i].noiseGain; i++)
            ;

        delta_y = noise_table_tuning->noiseEntries[i].noiseGainMultiplier
                  - noise_table_tuning->noiseEntries[i - 1].noiseGainMultiplier;
        delta_x = noise_table_tuning->noiseEntries[i].noiseGain - noise_table_tuning->noiseEntries[i - 1].noiseGain;
        multiplier = delta_y / delta_x * (noise_gain - noise_table_tuning->noiseEntries[i - 1].noiseGain)
                     + noise_table_tuning->noiseEntries[i - 1].noiseGainMultiplier;
    }

    HAF_MSG_HIGH("noise_gain=%0.2f, multiplier=%0.2f", noise_gain, multiplier);
    return multiplier;
}

/** af_pdaf_get_dof_multiplier:
 * Estimate the depth of focus multiplier.
 *
 * @pd: internal pd data structure
 *
 *  Return: FLOAT
 **/
FLOAT af_pdaf_get_dof_multiplier(
    af_alg_PDAF_internal_t* pd, INT stats_pos)
{
    af_pdaf_params_t* p_params = &pd->params;
    chromatixPDAFType *pPDAFTuning = pd->pTuningInfo->pPDAFTuningData;
    FLOAT norm_lens_pos, multiplier = 1.0f;
    INT i, numEntries;
    FLOAT delta_x, delta_y;

    if ((p_params->far_limit == p_params->near_limit) || (p_params->far_limit == stats_pos))
    {
        norm_lens_pos = 0.0f;
    }
    else
    {
        norm_lens_pos = +
                        (float) abs(p_params->far_limit - stats_pos) /
                        (float) abs(p_params->far_limit - p_params->near_limit);
    }

    if (norm_lens_pos < 0)
    {
        norm_lens_pos = 0.0f;
    }
    else if (norm_lens_pos > 1)
    {
        norm_lens_pos = 1.0f;
    }

    norm_lens_pos = MIN( MAX( norm_lens_pos, 0 ), 1);
    numEntries = pPDAFTuning->dofScalerTable.dofScalerEntriesCount;

    if (numEntries < 1)
    {
        multiplier = 1.0f;
    }

    if (norm_lens_pos <= pPDAFTuning->dofScalerTable.dofScalerEntries[0].normalizedLensPosition)
    {
        multiplier = pPDAFTuning->dofScalerTable.dofScalerEntries[0].dofScaler;
    }
    else if (norm_lens_pos >= pPDAFTuning->dofScalerTable.dofScalerEntries[numEntries - 1].normalizedLensPosition)
    {
        multiplier = pPDAFTuning->dofScalerTable.dofScalerEntries[numEntries - 1].dofScaler;
    }
    else
    {
        i = 1;

        while (norm_lens_pos > pPDAFTuning->dofScalerTable.dofScalerEntries[i].normalizedLensPosition)
        {
            i++;
        }

        delta_y = (FLOAT)(pPDAFTuning->dofScalerTable.dofScalerEntries[i].dofScaler -
            pPDAFTuning->dofScalerTable.dofScalerEntries[i - 1].dofScaler);
        delta_x = (FLOAT)(pPDAFTuning->dofScalerTable.dofScalerEntries[i].normalizedLensPosition -
            pPDAFTuning->dofScalerTable.dofScalerEntries[i - 1].normalizedLensPosition);
        multiplier = delta_y / delta_x * (norm_lens_pos -
            pPDAFTuning->dofScalerTable.dofScalerEntries[i - 1].normalizedLensPosition) +
            pPDAFTuning->dofScalerTable.dofScalerEntries[i - 1].dofScaler;
    }

    HAF_MSG_LOW("lens_pos=%d, norm_lens_pos=%0.2f, multiplier=%1.1f",
                stats_pos, norm_lens_pos, multiplier);
    return multiplier;
}

/** af_pdaf_get_high_conf:
 * Estimate the trust confidence level without fine search.
 *
 * @pd: internal pd data structure
 *
 *  Return: INT
 **/
static INT af_pdaf_get_high_conf(
    af_alg_PDAF_internal_t* pd)
{
    af_pdaf_params_t* p_params = &pd->params;
    chromatixPDConfidenceTableType* confidence_tuning = &pd->pTuningInfo->pPDAFTuningData->confidenceHighTable;
    af_pdaf_converge_status_enum_type converge_status = pd->conv_data.status;
    FLOAT noise_gain, delta_x, delta_y;
    INT i, num_entries, high_conf;
    noise_gain = p_params->roi[p_params->focus_roi_id].noise_gain;
    num_entries = confidence_tuning->confidenceEntriesCount;

    if (num_entries < 1)
    {
        high_conf = 0;
    }
    else if (noise_gain <= confidence_tuning->confidenceEntries[0].noiseGain)
    {
        high_conf = confidence_tuning->confidenceEntries[0].minimumConfidence;
    }
    else if (noise_gain > confidence_tuning->confidenceEntries[num_entries - 1].noiseGain)
    {
        high_conf = confidence_tuning->confidenceEntries[num_entries - 1].minimumConfidence;
    }
    else
    {
        // get index for noise gain passes the entries
        for (i = 1; noise_gain > confidence_tuning->confidenceEntries[i].noiseGain; i++)
            ;

        delta_y = (FLOAT)(confidence_tuning->confidenceEntries[i].minimumConfidence -
            confidence_tuning->confidenceEntries[i - 1].minimumConfidence);
        delta_x = confidence_tuning->confidenceEntries[i].noiseGain -
            confidence_tuning->confidenceEntries[i - 1].noiseGain;
        high_conf = (INT)(delta_y / delta_x * (noise_gain - confidence_tuning->confidenceEntries[i - 1].noiseGain)
                          + (FLOAT)confidence_tuning->confidenceEntries[i - 1].minimumConfidence);
    }

    /* haf_state == AF_HAF_STATE_FOCUS_CONVERGE : this check ensures that
     once PDAF is triggered in Monitor, we lower confidence level during focus
     converge to make it tougher to exit out of PDAF. */
    if (p_params->enable_soft_conf_thresh && converge_status == AF_PDAF_CONVERGE_RUNNING)
        high_conf = (INT)MAX(0, high_conf - p_params->soft_conf_thresh_delta);

    HAF_MSG_LOW("noise_gain=%0.2f, min_conf=%d enable_soft_conf %d", noise_gain, high_conf, p_params->enable_soft_conf_thresh);
    return high_conf;
}

/** af_pdaf_get_min_conf:
 * Estimate the minimum confidence level.
 *
 * @pd: internal pd data structure
 *
 *  Return: INT
 **/
INT af_pdaf_get_min_conf(
    af_alg_PDAF_internal_t* pd)
{
    af_pdaf_params_t* p_params = &pd->params;
    af_pdaf_converge_status_enum_type converge_status = pd->conv_data.status;
    FLOAT noise_gain, delta_x, delta_y;
    INT i, num_entries, min_conf;
    chromatixPDConfidenceTableType* confidence_tuning = &pd->pTuningInfo->pPDAFTuningData->confidenceTable;
    num_entries = confidence_tuning->confidenceEntriesCount;
    noise_gain = p_params->roi[p_params->focus_roi_id].noise_gain;

    if (num_entries < 1)
    {
        min_conf = 0;
    }
    else if (noise_gain <= confidence_tuning->confidenceEntries[0].noiseGain)
    {
        min_conf = confidence_tuning->confidenceEntries[0].minimumConfidence;
    }
    else if (noise_gain > confidence_tuning->confidenceEntries[num_entries - 1].noiseGain)
    {
        min_conf = confidence_tuning->confidenceEntries[num_entries - 1].minimumConfidence;
    }
    else
    {
        // get index for noise gain passes the entries
        for (i = 1; noise_gain > confidence_tuning->confidenceEntries[i].noiseGain; i++)
            ;

        delta_y = (FLOAT)(confidence_tuning->confidenceEntries[i].minimumConfidence
                          - confidence_tuning->confidenceEntries[i - 1].minimumConfidence);
        delta_x = confidence_tuning->confidenceEntries[i].noiseGain - confidence_tuning->confidenceEntries[i - 1].noiseGain;
        min_conf = (INT)(delta_y / delta_x * (noise_gain - confidence_tuning->confidenceEntries[i - 1].noiseGain)
                         + (FLOAT) confidence_tuning->confidenceEntries[i - 1].minimumConfidence);
    }

    /* haf_state == AF_HAF_STATE_SEARCH : this check ensures that
     once PDAF is triggered in Monitor, we lower confidence level during focus
     converge to make it tougher to exit out of PDAF. */
    if (p_params->enable_soft_conf_thresh && converge_status == AF_PDAF_CONVERGE_RUNNING)
        min_conf = (INT)MAX(0, min_conf - p_params->soft_conf_thresh_delta);

    HAF_MSG_LOW("noise_gain=%0.2f, min_conf=%d soft_conf_thresh %d, converge_status %d",
		noise_gain, min_conf, p_params->enable_soft_conf_thresh, converge_status );
    return min_conf;
}

/** af_pdaf_get_min_stable_cnt:
 * Estimate the minimum confidence level.
 *
 * @pd: internal pd data structure
 *
 *  Return: INT
 **/
static INT af_pdaf_get_min_stable_cnt(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_input_t* input)
{
    FLOAT delta_x, delta_y;
    INT i, num_entries, fps, min_stable_cnt;
    chromatixPDStableTableType* stable_tuning = &pd->pTuningInfo->pPDAFTuningData->stableTable;
    num_entries = stable_tuning->stableEntriesCount;
    fps = (INT) input->fps;

    if (num_entries < 1)
    {
        min_stable_cnt = 1;
    }
    else if (fps <= stable_tuning->stableEntries[0].FPS)
    {
        min_stable_cnt = stable_tuning->stableEntries[0].minimumStableCount;
    }
    else if (fps > stable_tuning->stableEntries[num_entries - 1].FPS)
    {
        min_stable_cnt = stable_tuning->stableEntries[num_entries - 1].minimumStableCount;
    }
    else
    {
        // get index with fps passes the entries
        for (i = 1; fps > stable_tuning->stableEntries[i].FPS; i++)
            ;

        delta_y = (FLOAT)(stable_tuning->stableEntries[i].minimumStableCount
                          - stable_tuning->stableEntries[i - 1].minimumStableCount);
        delta_x = (FLOAT)(stable_tuning->stableEntries[i].FPS - stable_tuning->stableEntries[i - 1].FPS);
        min_stable_cnt = (INT)(delta_y / delta_x * (FLOAT)(fps - stable_tuning->stableEntries[i - 1].FPS)
                               + (FLOAT) stable_tuning->stableEntries[i - 1].minimumStableCount);
    }

    HAF_MSG_HIGH("fps=%d, min_stable_cnt=%d", fps, min_stable_cnt);
    return min_stable_cnt;
}

/** af_pdaf_get_focus_roi_id:
 * Get focus roi id
 *
 * @pd: internal pd data structure
 *
 *  Return: INT
 **/
INT af_pdaf_get_focus_roi_id(
    af_alg_PDAF_internal_t* pd)
{
    af_pdaf_params_t* p_params = &pd->params;
    chromatixPDROIType* roi_tuning = &pd->pTuningInfo->pPDAFTuningData->ROI;
    UINT32 i, focus_roi_id = 0;

    // select focus roi based on confidence and stability
    // select confident and stable roi
    for (i = 0;
            i < roi_tuning->ConfigurationCount
            && (p_params->roi[i].is_conf == FALSE || p_params->roi[i].is_stable == FALSE); i++)
        ;

    if (i < roi_tuning->ConfigurationCount)
    {
        focus_roi_id = i;
    }

    HAF_MSG_LOW("focus_roi_id=%d, is_conf=%s, "
                "is_stable=%s",
                focus_roi_id,
                p_params->roi[focus_roi_id].is_conf ? "TRUE" : "FALSE",
                p_params->roi[focus_roi_id].is_stable ? "TRUE" : "FALSE");
    return focus_roi_id;
}

/** af_pdaf_proc_pd_single:
 * Processing engine of Pdaf for single ROI
 *
 * @pd: internal pd data structure
 *
 *  Return: VOID
 **/

static VOID af_pdaf_proc_pd_single(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_input_t* input)
{
    af_pdaf_params_t* p_params = &pd->params;
    chromatixPDROIType* roi_tuning = &pd->pTuningInfo->pPDAFTuningData->ROI;
    INT i, j, k;
    INT grid_id, grid_cnt, grid_num_cols, grid_num_rows;
    UINT min_conf;
    FLOAT pd_grid, pd_roi, defocus_roi, weight;
    grid_num_cols = (INT) input->pdaf_info.x_win_num;
    grid_num_rows = (INT) input->pdaf_info.y_win_num;
    grid_cnt = grid_num_cols * grid_num_rows;
    HAF_MSG_LOW("grid: num rows=%d, num cols=%d", grid_num_rows, grid_num_cols);

    // Process Each PD stats ROI
    for (i = 0; i < (INT) roi_tuning->ConfigurationCount; i++)
    {
        p_params->roi[i].conf = 0;

        for (j = 0; j < roi_tuning->Configuration[i].ROINumRows; j++)
        {
            for (k = 0; k < roi_tuning->Configuration[i].ROINumCols; k++)
            {
                grid_id = (roi_tuning->Configuration[i].ROILocationY + j) * grid_num_cols
                          + roi_tuning->Configuration[i].ROILocationX + k;

                if (grid_id < 0)
                {
                    grid_id = 0;
                }
                else if (grid_id >= grid_cnt)
                {
                    grid_id = grid_cnt - 1;
                }

                // determine minimum confidence threshold
                min_conf = (UINT) af_pdaf_get_min_conf(pd);

                if (p_params->enable_inc_min_conf_thres_by_pd)
                {
                    if (fabs(input->pdaf_info.defocus[grid_id].defocus * input->actuator_sensitivity)
                            > p_params->pd_max_limit_inc_min_conf_thres)
                    {
                        min_conf = (UINT)((FLOAT) min_conf * p_params->inc_min_conf_thres_by_pd_scaler);
                    }
                }

                // Compute CONFIDENCE and phase difference for each PD ROI
                if (input->pdaf_info.defocus[grid_id].df_conf_level > min_conf)
                {
                    // Compute confident
                    pd->params.roi[i].conf += (INT) input->pdaf_info.defocus[grid_id].df_conf_level;
                }
            }
        }
    }

    // compute phase difference for each PD ROI
    for (i = 0; i < (INT) roi_tuning->ConfigurationCount; i++)
    {
        pd_roi = 0;
        defocus_roi = 0;
        pd->params.roi[i].is_conf = FALSE;

        for (j = 0; j < roi_tuning->Configuration[i].ROINumRows; j++)
        {
            for (k = 0; k < roi_tuning->Configuration[i].ROINumCols; k++)
            {
                grid_id = (roi_tuning->Configuration[i].ROILocationY + j) * grid_num_cols
                          + roi_tuning->Configuration[i].ROILocationX + k;

                if (grid_id < 0)
                {
                    grid_id = 0;
                }
                else if (grid_id >= grid_cnt)
                {
                    grid_id = grid_cnt - 1;
                }

                pd_grid = input->pdaf_info.defocus[grid_id].phase_diff;
                // determine minimum confidence threshold
                min_conf = (UINT) af_pdaf_get_min_conf(pd);

                if (p_params->enable_inc_min_conf_thres_by_pd == TRUE
                        && fabs(input->pdaf_info.defocus[grid_id].defocus * input->actuator_sensitivity)
                        > p_params->pd_max_limit_inc_min_conf_thres)
                {
                    min_conf = (UINT)((FLOAT) min_conf * p_params->inc_min_conf_thres_by_pd_scaler);
                }

                // Compute CONFIDENCE and phase difference for each PD ROI
                if (input->pdaf_info.defocus[grid_id].df_conf_level > min_conf)
                {
                    p_params->roi[i].is_conf = TRUE;
                    weight = (FLOAT) input->pdaf_info.defocus[grid_id].df_conf_level / (FLOAT) pd->params.roi[i].conf;
                }
                else
                {
                    weight = 0;
                }

                pd_roi += weight * pd_grid;
                defocus_roi += weight * (FLOAT) input->pdaf_info.defocus[grid_id].defocus * input->actuator_sensitivity;
                pd->params.roi[i].orig_conf = (INT) input->pdaf_info.defocus[grid_id].df_conf_level;
                pd->params.roi[i].orig_pd = (FLOAT) input->pdaf_info.defocus[grid_id].phase_diff;
                pd->params.roi[i].orig_defocus = (INT) input->pdaf_info.defocus[grid_id].defocus;
                HAF_MSG_LOW("grid(%d) pd=%0.2f, defocus(dac)=%d, conf=%d, weight=%0.2f",
                            grid_id,
                            input->pdaf_info.defocus[grid_id].phase_diff,
                            input->pdaf_info.defocus[grid_id].defocus,
                            input->pdaf_info.defocus[grid_id].df_conf_level,
                            weight);
#ifndef VENDOR_EDIT
                CAF_SCAN_MSG("CAF_SCAN_PDAF grid(%d) pd=%0.2f, defocus(dac)=%d,"
                             " conf=%d, weight=%0.2f",
                             grid_id,
                             input->pdaf_info.defocus[grid_id].phase_diff,
                             (INT )defocus_roi,
                             input->pdaf_info.defocus[grid_id].df_conf_level,
                             weight);

#else
                HAF_MSG_LOW("CAF_SCAN_PDAF grid(%d) pd=%0.2f, defocus(dac)=%d,"
                             " conf=%d, weight=%0.2f",
                             grid_id,
                             input->pdaf_info.defocus[grid_id].phase_diff,
                             (INT )defocus_roi,
                             input->pdaf_info.defocus[grid_id].df_conf_level,
                             weight);
#endif
            }
        }

        // override PD lib output
        if (p_params->enable_do_not_use_defocus_sample_by_pd == TRUE)
        {
            if (fabs(defocus_roi) > p_params->pd_max_limit_do_not_use_defocus_sample)
            {
                p_params->roi[i].is_conf = FALSE;
            }
        }

        p_params->roi[i].defocus = (INT) defocus_roi;
        p_params->roi[i].pd = pd_roi;

        /* Heuristic: If cur lens position is at near or far limit, and
         defocus is beyond that limit. then we reset defocus to 0
         */
        if ((input->lens_pos == p_params->near_limit && p_params->roi[i].defocus < 0) ||
            (input->lens_pos == p_params->far_limit && p_params->roi[i].defocus > 0))
        {
            HAF_MSG_INFO("Overwriting PDAF output, LensPos %d, Defocus %d",
              input->lens_pos, p_params->roi[i].defocus);
            p_params->roi[i].defocus = 0;
            p_params->roi[i].pd = 0;
        }

        // update defocus history
        if (p_params->roi[i].is_conf == TRUE)
        {
            p_params->roi[i].defocus_history[p_params->idx] = (INT) defocus_roi;
        }
        else
        {
            p_params->roi[i].defocus_history[p_params->idx] = p_params->roi[i].defocus_history[PD_IDX(p_params->idx - 1)];
        }
#ifdef VENDOR_EDIT
        if (TRUE == p_params->hunting_prevent.enable) {
            if (TRUE == p_params->roi[i].is_conf && p_params->roi[i].conf > 1000) {  //adjust hunting conf threshold for hunting prevent enable
                p_params->hunting_prevent.roi[i].target_pos_hist[p_params->idx].pos =
                  (int) (defocus_roi + input->lens_pos + 0.5f);
                if (p_params->hunting_prevent.roi[i].target_pos_hist[p_params->idx].pos > (int)(pd->pTuningInfo->pAlgoTuningData->searchLimit[0].farEnd) ) {
                    p_params->hunting_prevent.roi[i].target_pos_hist[p_params->idx].pos = (int)(pd->pTuningInfo->pAlgoTuningData->searchLimit[0].farEnd);
                } else if (p_params->hunting_prevent.roi[i].target_pos_hist[p_params->idx].pos < (int)(pd->pTuningInfo->pAlgoTuningData->searchLimit[0].nearEnd) ) {
                    p_params->hunting_prevent.roi[i].target_pos_hist[p_params->idx].pos = (int)(pd->pTuningInfo->pAlgoTuningData->searchLimit[0].nearEnd);
                } else {
                    //TODO
                }
                p_params->hunting_prevent.roi[i].target_pos_hist[p_params->idx].is_conf = TRUE;
            } else {
                p_params->hunting_prevent.roi[i].target_pos_hist[p_params->idx].pos = input->lens_pos;
                p_params->hunting_prevent.roi[i].target_pos_hist[p_params->idx].is_conf = FALSE;
            }
        }
#endif
        p_params->roi[i].conf = p_params->roi[i].conf
                                / (roi_tuning->Configuration[i].ROINumRows * roi_tuning->Configuration[i].ROINumCols);
        p_params->roi[i].is_soft_conf_rgn = FALSE;

        if (p_params->enable_soft_conf_thresh && pd->conv_data.status == AF_PDAF_CONVERGE_RUNNING)
        {
            min_conf = (UINT) af_pdaf_get_min_conf(pd);

            if (p_params->roi[i].conf > (INT) min_conf
                    && p_params->roi[i].conf < ((INT) min_conf + p_params->soft_conf_thresh_delta))
            {
                p_params->roi[i].is_soft_conf_rgn = TRUE;
            }
        }

        // update not conf count
        if (p_params->roi[i].is_conf == TRUE)
        {
            p_params->roi[i].not_conf_cnt = 0;
        }
        else
        {
            p_params->roi[i].not_conf_cnt++;
        }

        HAF_MSG_HIGH("roi(%d) lens_pos=%d index=%d, pd=%0.2f, defocus(um)=%d, "
                     "conf=%d, is_conf=%s, not_conf_cnt=%d, is_stable=%s",
                     i,
                     input->lens_pos,
                     p_params->idx,
                     pd_roi,
                     p_params->roi[i].defocus,
                     p_params->roi[i].conf,
                     p_params->roi[i].is_conf ? "TRUE" : "FALSE",
                     p_params->roi[i].not_conf_cnt,
                     p_params->roi[i].is_stable ? "TRUE" : "FALSE");
        HAF_MSG_LOW("roi(%d) orig_conf = %d orig_pd=%0.2f, orig_defocus(dac)=%d",
                    i,
                    p_params->roi[i].orig_conf,
                    p_params->roi[i].orig_pd,
                    p_params->roi[i].orig_defocus);
    }

    return;
}

/** af_pdaf_proc_pd:
 * Processing engine of Pdaf
 *
 * @pd: internal pd data structure
 *
 *  Return: VOID
 **/
static VOID af_pdaf_proc_pd(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_input_t* input)
{
    // increment stats index
    pd->params.prev_idx = pd->params.idx;
    pd->params.idx++;
    chromatixPDMultiwindowType* mw_tuning = &(pd->pTuningInfo->pPDAFTuningData->PDMultiWindow);

    if (pd->params.idx >= PDAF_MAX_NUM_PD_HISTORY)
    {
        pd->params.idx = 0;
    }

    // If multi-window is enabled through chromatix and Setprop

    if (pd->settingsInfo->mwEnable &&
            mw_tuning->enable &&
            AF_ROI_TYPE_GENERAL == pd->params.roi_info.type)
    {
        af_pdaf_proc_pd_mw(pd, input);
    }
    else
    {
        af_pdaf_proc_pd_single(pd, input);
    }
}

/** af_pdaf_proc_noise:
 * Estimate the noise level profile.
 *
 * @pd: internal pd data structure
 * @input : input structure of pdaf
 *  Return: INT
 **/
static VOID af_pdaf_proc_noise(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_input_t* input)
{
    UINT32 i;
    INT roi_cnt, grid_cnt, grid_num_cols, grid_num_rows;
    FLOAT norm_roi_area;
    FLOAT noise_gain;
    chromatixPDROIType* roi_tuning = &pd->pTuningInfo->pPDAFTuningData->ROI;
    grid_num_cols = (INT) input->pdaf_info.x_win_num;
    grid_num_rows = (INT) input->pdaf_info.y_win_num;
    grid_cnt = grid_num_cols * grid_num_rows;

    // compute noise gain
    for (i = 0; i < roi_tuning->ConfigurationCount; i++)
    {
        roi_cnt = roi_tuning->Configuration[i].ROINumRows * roi_tuning->Configuration[i].ROINumCols;
        norm_roi_area = (FLOAT) roi_cnt / (FLOAT) grid_cnt;
        noise_gain = (FLOAT)(20.0f * log10(input->sensor_gain));
        //noise_gain -= 20.0f * log10( norm_roi_area * (5344*4016/320/320)  );
        // noise_gain +=  20.0f * log10( lens_roll_off );  to be added later for PDAF on TAF
        pd->params.roi[i].noise_gain = noise_gain;
        HAF_MSG_LOW("roi(%d) index=%d, norm_roi_area=%0.2f, sensor_gain=%0.2f, "
                    "noise_gain = %0.2f", i, pd->params.idx, norm_roi_area, input->sensor_gain, pd->params.roi[i].noise_gain);
    }

    return;
}

/** af_pdaf_proc_depth_stable:
 * Estimate if depth value is stable.
 *
 * @pd: internal pd data structure
 *
 *  Return: INT
 **/
static VOID af_pdaf_proc_depth_stable(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_input_t* input)
{
    af_pdaf_params_t* p_params = &pd->params;
    chromatixPDAFType* p_tuning = pd->pTuningInfo->pPDAFTuningData;
    INT i, j, min_stable_cnt;
    INT defocus, defocus_max, defocus_min;
    INT lens_pos = input->lens_pos;
    min_stable_cnt = af_pdaf_get_min_stable_cnt(pd, input);

    // determine if depth is stable for given PD ROI
    for (i = 0; i < (INT) p_tuning->ROI.ConfigurationCount; i++)
    {
        defocus = p_params->roi[i].defocus_history[PD_IDX(p_params->idx)];
        defocus_max = defocus;
        defocus_min = defocus;

        for (j = 1; j < p_params->defocus_stable_filter_len; j++)
        {
            defocus = p_params->roi[i].defocus_history[PD_IDX(p_params->idx - j)];

            if (defocus_max < defocus)
            {
                defocus_max = defocus;
            }
            else if (defocus_min > defocus)
            {
                defocus_min = defocus;
            }
        }

        if (abs(defocus_max - defocus_min) < p_tuning->sceneMonitor.depthStableThreshold * af_pdaf_get_noise_multiplier(pd))
        {
            p_params->roi[i].stable_cnt++;
        }
        else
        {
            p_params->roi[i].stable_cnt = 0;
        }

        /* Heuristic: If cur lens position is at near or far limit, and
         defocus is beyond that limit. then we declare the defocus to be unstable
         */
        // perform simple boundary check
        if (lens_pos == p_params->near_limit && p_params->roi[i].defocus < 0)
        {
            p_params->roi[i].stable_cnt = 0;
        }
        else if (lens_pos == p_params->far_limit && p_params->roi[i].defocus > 0)
        {
            p_params->roi[i].stable_cnt = 0;
        }
#ifdef VENDOR_EDIT
        /**
               * when hunting_prevent enabled, we check the is_conf of previous
               * defocus_stable_filter_len pd.
               * when is_conf is false, we reset stable_cnt to 0
              **/
        if (TRUE == p_params->hunting_prevent.enable) {
            for (j = 0; j < p_params->defocus_stable_filter_len; j++) {
                if (FALSE == p_params->hunting_prevent.roi[i].target_pos_hist[PD_IDX(p_params->idx - j)].is_conf) {
                    p_params->roi[i].stable_cnt = 0;
                    break;
                }
            }
        }
#endif
        HAF_MSG_LOW("roi(%d) defocus history [%d, %d, %d], delta=%d",
                    i,
                    p_params->roi[i].defocus_history[PD_IDX(p_params->idx - 2)],
                    p_params->roi[i].defocus_history[PD_IDX(p_params->idx - 1)],
                    p_params->roi[i].defocus_history[PD_IDX(p_params->idx)],
                    (defocus_max - defocus_min));
        // set is_stable flag if condition met
        p_params->roi[i].is_stable = (p_params->roi[i].stable_cnt >= min_stable_cnt) ? TRUE : FALSE;
        HAF_MSG_LOW("roi(%d) stable_cnt=%d, min_stable_cnt=%d, is_stable= %s",
                    i,
                    p_params->roi[i].stable_cnt,
                    min_stable_cnt,
                    p_params->roi[i].is_stable ? "TRUE" : "FALSE");
    }

    return;
}

/** af_pdaf_proc_rev_dir:
 * Estimate if focus hunting is occuring
 *
 * @pd: internal pd data structure
 *
 *  Return: INT
 **/
static VOID af_pdaf_proc_rev_dir(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_input_t* input)
{
    UINT32 i;
    INT j, min_dir_stable_cnt, min_fine_step, DOF_fine_step;
    FLOAT dof_multiplier;
    INT target_pos, lens_pos;
    af_pdaf_params_t* p_params = &pd->params;
    chromatixPDAFType* pd_tuning = pd->pTuningInfo->pPDAFTuningData;
    // if defocus direction for N successive frames
    // use min stable count as N for now
    min_dir_stable_cnt = af_pdaf_get_min_stable_cnt(pd, input);
    lens_pos = input->lens_pos;
    target_pos = af_pdaf_get_target_pos(pd, input, lens_pos);
    min_fine_step = pd_tuning->focusScan.fineScanStepSize;
    dof_multiplier = af_pdaf_get_dof_multiplier(pd, target_pos);
    DOF_fine_step = (INT)((FLOAT) min_fine_step * dof_multiplier);

    if (DOF_fine_step < min_fine_step)
    {
        DOF_fine_step = min_fine_step;
    }

    for (i = 0; i < pd_tuning->ROI.ConfigurationCount; i++)
    {
        if (p_params->roi[i].is_conf == TRUE)
        {
            // increment reverse dir count if defocus sign changes
            if (p_params->roi[i].defocus_history[PD_IDX(p_params->idx)]
                    * p_params->roi[i].defocus_history[PD_IDX(p_params->idx - 1)] < 0 &&
                abs(p_params->roi[i].defocus_history[PD_IDX(p_params->idx)] - p_params->roi[i].defocus_history[PD_IDX(p_params->idx - 1)]) > VIDEO_MOVE_PCT_DROP_PD_DIF_THR)
            {
                p_params->roi[i].rev_cnt++;
            }

            if ((p_params->roi[i].defocus_history[PD_IDX(p_params->idx)]
                    * p_params->roi[i].defocus_history[PD_IDX(p_params->idx - 1)] < 0)
                    && abs(p_params->roi[i].defocus_history[PD_IDX(p_params->idx)]) > DOF_fine_step
                    && abs(p_params->roi[i].defocus_history[PD_IDX(p_params->idx - 1)]) > DOF_fine_step)
            {
                p_params->roi[i].converge_rev_cnt++;
            }
        }

        // reset reverse dir count only if PD sign is persistent by min stable count
        for (j = 0;
                j < min_dir_stable_cnt
                && p_params->roi[i].defocus_history[PD_IDX(p_params->idx - j)]
                * p_params->roi[i].defocus_history[PD_IDX(p_params->idx - j - 1)] >= 0;
                j++)
            ;

        if (j >= min_dir_stable_cnt)
        {
            p_params->roi[i].rev_cnt = 0;
        }

        HAF_MSG_INFO("roi(%d) defocus=%d, rev_cnt=%d, converge_rev_cnt=%d, min_dir_stable_cnt=%d",
                    i,
                    p_params->roi[i].defocus,
                    p_params->roi[i].rev_cnt,
                    p_params->roi[i].converge_rev_cnt,
                    min_dir_stable_cnt); //VENDOR_EDIT
    }
}

/** af_pdaf_proc_defocused:
 * Estimate if depth value is stable.
 *
 * @pd: internal pd data structure
 *
 *  Return: INT
 **/
static VOID af_pdaf_proc_defocused(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_input_t* input)
{
    af_pdaf_params_t* p_params = &pd->params;
    FLOAT min_defocus_thres, dof_multiplier, noise_multiplier, defocus_thres;
    UINT32 i;
    INT min_defocus_cnt;
    chromatixPDAFType* pd_tuning = pd->pTuningInfo->pPDAFTuningData;
    // adjust defocus threshold by noise estimate
    min_defocus_thres = pd_tuning->sceneMonitor.defocusedThreshold;
    dof_multiplier = af_pdaf_get_dof_multiplier(pd, pd->input.lens_pos);
    noise_multiplier = af_pdaf_get_noise_multiplier(pd);
    defocus_thres = min_defocus_thres * MAX(dof_multiplier, noise_multiplier);

    if (defocus_thres < min_defocus_thres)
    {
        defocus_thres = min_defocus_thres;
    }

    // further adjust defocus threshold by is_prev_focus_conv_failed
    if (p_params->is_prev_focus_conv_failed == TRUE)
    {
        defocus_thres *= p_params->inc_defocus_thres_prev_error;
    }

    for (i = 0; i < pd_tuning->ROI.ConfigurationCount; i++)
    {
        // skip defocuse analystics if defocus not confident
        if (p_params->roi[i].is_conf == TRUE)
        {
            if (abs(p_params->roi[i].defocus) < defocus_thres || p_params->roi[i].rev_cnt > 0)
            {
                p_params->roi[i].defocus_cnt = 0;
            }
            else
            {
                p_params->roi[i].defocus_cnt++;
            }
        }

        // defocus count must satisfy min stability count
        // min defocus count may be pulled as separate tuning params later
        min_defocus_cnt = af_pdaf_get_min_stable_cnt(pd, input);
        p_params->roi[i].is_defocused = (p_params->roi[i].defocus_cnt >= min_defocus_cnt) ?
                                        TRUE :
                                        FALSE;
        HAF_MSG_HIGH("roi(%d) defocus=%d, dof_mult=%1.1f, defocus_thres=%1.1f, "
                     "defocus_cnt=%d, min_defocus_cnt=%d, is_defocused=%s",
                     i,
                     p_params->roi[i].defocus,
                     dof_multiplier,
                     defocus_thres,
                     p_params->roi[i].defocus_cnt,
                     min_defocus_cnt,
                     p_params->roi[i].is_defocused ? "TRUE" : "FALSE");
    }
}

/** af_pdaf_is_valid_roi_to_run_pd:
 * check if defocus and confidence is enough
 * to run PDAF in Touch/Face AF
 *
 * @pd: internal pd data structure
 *
 * @return BOOL
 **/
BOOL af_pdaf_is_valid_roi_to_run_pd(
    af_alg_PDAF_internal_t* pd)
{
    af_pdaf_params_t* p_params = &pd->params;
    chromatixPDAFType* pPDAFTuningData = pd->pTuningInfo->pPDAFTuningData;
    INT focus_roi, confidence;

    if (pd->haf_roi_enable == FALSE)
    {
        return FALSE;
    }

    // Get PD information
    focus_roi = p_params->focus_roi_id;
    confidence = p_params->roi[focus_roi].conf;
    // Check if confidence and defocus are higher than thresholds
    HAF_MSG_LOW("conf %d conf_thres %d", confidence, pPDAFTuningData->sparsePDTouchFaceConfidenceThreshold);

    if (confidence > pPDAFTuningData->sparsePDTouchFaceConfidenceThreshold)
    {
        return TRUE;
    }

    return FALSE;
}

/** af_pdaf_calculate_fallback_opt:
 * Get fallback option from PDAF.
 *
 * @haf: internal Haf data structure
 *
 *  Return: VOID
 **/
static VOID af_pdaf_calculate_fallback_opt(
    af_alg_PDAF_internal_t* pd)
{
    HAF_MSG_LOW("Enter %s!", __func__);
    af_pdaf_params_t* p_params = &pd->params;
    af_pdaf_fallback_opt_t* opt = &p_params->fallback_opt_info;
    chromatixPDAFType* pPDAFTuningData = pd->pTuningInfo->pPDAFTuningData;
    INT focus_roi, defocus, confidence;
    INT move_step;
    BOOL is_conf_high, is_defocus_high;
    INT cur_pos = pd->cur_pos;
    INT min_conf;
    FLOAT focus_pcnt, noise_multiplier;
    FLOAT min_defocus_thres, dof_multiplier, defocus_thres;
    /* adjust defocus threshold by noise estimate */
    min_defocus_thres = pPDAFTuningData->sceneMonitor.defocusedThreshold;
    dof_multiplier = af_pdaf_get_dof_multiplier(pd, pd->input.lens_pos);
    noise_multiplier = af_pdaf_get_noise_multiplier(pd);
    /* Scale Defocus base of max of DOF and Noise*/
    defocus_thres = min_defocus_thres * MAX(dof_multiplier, noise_multiplier);

    if (defocus_thres < min_defocus_thres)
    {
        defocus_thres = min_defocus_thres;
    }

    if (TRUE == af_pdaf_is_running_2PD(pd))
    {
        min_conf = (int) pPDAFTuningData->fallbackConfidence;
    }
    else
    {
        min_conf = af_pdaf_get_min_conf(pd);
    }

    /* Get PD information */
    focus_roi = p_params->focus_roi_id;
    defocus = p_params->roi[focus_roi].defocus;
    confidence = p_params->roi[focus_roi].conf;
    focus_pcnt = af_pdaf_get_focus_pcnt(pd, &(pd->input), defocus);
    move_step = af_pdaf_get_focus_move_steps(pd, focus_pcnt);
    /* Check if confidence and defocus are higher than thresholds */
    is_defocus_high = abs(defocus) > defocus_thres;
    is_conf_high = confidence > min_conf;
    HAF_MSG_LOW("PD-OPT: Is_high_conf %d is_defocus_high %d conf %d defocus %d min_conf %d defocus_thres %f",
                is_conf_high, is_defocus_high, confidence, defocus, min_conf, defocus_thres);
    HAF_MSG_LOW("focus_pcnt %f move_step %d near_limit %d far_limit %d",
                focus_pcnt, move_step, p_params->near_limit, p_params->far_limit);

    /* Check if optimization can be provided */
    if (is_conf_high)
    {
        opt->search_dir = (defocus > 0) ? AF_MOVE_FAR : AF_MOVE_NEAR;
        opt->search_range_near_pos = p_params->near_limit;
        opt->search_range_far_pos = p_params->far_limit;

        if (is_defocus_high)
        {
            opt->start_pos = cur_pos + move_step;
        }
        else
        {
            opt->start_pos = cur_pos;
        }
    }
    else
    {
        opt->search_dir = AF_MOVE_INVALID;
        opt->start_pos = cur_pos;
        opt->search_range_near_pos = p_params->near_limit;
        opt->search_range_far_pos = p_params->far_limit;
    }

    /* Boundary check */
    if (opt->start_pos > p_params->far_limit)
        opt->start_pos = p_params->far_limit;
    else if (opt->start_pos < p_params->near_limit)
        opt->start_pos = p_params->near_limit;

    if (opt->search_range_far_pos > p_params->far_limit)
        opt->search_range_far_pos = p_params->far_limit;
    else if (opt->search_range_far_pos < p_params->near_limit)
        opt->search_range_far_pos = p_params->near_limit;

    if (opt->search_range_near_pos > p_params->far_limit)
        opt->search_range_near_pos = p_params->far_limit;
    else if (opt->search_range_near_pos < p_params->near_limit)
        opt->search_range_near_pos = p_params->near_limit;

    HAF_MSG_LOW(" PD-OPT: dir %d start_pos %d cur %d near %d far %d)",
                opt->search_dir, opt->start_pos, cur_pos,
                opt->search_range_near_pos, opt->search_range_far_pos);
    return;
}

static BOOL af_pdaf_face_change_refocus_trigger(
    af_alg_PDAF_internal_t* pd)
{
    BOOL need_face_refocus = FALSE;
    chromatixPDAFType* pPDAFTuningData = pd->pTuningInfo->pPDAFTuningData;

    if (pPDAFTuningData->face.faceChangeRefocusTrigger.enable)
    {
        HAF_MSG_LOW("Face Sparse PD: Cur face ROI x,y,dx,dy = [%d, %d, %d, %d], ref = [%d, %d, %d, %d]",
                    pd->params.roi_info.roi[pd->params.focus_roi_id].x,
                    pd->params.roi_info.roi[pd->params.focus_roi_id].y,
                    pd->params.roi_info.roi[pd->params.focus_roi_id].dx,
                    pd->params.roi_info.roi[pd->params.focus_roi_id].dy,
                    pd->params.ref_face_roi_info.roi[pd->params.focus_roi_id].x,
                    pd->params.ref_face_roi_info.roi[pd->params.focus_roi_id].y,
                    pd->params.ref_face_roi_info.roi[pd->params.focus_roi_id].dx,
                    pd->params.ref_face_roi_info.roi[pd->params.focus_roi_id].dy);
        // delta_ratio = (cur_dx - ref_dx) / min(cur_width, ref_width)
        FLOAT width_delta_ratio =
            (FLOAT)ABS((INT)pd->params.roi_info.roi[pd->params.focus_roi_id].dx -
                       (INT)pd->params.ref_face_roi_info.roi[pd->params.focus_roi_id].dx) /
            (FLOAT)MIN(pd->params.roi_info.roi[pd->params.focus_roi_id].dx,
                       pd->params.ref_face_roi_info.roi[pd->params.focus_roi_id].dx);
        FLOAT height_delta_ratio =
            (FLOAT)ABS((INT)pd->params.roi_info.roi[pd->params.focus_roi_id].dy -
                       (INT)pd->params.ref_face_roi_info.roi[pd->params.focus_roi_id].dy) /
            (FLOAT)MIN(pd->params.roi_info.roi[pd->params.focus_roi_id].dy,
                       pd->params.ref_face_roi_info.roi[pd->params.focus_roi_id].dy);
        HAF_MSG_LOW("Face Sparse PD: face refocus width_delta_ratio = %f, height_delta_ratio = %f",
            width_delta_ratio, height_delta_ratio);

        /* Critiera: if face ROI delta width or height is greater than threshold, report refocus */
        if (width_delta_ratio > pPDAFTuningData->face.faceChangeRefocusTrigger.roiChangeThreshPercentage||
            height_delta_ratio > pPDAFTuningData->face.faceChangeRefocusTrigger.roiChangeThreshPercentage)
        {
            if (af_pdaf_face_roi_is_stable(pd) == TRUE)
            {
                /* Face is stable, send face refocus trigger */
                HAF_MSG_LOW("Face Sparse PD: face refocus trigger! w %f > %f || h %f > %f",
                            width_delta_ratio,
                            pPDAFTuningData->face.faceChangeRefocusTrigger.roiChangeThreshPercentage,
                            height_delta_ratio,
                            pPDAFTuningData->face.faceChangeRefocusTrigger.roiChangeThreshPercentage);
                need_face_refocus = TRUE;
            }
            else
            {
                HAF_MSG_LOW("Face Sparse PD: face size change but yet stable! width_delta_ratio = %f, height_delta_ratio = %f"
                            , width_delta_ratio, height_delta_ratio);
            }
        }
        else
        {
            /* Do nothing */
            HAF_MSG_LOW("Face Sparse PD: No face size change");
        }
    }

    return need_face_refocus;
}

static BOOL af_pdaf_face_roi_is_stable(
    af_alg_PDAF_internal_t* pd)
{
    boolean result = FALSE;
    chromatixPDAFType* pPDAFTuningData = pd->pTuningInfo->pPDAFTuningData;
    /* Calculate face stable by delta between current face ROI and ref face ROI */
    INT width_delta_versus_last_frame =
        ABS((INT)pd->params.roi_info.roi[pd->params.focus_roi_id].dx -
            (INT)pd->params.prev_roi_info.roi[pd->params.focus_roi_id].dx);
    INT height_delta_versus_last_frame =
        ABS((INT)pd->params.roi_info.roi[pd->params.focus_roi_id].dy -
            (INT)pd->params.prev_roi_info.roi[pd->params.focus_roi_id].dy);
    INT width_coord_versus_last_frame =
        ABS((INT)pd->params.roi_info.roi[pd->params.focus_roi_id].x -
            (INT)pd->params.prev_roi_info.roi[pd->params.focus_roi_id].x);
     INT height_coord_versus_last_frame =
        ABS((INT)pd->params.roi_info.roi[pd->params.focus_roi_id].y -
            (INT)pd->params.prev_roi_info.roi[pd->params.focus_roi_id].y);

    if (width_delta_versus_last_frame < (INT)pPDAFTuningData->face.faceChangeRefocusTrigger.roiSizeStableThresh &&
        height_delta_versus_last_frame < (INT)pPDAFTuningData->face.faceChangeRefocusTrigger.roiSizeStableThresh &&
        width_coord_versus_last_frame <= (INT)pPDAFTuningData->face.faceChangeRefocusTrigger.roiSizeStableThresh &&
        height_coord_versus_last_frame <= (INT)pPDAFTuningData->face.faceChangeRefocusTrigger.roiSizeStableThresh)
    {
        result = TRUE;
    }
    else
    {
        result = FALSE;
    }

    HAF_MSG_LOW("Face Sparse PD: face ROI is stable = %d, width_delta = %d, height_delta = %d", result,
                width_delta_versus_last_frame, height_delta_versus_last_frame);
    return result;
}

BOOL af_pdaf_face_tracking_move_suppression_check(
    af_alg_PDAF_internal_t* pd)
{
    BOOL need_suppress = FALSE;
    chromatixPDAFType* pPDAFTuningData = pd->pTuningInfo->pPDAFTuningData;

    if (TRUE == pPDAFTuningData->face.ROIMoveConvergeSuppress.enable)
    {
        HAF_MSG_LOW("Face tracking PD: roi_moving_rate x and y [%f , %f]",
                    pd->params.roi_info.roi_moving_rate_x,
                    pd->params.roi_info.roi_moving_rate_y);

        if (pd->params.roi_info.roi_moving_rate_x > pPDAFTuningData->face.ROIMoveConvergeSuppress.moveThresholdInX
                || pd->params.roi_info.roi_moving_rate_y
                > pPDAFTuningData->face.ROIMoveConvergeSuppress.moveThresholdInY)
        {
            // Skip focus converge of current frame if face is moving
            HAF_MSG_LOW("Face tracking PD: skip focus converge as ROI is moving more than stable threshold");
            pd->params.face_stable_counter = 0;
            need_suppress = TRUE;
        }
        else
        {
            pd->params.face_stable_counter++;

            if (pd->params.face_stable_counter < pPDAFTuningData->face.ROIMoveConvergeSuppress.stableCountThreshold)
            {
                HAF_MSG_LOW("Face tracking PD: skip focus converge stable counter = %d < %d",
                            pd->params.face_stable_counter,
                            pPDAFTuningData->face.ROIMoveConvergeSuppress.stableCountThreshold);
                // Still suppress untill stable counter is over the threshold
                need_suppress = TRUE;
            }
        }
    }

    return need_suppress;
}

/** af_pdaf_get_roi_to_center_distance
 * Calculate and return the distance of current ROI center to CAMIF center in terms of CAMIF size percentage
 *
 *  Return: FLOAT
 **/
static FLOAT af_pdaf_get_roi_to_center_distance(
    af_alg_PDAF_internal_t* pd)
{
    UINT roi_ctr_x = pd->params.roi_info.roi[0].x + (UINT16) (pd->params.roi_info.roi[0].dx / 2);
    UINT roi_ctr_y = pd->params.roi_info.roi[0].y + (UINT16) (pd->params.roi_info.roi[0].dy / 2);
    UINT16 camif_ctr_x = (UINT16) (pd->params.sensor_info.sensor_res_width / 2.0f);
    UINT16 camif_ctr_y = (UINT16) (pd->params.sensor_info.sensor_res_height / 2.0f);
    UINT16 dist_x = (UINT16) abs((INT)(roi_ctr_x - camif_ctr_x));
    UINT16 dist_y = (UINT16) abs((INT)(roi_ctr_y - camif_ctr_y));
    FLOAT roi_dist_to_ctr = (FLOAT)(sqrt((double) (dist_x * dist_x + dist_y * dist_y))
                                    / sqrt((double) (camif_ctr_x * camif_ctr_x + camif_ctr_y * camif_ctr_y)));
    AF_MSG_LOW("Distance ratio between touch ROI and center : %f", roi_dist_to_ctr);
    return roi_dist_to_ctr;
}

/** af_pdaf_2pd_is_scene_focused
 * Check if scene is focused and confident
 *
 *  Return: BOOL
 **/
BOOL af_pdaf_is_scene_focused(
    af_alg_PDAF_internal_t* pd)
{
    HAF_MSG_LOW("Enter");
    af_pdaf_params_t* p_params = &pd->params;
    FLOAT noise_multiplier, dof_multiplier;
    INT min_focus_done_thres, focus_done_thresh;
    INT defocus, focus_roi_id, confidence;
    BOOL is_roi_conf  = FALSE;
    BOOL is_focus_converge_done = FALSE;
    focus_roi_id = p_params->focus_roi_id;
    defocus = p_params->roi[focus_roi_id].defocus;
    confidence = p_params->roi[focus_roi_id].conf;
    chromatixPDAFType* p_tuning = pd->pTuningInfo->pPDAFTuningData;

    if (af_pdaf_is_running_2PD(pd))
    {
        is_roi_conf = (boolean) p_params->roi[focus_roi_id].is_conf;
    }
    else
    {
        is_roi_conf = confidence > p_tuning->sparsePDTouchFaceLowDefocusConfidenceThreshold;
    }

    min_focus_done_thres = p_tuning->focusScan.focusDoneThreshold;
    dof_multiplier = af_pdaf_get_dof_multiplier(pd, pd->input.lens_pos);
    noise_multiplier = af_pdaf_get_noise_multiplier(pd);
    focus_done_thresh = (INT)((FLOAT) min_focus_done_thres * dof_multiplier * noise_multiplier + 0.5f);

    if (focus_done_thresh < min_focus_done_thres)
    {
        focus_done_thresh = min_focus_done_thres;
    }

    if (is_roi_conf == TRUE && abs(defocus) < focus_done_thresh)
    {
        is_focus_converge_done = TRUE;
    }

    AF_MSG_LOW("is_conf %x defocus %d conf %d focus_thres %d conf_thres %d",
               is_roi_conf, defocus, confidence, focus_done_thresh,
               p_tuning->sparsePDTouchFaceLowDefocusConfidenceThreshold);
    return is_focus_converge_done;
}

static BOOL af_pdaf_set_sensitivity(
    af_alg_interface_t* p_alg,
    HAF_set_sensitivity_type sensitivity)
{
    // Typecast from af_alg_interface_t to af_alg_pdaf_internal_t
    af_alg_PDAF_internal_t* pd = (af_alg_PDAF_internal_t*) p_alg;
    INT updated_sens_val = 0;

    // set sensitivity based on the topology_index
    switch (pd->topology_index)
    {
        case chromatixTopoPDAFEnum::PDAFTopoProfileSparse:
        case chromatixTopoPDAFEnum::PDAFTopoProfile2PD:
        case chromatixTopoPDAFEnum::PDAFTopoProfileAdvanced:
        case chromatixTopoPDAFEnum::PDAFTopoProfileSparseAdvanced:
        case chromatixTopoPDAFEnum::PDAFTopoProfile2PDAdvanced:
        case chromatixTopoPDAFEnum::PDAFTOPOProfileMAX:
            /* update value monitor sensitivities:
             *  (1) assume 50 is the default sensitivity from the mixer layer */
            updated_sens_val = af_pdaf_check_sensitivity_range(sensitivity);
            pd->p_vm_defocus_low->set_sensitivity(pd->p_vm_defocus_low, updated_sens_val);
            HAF_MSG_LOW("setting PDAF-defocus_low sensitivity to %d", updated_sens_val);
            pd->p_vm_defocus_high->set_sensitivity(pd->p_vm_defocus_high, updated_sens_val);
            HAF_MSG_LOW("setting PDAF-defocus_high sensitivity to %d", updated_sens_val);
            pd->p_vm_sad_panning->set_sensitivity(pd->p_vm_sad_panning, updated_sens_val);
            HAF_MSG_LOW("setting PDAF-sad_panning sensitivity to %d", updated_sens_val);
            pd->p_vm_gyro_panning->set_sensitivity(pd->p_vm_gyro_panning, updated_sens_val);
            HAF_MSG_LOW("setting PDAF-gyro_panning sensitivity to %d", updated_sens_val);
            break;

        default: // if new topology is utilized, creating new case is required
            AF_MSG_ERROR("No matched PDAF Topology Index!");
            break;
    }

    return TRUE;
}

/** af_pdaf_check_sensitivity_range: ensures the sensitivity range is within [1-100]
 *
 * Return INT
 **/
static INT af_pdaf_check_sensitivity_range(
    INT sensitivity)
{
    INT sens_val = sensitivity;

    if (sens_val > 100)
    {
        sens_val = 100;
    }
    else if (sens_val < 1)
    {
        sens_val = 1;
    }

    return sens_val;
}

static VOID af_pdaf_mw_massage_pd_data(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_roi_t* roi,
    INT size)
{
    af_pdaf_params_t* p_params = &pd->params;
    chromatixPDAFType* pPDAFTuning = pd->pTuningInfo->pPDAFTuningData;
    UINT min_conf;
    FLOAT defocus = 0.0;
    INT grid_id = 0;

    for (grid_id = 0; grid_id < size; grid_id++)
    {
        // determine minimum confidence threshold
        min_conf = (UINT) af_pdaf_get_min_conf(pd);

        // Check if to increase minimum conf threshold based on defocus value
        if (p_params->enable_inc_min_conf_thres_by_pd)
        {
            if (abs(roi[grid_id].defocus) > p_params->pd_max_limit_inc_min_conf_thres)
            {
                min_conf = (UINT)((FLOAT) min_conf * p_params->inc_min_conf_thres_by_pd_scaler);
            }
        }

        min_conf = (UINT)((FLOAT)min_conf * pPDAFTuning->PDMultiWindow.confidenceThMultiple);
        HAF_MSG_LOW("min_conf: %u enable_inc_min_conf_thres_by_pd: %d"
                    "pd_max_limit_inc_min_conf_thres: %f ",
                    min_conf,
                    p_params->enable_inc_min_conf_thres_by_pd,
                    p_params->pd_max_limit_inc_min_conf_thres);

        if (roi[grid_id].conf > (INT) min_conf)
        {
            roi[grid_id].is_conf = TRUE;
        }
        else
        {
            roi[grid_id].is_conf = FALSE;
        }

        if (p_params->enable_do_not_use_defocus_sample_by_pd == TRUE)
        {
            if (abs(roi[grid_id].defocus) > p_params->pd_max_limit_do_not_use_defocus_sample)
            {
                HAF_MSG_LOW("Defocus %f is beyond threshold: %d! Set conf to FALSE!",
                            defocus,
                            p_params->pd_max_limit_do_not_use_defocus_sample);
                roi[grid_id].is_conf = FALSE;
            }
        }

        // If soft confidence region is enabled
        roi[grid_id].is_soft_conf_rgn = FALSE;

        if (p_params->enable_soft_conf_thresh)
        {
            if ((roi[grid_id].conf > (INT) min_conf)
                    && (roi[grid_id].conf < ((INT) min_conf + p_params->soft_conf_thresh_delta)))
            {
                roi[grid_id].is_soft_conf_rgn = TRUE;
            }
        }

        if (TRUE == roi[grid_id].is_conf)
        {
            roi[grid_id].not_conf_cnt = 0;
        }
        else
        {
            roi[grid_id].not_conf_cnt++;
        }
    }
}

/** af_pdaf_mw_proc_pd_windows:
 * Process input multi-window PD stats windows
 *
 * @pd: internal data structure
 * @input: input pd data
 * @roi_out: output pd data
 *
 *  Return: TRUE on success FALSE on failure
 **/
static BOOL af_pdaf_mw_proc_pd_windows(
    af_alg_PDAF_internal_t* pd,
    pdaf_defocus_t* defocus,
    af_pdaf_roi_t* roi_out,
    INT grid_count)
{
    INT i = 0;

    for (i = 0; i < grid_count; i++)
    {
        // Save grid id for future reference
        roi_out[i].id = i;
        roi_out[i].conf = (INT) defocus[i].df_conf_level;
        roi_out[i].pd = defocus[i].phase_diff;
        roi_out[i].defocus = (INT)(defocus[i].defocus * pd->input.actuator_sensitivity);
        roi_out[i].is_conf = (0 == defocus[i].df_confidence) ? TRUE : FALSE;
        HAF_MSG_LOW("Grid-id: %d pd=%0.2f defocus(um)=%d conf=%d is_conf=%d",
                    i,
                    roi_out[i].pd,
                    roi_out[i].defocus,
                    roi_out[i].conf,
                    roi_out[i].is_conf);
    }

    // Massage pd data based on different parameters
    af_pdaf_mw_massage_pd_data(pd, roi_out, i);

    for (i = 0; i < grid_count; i++)
    {
        HAF_MSG_LOW("Grid-id: %d pd=%0.2f defocus(um)=%d conf=%d is_conf=%d",
                    roi_out[i].id,
                    roi_out[i].pd,
                    roi_out[i].defocus,
                    roi_out[i].conf,
                    roi_out[i].is_conf);
    }

    return TRUE;
}

/** af_pdaf_mw_sanitize_windows:
 * Sanitize input PD grids to get valid set of
 * data to run multi-window logic
 *
 * @pd: internal data structure
 * @data: final output data to run multi-window algorithm
 * @grid_count: number of grids
 * @size: size of the output pd data set
 *
 *  Return: None
 **/
static VOID af_pdaf_mw_sanitize_windows(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_roi_t* data,
    INT grid_count,
    INT* size)
{
    INT i = 0;
    FLOAT defocus_th = 0.0, dof_multiplier = 0.0, min_defocus_th = 0.0;
    INT size_out = 0;
    int sample_size = 0;
    chromatixPDAFType* pPDTuning = pd->pTuningInfo->pPDAFTuningData;
    // calculate defocus threshold
    min_defocus_th = pd->pTuningInfo->pPDAFTuningData->sceneMonitor.defocusedThreshold;
    dof_multiplier = af_pdaf_get_dof_multiplier(pd, pd->input.lens_pos);
    defocus_th = min_defocus_th * dof_multiplier;
    HAF_MSG_LOW("min_defocus_th: %f dof_multiplier: %f defocus_th: %f", min_defocus_th, dof_multiplier, defocus_th);

    for (i = 0; i < grid_count; i++)
    {
        if (TRUE == data[i].is_conf)
        {
            data[size_out] = data[i];
            HAF_MSG_LOW("grid_id: %d defocus: %d defocus_th: %f is_conf: %d ",
                        data[size_out].id,
                        data[size_out].defocus,
                        defocus_th,
                        data[size_out].is_conf);
            size_out++;
        }
    }

    /* sample_size is read from chromatix which is in terms
       of percentile */
    sample_size = (int) ((pPDTuning->PDMultiWindow.PDPercentile * grid_count) + 0.5);
    sample_size = MIN(grid_count, sample_size);
    /* Just using the Nth percentile window */
    sample_size = MAX(0, sample_size - 1);

    if (size_out >= sample_size)
    {
        *size = size_out;
    }
    else
    {
        *size = 0;
    }
}

/** af_pdaf_mw_sanitize_peripheral_windows:
 * Sanitize input PD grids to get valid set of
 * data to run multi-window logic
 *
 * @pd: internal data structure
 * @data: final output data to run multi-window algorithm
 * @grid_count: number of grids
 * @size: size of the output pd data set
 *
 *  Return: None
 **/
static void af_pdaf_mw_sanitize_peripheral_windows(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_roi_t* data,
    INT grid_count,
    INT* size)
{
    INT i = 0;
    FLOAT defocus_th = 0.0f, dof_multiplier = 0.0f, min_defocus_th = 0.0f;
    INT size_out = 0;
    /* calculate defocus threshold */
    min_defocus_th = pd->pTuningInfo->pPDAFTuningData->sceneMonitor.defocusedThreshold;
    dof_multiplier = af_pdaf_get_dof_multiplier(pd, pd->input.lens_pos);
    defocus_th = min_defocus_th * dof_multiplier;
    HAF_MSG_LOW("min_defocus_th: %f dof_multiplier: %f defocus_th: %f",
                min_defocus_th, dof_multiplier, defocus_th);

    for (i = 0; i < grid_count; i++)
    {
        if (TRUE == data[i].is_conf)
        {
            data[size_out] = data[i];
            HAF_MSG_LOW("grid_id: %d defocus: %d defocus_th: %f is_conf: %d ",
                        data[size_out].id, data[size_out].defocus, defocus_th,
                        data[size_out].is_conf);
            size_out++;
        }
    }

    *size = size_out;
}

/** af_pdaf_mw_sort_windows:
 * Sort pdaf windows in increasing order.
 *
 * @roi: data set to run multi-window algorithm
 * @size: size of the  pd data set
 *
 *  Return: None
 **/
static VOID af_pdaf_mw_sort_windows(
    af_pdaf_roi_t* data,
    INT size)
{
    af_pdaf_roi_t temp;
    INT i = 0, j = 0;

    /* Use insertion sort to sort the input data.
     For smaller dataset, insertion sort gives a comparably good
     performance. */

    for (i = 1; i < size; i++)
    {
        temp = data[i];

        for (j = i - 1; (j >= 0) && (data[j].defocus > temp.defocus); j--)
        {
            data[j + 1] = data[j];
        }

        data[j + 1] = temp;
    }

    for (i = 0; i < size; i++)
    {
        HAF_MSG_HIGH("grid_id: %d defocus: %d is_conf: %d conf: %d",
                     data[i].id, data[i].defocus, data[i].is_conf, data[i].conf);
    }
}

static void af_pdaf_mw_sort_peripheral_windows(
    af_pdaf_roi_t* data,
    INT size)
{
    af_pdaf_roi_t temp;
    INT i = 0, j = 0;

    /* After a lot of testing, for peripheral we need to choose
       the window with best or high confidence, choosing closest
       or farthest can cause hunting giving rise to tug of war
       kind of scenario*/

    for (i = 1; i < size; i++)
    {
        temp = data[i];

        for (j = i - 1; (j >= 0) && (data[j].conf < temp.conf); j--)
        {
            data[j + 1] = data[j];
        }

        data[j + 1] = temp;
    }

    for (i = 0; i < size; i++)
    {
        HAF_MSG_HIGH("grid_id: %d defocus: %d is_conf: %d conf: %d",
                     data[i].id, data[i].defocus, data[i].is_conf,
                     data[i].conf);
    }
}


/** af_pdaf_mw_select_best_window:
 * Select the best window to focus on.
 *
 * @roi: data set to run multi-window algorithm
 * @size: size of the  pd data set
 *
 *  Return: id of the ROI to focus on
 **/
static INT af_pdaf_mw_select_best_window(
    af_alg_PDAF_internal_t* af,
    af_pdaf_roi_t* data,
    INT size)
{
    INT index = 0;
    INT grid_count = 0;
    INT sample_size = 0;
    chromatixPDMultiwindowType* mw_tuning = &af->pTuningInfo->pPDAFTuningData->PDMultiWindow;

    // If only there's total more than one element
    if (size > 1)
    {
        // Get sample size to calcuate median of
        grid_count = (INT)(af->input.pdaf_info.x_win_num * af->input.pdaf_info.y_win_num);
        /* sample_size is read from chromatix which is in terms
         of percentile */
        sample_size = (INT)((mw_tuning->PDPercentile * grid_count) + 0.5);
        sample_size = MIN(size, sample_size);
        // Just using the Nth percentile window
        index = MAX(0, sample_size - 1);
        HAF_MSG_LOW("Sample Size: %d index: %d Selected grid: %d", sample_size, index, data[index].id);
    }

    return index;
}

/** af_pdaf_mw_collect_similar_windows:
 * Get windows with similar characteristics as the one selected
 *
 * @pd: internal data structure
 * @input: list of good ROIs
 * @size: size of roi
 *  @active_index: index of the best ROI
 *  Return: None
 **/
static VOID af_pdaf_mw_collect_similar_windows(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_roi_t* data,
    INT size,
    INT active_index)
{
    INT i = 0;
    /* start from 1st index. 0th is always the best ROI */
    INT index = 1;
    //pd->mw_roi_info.type = PDAF_MW_CENTRAL;
    chromatixPDMultiwindowType* mw_tuning = &pd->pTuningInfo->pPDAFTuningData->PDMultiWindow;
    // save the best one first
    pd->mw_roi_info.roi_index[0] = data[active_index].id;

    for (i = 0; i < size; i++)
    {
        if ((i != active_index) && (abs(data[active_index].defocus - data[i].defocus) < mw_tuning->defocusThreshold))
        {
            // start from 1st index. 0th is always the best ROI
            pd->mw_roi_info.roi_index[index++] = data[i].id;
        }
    }

    pd->mw_roi_info.selected_win_count = index;
}

/** af_pdaf_mw_update_active_roi:
 * Update active roi to outside world
 *
 * @pd: internal data structure
 * @active_roi: roi info to update
 *
 *  Return: None
 **/
static VOID af_pdaf_mw_update_active_roi(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_roi_t* active_roi)
{
    af_pdaf_params_t* p_params = &pd->params;

    if (NULL != active_roi)
    {
        p_params->roi[0].id = active_roi->id;
        p_params->roi[0].conf = active_roi->conf;
        p_params->roi[0].defocus = active_roi->defocus;
        p_params->roi[0].is_conf = active_roi->is_conf;
        p_params->roi[0].pd = active_roi->pd;
        p_params->roi[0].not_conf_cnt = 0;
        p_params->roi[0].defocus_history[p_params->idx] = active_roi->defocus;
    }
    else
    {
        p_params->roi[0].not_conf_cnt++;
        p_params->roi[0].is_conf = FALSE;
        p_params->roi[0].conf = 0;
        p_params->roi[0].defocus_history[p_params->idx] = p_params->roi[0].defocus_history[PD_IDX(p_params->idx - 1)];
    }
}


static boolean af_pdaf_mw_proc_pd_single_window(
    af_alg_PDAF_internal_t* pd,
    pdaf_defocus_t* defocus,
    af_pdaf_roi_t* roi_out,
    INT grid_id)
{
    af_pdaf_params_t* p_params = &pd->params;
    UINT min_conf;
    roi_out[grid_id].id = grid_id;
    roi_out[grid_id].conf = (INT) defocus[grid_id].df_conf_level;
    roi_out[grid_id].pd  = defocus[grid_id].phase_diff;
    roi_out[grid_id].defocus = (INT)(defocus[grid_id].defocus * pd->input.actuator_sensitivity);
    roi_out[grid_id].is_conf =
        (0 == defocus[grid_id].df_confidence) ? TRUE : FALSE;
    /* determine minimum confidence threshold */
    min_conf = (unsigned int) af_pdaf_get_min_conf(pd);

    /* Check if to increase minimum conf threshold based on defocus value */
    if (p_params->enable_inc_min_conf_thres_by_pd)
    {
        if (abs(roi_out[grid_id].defocus) >
                p_params->pd_max_limit_inc_min_conf_thres)
        {
            min_conf = (UINT) ((float) min_conf
                               * p_params->inc_min_conf_thres_by_pd_scaler);
        }
    }

    HAF_MSG_LOW("min_conf: %u enable_inc_min_conf_thres_by_pd: %d"
                "pd_max_limit_inc_min_conf_thres: %f ",
                min_conf, p_params->enable_inc_min_conf_thres_by_pd,
                p_params->pd_max_limit_inc_min_conf_thres);

    if (roi_out[grid_id].conf > (int) min_conf)
    {
        roi_out[grid_id].is_conf = TRUE;
    }
    else
    {
        roi_out[grid_id].is_conf = FALSE;
    }

    if (p_params->enable_do_not_use_defocus_sample_by_pd == TRUE)
    {
        if (abs(roi_out[grid_id].defocus) >
                p_params->pd_max_limit_do_not_use_defocus_sample)
        {
            HAF_MSG_LOW("Defocus %f is beyond threshold: %d! Set conf to FALSE!",
                        defocus, p_params->pd_max_limit_do_not_use_defocus_sample);
            roi_out[grid_id].is_conf = FALSE;
        }
    }

    /* If soft confidence region is enabled */
    roi_out[grid_id].is_soft_conf_rgn = FALSE;

    if (p_params->enable_soft_conf_thresh)
    {
        if ((roi_out[grid_id].conf > (int) min_conf) &&
                (roi_out[grid_id].conf <
                 ((int) min_conf + p_params->soft_conf_thresh_delta)))
        {
            roi_out[grid_id].is_soft_conf_rgn = TRUE;
        }
    }

    if (TRUE == roi_out[grid_id].is_conf)
    {
        roi_out[grid_id].not_conf_cnt = 0;
    }
    else
    {
        roi_out[grid_id].not_conf_cnt++;
    }

    return TRUE;
}

static BOOL af_pdaf_mw_multi_depth_detect(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_roi_t* data,
    INT size)
{
    INT detect_size ;
    INT i;
    FLOAT near_defocus_avg = 0.0;
    FLOAT far_defocus_avg = 0.0;
    af_pdaf_params_t* p_params = &pd->params;
    chromatixPDAFType* pPDAFTuning = pd->pTuningInfo->pPDAFTuningData;
    detect_size = (int) ((pPDAFTuning->PDMultiWindow.multiDepthDetect.percentile * size) + 0.5);
    detect_size = MIN(size, detect_size);
    detect_size = MAX(1, detect_size);
    HAF_MSG_HIGH("detect Size: %d size: %d", detect_size, size );

    for (i = 0 ; i < detect_size ; i++)
    {
        near_defocus_avg += data[i].defocus;
        far_defocus_avg += data[size - 1 - i].defocus;
    }

    near_defocus_avg /= detect_size;
    far_defocus_avg /= detect_size;

    if (p_params->MW_is_multi_depth &&
        (fabs(near_defocus_avg - far_defocus_avg) <= pPDAFTuning->PDMultiWindow.multiDepthDetect.detectThLow))
    {
        p_params->MW_is_multi_depth = FALSE ;
    }
    else if (fabs(near_defocus_avg - far_defocus_avg) >= pPDAFTuning->PDMultiWindow.multiDepthDetect.detectThHigh)
    {
        p_params->MW_is_multi_depth = TRUE ;
    }

    HAF_MSG_HIGH("near: %f far: %f, is_multi_depth %d  detect_th_low %f detect_th_high %f ",
                 near_defocus_avg, far_defocus_avg, p_params->MW_is_multi_depth,
                 pPDAFTuning->PDMultiWindow.multiDepthDetect.detectThLow,
                 pPDAFTuning->PDMultiWindow.multiDepthDetect.detectThHigh);
    return p_params->MW_is_multi_depth;
}


static boolean af_pdaf_mw_is_low_light(
    af_alg_PDAF_internal_t* pd)
{
    af_pdaf_params_t* p_params = &pd->params;
    chromatixPDAFType* pPDAFTuning = pd->pTuningInfo->pPDAFTuningData;

    if (pd->set_parameters.aec_input.cur_real_gain <= pPDAFTuning->PDMultiWindow.lowLightDetect.gainThLow)
    {
        p_params->MW_in_lowlight = FALSE;
    }
    else if (pd->set_parameters.aec_input.cur_real_gain >= pPDAFTuning->PDMultiWindow.lowLightDetect.gainThHigh)
    {
        p_params->MW_in_lowlight = TRUE;
    }

    HAF_MSG_LOW(" AEC cur_real_gain: %f mw_low_light_gain_th_low: %f, mw_low_light_gain_th_high: %f, p_params->mw_in_lowlight %d",
                  pd->set_parameters.aec_input.cur_real_gain,
                  pPDAFTuning->PDMultiWindow.lowLightDetect.gainThLow,
                  pPDAFTuning->PDMultiWindow.lowLightDetect.gainThHigh,
                  p_params->MW_in_lowlight);
    return p_params->MW_in_lowlight;
}




/** af_pdaf_mw_process_central:
 * Entry point for processing central multi-window PD stats.
 *
 * @pd: internal data structure
 * @input: input pd data
 *
 *  Return: id of the chosen roi, -1 if failure
 **/
static INT af_pdaf_mw_process_central(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_input_t* input)
{
    BOOL ret_val = FALSE;
    af_pdaf_roi_t data[PDAF_MAX_NUM_PD_ROI];
    INT size = 0;
    INT id = 0;
    INT h_num = 0;
    INT v_num = 0;
    INT grid_count = 0;
    INT single_id;
    af_pdaf_roi_t single_roi;
    h_num = (INT) input->pdaf_info.x_win_num;
    v_num = (INT) input->pdaf_info.y_win_num;
    grid_count = h_num * v_num;
    single_id = h_num * v_num;
    // reset the container for sorting
    Q3A_MEMSET(data, 0, sizeof(data));
    Q3A_MEMSET(&single_roi, 0, sizeof(af_pdaf_roi_t));
    chromatixPDMultiwindowType* mw_tuning =
        &pd->pTuningInfo->pPDAFTuningData->PDMultiWindow;
    af_pdaf_params_t* p_params = &pd->params;
    BOOL mw_in_low_light;
    BOOL mw_in_central_mode;
    mw_in_low_light = af_pdaf_mw_is_low_light(pd);
    mw_in_central_mode = (mw_tuning->HNumWindows * mw_tuning->VNumWindows) == grid_count ?
                         TRUE : FALSE;

    /* MW Algorithm has following steps:
     - Process PD stats for multi-window
     - Discard windows that do not satisfy certain conditions
     - Sort PD values in increasing order
     - Pick the  closest N windows
     */
    // Process multi-window PD stats
    if (mw_in_central_mode)
    {
        ret_val = af_pdaf_mw_proc_pd_single_window(pd, input->pdaf_info.defocus, data, single_id);

        if (FALSE == ret_val)
        {
            HAF_MSG_ERROR("Error process multi-window pd stats!");
            return -1;
        }

        single_roi = data[single_id];
    }

    HAF_MSG_HIGH("single_id: %d pd=%0.2f defocus(um)=%d conf=%d is_conf=%d",
                 single_roi.id,
                 single_roi.pd,
                 single_roi.defocus,
                 single_roi.conf,
                 single_roi.is_conf);
    ret_val = af_pdaf_mw_proc_pd_windows(pd, input->pdaf_info.defocus, data, grid_count);

    if (FALSE == ret_val)
    {
        HAF_MSG_ERROR("Error process multi-window pd stats!");
        return -1;
    }

    if (p_params == NULL)
    {
        HAF_MSG_ERROR("p_params is NULL");
        return -1;
    }

    if (p_params->fallback_single_during_search == 1 && single_roi.is_conf == TRUE)
    {
        af_pdaf_mw_update_active_roi(pd, &single_roi);
        pd->mw_roi_info.selected_win_count = 0;
        pd->mw_roi_info.type = PDAF_MW_CENTRAL;
        HAF_MSG_HIGH("fallback to single ROI during search, update single ROI directly,"
                     "single_roi.is_conf %d, p_params->focus_converge_cnt %d ",
                     single_roi.is_conf, p_params->focus_converge_cnt);
        return single_id;
    }

    // Discard windows that don't meet the requrements
    af_pdaf_mw_sanitize_windows(pd, data, grid_count, &size);

    if (size > 0)
    {
        af_pdaf_mw_sort_windows(data, size);
        // Get the best window to focus on
        id = af_pdaf_mw_select_best_window(pd, data, size);

        if (mw_in_central_mode)
        {
            if ((!af_pdaf_mw_multi_depth_detect(pd, data, size) ||
                    mw_in_low_light) && single_roi.is_conf == TRUE)
            {
                HAF_MSG_HIGH("not multi depth/mw_in_low_light,update the single ROI! single_roi.is_conf %d mw_in_low_light %d",
                             single_roi.is_conf, mw_in_low_light);
                af_pdaf_mw_update_active_roi(pd, &single_roi);
                pd->mw_roi_info.selected_win_count = 0;
                pd->mw_roi_info.type = PDAF_MW_CENTRAL;
                HAF_MSG_HIGH("p_params->focus_converge_cnt %d ", p_params->focus_converge_cnt);

                if (p_params->focus_converge_cnt > 0)
                {
                    pd->params.fallback_single_during_search = 1;
                }

                return single_id;
            }
        }

        /* Get other windows with similar depth. This can be useful for other
         modules like contrast AF where multiple windows give better focus
         metric than single */
        af_pdaf_mw_collect_similar_windows(pd, data, size, id);
        /* TODO: Currently PD algorithm uses roi[0] for all the processing.
         So need to update roi[0] with the selected data. Need further work
         to design the proper implementation. */
        af_pdaf_mw_update_active_roi(pd, &data[id]);
        id = data[id].id;
        /*update the mw roi type*/
        pd->mw_roi_info.type = (mw_in_central_mode) ?
                               PDAF_MW_CENTRAL : PDAF_MW_PERIPHERAL;

        if (mw_in_central_mode && p_params->focus_converge_cnt > 0)
        {
            pd->params.MW_grids_converge_used = 1;
            HAF_MSG_HIGH(" pd->params.MW_grids_converge_used %d,disable fine search ",
                         pd->params.MW_grids_converge_used);
        }
    }
    else
    {
        HAF_MSG_HIGH("Satisfying central grids not found!");

        if (single_roi.is_conf == TRUE && mw_in_central_mode)
        {
            HAF_MSG_HIGH("update single window ! single_roi.is_conf %d ", single_roi.is_conf);
            af_pdaf_mw_update_active_roi(pd, &single_roi);
            id = single_id;
            pd->mw_roi_info.selected_win_count = 0;
            pd->mw_roi_info.type = mw_in_central_mode ? PDAF_MW_CENTRAL : PDAF_MW_PERIPHERAL;
        }
        else
        {
            HAF_MSG_HIGH(" single not conf for central_mode or not conf for peripheral window!");
            af_pdaf_mw_update_active_roi(pd, NULL);
            id = -1;
        }
    }

    return id;
}

/** af_pdaf_mw_process_peripheral:
 * Entry point for processing peripheral multi-window PD stats.
 *
 * @pd: internal data structure
 * @input: input pd data
 *
 *  Return: id of the chosen roi, -1 if failure
 **/
static INT af_pdaf_mw_process_peripheral(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_input_t* input)
{
    af_pdaf_roi_t data[MAX_PERIPHERAL_WINDOWS];
    INT size = 0;
    INT id = -1;
    af_pdaf_params_t* p_params = &pd->params;

    if (input->pdaf_info.is_peripheral_valid == 0)
        return id;

    Q3A_MEMSET(data, 0, sizeof(data));
    /* Currently there are four peripheral regions.
     We will choose the one that is closest */
    af_pdaf_mw_proc_pd_windows(pd, input->pdaf_info.peripheral, data,
                               MAX_PERIPHERAL_WINDOWS);
    // Discard windows that don't meet the requrements
    af_pdaf_mw_sanitize_peripheral_windows(pd, data,
                                           MAX_PERIPHERAL_WINDOWS, &size);

    if (size > 0)
    {
        // Sort PD values in increasig order
        af_pdaf_mw_sort_peripheral_windows(data, size);
        // Update mw roi
        pd->mw_roi_info.type = PDAF_MW_PERIPHERAL;
        pd->mw_roi_info.roi_index[0] = data[0].id;
        pd->mw_roi_info.selected_win_count = 1;
        id = data[0].id;

        if (p_params->focus_converge_cnt > 0)
        {
            p_params->MW_switch_to_peripheral_during_search = 1;
            HAF_MSG_HIGH("MW_switch_to_peripheral_during_search %d, disable fine search ",
                         p_params->MW_switch_to_peripheral_during_search);
        }
    }
    else
    {
        HAF_MSG_ERROR("No eligible window found!");
        id = -1;
    }

    /* TODO: Currently PD algorithm uses roi[0] for all the processing.
     So need to update roi[0] with the selected data. Need further work
     to design the proper implementation. */
    af_pdaf_mw_update_active_roi(pd, (id < 0) ? NULL : &data[0]);
    return id;
}

/** af_pdaf_proc_pd_mw:
 * Entry point for processing multi-window PD stats.
 *
 * @pd: internal data structure
 * @input: input pd data
 *
 *  Return: TRUE on success FALSE on failure
 **/
BOOL af_pdaf_proc_pd_mw(
    af_alg_PDAF_internal_t* pd,
    af_pdaf_input_t* input)
{
    INT id = 0;
    BOOL retval = TRUE;
    /* If peripheral regions are valid, we would only process peripheral pd data.
     This is optimization done in pd lib where only if central ROI pd data are
     not valid, peripheral pd data will be generated */
    id = af_pdaf_mw_process_central(pd, input);
    if (id < 0)
    {
        HAF_MSG_HIGH("No Central window found! Checking peripheral!!");
        id = af_pdaf_mw_process_peripheral(pd, input);
    }

    // if id is negative, we failed to find the ROI to focus on
    if (id < 0)
    {
        HAF_MSG_HIGH("Failure finding right window to focus on!!!");
        pd->mw_roi_info.type = PDAF_MW_INVALID;
        retval = FALSE;
    }
    // PrINT what's been there on top of active ROI
    HAF_MSG_LOW("Current ROI - type: %d grid-id: %d defocus: %d pd: %f conf:%d "
                "is_conf: %d",
                pd->mw_roi_info.type,
                pd->params.roi[0].id,
                pd->params.roi[0].defocus,
                pd->params.roi[0].pd,
                pd->params.roi[0].conf,
                pd->params.roi[0].is_conf);
    return retval;
}

#ifdef VENDOR_EDIT
static void af_pdaf_check_hunting_prevent(af_alg_PDAF_internal_t * pd, int hunting_final_pos)
   {
     af_pdaf_params_t * p_params = &pd->params;
     af_pdaf_hunting_prevent_t *hunting_prevent = &pd->params.hunting_prevent;
     int roi_index = p_params->focus_roi_id;
         chromatixPDROIType* roi_tuning = &pd->pTuningInfo->pPDAFTuningData->ROI;

     int defocus = 0;
     //boolean defocused = FALSE;
     int idx = 0, i = 0, j = 0;
     float defocus_deviation = 0.0f;
     boolean is_conf = TRUE;
     float target_pos_avg = 0.0f;
     int not_conf_cnt = 0;

     if (!hunting_prevent->enable) {
       return;
     }
     int num_pd_roi = roi_tuning->ConfigurationCount;

     for (i = 0; i < num_pd_roi; ++i) {
       hunting_prevent->roi[i].hunting_prevent_trigger = FALSE;
       defocus_deviation = 0.0f;
       is_conf = TRUE;
       target_pos_avg = 0.0f;
       defocus = 0;
       not_conf_cnt = 0;
       for (j = 0; j < hunting_prevent->check_size; ++j) {
         idx = (p_params->idx - j + PDAF_MAX_NUM_PD_HISTORY) % PDAF_MAX_NUM_PD_HISTORY;
         if (FALSE == hunting_prevent->roi[i].target_pos_hist[idx].is_conf) {
            not_conf_cnt++;
         }
         target_pos_avg += hunting_prevent->roi[i].target_pos_hist[idx].pos;
       }
       is_conf = (not_conf_cnt == hunting_prevent->check_size)? FALSE :TRUE;
       HAF_MSG_INFO("is_conf %d not_conf_cnt %d", is_conf, not_conf_cnt);
       if (FALSE == is_conf) {
           HAF_MSG_INFO("the target pos hist is un-confidence. force the monitor hunting_prevent_trigger == TRUE");
         if (hunting_prevent->flag & AF_PD_HUNTING_PREVENT_CHECK_CONF) {
           hunting_prevent->roi[i].hunting_prevent_trigger = TRUE;
           hunting_prevent->roi[i].defocus_base = 0;
         } else {
           hunting_prevent->roi[i].hunting_prevent_trigger = FALSE;
           hunting_prevent->roi[i].defocus_base = 0;
         }
         continue;
       }
       target_pos_avg /= MAX(hunting_prevent->check_size, 1);

       for (j = 0; j < hunting_prevent->check_size; ++j) {
         idx = (p_params->idx - j + PDAF_MAX_NUM_PD_HISTORY) % PDAF_MAX_NUM_PD_HISTORY;
         defocus_deviation += (float)ABS(hunting_prevent->roi[i].target_pos_hist[idx].pos - target_pos_avg);
       }
       hunting_prevent->roi[i].defocus_base = target_pos_avg - hunting_final_pos;
       defocus_deviation /= (MAX(hunting_prevent->search_range, 1) * MAX(hunting_prevent->check_size, 1));
       if (i == 0) {
         HAF_MSG_HIGH("defocus_deviation = %f", defocus_deviation);
       }
       if (hunting_prevent->flag & AF_PD_HUNTING_PREVENT_DEFOCUS_BASE) {
         if (ABS(hunting_prevent->roi[i].defocus_base) > hunting_prevent->defocus_thres) {
           hunting_prevent->roi[i].hunting_prevent_trigger = TRUE;
         }
       }
       if (hunting_prevent->flag & AF_PD_HUNTING_PREVENT_CHECK_DEVIATION) {
         if (defocus_deviation > hunting_prevent->defocus_deviation_thres ||
             (p_params->roi_info.type == AF_ROI_TYPE_FACE && defocus_deviation > hunting_prevent->defocus_deviation_face_thres)) {
           hunting_prevent->roi[i].hunting_prevent_trigger = TRUE;
         }
       }
     }

     /**
      * we use the roi 0 to check the prevent_hunting always.
      * pd roi 0 region match the baf roi default.
      **/
     roi_index = 0;
     if (hunting_prevent->flag & AF_PD_HUNTING_PREVENT_DEFOCUS_BASE) {
       hunting_prevent->defocus_base = hunting_prevent->roi[roi_index].defocus_base;
     } else {
       hunting_prevent->defocus_base = 0;
     }

     if (hunting_prevent->roi[roi_index].hunting_prevent_trigger) {
         hunting_prevent->trigger_cnt++;
     } else {
         hunting_prevent->trigger_cnt = 0;
     }
     hunting_prevent->hunting_prevent_trigger =  (hunting_prevent->trigger_cnt >=  AF_PD_HUNTING_PREVENT_CONTINUE_CNT) ? 1:0;

     ALOGI("%s hunting_final_pos %d , defocus_base = %d, defocus_thres %d , defocus_deviation %f  defocus_deviation_thres %f is low light %d, hunting_prevent->trigger_cnt %d hunting_prevent_trigger %d ",
         __func__, hunting_final_pos , hunting_prevent->defocus_base,hunting_prevent->defocus_thres,
         defocus_deviation, hunting_prevent->defocus_deviation_thres,
         af_pdaf_is_low_light( pd ),hunting_prevent->trigger_cnt,
         hunting_prevent->hunting_prevent_trigger);
     return;
   }
#endif
