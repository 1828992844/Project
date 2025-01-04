#include "czi_util_json.h"

// 读取JSON文件
cJSON* CziUtilJson_ReadJsonFile(const char *filename)
{
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(fileSize + 1);
    fread(buffer, 1, fileSize, file);
    buffer[fileSize] = '\0';

    fclose(file);

    cJSON* json = cJSON_Parse(buffer);
    free(buffer);

    return json;
}

// 添加一个键值
void CziUtilJson_AddJsonKeyValue(cJSON *json, const char *key, const char *value)
{
    cJSON_AddStringToObject(json, key, value);
}

// 保存JSON文件
void CziUtilJson_SaveJsonFile(const char *filename, cJSON *json)
{
    FILE* file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Failed to open file for writing: %s\n", filename);
        return;
    }
    char* jsonString = cJSON_Print(json);
    fprintf(file, "%s", jsonString);
    fclose(file);
    free(jsonString);
}

void CziUtilJson_ModifyKey(cJSON *json, const char *key, const char *value)
{
    cJSON *item = cJSON_GetObjectItem(json, key);
    strcpy(item->valuestring, value);
    printf("modified item:%s\n",cJSON_Print(item));
    if (item == NULL) {
        printf("Key '%s' not found.", key);
        return;
    }
}

void CziUtilJson_CJsonSaveKey(cJSON *json, const char *key, const char *value)
{
    cJSON *item = cJSON_GetObjectItem(json, key);
    if (item == NULL) {
        cJSON_AddStringToObject(json, key, value);
        CziUtilJson_SaveJsonFile(JSON_FILE_PATH, json);
        printf("Key '%s' not found.\n", key);
        return;
    }
    strcpy(item->valuestring, value);
    printf("save item: %s %s\n", key, cJSON_Print(item));
    CziUtilJson_SaveJsonFile(JSON_FILE_PATH, json);
}

unsigned int CziUtilJson_ReadKeyString(cJSON *json, const char *key, char *save_data)
{
    cJSON *item = cJSON_GetObjectItem(json, key);
    if (item == NULL) {
        printf("Key '%s' not found.", key);
        return -1;
    };
    printf("get get :: %s",item->valuestring);
    strcpy(save_data, item->valuestring);
    return 0;
}

int CziUtilJson_ReadKeyInt(cJSON *json, const char *key, int *save_data)
{
    cJSON *item = cJSON_GetObjectItem(json, key);
    if (item == NULL) {
        printf("Key '%s' not found.", key);
        return -1;
    }
    *save_data = item->valueint;
    return 0;
}

int CziUtilJson_ReadJsonKey_StringtoInt(cJSON *json, const char *key)
{
    cJSON *item = cJSON_GetObjectItem(json, key);
    if(item == NULL){
        CziUtilJson_CJsonSaveKey(json, key, "0");
        return -1;
    }
    return (int)*item->valuestring;
}