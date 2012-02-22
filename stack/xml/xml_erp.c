/******************************************************************************
 **                                                                           
 **  Name:          xml_erp.c                                       
 **                                                                           
 **  Description:   This module contains xml parser of MAP event report object  
 **                                                                           
 **  Copyright (c) 2004-2011, Broadcom Corporation, All Rights Reserved.      
 **  Broadcom Bluetooth Core. Proprietary and confidential.                   
 ******************************************************************************/

#include "bt_target.h"
#include "gki.h"
#include "xml_erp_api.h"

#include <string.h>
#include <stdlib.h>

#ifndef EVT_RPT_DEBUG_XML
#define EVT_RPT_DEBUG_XML TRUE
#endif
#define EVT_RPT_DEBUG_LEN 50
#if (defined(EVT_RPT_DEBUG_XML) && EVT_RPT_DEBUG_XML == TRUE)
#define XML_TRACE_DEBUG0(m)                     {BT_TRACE_0(TRACE_LAYER_GOEP, TRACE_TYPE_DEBUG, m);}
#define XML_TRACE_DEBUG1(m,p1)                  {BT_TRACE_1(TRACE_LAYER_GOEP, TRACE_TYPE_DEBUG, m,p1);}
#define XML_TRACE_DEBUG2(m,p1,p2)               {BT_TRACE_2(TRACE_LAYER_GOEP, TRACE_TYPE_DEBUG, m,p1,p2);}
#define XML_TRACE_DEBUG3(m,p1,p2,p3)            {BT_TRACE_3(TRACE_LAYER_GOEP, TRACE_TYPE_DEBUG, m,p1,p2,p3);}
#define XML_TRACE_DEBUG4(m,p1,p2,p3,p4)         {BT_TRACE_4(TRACE_LAYER_GOEP, TRACE_TYPE_DEBUG, m,p1,p2,p3,p4);}
#define XML_TRACE_DEBUG5(m,p1,p2,p3,p4,p5)      {BT_TRACE_5(TRACE_LAYER_GOEP, TRACE_TYPE_DEBUG, m,p1,p2,p3,p4,p5);}
#define XML_TRACE_DEBUG6(m,p1,p2,p3,p4,p5,p6)   {BT_TRACE_6(TRACE_LAYER_GOEP, TRACE_TYPE_DEBUG, m,p1,p2,p3,p4,p5,p6);}
#else
#define XML_TRACE_DEBUG0(m)                     
#define XML_TRACE_DEBUG1(m,p1)                  
#define XML_TRACE_DEBUG2(m,p1,p2)               
#define XML_TRACE_DEBUG3(m,p1,p2,p3)            
#define XML_TRACE_DEBUG4(m,p1,p2,p3,p4)         
#define XML_TRACE_DEBUG5(m,p1,p2,p3,p4,p5)      
#define XML_TRACE_DEBUG6(m,p1,p2,p3,p4,p5,p6)   
#endif

#define XML_PERM_LEN_MAX    4

/*****************************************************************************
**   Constants
*****************************************************************************/

const UINT8 xml_evt_rpt_elem[]                  = "MAP-event-report";
const UINT8 xml_evt_rpt_event_elem[]            = "event";
const UINT8 xml_evt_rpt_type_attr[]             = "type";
const UINT8 xml_evt_rpt_handle_attr[]           = "handle";
const UINT8 xml_evt_rpt_folder_attr[]           = "folder";
const UINT8 xml_evt_rpt_old_folder_attr[]       = "old_folder";
const UINT8 xml_evt_rpt_msg_type_attr[]         = "msg_type";
const UINT8 xml_evt_rpt_version_attr[]          = "version";
const UINT8 xml_evt_rpt_unknown[]               = "unknown";

#define XML_EVT_RPT_ELEM_ID                 0x01
#define XML_EVT_RPT_EVT_ELEM_ID             0x02
#define XML_EVT_RPT_MAX_OBJ_TAG_ID          XML_EVT_RPT_ELEM_ID
#define XML_EVT_RPT_TYPE_ATTR_ID            0x03
#define XML_EVT_RPT_HANDLE_ATTR_ID          0x04
#define XML_EVT_RPT_FOLDER_ATTR_ID          0x05
#define XML_EVT_RPT_OLD_FOLDER_ATTR_ID      0x06
#define XML_EVT_RPT_MSG_TYPE_ATTR_ID        0x07
#define XML_EVT_RPT_VERSION_ATTR_ID         0x08
#define XML_EVT_RPT_UNKNOWN_ID              0x09
#define XML_EVT_RPT_MAX_ID                  0x0a   /* keep in sync with above */
#define XML_EVT_RPT_TAG_END_ID              0x0b   /* closing tag found end=true */
#define XML_EVT_RPT_PAUSE_ID                0x0c   /* closing tag found end=false */


#define XML_EVT_RPT_TTBL_SIZE (XML_EVT_RPT_MAX_ID+1)

/*****************************************************************************
**   Type Definitions
*****************************************************************************/

typedef struct
{
    const UINT8 *p_name;
    UINT8        len;
} tXML_EVT_RPT_TTBL_ELEM;

