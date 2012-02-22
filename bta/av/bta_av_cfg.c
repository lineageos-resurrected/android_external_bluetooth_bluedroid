/*****************************************************************************
**
**  Name:           bta_av_cfg.c
**
**  Description:    This file contains compile-time configurable constants
**                  for advanced audio/video
**
**  Copyright (c) 2005-2009, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"
#include "gki.h"
#include "bta_api.h"
#include "bta_av_int.h"

#ifndef BTA_AV_VDP_INCLUDED
#define BTA_AV_VDP_INCLUDED     TRUE
#endif

#if ((VDP_INCLUDED == FALSE) && (BTA_AV_VDP_INCLUDED == TRUE))
#undef BTA_AV_VDP_INCLUDED
#define BTA_AV_VDP_INCLUDED        FALSE
#endif

#ifndef BTA_AV_RC_PASS_RSP_CODE
#define BTA_AV_RC_PASS_RSP_CODE     BTA_AV_RSP_NOT_IMPL
#endif

#if (BTA_AV_VDP_INCLUDED == TRUE)
#define BTA_AV_NUM_A2DP_STRS     (BTA_AV_NUM_STRS - 1)
#else
#define BTA_AV_NUM_A2DP_STRS     (BTA_AV_NUM_STRS)
#endif

const UINT32  bta_av_meta_caps_co_ids[] = {
    AVRC_CO_METADATA,
    AVRC_CO_BROADCOM
};

/* AVRCP cupported categories */
#define BTA_AV_RC_SUPF_CT       (AVRC_SUPF_CT_CAT2)


/* Added to modify
**	1. flush timeout
**	2. Remove Group navigation support in SupportedFeatures
**	3. GetCapabilities supported event_ids list
**	4. GetCapabilities supported event_ids count
*/
#ifdef ANDROID_APP_INCLUDED
/* Flushing partial avdtp packets can cause some headsets to disconnect the link
   if receiving partial a2dp frames */
const UINT16  bta_av_audio_flush_to[] = {
     0, /* 1 stream */
     0, /* 2 streams */
     0, /* 3 streams */
     0, /* 4 streams */
     0  /* 5 streams */
};     /* AVDTP audio transport channel flush timeout */

/* Note: Android doesnt support AVRC_SUPF_TG_GROUP_NAVI  */
#if AVRC_ADV_CTRL_INCLUDED == TRUE
#define BTA_AV_RC_SUPF_TG       (AVRC_SUPF_TG_CAT1|AVRC_SUPF_TG_APP_SETTINGS|AVRC_SUPF_TG_BROWSE|AVRC_SUPF_TG_MULTI_PLAYER)
#else
#if AVRC_METADATA_INCLUDED == TRUE
#define BTA_AV_RC_SUPF_TG       (AVRC_SUPF_TG_CAT1|AVRC_SUPF_TG_APP_SETTINGS)
#else
#define BTA_AV_RC_SUPF_TG       (AVRC_SUPF_TG_CAT1)
#endif
#endif


/*
 * If the number of event IDs is changed in this array, BTA_AV_ NUM_RC_EVT_IDS   also needs to be changed.
 */
const UINT8  bta_av_meta_caps_evt_ids[] = {
    AVRC_EVT_PLAY_STATUS_CHANGE,
    AVRC_EVT_TRACK_CHANGE,
    AVRC_EVT_PLAY_POS_CHANGED,
    AVRC_EVT_APP_SETTING_CHANGE,
#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
	AVRC_EVT_NOW_PLAYING_CHANGE,
	AVRC_EVT_AVAL_PLAYERS_CHANGE,
	AVRC_EVT_ADDR_PLAYER_CHANGE,
//	AVRC_EVT_UIDS_CHANGE,
	AVRC_EVT_VOLUME_CHANGE
#endif
};
#ifndef BTA_AV_NUM_RC_EVT_IDS
#define BTA_AV_NUM_RC_EVT_IDS   (sizeof(bta_av_meta_caps_evt_ids) / sizeof(bta_av_meta_caps_evt_ids[0]))
#endif /* BTA_AV_NUM_RC_EVT_IDS */ 


#else /* !ANDROID_APP_INCLUDED */

/* Note: if AVRC_SUPF_TG_GROUP_NAVI is set, bta_av_cfg.avrc_group should be TRUE */
#if AVRC_ADV_CTRL_INCLUDED == TRUE
#define BTA_AV_RC_SUPF_TG       (AVRC_SUPF_TG_CAT1|AVRC_SUPF_TG_APP_SETTINGS|AVRC_SUPF_TG_GROUP_NAVI|AVRC_SUPF_TG_BROWSE|AVRC_SUPF_TG_MULTI_PLAYER)
#else
#if AVRC_METADATA_INCLUDED == TRUE
#define BTA_AV_RC_SUPF_TG       (AVRC_SUPF_TG_CAT1|AVRC_SUPF_TG_APP_SETTINGS|AVRC_SUPF_TG_GROUP_NAVI)
#else
#define BTA_AV_RC_SUPF_TG       (AVRC_SUPF_TG_CAT1)
#endif
#endif

