///
///
/// FACILITY    : Get and Prepare Subscribers(Customer Account and Mobile Account) as Entities for HPE FRM
///
/// FILE NAME   : frm_ent_feed.c
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
///     1.0         21-Jun-2019     First Version
///     1.1.1       31-Jul-2019     get customer account for change group process
///
///

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <libgen.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

#include "minIni.h"
#include "procsig.h"
#include "frm_ent_feed.h"
#include "strlogutl.h"
#include "frm_ent_feed_dbu.h"

char gszAppName[SIZE_ITEM_S];
char gszAppMode[SIZE_ITEM_S];
char gszIniFile[SIZE_FULL_NAME];

#define MODE_ENT_FED    0
#define MODE_UPD_UNK    1
#define MODE_RECONCL    2
#define MODE_CHN_GRP    3
#define MODE_PRG_TAB    4
int gzMode;


const char gszIniStrSection[E_NOF_SECTION][SIZE_ITEM_T] = {
    "OUTPUT",
    "COMMON",
    "DB_CONNECTION_SFN",
    "DB_CONNECTION_ERM",
    "DEFAULT_GROUP",
    "INFORM_HTML_LETTER"
};

const char gszIniStrOutput[E_NOF_PAR_OUTPUT][SIZE_ITEM_T] = {
    "OUTPUT_DIR",
    "OUT_FILE_PREFIX",
    "OUT_FILE_SUFFIX"
};

const char gszIniStrCommon[E_NOF_PAR_COMMON][SIZE_ITEM_T] = {
    "TMP_DIR",
    "BACKUP",
    "BACKUP_DIR",
    "LOG_DIR",
    "LOG_LEVEL",
    "REC_COMMIT"
};

const char gszIniStrDbConnSfn[E_NOF_PAR_DBCONN_SFN][SIZE_ITEM_T] = {
    "SFN_USER_NAME",
    "SFN_PASSWORD",
    "SFN_DB_SID",
    "SFN_PURGE_DAY"
};

const char gszIniStrDbConnErm[E_NOF_PAR_DBCONN_ERM][SIZE_ITEM_T] = {
    "ERM_USER_NAME",
    "ERM_PASSWORD",
    "ERM_DB_SID"
};

const char gszIniStrDefGrp[E_NOF_PAR_DEFGRP][SIZE_ITEM_T] = {
    "CUSTOMER",
    "MOBILE",
    "DEALER"
};

const char gszIniStrInfMail[E_NOF_PAR_INF_MAIL][SIZE_ITEM_T] = {
    "RECONC_TEMPLATE_FILE",
    "RECONC_PREFIX_OUTF",
    "RECONC_OUTPUT_DIR",
    "UPDATE_TEMPLATE_FILE",
    "UPDATE_PREFIX_OUTF",
    "UPDATE_OUTPUT_DIR"
};


char gszIniParOutput[E_NOF_PAR_OUTPUT][SIZE_ITEM_L];
char gszIniParCommon[E_NOF_PAR_COMMON][SIZE_ITEM_L];
char gszIniParDbConnSfn[E_NOF_PAR_DBCONN_SFN][SIZE_ITEM_L];
char gszIniParDbConnErm[E_NOF_PAR_DBCONN_ERM][SIZE_ITEM_L];
char gszIniParDefGrp[E_NOF_PAR_DEFGRP][SIZE_ITEM_L];
char gszIniParInfMail[E_NOF_PAR_INF_MAIL][SIZE_ITEM_L];