typedef tXML_EVT_RPT_TTBL_ELEM tXML_EVT_RPT_TTBL[];              /* Tag Table */


static const tXML_EVT_RPT_TTBL_ELEM xml_erp_ttbl[XML_EVT_RPT_TTBL_SIZE] = 
{                      /* index (EVT_RPT_XP_*_ID) & name                 */
    {(UINT8*) "",  XML_EVT_RPT_MAX_ID-1}, /* \x00  Number of elements in array */
    /* XML EVT_RPT element (TAG) name */
    {xml_evt_rpt_elem, 16},             /* x01 MAP-event-report */
    {xml_evt_rpt_event_elem, 5},        /* 0x02 event */
    {xml_evt_rpt_type_attr, 4},         /* x03 type */
    {xml_evt_rpt_handle_attr, 6},       /* x04 handle */
    {xml_evt_rpt_folder_attr, 6},       /* x05 folder */
    {xml_evt_rpt_old_folder_attr, 10},  /* x06 old_folder */
    {xml_evt_rpt_msg_type_attr, 8},     /* x07 msg_type */
    {xml_evt_rpt_version_attr, 7},      /* x08 version */
    {xml_evt_rpt_unknown, 7}            /* x09 unknown */
};

#define XML_MAP_EVT_RPT_PTBL_SIZE 0x03
typedef UINT8 * tXML_MAP_EVT_RPT_PTBL_ELEM;

static const tXML_MAP_EVT_RPT_PTBL_ELEM xml_erp_ptbl[XML_MAP_EVT_RPT_PTBL_SIZE] = 
{
    (UINT8 *) "\x01",  /* index x00, all valide attributes in above list */
    (UINT8 *) "\x08\x02",  /* x01 attributes and sub-tags supported */
    (UINT8 *) "\x03\x04\x05\x06\x07" /* x02: event report attributes */
};


#if (EVT_RPT_DEBUG_XML == TRUE)
void xml_erp_debug_str(tXML_STR *p_str, UINT8 *p_buf)
{
    int          dbg_len;
    if ( (p_str == NULL) || (NULL==p_str->p))
        BCM_STRNCPY_S( (char *)p_buf, EVT_RPT_DEBUG_LEN, "(NULL)", EVT_RPT_DEBUG_LEN-1 );
    else
    {
        dbg_len = p_str->len;
        if ( dbg_len >= EVT_RPT_DEBUG_LEN)
            dbg_len = EVT_RPT_DEBUG_LEN - 1;
        BCM_STRNCPY_S( (char *)p_buf, EVT_RPT_DEBUG_LEN, (char *)p_str->p, dbg_len);
        p_buf[dbg_len] = 0;
    }
}

#else
#define xml_erp_debug_str(p_str, p_buf)
#endif

/*****************************************************************************
 ** Function     xml_erp_proc_tag
 ** Description
 ** Parameters   -
 ** Returns
 *****************************************************************************/
static UINT8 xml_erp_proc_tag( tXML_EVT_RPT_STATE *p_st, tXML_STR *p_name, tXML_STACK *p_stk)
{
    const UINT8 *p_stag;                        /* sub tag pointer */
    UINT8        curr = p_stk->stack[p_stk->top];
#if (defined(EVT_RPT_DEBUG_XML) && EVT_RPT_DEBUG_XML == TRUE)
    UINT8        dbg_name[EVT_RPT_DEBUG_LEN];
#endif


    if (curr < XML_MAP_EVT_RPT_PTBL_SIZE)
    {
        /* Iterate over allowed sub-tags for the current tag. */
        for (p_stag = xml_erp_ptbl[curr]; p_stag && *p_stag ; p_stag++)
        {
            if (*p_stag >= XML_EVT_RPT_TTBL_SIZE)
                continue;
            if (p_name->len == xml_erp_ttbl[*p_stag].len &&
                strncmp((char *)p_name->p, (char *)xml_erp_ttbl[*p_stag].p_name, p_name->len) == 0)
            {
                p_stk->top++;
                p_stk->stack[p_stk->top] = *p_stag;

                return *p_stag;
            }
        }
    }

#if (defined(EVT_RPT_DEBUG_XML) && EVT_RPT_DEBUG_XML == TRUE)
    xml_erp_debug_str(p_name, dbg_name);
    XML_TRACE_DEBUG1("xml_erp_proc_tag: bad name:%s", dbg_name );
#endif

    return XML_EVT_RPT_UNKNOWN_ID;
}


/*****************************************************************************
 ** Function     xml_erp_proc_attr
 ** Description
 ** Parameters   -
 ** Returns
 *****************************************************************************/