const UINT16  bta_av_audio_flush_to[] = {
    120, /* 1 stream  */
    100, /* 2 streams */
     80, /* 3 streams */
     60, /* 4 streams */
     40  /* 5 streams */
};     /* AVDTP audio transport channel flush timeout */


/*
 * If the number of event IDs is changed in this array, BTA_AV_ NUM_RC_EVT_IDS   also needs to be changed.
 */
const UINT8  bta_av_meta_caps_evt_ids[] = {
    AVRC_EVT_PLAY_STATUS_CHANGE,
    AVRC_EVT_TRACK_CHANGE,
    AVRC_EVT_TRACK_REACHED_END,
    AVRC_EVT_TRACK_REACHED_START,
    AVRC_EVT_PLAY_POS_CHANGED,
    AVRC_EVT_BATTERY_STATUS_CHANGE,
    AVRC_EVT_SYSTEM_STATUS_CHANGE,
    AVRC_EVT_APP_SETTING_CHANGE,
#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
    AVRC_EVT_NOW_PLAYING_CHANGE,
    AVRC_EVT_AVAL_PLAYERS_CHANGE,
    AVRC_EVT_ADDR_PLAYER_CHANGE,
    AVRC_EVT_UIDS_CHANGE,
    AVRC_EVT_VOLUME_CHANGE
#endif
};

#ifndef BTA_AV_NUM_RC_EVT_IDS
#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
#define BTA_AV_NUM_RC_EVT_IDS   13
#else
#define BTA_AV_NUM_RC_EVT_IDS   8
#endif
#endif

#endif /* ANDROID_APP_INCLUDED */

/* the MTU for the AVRCP browsing channel */
#ifndef BTA_AV_MAX_RC_BR_MTU
#define BTA_AV_MAX_RC_BR_MTU      1008
#endif

const tBTA_AV_CFG bta_av_cfg =
{
    AVRC_CO_BROADCOM,       /* AVRCP Company ID */
#if AVRC_METADATA_INCLUDED == TRUE
    512,                    /* AVRCP MTU at L2CAP for control channel */
    BTA_AV_MAX_RC_BR_MTU,   /* AVRCP MTU at L2CAP for browsing channel */
#else
    48,                     /* AVRCP MTU at L2CAP for control channel */
    BTA_AV_MAX_RC_BR_MTU,   /* AVRCP MTU at L2CAP for browsing channel */
#endif
    BTA_AV_RC_SUPF_CT,      /* AVRCP controller categories */
    BTA_AV_RC_SUPF_TG,      /* AVRCP target categories */
    672,                    /* AVDTP signaling channel MTU at L2CAP */
    BTA_AV_MAX_A2DP_MTU,    /* AVDTP audio transport channel MTU at L2CAP */
    bta_av_audio_flush_to,  /* AVDTP audio transport channel flush timeout */
    6,                      /* AVDTP audio channel max data queue size */
    BTA_AV_MAX_VDP_MTU,     /* AVDTP video transport channel MTU at L2CAP */
    600,                    /* AVDTP video transport channel flush timeout */
    TRUE,                   /* TRUE, to accept AVRC 1.3 group nevigation command */
    2,                      /* company id count in p_meta_co_ids */
    BTA_AV_NUM_RC_EVT_IDS, /* event id count in p_meta_evt_ids */
    BTA_AV_RC_PASS_RSP_CODE,/* the default response code for pass through commands */
    bta_av_meta_caps_co_ids,/* the metadata Get Capabilities response for company id */
    bta_av_meta_caps_evt_ids,/* the the metadata Get Capabilities response for event id */
#if BTA_AV_VDP_INCLUDED == TRUE
    (const tBTA_AV_ACT *)bta_av_vdp_action,/* the action table for VDP */
    bta_av_reg_vdp          /* action function to register VDP */
#else
    (const tBTA_AV_ACT *)NULL,/* the action table for VDP */
    NULL                    /* action function to register VDP */
#endif
};

tBTA_AV_CFG *p_bta_av_cfg = (tBTA_AV_CFG *) &bta_av_cfg;