int main(int argc, char *argv[])
{

    memset(gszIniFile, 0x00, sizeof(gszIniFile));
    gzMode = -1;

    // 1. read ini file
    if ( readConfig(argc, argv) != SUCCESS ) {
        return EXIT_FAILURE;
    }

    if ( procLock(gszAppMode, E_CHK) != SUCCESS ) {
        fprintf(stderr, "another instance of %s is running (%s)\n", gszAppName, gszAppMode);
        return EXIT_FAILURE;
    }

    if ( handleSignal() != SUCCESS ) {
        fprintf(stderr, "init handle signal failed: %s\n", getSigInfoStr());
        return EXIT_FAILURE;
    }

    if ( startLogging(gszIniParCommon[E_LOG_DIR], gszAppMode, atoi(gszIniParCommon[E_LOG_LEVEL])) != SUCCESS ) {
       return EXIT_FAILURE;
    }

    if ( validateIni() == FAILED ) {
        return EXIT_FAILURE;
    }
    logHeader();

    procLock(gszAppMode, E_SET);

    // main process flow:
    // 1. connect to dbs (and also retry if any)
    // 2. query Order_FMS table to get a what to do
    // 2.1 if nothing to do then exit program
    // 2.2 if there are job to be done then process all of them finally exit program
    if ( connectDbSfn(gszIniParDbConnSfn[E_SFN_USER], gszIniParDbConnSfn[E_SFN_PASSWORD], gszIniParDbConnSfn[E_SFN_DB_SID]) != SUCCESS ) {
        procLock(gszAppMode, E_CLR);
        stopLogging();
        return EXIT_FAILURE;
    }

    if ( connectDbErm(gszIniParDbConnErm[E_ERM_USER], gszIniParDbConnErm[E_ERM_PASSWORD], gszIniParDbConnErm[E_ERM_DB_SID]) != SUCCESS ) {
        procLock(gszAppMode, E_CLR);
        stopLogging();
        return EXIT_FAILURE;
    }

    setCommitRec(atoi(gszIniParCommon[E_REC_COMMIT]));
    if ( gzMode == MODE_ENT_FED ) { // query order_fms where flag is null
        writeLog(LOG_INF, "starting with Entity Feed Mode");
        procOrderFms(gszIniParOutput, gszIniParCommon, gszIniParDefGrp, FLG_CMPL_FED);
    }
    else if ( gzMode == MODE_UPD_UNK ) {    // insert or update to order_fms with flag is null
        writeLog(LOG_INF, "starting with Unknow Entity Check Mode");
        checkTheUnknowSub(gszIniParDefGrp[E_MOBILE]);
        updAcctForUnkSub();
    }
    else if ( gzMode == MODE_RECONCL ) {
        writeLog(LOG_INF, "starting with Reconciliation Mode");
        checkForReconcile(gszIniParInfMail);
        writeLog(LOG_INF, "preparing output record of the reconciliation");
        procOrderFms(gszIniParOutput, gszIniParCommon, gszIniParDefGrp, FLG_CMPL_REC);
    }
    else if ( gzMode == MODE_CHN_GRP ) {
        writeLog(LOG_INF, "starting with Update Subscribers Group Mode");
        changeGroupOfMatureSubscribers(gszIniParOutput, gszIniParCommon, gszIniParInfMail);
    }
    else if ( gzMode == MODE_PRG_TAB ) {
        writeLog(LOG_INF, "starting with Purge Order_FMS table Mode");
        purgeTable(atoi(gszIniParDbConnSfn[E_SFN_PURGE_DAY]));
    }
    else {
        writeLog(LOG_WRN, "running mode is not specified");
    }

    disconnSfn();
    disconnErm();
    if ( isTerminated() ) {
        writeLog(LOG_INF, "%s", getSigInfoStr());
    }
    writeLog(LOG_INF, "------- %s process completely stop -------", _APP_NAME_);

    procLock(gszAppMode, E_CLR);
    stopLogging();

    return EXIT_SUCCESS;

}