static UINT8 xml_erp_proc_attr(tXML_EVT_RPT_STATE *p_st, tXML_STR *p_name, tXML_STACK *p_stk)
{
    const UINT8 *p_stag;                        /* sub tag pointer */
    UINT8       curr = p_stk->stack[p_stk->top];
#if (EVT_RPT_DEBUG_XML == TRUE)
    UINT8       dbg_name[EVT_RPT_DEBUG_LEN];
#endif
    if (curr < XML_MAP_EVT_RPT_PTBL_SIZE)
    {
        /* Iterate over allowed sub-tags for the current tag. */
        for (p_stag = xml_erp_ptbl[curr]; p_stag && *p_stag; p_stag++)
        {
            if (*p_stag >= XML_EVT_RPT_TTBL_SIZE)
                continue;
            if (p_name->len == xml_erp_ttbl[*p_stag].len &&
                strncmp((char *)p_name->p, (char *)xml_erp_ttbl[*p_stag].p_name, p_name->len) == 0)
            {
                p_stk->top++;
                p_stk->stack[p_stk->top] = *p_stag;

                return *p_stag;
            }
        }
    }

#if (defined(EVT_RPT_DEBUG_XML) && EVT_RPT_DEBUG_XML == TRUE)
    xml_erp_debug_str(p_name, dbg_name);
    XML_TRACE_DEBUG1("xml_erp_proc_attr: bad name:%s", dbg_name);
#endif
    return XML_EVT_RPT_UNKNOWN_ID;
}

/*****************************************************************************
 ** Function         xml_erp_find_ch_n_copy
 ** Description      copy any chacter till one char in p_str. Any char in p_str
 **                  will stop copy pointed by p_begin
 ** Parameters
 ** Returns
 *****************************************************************************/
static void xml_erp_find_ch_n_copy( tXML_MCOPY *p_copy )
{
    const UINT8   *p_tmp;
    const UINT8   *p_str  = (UINT8 *)">";     /* was: ":=/> \t\n\r". i think we should copy till 
                                                 closing flag */
    unsigned int   last   = XML_EVT_RPT_CARRY_OVER_LEN;     /* maximum carry over len we can support */
    UINT8         *p_last = p_copy->last.p + p_copy->last.len - 1; /* point to the last char of carry
                                                                      over buffer */
    BOOLEAN        found  = FALSE;
    /* check if the last char in p_last is in p_str */
    for (p_tmp = p_str; *p_tmp; p_tmp++)
    {
        if (*p_last == *p_tmp)
        {
            found = TRUE;
            break;
        }
    } /* for */

    if (found == FALSE)
    {
        /* if not in p_str, move chars from p_begin to p_last
         * until reached last_len or any char in p_str */
        p_last++;
        last -= p_copy->last.len;       /* calculate the maximum number of chars we can copy */
        while (*(p_copy->p_begin) && last) /* rl: not sure that this buffer is termninated by a 0 */
        {
            /* copy from source (new buffer) to carry over. adjust only carry over ptr. */
            *p_last++ = *p_copy->p_begin;
            last--;
            for (p_tmp = p_str; *p_tmp; p_tmp++)
            {
                if (*p_copy->p_begin == *p_tmp)
                {
                    p_copy->p_begin++;  /* adjust pointer to point to next char to read */
                    /* calculate new length of carry over buffer contents */
                    p_copy->last.len = XML_EVT_RPT_CARRY_OVER_LEN-last;
                    *p_last = 0;        /* NULL terminate p_last. rl: not really neccessary. */
                    return;
                }
            } /* for */
            p_copy->p_begin++;  /* update now to next char. this way abort char is also copied */
        } /* while */
    } /* !found */
}

