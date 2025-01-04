/**
 ********************************************************************
 * @author  CYN 
 * @file    czi_log.c
 * @version V0.0.0
 * @date    2024/3/8
 * @brief   This file is to indicate log.
 * @attention Code file rules:
 * rule: file encoding use UTF8;
 * rule: max line length 120 characters;
 * rule: line separator \r\n;
 * rule: use clion auto code format tool.
 */

#include <pthread.h>
#include <libgen.h>
#include <string.h>
#include <dirent.h>
#include "czi_log.h"
#include "czi_mailbox.h"
#include "czi_config.h"
#include "czi_packet.h"
#include "elog_file_cfg.h"
#include "czi_logClient.h"

static void *CziCore_TaskLogServer(void *param);
/*
 * @brief init elog 
 * @param log_name The name of log.
 * @return none
 */
void CziElog_LogInit(const char *log_name, const char *index)
{
    static char date_log[256] = "";
    struct tm nowtime;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec,&nowtime);

    sprintf(date_log,"%s_%s_%d-%d-%d_%d-%d-%d.log",
            log_name,
            index,
            nowtime.tm_year+1900,
            nowtime.tm_mon+1,
            nowtime.tm_mday,
            nowtime.tm_hour,
            nowtime.tm_min,
            nowtime.tm_sec);

    elog_init_p(date_log);
}

/*
 * @brief Log get maxIndex number 
 * @param (*buff)[LOG_INDEX_STR_LEN]:log index buff 
 * @param len:log len. 
 * @return maxIndex
 */
static unsigned int Log_GetMaxIndexNum(char (*buff)[LOG_INDEX_STR_LEN], int len)
{
    char *maxIndex = (char *)malloc(sizeof(char) * LOG_INDEX_STR_LEN);
    unsigned int indexToInt;
    strcpy(maxIndex, buff[0]);
    for (int i = 0; i < len; i++) {
        //printf("get buff : %s \n", buff[i]); 
        if (strcmp(buff[i], maxIndex) > 0) {
            strcpy(maxIndex, buff[i]);
        }
    }
    indexToInt = atol(maxIndex);  
    free(maxIndex);
    return indexToInt; 
}

/*
 * @brief Log file renumnber 
 * @param combFilePath:path and file combination. eg: /etc/czzn/czi_log
 * @param path:log bbsolute path
 * @param lDate: log date and index
 * @param lFileName: log file prefix name
 * @return maxIndex
 */
static void Log_FileRenumber(char *combFilePath, char *path, char *lDate, char *lFileName)
{
    DIR *dir;
    struct dirent *ent;
    char extractTemp[10] = "";
    char newFileName[LOG_NAME_BUFF_LEN];
    char oldFileName[LOG_NAME_BUFF_LEN];

    dir = opendir(path);
    if (NULL == dir) {
        printf("open %s directory error\n", path);
        return;
    }
    memset(extractTemp, 0x00, sizeof(extractTemp));
    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(ent->d_name, lFileName, strlen(lFileName)) != 0)
            continue;
            
        //lDate = ent->d_name + strlen(basename(CZI_USER_LOG_LOCATION));      
        lDate = ent->d_name + strlen(basename(combFilePath));      
        strncpy(extractTemp, lDate + 1, 4);
        if (strcmp(extractTemp, "0001") == 0) {
            sprintf(oldFileName, "%s/%s", path, ent->d_name);
            printf("old name : %s\n",oldFileName);
            remove(oldFileName);
            continue;
        }
        sprintf(extractTemp, "%04ld", atol(extractTemp) - 1);
        snprintf(newFileName, LOG_NAME_BUFF_LEN, "%s/%s_%s%s", path, basename(combFilePath), extractTemp, lDate + LOG_INDEX_STR_LEN);
        snprintf(oldFileName, LOG_NAME_BUFF_LEN, "%s/%s", path, ent->d_name);
        
        if (rename(oldFileName, newFileName) != 0) {
            printf("rename error\n");
        }
    }
    closedir(dir);
} 

