/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  This program is the proprietary software of Broadcom Corporation and/or its
 *  licensors, and may only be used, duplicated, modified or distributed
 *  pursuant to the terms and conditions of a separate, written license
 *  agreement executed between you and Broadcom (an "Authorized License").
 *  Except as set forth in an Authorized License, Broadcom grants no license
 *  (express or implied), right to use, or waiver of any kind with respect to
 *  the Software, and Broadcom expressly reserves all rights in and to the
 *  Software and all intellectual property rights therein.
 *  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS
 *  SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 *  ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *         constitutes the valuable trade secrets of Broadcom, and you shall
 *         use all reasonable efforts to protect the confidentiality thereof,
 *         and to use this information only in connection with your use of
 *         Broadcom integrated circuit products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *         "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 *         REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 *         OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 *         DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 *         NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 *         ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 *         CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
 *         OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *         ITS LICENSORS BE LIABLE FOR
 *         (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
 *               DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
 *               YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *               HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
 *         (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *               SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 *               LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
 *               ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Filename:      bt_hci_bdroid.c
 *
 *  Description:   Bluedroid Bluetooth Host/Controller interface library
 *                 implementation
 *
 ******************************************************************************/

#define LOG_TAG "bt_hci_bdroid"

#include <utils/Log.h>
#include <pthread.h>
#include "bt_hci_bdroid.h"
#include "bt_vendor_lib.h"
#include "utils.h"
#include "userial.h"

#ifndef BTHC_DBG
#define BTHC_DBG FALSE
#endif

#if (BTHC_DBG == TRUE)
#define BTHCDBG(param, ...) {LOGD(param, ## __VA_ARGS__);}
#else
#define BTHCDBG(param, ...) {}
#endif

/******************************************************************************
**  Externs
******************************************************************************/

extern bt_vendor_interface_t *bt_vnd_if;
extern int num_hci_cmd_pkts;
void hci_h4_init(void);
void hci_h4_cleanup(void);
void hci_h4_send_msg(HC_BT_HDR *p_msg);
uint16_t hci_h4_receive_msg(void);
void hci_h4_get_acl_data_length(void);
void lpm_init(void);
void lpm_cleanup(void);
void lpm_enable(uint8_t turn_on);
void lpm_wake_deassert(void);
void lpm_allow_bt_device_sleep(void);
void lpm_wake_assert(void);
void init_vnd_if(unsigned char *local_bdaddr);
void btsnoop_open(char *p_path);
void btsnoop_close(void);

/******************************************************************************
**  Variables
******************************************************************************/

bt_hc_callbacks_t *bt_hc_cbacks = NULL;
BUFFER_Q tx_q;

/******************************************************************************
**  Local type definitions
******************************************************************************/

/* Host/Controller lib thread control block */
typedef struct
{
    pthread_t       worker_thread;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
} bt_hc_cb_t;

/******************************************************************************
**  Static Variables
******************************************************************************/

static bt_hc_cb_t hc_cb;
static volatile uint8_t lib_running = 0;
static volatile uint16_t ready_events = 0;
static volatile uint8_t tx_cmd_pkts_pending = FALSE;

static const tUSERIAL_CFG userial_init_cfg =
{
    (USERIAL_DATABITS_8 | USERIAL_PARITY_NONE | USERIAL_STOPBITS_1),
    USERIAL_BAUD_115200
};

/******************************************************************************
**  Functions
******************************************************************************/

static void *bt_hc_worker_thread(void *arg);

void bthc_signal_event(uint16_t event)
{
    pthread_mutex_lock(&hc_cb.mutex);
    ready_events |= event;
    pthread_cond_signal(&hc_cb.cond);
    pthread_mutex_unlock(&hc_cb.mutex);
}

/*****************************************************************************
**
**   BLUETOOTH HOST/CONTROLLER INTERFACE LIBRARY FUNCTIONS
**
*****************************************************************************/

static int init(const bt_hc_callbacks_t* p_cb, unsigned char *local_bdaddr)
{
    pthread_attr_t thread_attr;
    struct sched_param param;
    int policy, result;

    LOGI("init");

    if (p_cb == NULL)
    {
        LOGE("init failed with no user callbacks!");
        return BT_HC_STATUS_FAIL;
    }

    /* store reference to user callbacks */
    bt_hc_cbacks = (bt_hc_callbacks_t *) p_cb;

    init_vnd_if(local_bdaddr);

    utils_init();
    hci_h4_init();
    userial_init();
    lpm_init();

    utils_queue_init(&tx_q);

    if (lib_running)
    {
        LOGW("init has been called repeatedly without calling cleanup ?");
    }

    lib_running = 1;
    ready_events = 0;
    pthread_mutex_init(&hc_cb.mutex, NULL);
    pthread_cond_init(&hc_cb.cond, NULL);
    pthread_attr_init(&thread_attr);

    if (pthread_create(&hc_cb.worker_thread, &thread_attr, \
                       bt_hc_worker_thread, NULL) != 0)
    {
        LOGE("pthread_create failed!");
        lib_running = 0;
        return BT_HC_STATUS_FAIL;
    }

    if(pthread_getschedparam(hc_cb.worker_thread, &policy, &param)==0)
    {
        policy = BTHC_LINUX_BASE_POLICY;
#if (BTHC_LINUX_BASE_POLICY!=SCHED_NORMAL)
        param.sched_priority = BTHC_MAIN_THREAD_PRIORITY;
#endif
        result = pthread_setschedparam(hc_cb.worker_thread, policy, &param);
        if (result != 0)
        {
            LOGW("libbt-hci init: pthread_setschedparam failed (%s)", \
                  strerror(result));
        }
    }

    return BT_HC_STATUS_SUCCESS;
}


/** Chip power control */
static void set_power(bt_hc_chip_power_state_t state)
{
    int pwr_state;

    BTHCDBG("set_power %d", state);

    /* Calling vendor-specific part */
    pwr_state = (state == BT_HC_CHIP_PWR_ON) ? BT_VND_PWR_ON : BT_VND_PWR_OFF;

    if (bt_vnd_if)
        bt_vnd_if->op(BT_VND_OP_POWER_CTRL, &pwr_state);
    else
        LOGE("vendor lib is missing!");
}


/** Configure low power mode wake state */
static int lpm(bt_hc_low_power_event_t event)
{
    uint8_t status = TRUE;

    switch (event)
    {
        case BT_HC_LPM_DISABLE:
            bthc_signal_event(HC_EVENT_LPM_DISABLE);
            break;

        case BT_HC_LPM_ENABLE:
            bthc_signal_event(HC_EVENT_LPM_ENABLE);
            break;

        case BT_HC_LPM_WAKE_ASSERT:
            bthc_signal_event(HC_EVENT_LPM_WAKE_DEVICE);
            break;

        case BT_HC_LPM_WAKE_DEASSERT:
            bthc_signal_event(HC_EVENT_LPM_ALLOW_SLEEP);
            break;
    }

    return(status == TRUE) ? BT_HC_STATUS_SUCCESS : BT_HC_STATUS_FAIL;
}


/** Called prio to stack initialization */
static void preload(TRANSAC transac)
{
    BTHCDBG("preload");
    bthc_signal_event(HC_EVENT_PRELOAD);
}


/** Called post stack initialization */
static void postload(TRANSAC transac)
{
    BTHCDBG("postload");
    bthc_signal_event(HC_EVENT_POSTLOAD);
}


/** Transmit frame */
static int transmit_buf(TRANSAC transac, char *p_buf, int len)
{
    utils_enqueue(&tx_q, (void *) transac);

    bthc_signal_event(HC_EVENT_TX);

    return BT_HC_STATUS_SUCCESS;
}


/** Controls receive flow */
static int set_rxflow(bt_rx_flow_state_t state)
{
    BTHCDBG("set_rxflow %d", state);

    userial_ioctl(\
     ((state == BT_RXFLOW_ON) ? USERIAL_OP_RXFLOW_ON : USERIAL_OP_RXFLOW_OFF), \
     NULL);

    return BT_HC_STATUS_SUCCESS;
}


/** Controls HCI logging on/off */
static int logging(bt_hc_logging_state_t state, char *p_path)
{
    BTHCDBG("logging %d", state);

    if (state == BT_HC_LOGGING_ON)
    {
        if (p_path != NULL)
            btsnoop_open(p_path);
    }
    else
    {
        btsnoop_close();
    }

    return BT_HC_STATUS_SUCCESS;
}


/** Closes the interface */
static void cleanup( void )
{
    BTHCDBG("cleanup");

    if (lib_running)
    {
        lib_running = 0;
        bthc_signal_event(HC_EVENT_EXIT);
        pthread_join(hc_cb.worker_thread, NULL);
    }

    lpm_cleanup();
    userial_close();
    hci_h4_cleanup();
    utils_cleanup();

    /* Calling vendor-specific part */
    if (bt_vnd_if)
        bt_vnd_if->cleanup();

    bt_hc_cbacks = NULL;
}


static const bt_hc_interface_t bluetoothHCLibInterface = {
    sizeof(bt_hc_interface_t),
    init,
    set_power,
    lpm,
    preload,
    postload,
    transmit_buf,
    set_rxflow,
    logging,
    cleanup
};


/*******************************************************************************
**
** Function        bt_hc_worker_thread
**
** Description     Mian worker thread
**
** Returns         void *
**
*******************************************************************************/
static void *bt_hc_worker_thread(void *arg)
{
    uint16_t events;
    HC_BT_HDR *p_msg, *p_next_msg;

    BTHCDBG("bt_hc_worker_thread started");
    tx_cmd_pkts_pending = FALSE;

    while (lib_running)
    {
        pthread_mutex_lock(&hc_cb.mutex);
        while (ready_events == 0)
        {
            pthread_cond_wait(&hc_cb.cond, &hc_cb.mutex);
        }
        events = ready_events;
        ready_events = 0;
        pthread_mutex_unlock(&hc_cb.mutex);

        if (events & HC_EVENT_RX)
        {
            hci_h4_receive_msg();

            if ((tx_cmd_pkts_pending == TRUE) && (num_hci_cmd_pkts > 0))
            {
                /* Got HCI Cmd Credits from Controller.
                 * Prepare to send prior pending Cmd packets in the
                 * following HC_EVENT_TX session.
                 */
                events |= HC_EVENT_TX;
            }
        }

        if (events & HC_EVENT_PRELOAD)
        {
            userial_open(USERIAL_PORT_1, (tUSERIAL_CFG *) &userial_init_cfg);

            /* Calling vendor-specific part */
            if (bt_vnd_if)
            {
                bt_vnd_if->op(BT_VND_OP_FW_CFG, NULL);
            }
            else
            {
                if (bt_hc_cbacks)
                    bt_hc_cbacks->preload_cb(NULL, BT_HC_PRELOAD_FAIL);
            }
        }

        if (events & HC_EVENT_POSTLOAD)
        {
            /* Start from SCO related H/W configuration, if SCO configuration
             * is required. Then, follow with reading requests of getting 
             * ACL data length for both BR/EDR and LE.
             */
            int result = -1;

            /* Calling vendor-specific part */
            if (bt_vnd_if)
                result = bt_vnd_if->op(BT_VND_OP_SCO_CFG, NULL);

            if (result == -1)
                hci_h4_get_acl_data_length();
        }

        if (events & HC_EVENT_TX)
        {
            /* 
             *  We will go through every packets in the tx queue.
             *  Fine to clear tx_cmd_pkts_pending.
             */
            tx_cmd_pkts_pending = FALSE;

            p_next_msg = tx_q.p_first;
            while (p_next_msg)
            {
                if ((p_next_msg->event & MSG_EVT_MASK)==MSG_STACK_TO_HC_HCI_CMD)
                {
                    /*
                     *  if we have used up controller's outstanding HCI command 
                     *  credits (normally is 1), skip all HCI command packets in 
                     *  the queue. 
                     *  The pending command packets will be sent once controller 
                     *  gives back us credits through CommandCompleteEvent or 
                     *  CommandStatusEvent.
                     */
                    if ((tx_cmd_pkts_pending == TRUE) || (num_hci_cmd_pkts <= 0))
                    {
                        tx_cmd_pkts_pending = TRUE;
                        p_next_msg = utils_getnext(p_next_msg);
                        continue;
                    }
                }

                p_msg = p_next_msg;
                p_next_msg = utils_getnext(p_msg);
                utils_remove_from_queue(&tx_q, p_msg);
                hci_h4_send_msg(p_msg);
            }

            if (tx_cmd_pkts_pending == TRUE)
                BTHCDBG("Used up Tx Cmd credits");

        }

        if (events & HC_EVENT_LPM_ENABLE)
        {
            lpm_enable(TRUE);
        }

        if (events & HC_EVENT_LPM_DISABLE)
        {
            lpm_enable(FALSE);
        }

        if (events & HC_EVENT_LPM_IDLE_TIMEOUT)
        {
            lpm_wake_deassert();
        }

        if (events & HC_EVENT_LPM_ALLOW_SLEEP)
        {
            lpm_allow_bt_device_sleep();
        }

        if (events & HC_EVENT_LPM_WAKE_DEVICE)
        {
            lpm_wake_assert();
        }

        if (events & HC_EVENT_EXIT)
            break;
    }

    BTHCDBG("bt_hc_worker_thread exiting");

    pthread_exit(NULL);

    return NULL;    // compiler friendly
}


/*******************************************************************************
**
** Function        bt_hc_get_interface
**
** Description     Caller calls this function to get API instance
**
** Returns         API table
**
*******************************************************************************/
const bt_hc_interface_t *bt_hc_get_interface(void)
{
    return &bluetoothHCLibInterface;
}