/*****************************************************************************
** Function         xml_evt_rpt_cback
** Description      
**
** Parameters
** Returns
*****************************************************************************/
static BOOLEAN xml_evt_rpt_cback( tXML_EVENT event, void *p_event_data, void *p_usr_data)
{
    tXML_MEVT_DATA      *p_ed = (tXML_MEVT_DATA*) p_event_data;
    tXML_EVT_RPT_STATE  *p_st = (tXML_EVT_RPT_STATE *) p_usr_data;
    tXML_PROP           *p_cp = &p_st->p_prop[p_st->prop_index];
    BOOLEAN              ret = TRUE;
    UINT8                next;  /* next element */
    UINT8                curr = p_ed->stack.stack[p_ed->stack.top]; /* current element */
#if (EVT_RPT_DEBUG_XML == TRUE)
    UINT8           dbg_name[EVT_RPT_DEBUG_LEN];
    UINT8           dbg_prefix[EVT_RPT_DEBUG_LEN];
    UINT8           dbg_value[EVT_RPT_DEBUG_LEN];
#endif

#if (EVT_RPT_DEBUG_XML == TRUE)
    XML_TRACE_DEBUG1("xml_evt_rpt_cback:%d", event);
#endif

    switch (event)
    {
        case XML_TAG :  /* <tag-name */
            next = xml_erp_proc_tag(p_st, &p_ed->tag.name, &p_ed->stack);
#if (EVT_RPT_DEBUG_XML == TRUE)
            xml_erp_debug_str(&p_ed->tag.name, dbg_name);
            xml_erp_debug_str(&p_ed->tag.prefix, dbg_prefix);
            XML_TRACE_DEBUG2("XML_TAG: p:%s, name:%s", dbg_prefix, dbg_name);
            XML_TRACE_DEBUG2("top:x%x, stk:x%x", p_ed->stack.top, p_ed->stack.stack[p_ed->stack.top]);

#endif
            if (next != 0)
            {
                if (next <= XML_EVT_RPT_MAX_OBJ_TAG_ID)
                    p_st->obj = next;

                if (p_st->prop_index <p_st->max_num_prop)
                {
                    /* we do not use prefix in FTC */
                    p_cp->name   = next;
                    p_cp->p_data = NULL;
                    p_cp->len    = 0;
                    p_cp->level  = p_ed->stack.top;
                    p_st->prop_index++;
                }
                else
                    ret = FALSE;
            }
            break;

        case XML_ATTRIBUTE : /* attr-name="attr-value" */
            curr = xml_erp_proc_attr(p_st, &p_ed->attr.name, &p_ed->stack);
#if (defined(EVT_RPT_DEBUG_XML) && EVT_RPT_DEBUG_XML == TRUE)
            xml_erp_debug_str(&p_ed->attr.name, dbg_name);
            xml_erp_debug_str(&p_ed->attr.prefix, dbg_prefix);
            xml_erp_debug_str(&p_ed->attr.value, dbg_value);
            XML_TRACE_DEBUG4("[xml evt_rpt] XML_ATTRIBUTE: p:%s, name:%s, v:%s, curr:%x",
                             dbg_prefix, dbg_name, dbg_value, curr );
            XML_TRACE_DEBUG2("top 1:x%x, stk:x%x", p_ed->stack.top, p_ed->stack.stack[p_ed->stack.top]);
#endif
            if ((curr != 0) && (curr != XML_EVT_RPT_UNKNOWN_ID))
            {
                if (p_st->prop_index <p_st->max_num_prop)
                {
                    p_cp->name   = curr;
                    p_cp->p_data = p_ed->attr.value.p;
                    p_cp->len    = p_ed->attr.value.len;
                    p_cp->level  = p_ed->stack.top;
                    p_st->prop_index++;
                }
                else
                    ret = FALSE;
                p_ed->stack.top--;
                XML_TRACE_DEBUG2("top 2:x%x, stk:x%x", p_ed->stack.top, p_ed->stack.stack[p_ed->stack.top]);
            }
            break;

        case XML_CHARDATA :
#if (defined(EVT_RPT_DEBUG_XML) && EVT_RPT_DEBUG_XML == TRUE)
            xml_erp_debug_str(&p_ed->ch_data.value, dbg_value);
            XML_TRACE_DEBUG2("XML_CHARDATA: v:%s, last:%d", dbg_value, p_ed->ch_data.last);
#endif
            break;

        case XML_ETAG : /* </tag-name> */
            if (p_ed->stack.top > 0)
            {
                p_ed->stack.stack[p_ed->stack.top] = 0;
                p_ed->stack.top--;
                p_st->ended = (BOOLEAN) (p_ed->stack.top == 0);
            }
#if (defined(EVT_RPT_DEBUG_XML) && EVT_RPT_DEBUG_XML == TRUE)
            xml_erp_debug_str(&p_ed->etag.name, dbg_name);
            xml_erp_debug_str(&p_ed->etag.prefix, dbg_prefix);
            XML_TRACE_DEBUG2("[xml evt_rpt] XML_ETAG: p:%s, name:%s", dbg_prefix, dbg_name);
            XML_TRACE_DEBUG2("[xml evt_rpt] top:x%x, stk:x%x", p_ed->stack.top, p_ed->stack.stack[p_ed->stack.top]);
#endif
            break;

        case XML_TAG_END: /* /> */
            curr = p_ed->stack.stack[p_ed->stack.top];

            if (p_st->prop_index <p_st->max_num_prop)
            {
                if (p_ed->empty_elem.end)
                    p_cp->name   = XML_EVT_RPT_TAG_END_ID;
                else
                    p_cp->name   = XML_EVT_RPT_PAUSE_ID;
                p_cp->p_data = NULL;
                p_cp->len    = 0;
                p_cp->level  = p_ed->stack.top;
                p_st->prop_index++;
                if (p_ed->empty_elem.end && p_ed->stack.top > 0)
                {
                    p_ed->stack.top--;
                }
            }
            else
                ret = FALSE;

#if (defined(EVT_RPT_DEBUG_XML) && EVT_RPT_DEBUG_XML == TRUE)
            XML_TRACE_DEBUG4("[xml evt_rpt] XML_TAG_END: %d, top:x%x, stk:x%x, curr:x%x",
                             p_ed->empty_elem.end, p_ed->stack.top, p_ed->stack.stack[p_ed->stack.top], curr);
#endif
            break;

        case XML_PARTIAL:
            if (p_st->p_prop[p_st->prop_index-1].name != XML_EVT_RPT_TAG_END_ID)
            {
                p_ed->stack.top--;
#if (defined(EVT_RPT_DEBUG_XML) && EVT_RPT_DEBUG_XML == TRUE)
                XML_TRACE_DEBUG0("[xml evt_rpt] adjust due to XML_PARTIAL");
#endif
            }
            break;

        case XML_COPY:
            xml_erp_find_ch_n_copy( &p_ed->copy );
            XML_TRACE_DEBUG1("[xml evt_rpt] XML_COPY: %s", p_ed->copy.last.p);
            break;

        default :
#if (defined(EVT_RPT_DEBUG_XML) && EVT_RPT_DEBUG_XML == TRUE)
            XML_TRACE_DEBUG1("[xml evt_rpt] default: XML event: %d", event);
#endif
            break;
    }

    return ret;
}




