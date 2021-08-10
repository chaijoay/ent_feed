///
///
/// FACILITY    : db utility for
///
/// FILE NAME   : frm_ent_feed_dbu.h
///
/// AUTHOR      : Thanakorn Nitipiromchai
///
/// CREATE DATE : 21-Jun-2019
///
/// CURRENT VERSION NO : 1.0
///
/// LAST RELEASE DATE  : 21-Jun-2019
///
/// MODIFICATION HISTORY :
///     1.0     21-Jun-2019     First Version
///
///
#ifndef __ENT_FEED_DBU_H__
#define __ENT_FEED_DBU_H__

#ifdef  __cplusplus
    extern "C" {
#endif

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>

// #include <sqlca.h>
// #include <sqlda.h>
// #include <sqlcpr.h>

#include "frm_ent_feed.h"
#include "strlogutl.h"
#include "glb_str_def.h"
#include "procsig.h"

#define NOT_FOUND               1403
#define KEY_DUP                 -1
#define DEADLOCK                -60
#define FETCH_OUTOFSEQ          -1002

#define SIZE_GEN_STR            100
#define SIZE_SQL_STR            1024

#define     PREPAID             "0"
#define     POSTPAID            "1"

#define TWO                     2
#define DAY_TO_CHN_GROUP        90
#define INS_CUS                 "IC"
#define INS_MOB                 "IS"
#define INS_DLR                 "ID"
#define DEL_CUS                 "DC"
#define DEL_MOB                 "DS"
#define DEL_DLR                 "DD"

#define FLG_CMPL_FED            'Y'
#define FLG_CMPL_REC            'R'
#define FLG_2B_RECON            'X'
#define FLG_RECON_ERR           'E'

#define GRP_ASSG_ALL            0
#define GRP_COND_QRY            1

#define CUS_GRP                 0
#define SYS_GRP                 1

#define FMS_K_FRAGMENT_ID_COUNT 1152

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum {
    E_PROC_STAT = 0,
    E_COMPANY,
    E_CUSTCAT,
    E_FRD_TYPE,
    E_MOBSTATUS,
    E_CUSTSUBCAT,
    E_BILLSTART,
    E_PROVINCE,
    E_MOBSEGMENT,
    E_SUSP_REASON,
    E_FRD_GROUP,
    E_ORD_REASON,
    E_BILLCYCLE,
    E_NETTYPE,
    E_PROC_ID,
    E_CARDTYPE,
    E_PAYTYPE,
    E_NOF_LOV
} E_LOV_TYPE;

typedef enum {
    E_BIZ_NEW,
    E_BIZ_NOR,
    E_CORP,
    E_EXCL,
    E_GOVE,
    E_INHS,
    E_NEW_USR,
    E_NORMAL,
    E_NOF_GRP
} E_FRM_GROUP;

typedef enum {
    ET_CUSTOMER,
    ET_MOBILE,
    ET_FBB,
    ET_CPID,
    ET_SERVICEID,
    ET_DEALER,
    E_NOF_ETYPE
} E_FRM_ENT_TYPE;


// === common function ===
void    setCommitRec(int commit_cnt);
int     connectDbSfn(char *szDbUsr, char *szDbPwd, char *szDbSvr);
void    disconnSfn();
int     connectDbErm(char *szDbUsr, char *szDbPwd, char *szDbSvr);
void    disconnErm();
void    clearFields();
void    getGroupColName(int ent_type_id, char *grp_col_name);
void    getGroupId(const char *grp_name, int *gid, int *partid, int *entid, char *ent_name, int asg_mode, int qry_mode);
int     writeOutput(FILE *fp, const char *ord_type, const char *key_acc, const char *cust_acc, const char *bill_acc);


// === functions for daily entity feeding ===
int     getSubGroupAsFrmGroupId(const char *cust_cat, const char *cust_sub_cat, int active_day);
int     procOrderFms(char out_ini[][SIZE_ITEM_L], char comm_ini[][SIZE_ITEM_L], char defgrp_ini[][SIZE_ITEM_L], char feed_flg);
int     prepareRecord(char *ord_type, char *mob_no, char *cus_acc, char *bill_acc, char *dealer_cd, char *loc_cd, char *row_id, FILE *fp, char feed_flg);
int     getCustAcc(char *ord_type, char *cus_acc, FILE *fp);
int     getMobileSub(char *ord_type, char *mob_no, char *cus_acc, char *bill_acc, FILE *fp);
// int     getMobileInfo(char *mob_no, char *cus_acc, char *bill_acc);
void    getContractPhone(char *mob_no);
int     getBillingInfo(char *cus_acc, char *bill_acc);
// int     getOrderInfo(char *mob_no, char *cus_acc, char *bill_acc);
int     getCustActDate(char *cus_acc, char *actdate);
// void    getBillActDate(char *bill_acc, char *actdate);
void    getLov(int type, char *long_lov, char *lov);
int     updateProcRecord(char *row_id, char feed_flg);


// === functions for check event-loaded entity to be later update ===
int     checkTheUnknowSub(const char *def_group_name);
int     updOrderFmsForUnk(const char *token, const char *key_value);


// === functions for update subscriber group from entry level to normal user level ===
void    changeGroupOfMatureSubscribers(char out_ini[][SIZE_ITEM_L], char comm_ini[][SIZE_ITEM_L]);
int     changeMatureSubscriberGrp(int entid, const char *ent_name, int partid, int old_gid, int new_gid, const char *grp_col, char out_ini[][SIZE_ITEM_L], char comm_ini[][SIZE_ITEM_L]);


// === functions for reconcile between SUBFNT and ERM ===
int     checkForReconcile();
int     entIdToFragId(int entid);
int     strToFragIdEntMap(const char *key_value);

// === functions for purge old data from Order_FMS table at SUBFNT ===
void    purgeTable(int purge_day);
void    getEntityTypeId();

#ifdef  __cplusplus
    }
#endif

#endif