int readConfig(int argc, char *argv[])
{

    char appPath[SIZE_ITEM_L];
    char mode[SIZE_ITEM_S];
    int key, i;

    memset(gszIniFile, 0x00, sizeof(gszIniFile));
    memset(gszAppName, 0x00, sizeof(gszAppName));
    memset(gszAppMode, 0x00, sizeof(gszAppMode));
    memset(mode, 0x00, sizeof(mode));

    memset(gszIniParOutput, 0x00, sizeof(gszIniParOutput));
    memset(gszIniParCommon, 0x00, sizeof(gszIniParCommon));
    memset(gszIniParDbConnSfn, 0x00, sizeof(gszIniParDbConnSfn));
    memset(gszIniParDbConnErm, 0x00, sizeof(gszIniParDbConnErm));
    memset(gszIniParDefGrp, 0x00, sizeof(gszIniParDefGrp));

    strcpy(appPath, argv[0]);
    char *p = strrchr(appPath, '/');
    *p = '\0';

    for ( i = 1; i < argc; i++ ) {
        if ( strcmp(argv[i], "-n") == 0 ) {     // specified ini file
            strcpy(gszIniFile, argv[++i]);
        }
        else if ( strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0 ) {
            printUsage();
            return FAILED;
        }
        else if ( strcmp(argv[i], "-mkini") == 0 ) {
            makeIni();
            return FAILED;
        }
        else if ( strcmp(argv[i], "-ent_fed") == 0 ) {
            strcpy(mode, "_entfed");
            gzMode = MODE_ENT_FED;
        }
        else if ( strcmp(argv[i], "-upd_unk") == 0 ) {
            strcpy(mode, "_updunk");
            gzMode = MODE_UPD_UNK;
        }
        else if ( strcmp(argv[i], "-chn_grp") == 0 ) {
            strcpy(mode, "_chngrp");
            gzMode = MODE_CHN_GRP;
        }
        else if ( strcmp(argv[i], "-reconcl") == 0 ) {
            strcpy(mode, "_reconc");
            gzMode = MODE_RECONCL;
        }
        else if ( strcmp(argv[i], "-purge") == 0 ) {
            strcpy(mode, "_purged");
            gzMode = MODE_PRG_TAB;
        }
        else {
            fprintf(stderr, "unknow '%s' parameter\n", argv[i]);
            printUsage();
            return FAILED;
        }
    }

    if ( gszIniFile[0] == '\0' ) {
        sprintf(gszIniFile, "%s/%s.ini", appPath, _APP_NAME_);
    }
    sprintf(gszAppName, "%s", _APP_NAME_);
    sprintf(gszAppMode, "%s%s", _APP_NAME_, mode);

    if ( access(gszIniFile, F_OK|R_OK) != SUCCESS ) {
        fprintf(stderr, "unable to access ini file %s (%s)\n", gszIniFile, strerror(errno));
        return FAILED;
    }

    // Read config of OUTPUT Section
    for ( key = 0; key < E_NOF_PAR_OUTPUT; key++ ) {
        ini_gets(gszIniStrSection[E_OUTPUT], gszIniStrOutput[key], "NA", gszIniParOutput[key], sizeof(gszIniParOutput[key]), gszIniFile);
    }

    // Read config of COMMON Section
    for ( key = 0; key < E_NOF_PAR_COMMON; key++ ) {
        ini_gets(gszIniStrSection[E_COMMON], gszIniStrCommon[key], "NA", gszIniParCommon[key], sizeof(gszIniParCommon[key]), gszIniFile);
    }

    // Read config of SFN DB Connection Section
    for ( key = 0; key < E_NOF_PAR_DBCONN_SFN; key++ ) {
        ini_gets(gszIniStrSection[E_DBCONN_SFN], gszIniStrDbConnSfn[key], "NA", gszIniParDbConnSfn[key], sizeof(gszIniParDbConnSfn[key]), gszIniFile);
    }

    // Read config of ERM DB Connection Section
    for ( key = 0; key < E_NOF_PAR_DBCONN_ERM; key++ ) {
        ini_gets(gszIniStrSection[E_DBCONN_ERM], gszIniStrDbConnErm[key], "NA", gszIniParDbConnErm[key], sizeof(gszIniParDbConnErm[key]), gszIniFile);
    }

    // Read config of Default Group Section
    for ( key = 0; key < E_NOF_PAR_DEFGRP; key++ ) {
        ini_gets(gszIniStrSection[E_DEF_GRP], gszIniStrDefGrp[key], "NA", gszIniParDefGrp[key], sizeof(gszIniParDefGrp[key]), gszIniFile);
    }

    // Read config of Info Mail Section
    for ( key = 0; key < E_NOF_PAR_INF_MAIL; key++ ) {
        ini_gets(gszIniStrSection[E_INF_MAIL], gszIniStrInfMail[key], "NA", gszIniParInfMail[key], sizeof(gszIniParInfMail[key]), gszIniFile);
    }

    return SUCCESS;

}