/**********************************************************************************
** Function         xml_erp_int_fill_file_folder
** Description      fill in file/folder structure. 
**
** Parameters
** Returns          xx:   > 0 : number of properties scanned, folder entry is added
**                        = 0 : no end tag found, carry over to next parse
**                        = -1: no dst_resource avaibale, all prop left to next parse
**                        = -2: exceed max entry, no folder entry added
**********************************************************************************/
static INT16 xml_erp_int_fill_evt_rpt( tXML_EVT_RPT_STATE * p_xud,
                                       tXML_PROP *p_prop,
                                       UINT16 *num_prop,
                                       UINT8  **p_dst_data,
                                       UINT16 *dst_len)
{
    INT16       xx;
    UINT16      len;
    BOOLEAN     end = FALSE;
    tXML_PROP   *cur_prop = p_prop;
    UINT8       *p_cur_offset = *p_dst_data;
    BOOLEAN     copy_attr_info;
    BOOLEAN     no_buf_left;
    UINT8       **p_attr_data = NULL;
    UINT16      *p_attr_len = NULL;

    if ( p_xud->current_entry >= p_xud->max_entry )
        return -2;

    for (xx=0; (xx < *num_prop) && !end; xx++, cur_prop++)
    {
#if (defined(EVT_RPT_DEBUG_XML) && EVT_RPT_DEBUG_XML == TRUE)
        /*     XML_TRACE_DEBUG5( "[xml evt_rpt] fill: num_prop:%d, name id: x%x, p_prop: x%x, len: %d p_data:%s",
                              (*num_prop-xx) , cur_prop->name, cur_prop, cur_prop->len, cur_prop->p_data, ); */
#endif
        copy_attr_info = TRUE;
        no_buf_left = FALSE;
        len = cur_prop->len;

        switch (cur_prop->name)
        {
            case XML_EVT_RPT_TYPE_ATTR_ID:  
                if ((*dst_len - len) > 0)   /* enough data buffer available? */
                {
                    p_attr_data = &(p_xud->p_entry[p_xud->current_entry].type) ;
                    p_attr_len = &(p_xud->p_entry[p_xud->current_entry].type_len);
                }
                else    /* run out of dst buffer resource */
                    no_buf_left = TRUE;
                break;

            case XML_EVT_RPT_HANDLE_ATTR_ID:    
                if ((*dst_len - len) > 0)   /* enough data buffer available? */
                {
                    p_attr_data = &(p_xud->p_entry[p_xud->current_entry].handle) ;
                    p_attr_len = &(p_xud->p_entry[p_xud->current_entry].handle_len);
                }
                else    /* run out of dst buffer resource */
                    no_buf_left = TRUE;
                break;
            case XML_EVT_RPT_FOLDER_ATTR_ID:    
                if ((*dst_len - len) > 0)   /* enough data buffer available? */
                {
                    p_attr_data = &(p_xud->p_entry[p_xud->current_entry].folder) ;
                    p_attr_len = &(p_xud->p_entry[p_xud->current_entry].folder_len);
                }
                else    /* run out of dst buffer resource */
                    no_buf_left = TRUE;
                break;  
            case XML_EVT_RPT_OLD_FOLDER_ATTR_ID:  
                if ((*dst_len - len) > 0)   /* enough data buffer available? */
                {
                    p_attr_data = &(p_xud->p_entry[p_xud->current_entry].old_folder) ;
                    p_attr_len = &(p_xud->p_entry[p_xud->current_entry].old_folder_len);
                }
                else    /* run out of dst buffer resource */
                    no_buf_left = TRUE;
                break;
            case XML_EVT_RPT_MSG_TYPE_ATTR_ID:
                if ((*dst_len - len) > 0)   /* enough data buffer available? */
                {

                    p_attr_data = &(p_xud->p_entry[p_xud->current_entry].msg_type) ;
                    p_attr_len = &(p_xud->p_entry[p_xud->current_entry].msg_type_len);
                }
                else    /* run out of dst buffer resource */
                    no_buf_left = TRUE;
                break;     
            case XML_EVT_RPT_TAG_END_ID:
                /* -------------------- CUSTOMER SPECIFIC ---------------------- */
                p_xud->current_entry++;     /* increment only when end tag (/>) found */
                copy_attr_info = FALSE;
                XML_TRACE_DEBUG1("[xml ml]: current_entry=%d",p_xud->current_entry);
                break;
                /* case XML_VERSION_ATTR_ID: */      
            default:
                copy_attr_info = FALSE;
                XML_TRACE_DEBUG1("[xml evt_rpt] unknown attrib: %d", cur_prop->name );
                break;
        }

        if (copy_attr_info && p_attr_data &&  p_attr_len)
        {
            *p_attr_data    = p_cur_offset;
            *p_attr_len     =  len;

            memcpy( (void *)*p_attr_data,
                    (const void *)cur_prop->p_data,
                    len );                   
            (*p_attr_data)[len] = 0;    /* null terminate string */
            p_cur_offset += (len + 1);
            *dst_len = *dst_len - (len + 1) ;

            #if (defined(EVT_RPT_DEBUG_XML) && EVT_RPT_DEBUG_XML == TRUE)

            XML_TRACE_DEBUG5("[xml evt_rpt]: Attr ID=%d val=[%s] len=%d level=%d  dst_len lef=%d", 
                             cur_prop->name,
                             *p_attr_data,
                             *p_attr_len,
                             cur_prop->level,
                             *dst_len); 
            #endif
        }

        if (no_buf_left)
        {
            return -1;
        }

    }
#if (EVT_RPT_DEBUG_XML == TRUE)
    XML_TRACE_DEBUG2("[xml evt_rpt] fill_evt_rpt: end:%d, xx:%d", end, xx);
#endif
#if 0
    /* if end tag not found -> split over two buffers. but parser will still show rest of 
       found properties. so return still found properties. */
    if (end == FALSE)
        xx = 0;
#endif

    /* keep track of current data buffer offset */
    *p_dst_data = p_cur_offset;
    return xx;
} /* xml_erp_int_fill_file_evt_rpt() */


