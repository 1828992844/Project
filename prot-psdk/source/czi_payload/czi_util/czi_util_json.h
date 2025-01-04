#ifndef _CZI_UTIL_JSON_H_
#define _CZI_UTIL_JSON_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../dji_module/utils/cJSON.h"

#define JSON_FILE_PATH "/etc/czzn/config/data_save.json"

/**
 * @brief 添加一个键值
 * @param {cJSON} *json cjson结构体
 * @param {char} *key 键名
 * @param {char} *value 数值
 * @return {*}
 */
void CziUtilJson_AddJsonKeyValue(cJSON *json, const char *key,const char *value);
/**
 * @brief 读取json文件内容
 * @param {char} *filename 文件路径/名称
 * @return {*} cjson结构体
 */
cJSON* CziUtilJson_ReadJsonFile(const char *filename);
/**
 * @brief 将cjson结构体内容保存成一个文件
 * @param {char} *filename 文件路径/名称
 * @param {cJSON} *json cjson结构体
 * @return {*}
 */
void CziUtilJson_SaveJsonFile(const char *filename, cJSON *json);
/**
 * @brief 修改键值
 * @param {cJSON} *json cjson结构体
 * @param {char} *key 键名
 * @param {char} *value 数值
 * @return {*}
 */
void CziUtilJson_ModifyKey(cJSON *json, const char *key, const char *value);
/**
 * @brief 保存键值到文件中
 * @param {cJSON} *json cjson结构体
 * @param {char} *key 键名
 * @param {char} *value 数值
 * @return {*}
 */
void CziUtilJson_CJsonSaveKey(cJSON *json, const char *key, const char *value);
/**
 * @brief 读取字符数据
 * @param {cJSON} *json cjson结构体
 * @param {char} *key 键名
 * @param {char} *save_data 保存数据的指针
 * @return {*}
 */
unsigned int CziUtilJson_ReadKeyString(cJSON *json, const char *key, char *save_data);
/**
 * @brief 读取整形数值
 * @param {cJSON} *json cjson结构体
 * @param {char} *key 键名
 * @param {int} *save_data 保存数值的指针
 * @return {*}
 */
int CziUtilJson_ReadKeyInt(cJSON *json, const char *key, int *save_data);
/**
 * @brief 读取字符数据并转成整形数据
 * @param {cJSON} *json cjson结构体
 * @param {char} *key 键名
 * @return {*} 读取的数值
 */
int CziUtilJson_ReadJsonKey_StringtoInt(cJSON *json, const char *key);
#endif //_CZI_UTIL_JSON_H_