const UINT16 bta_av_rc_id[] =
{
    0x021F, /* bit mask: 0=SELECT, 1=UP, 2=DOWN, 3=LEFT,
                         4=RIGHT, 5=RIGHT_UP, 6=RIGHT_DOWN, 7=LEFT_UP,
                         8=LEFT_DOWN, 9=ROOT_MENU, 10=SETUP_MENU, 11=CONT_MENU,
                         12=FAV_MENU, 13=EXIT */

    0,      /* not used */

    0x0000, /* bit mask: 0=0, 1=1, 2=2, 3=3,
                         4=4, 5=5, 6=6, 7=7,
                         8=8, 9=9, 10=DOT, 11=ENTER,
                         12=CLEAR */

    0x0003, /* bit mask: 0=CHAN_UP, 1=CHAN_DOWN, 2=PREV_CHAN, 3=SOUND_SEL,
                         4=INPUT_SEL, 5=DISP_INFO, 6=HELP, 7=PAGE_UP,
                         8=PAGE_DOWN */

#if (BTA_AV_RC_PASS_RSP_CODE == BTA_AV_RSP_INTERIM)
    /* btui_app provides an example of how to leave the decision of rejecting a command or not 
     * based on which media player is currently addressed (this is only applicable for AVRCP 1.4 or later)
     * If the decision is per player for a particular rc_id, the related bit is clear (not set) */
    0x0070, /* bit mask: 0=POWER, 1=VOL_UP, 2=VOL_DOWN, 3=MUTE,
                         4=PLAY, 5=STOP, 6=PAUSE, 7=RECORD,
                         8=REWIND, 9=FAST_FOR, 10=EJECT, 11=FORWARD,
                         12=BACKWARD */
#else
#if (defined BTA_AVRCP_FF_RW_SUPPORT) && (BTA_AVRCP_FF_RW_SUPPORT == TRUE)
    0x1b70, /* bit mask: 0=POWER, 1=VOL_UP, 2=VOL_DOWN, 3=MUTE,
                         4=PLAY, 5=STOP, 6=PAUSE, 7=RECORD,
                         8=REWIND, 9=FAST_FOR, 10=EJECT, 11=FORWARD,
                         12=BACKWARD */
#else
    0x1870, /* bit mask: 0=POWER, 1=VOL_UP, 2=VOL_DOWN, 3=MUTE,
                         4=PLAY, 5=STOP, 6=PAUSE, 7=RECORD,
                         8=REWIND, 9=FAST_FOR, 10=EJECT, 11=FORWARD,
                         12=BACKWARD */
#endif
#endif
    
    0x0000, /* bit mask: 0=ANGLE, 1=SUBPICT */

    0,      /* not used */

    0x0000  /* bit mask: 0=not used, 1=F1, 2=F2, 3=F3,
                         4=F4, 5=F5 */
};

#if (BTA_AV_RC_PASS_RSP_CODE == BTA_AV_RSP_INTERIM)
const UINT16 bta_av_rc_id_ac[] =
{
    0x0000, /* bit mask: 0=SELECT, 1=UP, 2=DOWN, 3=LEFT,
                         4=RIGHT, 5=RIGHT_UP, 6=RIGHT_DOWN, 7=LEFT_UP,
                         8=LEFT_DOWN, 9=ROOT_MENU, 10=SETUP_MENU, 11=CONT_MENU,
                         12=FAV_MENU, 13=EXIT */

    0,      /* not used */

    0x0000, /* bit mask: 0=0, 1=1, 2=2, 3=3,
                         4=4, 5=5, 6=6, 7=7,
                         8=8, 9=9, 10=DOT, 11=ENTER,
                         12=CLEAR */

    0x0000, /* bit mask: 0=CHAN_UP, 1=CHAN_DOWN, 2=PREV_CHAN, 3=SOUND_SEL,
                         4=INPUT_SEL, 5=DISP_INFO, 6=HELP, 7=PAGE_UP,
                         8=PAGE_DOWN */

    /* btui_app provides an example of how to leave the decision of rejecting a command or not 
     * based on which media player is currently addressed (this is only applicable for AVRCP 1.4 or later)
     * If the decision is per player for a particular rc_id, the related bit is set */
    0x1800, /* bit mask: 0=POWER, 1=VOL_UP, 2=VOL_DOWN, 3=MUTE,
                         4=PLAY, 5=STOP, 6=PAUSE, 7=RECORD,
                         8=REWIND, 9=FAST_FOR, 10=EJECT, 11=FORWARD,
                         12=BACKWARD */
    
    0x0000, /* bit mask: 0=ANGLE, 1=SUBPICT */

    0,      /* not used */

    0x0000  /* bit mask: 0=not used, 1=F1, 2=F2, 3=F3,
                         4=F4, 5=F5 */
};
UINT16 *p_bta_av_rc_id_ac = (UINT16 *) bta_av_rc_id_ac;
#else
UINT16 *p_bta_av_rc_id_ac = NULL;
#endif

UINT16 *p_bta_av_rc_id = (UINT16 *) bta_av_rc_id;