/**********************************************************************************
** Function         xml_erp_int_fill_evt_data
**
** Description      fill in MAP event report structure. 
**
** Parameters
** Returns
**********************************************************************************/
static tXML_EVT_RPT_RES xml_erp_int_fill_evt_data( UINT8 op,
                                                   void *p_evt_data,
                                                   tXML_PROP **p_prop,
                                                   UINT16 *num_prop,
                                                   UINT8 **p_dst_data,
                                                   UINT16 *dst_len)
{
    tXML_EVT_RPT_STATE  *p_xud = (tXML_EVT_RPT_STATE *)p_evt_data;
    INT16               inc_prop;
    UINT8               prop_name;               /* Property name. */
    tXML_PROP           *cur_prop = *p_prop;
    UINT8               *p_cur_offset = *p_dst_data;

    tXML_EVT_RPT_RES     x_res = XML_EVT_RPT_OK;
    BOOLEAN             x_no_res = FALSE;       /* guard dest buffer resource */

    if ( op == 0 || op > XML_EVT_RPT_MAX_OBJ_TAG_ID || *num_prop == 0)
        return XML_EVT_RPT_ERROR;

#if EVT_RPT_DEBUG_XML
    XML_TRACE_DEBUG2( "[xml evt_rpt] xml_erp_int_fill_evt_data op:%d, num_prop:%d",
                      op, *num_prop);
#endif



    while ( *num_prop > 0 && !x_no_res )
    {
#if (defined(EVT_RPT_DEBUG_XML) && EVT_RPT_DEBUG_XML == TRUE)
        XML_TRACE_DEBUG2("end_prop:%d, name id: x%x", *num_prop, cur_prop->name);
#endif
        prop_name = cur_prop->name;
        cur_prop++;
        *num_prop -= 1;


        switch ( prop_name )
        {
            case XML_EVT_RPT_ELEM_ID:
                XML_TRACE_DEBUG0("[xml evt_rpt] xml_etag_elem");
                /* skip over version attribute which should always be 1.0. this is the top level */
                break;

            case XML_EVT_RPT_EVT_ELEM_ID:
                XML_TRACE_DEBUG0("[xml evt_rpt] xml_etag_elem");
                inc_prop = xml_erp_int_fill_evt_rpt( p_xud,
                                                     cur_prop,
                                                     num_prop,
                                                     &p_cur_offset,
                                                     dst_len);
                if (inc_prop == -1) /* no dst_buf available */
                {
                    /* backup one more prop to obtain the skipped evt_rpt/file entry header */
                    cur_prop --; 
                    *num_prop += 1;

                    x_res = XML_EVT_RPT_DST_NO_RES;
                    x_no_res = TRUE;
                }
                else if (inc_prop == -2) /* exceed max entry */
                {
                    x_no_res = TRUE;
                    x_res = XML_EVT_RPT_OUT_FULL;
                }
                else    /* found evt_rpt entry */
                {
                    cur_prop    += inc_prop;
                    *num_prop -= inc_prop;
                }
                break;

            case XML_EVT_RPT_PAUSE_ID:
                XML_TRACE_DEBUG0("[xml evt_rpt] xml_etag_elem");

#if (EVT_RPT_DEBUG_XML==TRUE)
                XML_TRACE_DEBUG0( "[xml evt_rpt] xml_erp_int_fill_evt_data(): XML_EVT_RPT_PAUSE_ID" );
#endif
                break;

            case XML_EVT_RPT_TAG_END_ID:
                XML_TRACE_DEBUG0("[xml evt_rpt] xml_etag_elem");
#if (EVT_RPT_DEBUG_XML==TRUE)
                XML_TRACE_DEBUG0( "[xml evt_rpt] xml_erp_int_fill_evt_data(): XML_EVT_RPT_TAG_END_ID" );
#endif
                break;

            default:
                XML_TRACE_DEBUG1( "[xml evt_rpt] xml_erp_int_fill_evt_data():unknown element: %d", prop_name );
                break;
        }
    }

    /* keep track of current filling position, and current available dst buffer */
    *p_prop = cur_prop;
    *p_dst_data = p_cur_offset;

    return x_res;
} /* xml_erp_int_fill_evt_data() */