void logHeader()
{
    writeLog(LOG_INF, "---- Start %s (v%s) with following parameters ----", _APP_NAME_, _APP_VERS_);
    // print out all ini file
    ini_browse(_ini_callback, NULL, gszIniFile);
}

void printUsage()
{

    fprintf(stderr, "\nusage: %s version %s\n\n", _APP_NAME_, _APP_VERS_);
    fprintf(stderr, "\tget and prepare subscribers data(customer/mobile account) for HPE FRM\n");
    fprintf(stderr, "\tas entities update (insert/delete/update) by reading from database\n");
    fprintf(stderr, "\tthen output to files as eFIT input\n\n");
    fprintf(stderr, "%s.exe <-ent_fed|-upd_unk|-chn_grp|-reconcl|-purge> [-n <ini_file>] [-mkini]\n", _APP_NAME_);
    fprintf(stderr, "\t-ent_fed\tto perform daily feed of entities (write eFIT file)\n");
    fprintf(stderr, "\t-upd_unk\tto perform event-loaded entity check for update\n");
    fprintf(stderr, "\t-chn_grp\tto perform change subscriber group from entry to next level\n");
    fprintf(stderr, "\t-reconcl\tto perform reconcile of entity between subfnt and erm\n");
    fprintf(stderr, "\t-purge  \tto perform purge order_fms table at subfnt\n");
    fprintf(stderr, "\tini_file\tto specify ini file other than default ini\n");
    fprintf(stderr, "\t-mkini\t\tto create ini template\n");
    fprintf(stderr, "\n");

}

int validateIni()
{
    int result = SUCCESS;

    // ----- Output Section -----
    if ( access(gszIniParOutput[E_OUT_DIR], F_OK|R_OK) != SUCCESS ) {
        result = FAILED;
        fprintf(stderr, "unable to access %s %s (%s)\n", gszIniStrOutput[E_OUT_DIR], gszIniParOutput[E_OUT_DIR], strerror(errno));
    }

    // ----- Common Section -----
    if ( access(gszIniParCommon[E_TMP_DIR], F_OK|R_OK) != SUCCESS ) {
        result = FAILED;
        fprintf(stderr, "unable to access %s %s (%s)\n", gszIniStrCommon[E_TMP_DIR], gszIniParCommon[E_TMP_DIR], strerror(errno));
    }
    if ( *gszIniParCommon[E_BCKUP] == 'Y' || *gszIniParCommon[E_BCKUP] == 'y' ) {
        strcpy(gszIniParCommon[E_BCKUP], "Y");
        if ( access(gszIniParCommon[E_BCKUP_DIR], F_OK|R_OK) != SUCCESS ) {
            result = FAILED;
            fprintf(stderr, "unable to access %s %s (%s)\n", gszIniStrCommon[E_BCKUP_DIR], gszIniParCommon[E_BCKUP_DIR], strerror(errno));
        }
    }
    if ( access(gszIniParCommon[E_LOG_DIR], F_OK|R_OK) != SUCCESS ) {
        result = FAILED;
        fprintf(stderr, "unable to access %s %s (%s)\n", gszIniStrCommon[E_LOG_DIR], gszIniParCommon[E_LOG_DIR], strerror(errno));
    }

    // ----- Db Connection Section -----
    if ( *gszIniParDbConnSfn[E_SFN_USER] == '\0' || strcmp(gszIniParDbConnSfn[E_SFN_USER], "NA") == 0 ) {
        result = FAILED;
        fprintf(stderr, "invalid %s '%s'\n", gszIniStrDbConnSfn[E_SFN_USER], gszIniParDbConnSfn[E_SFN_USER]);
    }
    if ( *gszIniParDbConnSfn[E_SFN_PASSWORD] == '\0' || strcmp(gszIniParDbConnSfn[E_SFN_PASSWORD], "NA") == 0 ) {
        result = FAILED;
        fprintf(stderr, "invalid %s '%s'\n", gszIniStrDbConnSfn[E_SFN_PASSWORD], gszIniParDbConnSfn[E_SFN_PASSWORD]);
    }
    if ( *gszIniParDbConnSfn[E_SFN_DB_SID] == '\0' || strcmp(gszIniParDbConnSfn[E_SFN_DB_SID], "NA") == 0 ) {
        result = FAILED;
        fprintf(stderr, "invalid %s '%s'\n", gszIniStrDbConnSfn[E_SFN_DB_SID], gszIniParDbConnSfn[E_SFN_DB_SID]);
    }

    if ( *gszIniParDbConnErm[E_ERM_USER] == '\0' || strcmp(gszIniParDbConnErm[E_ERM_USER], "NA") == 0 ) {
        result = FAILED;
        fprintf(stderr, "invalid %s '%s'\n", gszIniStrDbConnErm[E_ERM_USER], gszIniParDbConnErm[E_ERM_USER]);
    }
    if ( *gszIniParDbConnErm[E_ERM_PASSWORD] == '\0' || strcmp(gszIniParDbConnErm[E_ERM_PASSWORD], "NA") == 0 ) {
        result = FAILED;
        fprintf(stderr, "invalid %s '%s'\n", gszIniStrDbConnErm[E_ERM_PASSWORD], gszIniParDbConnErm[E_ERM_PASSWORD]);
    }
    if ( *gszIniParDbConnErm[E_ERM_DB_SID] == '\0' || strcmp(gszIniParDbConnErm[E_ERM_DB_SID], "NA") == 0 ) {
        result = FAILED;
        fprintf(stderr, "invalid %s '%s'\n", gszIniStrDbConnErm[E_ERM_DB_SID], gszIniParDbConnErm[E_ERM_DB_SID]);
    }

    return result;

}

