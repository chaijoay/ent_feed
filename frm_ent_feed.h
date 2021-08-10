///
///
/// FACILITY    : Get and Prepare Subscribers(Customer Account and Mobile Account) as Entities for HPE FRM
///
/// FILE NAME   : frm_ent_feed.h
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
#ifndef __ENT_FEED_H__
#define __ENT_FEED_H__

#ifdef  __cplusplus
    extern "C" {
#endif

#define _APP_NAME_          "frm_ent_feed"
#define _APP_VERS_          "1.0"
#define _MAX_REC_FILE_      1000000


// ----- INI Parameters -----
// All Section
typedef enum {
    E_OUTPUT = 0,
    E_COMMON,
    E_DBCONN_SFN,
    E_DBCONN_ERM,
    E_DEF_GRP,
    E_NOF_SECTION
} E_INI_SECTION;

typedef enum {
    // OUTPUT Section
    E_OUT_DIR = 0,
    E_OUT_FPREF,
    E_OUT_FSUFF,
    E_NOF_PAR_OUTPUT
} E_INI_OUTPUT_SEC;

typedef enum {
    // COMMON Section
    E_TMP_DIR,
    E_BCKUP,
    E_BCKUP_DIR,
    E_LOG_DIR,
    E_LOG_LEVEL,
    E_REC_COMMIT,
    E_NOF_PAR_COMMON
} E_INI_COMMON_SEC;

typedef enum {
    // SUBFNT DB Section
    E_SFN_USER = 0,
    E_SFN_PASSWORD,
    E_SFN_DB_SID,
    E_SFN_PURGE_DAY,
    E_NOF_PAR_DBCONN_SFN
} E_INI_DBCONN_SFN_SEC;

typedef enum {
    // ERM DB Section
    E_ERM_USER = 0,
    E_ERM_PASSWORD,
    E_ERM_DB_SID,
    E_NOF_PAR_DBCONN_ERM
} E_INI_DBCONN_ERM_SEC;

typedef enum {
    E_CUSTOMER,
    E_MOBILE,
    E_DEALER,
    E_NOF_PAR_DEFGRP
} E_INI_DEF_GROUP;


int     readConfig(int argc, char *argv[]);
void    logHeader();
void    printUsage();
int     validateIni();
int     _ini_callback(const char *section, const char *key, const char *value, void *userdata);
void    makeIni();


#ifdef  __cplusplus
    }
#endif


#endif  // __ENT_FEED_H__