/**************************************************************************************
** Function         XML_EvtRptInit
**
** Description      Initialize xml parser state machine.
**
** Parameters       p_xml_state: address of an xml parser state machine to be initialized, 
**                               allocate an additional space of size XML_EVT_RPT_CARRY_OVER_LEN 
**                               right after *p_xml_state to hold carry over data.
**                  p_entry    : points start of output directory entry. caller needs do 
**                               free this memory 
**                  max_entry  : max is 16 bit integer value which is the maximum number 
**                               of evt_rpt entries.
                    
**
** Returns          void
**************************************************************************************/

void XML_EvtRptInit( tXML_EVT_RPT_PARSER  *p_xml_state,
                     tXML_EVT_RPT_ENTRY    *p_entry,
                     const UINT16           max_entry  )
{
    XML_TRACE_DEBUG0("[xml evt_rpt] XML_EvtRptInit");
    /* Initialize the generic xml parser state machine.*/
    XML_InitPars( &p_xml_state->xml, xml_evt_rpt_cback, &p_xml_state->xml_user_data );

    /* User need to allocate an additional space of size XML_EVT_RPT_CARRY_OVER_LEN      */
    /* right after *p_xml_state to hold carry over data.                                */
    /* point to the end of the allocated buffer for p_xml_state, which is the           */
    /* beginning of buffer(XML_EVT_RPT_CARRY_OVER_LEN)                                   */
    p_xml_state->xml.last_bfr.p    = (UINT8 *)(p_xml_state + 1);  
    p_xml_state->xml.last_bfr.len  = XML_EVT_RPT_CARRY_OVER_LEN;

    /* Initialize user data     */
    p_xml_state->xml_user_data.p_entry = p_entry;   
    p_xml_state->xml_user_data.current_entry = 0;
    p_xml_state->xml_user_data.max_name_len  = XML_UI_ENTRY_MAX_NAME_LEN + 1;
    p_xml_state->xml_user_data.max_entry = (UINT16)max_entry;
    p_xml_state->xml_user_data.prop_num = 0;
}