unsigned int Log_GetLogIndex(char *combinedFilePath)
{
    DIR *dir;
    struct dirent *ent;
    char *strdupPath, *dirPath;
    char *logDate;
    char *logFileName;
    char extractBuff[MAX_LOG_NUM * ELOG_FILE_MAX_ROTATE][LOG_INDEX_STR_LEN];
    unsigned int maxIndex = 1;   //log began 1
    unsigned int logNum = 0;


    strdupPath = (char *)malloc(sizeof(char) * LOG_MALLOC_LEN);
    if (strdupPath == NULL) {
        printf("/****** strdup path memory failed ******/\n");
        return -1;
    }

    //strdupPath = strdup(CZI_USER_LOG_LOCATION);  
    strdupPath = strdup(combinedFilePath); 
    //printf("strdupPath = %s\n",strdupPath );
    logFileName = basename(combinedFilePath);
    //printf("logFileName = %s\n",logFileName); 
    dirPath = dirname(strdupPath);
    dir = opendir(dirPath);
    if (NULL == dir) {
        printf("/******* open %s directory error ******/\n", dirPath);
        free(strdupPath);
        return -1;
    }
    memset(extractBuff, 0x00, sizeof(extractBuff));
    
    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(ent->d_name, logFileName, strlen(logFileName)) != 0)
            continue;

        logDate = ent->d_name + strlen(logFileName);      
        strncpy(extractBuff[logNum], logDate + 1, 4);
        extractBuff[logNum][4] = '\0';
        logNum += 1;
    }
    closedir(dir);
    
    // for (int i = 0; i < logNum; i++) {
    //     printf("extractIndex : %s\n",extractBuff[i]);
    // }
    if (0 == logNum) {
        printf("logNum is 0\n");
        free(strdupPath);
        return maxIndex;
    }
    maxIndex = Log_GetMaxIndexNum(extractBuff, logNum);
    //printf("maxIndex : %d\n",maxIndex);
    //printf("logNum : %d\n",logNum); 
    if (MAX_LOG_NUM <= logNum) {
        Log_FileRenumber(combinedFilePath, dirPath, logDate, logFileName);
        maxIndex = MAX_LOG_NUM;
        free(strdupPath);
        return maxIndex;
    }
    free(strdupPath);
    return (maxIndex + 1);
}

int CziCommlog_BaseRecord(const char *level,  const char *tag, char *format, ...)
{
    char buffer[1024] = "";
    char args[256] = "";
    int ret = -1;
    va_list ptr;

    /* format the log */
    va_start(ptr, format);
    
    ret = vsprintf(args, format, ptr);
    if (-1 == ret)
        return -1;

    va_end(ptr);

    if(CZI_USER_LOG_MODE == SYS_MLOG_MODE){
        int logLen = 0;
        //snprintf(buffer, sizeof(buffer), "%stag:[%s]  log:(%s)", level, tag, args);
        snprintf(buffer, sizeof(buffer), "%s[%s]-[%s]-%s ", level, CZI_USER_LOG_TAG, tag, args);
    
        logLen =  strlen(tag) + strlen(level) + ret + 16; /* the maximum should be more than 16 refer to tag:xx log:xx */
        Czilog_SendData(buffer, logLen);

    } 
    if(CZI_USER_LOG_MODE == SYS_ELOG_MODE){
        snprintf(buffer, sizeof(buffer), "[%s]-[%s]-%s",CZI_USER_LOG_TAG, tag, args);
        if (0 == strcmp(level, MLOG_LOG_ASSERT_LEVEL)) 
            elog_a(tag, buffer);  
        else if (0 == strcmp(level, MLOG_LOG_ERROR_LEVEL)) 
            elog_e(tag, buffer);
        else if (0 == strcmp(level, MLOG_LOG_WARNING_LEVEL)) 
            elog_w(tag, buffer);
        else if (0 == strcmp(level, MLOG_LOG_INFO_LEVEL))
            elog_i(tag, buffer);        
        else if (0 == strcmp(level, MLOG_LOG_DEBUG_LEVEL))
            elog_d(tag, buffer);
        else if (0 == strcmp(level, MLOG_LOG_VERBOSE_LEVEL))
            elog_v(tag, buffer);
        else if (0 == strcmp(level, MLOG_LOG_NOTICE_LEVEL)) 
            elog_n(tag, buffer);
    }

    if ((CZI_USER_LOG_MODE != SYS_ELOG_MODE) && (CZI_USER_LOG_MODE != SYS_MLOG_MODE))
    {
        snprintf(buffer, sizeof(buffer), "tag:[%s]  log:(%s)", tag, args);
        printf("%s\n", buffer);
    }
}

