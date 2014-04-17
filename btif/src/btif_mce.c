/******************************************************************************
 *
 * Copyright (c) 2013, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/************************************************************************************
 *
 *  Filename:      btif_mce.c
 *
 *  Description:   Message Access Profile (MCE role) Bluetooth Interface
 *
 *
 ***********************************************************************************/

#include <hardware/bluetooth.h>
#include <hardware/bt_mce.h>
#include <stdlib.h>

#define LOG_TAG "BTIF_MCE"
#include "btif_common.h"
#include "btif_util.h"
#include "btif_profile_queue.h"
#include "bta_api.h"
#include "bta_mce_api.h"

#include "bd.h"

/*****************************************************************************
**  Static variables
******************************************************************************/

static btmce_callbacks_t *bt_mce_callbacks = NULL;

static void btif_mce_mas_discovery_comp_evt(UINT16 event, char *p_param)
{
    tBTA_MCE_MAS_DISCOVERY_COMP *evt_data = (tBTA_MCE_MAS_DISCOVERY_COMP*) p_param;
    btmce_mas_instance_t insts[BTA_MCE_MAX_MAS_INSTANCES];
    bt_bdaddr_t addr;
    int i;

    BTIF_TRACE_EVENT2("%s:  event = %d", __FUNCTION__, event);

    if (event != BTA_MCE_MAS_DISCOVERY_COMP_EVT)
        return;

    for (i = 0; i < evt_data->num_mas; i++)
    {
        insts[i].id = evt_data->mas[i].instance_id;
        insts[i].scn = evt_data->mas[i].scn;
        insts[i].msg_types = evt_data->mas[i].msg_type;
        insts[i].p_name = evt_data->mas[i].p_srv_name;
    }

    bdcpy(addr.address, evt_data->remote_addr);

    HAL_CBACK(bt_mce_callbacks, remote_mas_instances_cb, evt_data->status, &addr, evt_data->num_mas, insts);
}

static void mas_discovery_comp_copy_cb(UINT16 event, char *p_dest, char *p_src)
{
    tBTA_MCE_MAS_DISCOVERY_COMP *p_dest_data =  (tBTA_MCE_MAS_DISCOVERY_COMP *) p_dest;
    tBTA_MCE_MAS_DISCOVERY_COMP *p_src_data =  (tBTA_MCE_MAS_DISCOVERY_COMP *) p_src;
    char *p_dest_str;
    int i;

    if (!p_src)
        return;

    if (event != BTA_MCE_MAS_DISCOVERY_COMP_EVT)
        return;

    memcpy(p_dest_data, p_src_data, sizeof(tBTA_MCE_MAS_DISCOVERY_COMP));

    p_dest_str = p_dest + sizeof(tBTA_MCE_MAS_DISCOVERY_COMP);

    for (i = 0; i < p_src_data->num_mas; i++)
    {
        p_dest_data->mas[i].p_srv_name = p_dest_str;
        memcpy(p_dest_str, p_src_data->mas[i].p_srv_name, p_src_data->mas[i].srv_name_len);
        p_dest_str += p_src_data->mas[i].srv_name_len;
        *(p_dest_str++) = '\0';
    }
}

static void mce_dm_cback(tBTA_MCE_EVT event, tBTA_MCE *p_data, void *user_data)
{
    switch (event)
    {
        case BTA_MCE_MAS_DISCOVERY_COMP_EVT:
            {
                int i;
                UINT16 param_len = sizeof(tBTA_MCE);

                /* include space for all p_srv_name copies including null-termination */
                for (i = 0; i < p_data->mas_disc_comp.num_mas; i++)
                    param_len += (p_data->mas_disc_comp.mas[i].srv_name_len + 1);

                /* need to deepy copy p_srv_name and null-terminate */
                btif_transfer_context(btif_mce_mas_discovery_comp_evt, event,
                                        (char*)p_data, param_len, mas_discovery_comp_copy_cb);

                break;
            }
    }
}

static bt_status_t init(btmce_callbacks_t* callbacks)
{
    BTIF_TRACE_EVENT1("%s", __FUNCTION__);

    bt_mce_callbacks = callbacks;

    btif_enable_service(BTA_MAP_SERVICE_ID);

    return BT_STATUS_SUCCESS;
}

static bt_status_t get_remote_mas_instances(bt_bdaddr_t *bd_addr)
{
    bdstr_t bdstr;

    BTIF_TRACE_EVENT2("%s: remote_addr=%s", __FUNCTION__, bd2str(bd_addr, &bdstr));

    BTA_MceGetRemoteMasInstances(bd_addr->address);

    return BT_STATUS_SUCCESS;
}

static const btmce_interface_t mce_if = {
    sizeof(btmce_interface_t),
    init,
    get_remote_mas_instances,
};

const btmce_interface_t *btif_mce_get_interface(void)
{
    BTIF_TRACE_EVENT1("%s", __FUNCTION__);
    return &mce_if;
}

/*******************************************************************************
**
** Function         btif_mce_execute_service
**
** Description      Initializes/Shuts down the service
**
** Returns          BT_STATUS_SUCCESS on success, BT_STATUS_FAIL otherwise
**
*******************************************************************************/
bt_status_t btif_mce_execute_service(BOOLEAN b_enable)
{
    BTIF_TRACE_EVENT2("%s enable:%d", __FUNCTION__, b_enable);

     if (b_enable)
     {
         BTA_MceEnable(mce_dm_cback);
     }
     else
     {
         /* This is called on BT disable so no need to extra cleanup */
     }
     return BT_STATUS_SUCCESS;
}