/**************************************************************************************
** Function         XML_EvtRptParse
**
** Description      This function is called to parse the xml data received from OBEX 
**                  into folder entries associated with properties value. It can also be 
**                  used as clean up function to delete left over data from previous parse.
**                  This clean up function is typically used when application runs out of 
**                  resource and decides to discard extra xml data received.
**
** Parameters       p_xml_state: pointer to a xml parser initialized by XML_EvtRptInit().                      
**                  xml_data: valid pointer to OBEX list data in xml format.
**                  xml_len: length of the  package, must be non-zero value.
**                  dst_data: valid pointer to the buffer for holding converted folder entry name.
                              When dst_data is NULL, clean up all remaining data in the parser.
**                  dst_len: pointer to the length of dst_data buffer, its carry out value
**                          is the number of bytes remaining buffer.When dst_len is NULL, 
**                          it will cause to flush the internal data in the parser.
**                  num_entries: current number of entries, in the end it is the total number of entries
**
** Returns          tXML_EVT_RPT_RES (see xml_erp.h)
**                  XML_PENDING: parsing not completed
**                  XML_END_LIST: found /folder-listing but no final flag detected 
**                  XML_EVT_RPT_OUT_FULL: reached max_entry -> do not call parser anymore!!! dump data
**                  XML_EVT_RPT_DST_NO_RES : run out of dst buffer resource while  
**                                          some xml data still remains.

**************************************************************************************/
tXML_EVT_RPT_RES XML_EvtRptParse( tXML_EVT_RPT_PARSER   *p_xml_state,
                                  UINT8 *xml_data,      UINT16 xml_len, 
                                  UINT8 *dst_data,      UINT16 *dst_len,
                                  UINT16               *num_entries )
{
    tXML_OS               xos;
    BOOLEAN              is_remain = TRUE;
    tXML_MUL_STATE       *p_xml   = &p_xml_state->xml;
    tXML_EVT_RPT_STATE   *p_st    = &p_xml_state->xml_user_data;
    tXML_EVT_RPT_RES      x_res   = XML_EVT_RPT_OK;
    tXML_RESULT          res     = XML_NO_PROP;
    UINT16               max_num_prop = GKI_BUF3_SIZE/sizeof(tXML_PROP); /* i hope this is sufficient for 1 */


#if (defined(EVT_RPT_DEBUG_XML) && EVT_RPT_DEBUG_XML == TRUE)
    int         xx;
    UINT8       dbg_buf[EVT_RPT_DEBUG_LEN];
    tXML_STR    str;
#endif

    XML_TRACE_DEBUG0("[xml evt_rpt] XML_EvtRptParse");
    /* if dst_data is NULL, clean up remaining data */
    if (!dst_data || !dst_len)
    {
        /* clean out remained xml data left over from last parse */
        if (p_xml_state->xml_user_data.p_prop )
        {
            GKI_freebuf( p_st->p_prop );
            p_st->p_prop = p_st->offset_prop = NULL;
            p_st->prop_num = 0;
        }
#if (EVT_RPT_DEBUG_XML == TRUE)
        XML_TRACE_DEBUG0( "[xml evt_rpt] XML_EvtRptParse() Clean up left over!");
#endif
        return x_res;
    }

    /* if illegal OBEX data or dst buffer pointer received, return ERROR */
    if (xml_len == 0 || !xml_data)
    {
        return XML_EVT_RPT_ERROR;
    }

    /* XML_EvtRptParse receive new xml data, allocate buffer to hold parsed prop */
    if (p_st->offset_prop == NULL)
    {

#if (EVT_RPT_DEBUG_XML == TRUE)
        XML_TRACE_DEBUG0( "[xml evt_rpt] XML_EvtRptParse() Receive New Data!");
        XML_TRACE_DEBUG2( "[xml evt_rpt] XML_data start[%x] end [%x]",xml_data, xml_data+xml_len);
#endif        
        is_remain = FALSE;
        if ( NULL!= (p_st->p_prop = (tXML_PROP *)GKI_getbuf( GKI_BUF3_SIZE ) ) )
        {
            /* pointing next prop to be converted into file entry */
            p_st->prop_num      = 0;                 
        }
        else
        {
            GKI_freebuf( p_xml_state );
            x_res = XML_EVT_RPT_NO_RES;
            return x_res;
        }       
    }
#if (EVT_RPT_DEBUG_XML == TRUE)
    else
    {
        XML_TRACE_DEBUG0( "[xml evt_rpt] XML_EvtRptParse(): Keep cleaning up old xml data !");
    }
#endif 
    /* update the data address */
    xos.p_begin        = xml_data;
    xos.p_end          = xml_data + xml_len;

    while ( res == XML_NO_PROP )
    {
        /* if no remaining data in p_st->p_prop, parse new xml data */
        if (!is_remain)
        {
            p_st->max_num_prop = max_num_prop;
            p_st->prop_index     = 0;
            res = XML_MulParse( p_xml, &xos );


#if (EVT_RPT_DEBUG_XML == TRUE)
            XML_TRACE_DEBUG4( "xml_evt_rpt_parse obj: %x, max: %d, num: %d, res: %d",
                              p_st->obj, max_num_prop, p_st->prop_index, res);

            if (res != 0)
            {
                XML_TRACE_DEBUG1( "XML_MulParse Parsing: %d !!!", res);
            }

            for (xx=0; xx<p_st->prop_index; xx++)
            {
                if ( p_st->p_prop[xx].name < XML_EVT_RPT_MAX_ID )
                {
                    str.p   = p_st->p_prop[xx].p_data;
                    str.len = p_st->p_prop[xx].len;
                    xml_erp_debug_str(&str, dbg_buf);
                    XML_TRACE_DEBUG5( "[xml evt_rpt] parsed: index[%d]:%d:%s: %s(%d)", p_st->prop_index-xx, p_st->p_prop[xx].level,
                                      xml_erp_ttbl[p_st->p_prop[xx].name].p_name, dbg_buf, p_st->p_prop[xx].len);
                }
                else
                {
                    XML_TRACE_DEBUG3( "[xml evt_rpt] internal prop: %d:%d:%d", xx, p_st->p_prop[xx].level,
                                      p_st->p_prop[xx].name );
                }
            }
#endif
            p_st->prop_num  = p_st->prop_index ;
            p_st->offset_prop = p_st->p_prop;
        }
        else
        {
            /* This is left over data, pick up the result from the previous parse */
            res = p_xml->pars_res;
        }

        if ( res != XML_OBJ_ST_EMPTY )
        {
            x_res = xml_erp_int_fill_evt_data( p_st->obj, p_st, &p_st->offset_prop, &p_st->prop_num, &dst_data, dst_len );

            if ( (XML_EVT_RPT_OK == x_res) && (XML_NO_END == res) )
            {
                /* XML_NO_END means that the xml is not completly finished and fill returns
                   ok when when partial filling has been ok */
                x_res = XML_EVT_RPT_PENDING;
            }

            /* all parsed xml data has been converted into file entry */
            /* or exceed max entry number , break the parsing loop   */
            if (XML_EVT_RPT_OUT_FULL != x_res && XML_EVT_RPT_DST_NO_RES != x_res)
            {
                is_remain = FALSE;
            }
            else
                break;
        }
    } /* while */

    /* free property table. at next call a new one is allocated */
    if ((x_res != XML_EVT_RPT_DST_NO_RES && p_st->p_prop) ||
        XML_EVT_RPT_OUT_FULL == x_res)
    {
        GKI_freebuf( p_st->p_prop );  
        p_st->p_prop = NULL;
        p_st->offset_prop = NULL;
    }

    if ( x_res != XML_EVT_RPT_DST_NO_RES && p_st->ended)
    {
        /* this should happen on the same time as final flag in fact */
        x_res = XML_EVT_RPT_END_LIST;   /* found closing /evt_rpt-listing */
    }


    *num_entries = p_st->current_entry; /* report number of entries found. only when ended is true
                                               application really should be interested! */
    return x_res;
}