int _ini_callback(const char *section, const char *key, const char *value, void *userdata)
{
    if ( strstr(key, "PASSWORD") ) {
        writeLog(LOG_INF, "[%s]\t%s = ********", section, key);
    }
    else {
        writeLog(LOG_INF, "[%s]\t%s = %s", section, key, value);
    }
    return 1;
}

void makeIni()
{

    int key;
    char cmd[SIZE_ITEM_S];
    char tmp_ini[SIZE_ITEM_S];
    char tmp_itm[SIZE_ITEM_S];

    sprintf(tmp_ini, "./%s_XXXXXX", _APP_NAME_);
    mkstemp(tmp_ini);
    strcpy(tmp_itm, "<place_holder>");

    // Write config of OUTPUT Section
    for ( key = 0; key < E_NOF_PAR_OUTPUT; key++ ) {
        ini_puts(gszIniStrSection[E_OUTPUT], gszIniStrOutput[key], tmp_itm, tmp_ini);
    }

    // Write config of COMMON Section
    for ( key = 0; key < E_NOF_PAR_COMMON; key++ ) {
        ini_puts(gszIniStrSection[E_COMMON], gszIniStrCommon[key], tmp_itm, tmp_ini);
    }

    // Write config of DB Section
    for ( key = 0; key < E_NOF_PAR_DBCONN_SFN; key++ ) {
        ini_puts(gszIniStrSection[E_DBCONN_SFN], gszIniStrDbConnSfn[key], tmp_itm, tmp_ini);
    }

    // Write config of DB Section
    for ( key = 0; key < E_NOF_PAR_DBCONN_ERM; key++ ) {
        ini_puts(gszIniStrSection[E_DBCONN_ERM], gszIniStrDbConnErm[key], tmp_itm, tmp_ini);
    }

    for ( key = 0; key < E_NOF_PAR_INF_MAIL; key++ ) {
        ini_puts(gszIniStrSection[E_INF_MAIL], gszIniStrInfMail[key], tmp_itm, tmp_ini);
    }

    sprintf(cmd, "mv %s %s.ini", tmp_ini, tmp_ini);
    system(cmd);
    fprintf(stderr, "ini template file is %s.ini\n", tmp_ini);

}
