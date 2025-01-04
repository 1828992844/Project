/*******************************************************************
 * @file    czi_json_handler.h
 * @version V1.0
 * @date    2021/03/23
 * @author  Ligz
 * @copyright (c) CZZN Co.ltd
 ********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "./cJSON/czi_JSON.h"

#include "czi_json_handler.h"

static char* pfName = NULL;
static czi_JSON *root, *node;
static const char Result_Error[1024] = "F";
static int fileOpenCount = 0;

/********************************************************************
功能描述：打开配置文件函数
输入参数：fName -- 将要读取的文件名, isClear
输出参数：是否读取成�? [ 0 == 执行成功�?-1 == 执行失败 ]
********************************************************************/
char CziJsonHandler_Open(char* fName, char isClear)
{
    char Result = -1;
    long Len;
    char *Data;
    FILE* fp;

    fileOpenCount++;
    if(fileOpenCount > 1)
        printf("erorr: file muti opened in %s, pfName=%s\n\r", __func__, pfName);

    pfName = calloc(strlen(fName)+1, sizeof(char));
    snprintf(pfName, strlen(fName)+1, "%s", fName);

	if(isClear == 0)
	{
		int count = 0;
		int openFile(char * type){
			count++;
			if(count > 5)
				return -1;
			int errNum = 0;
			fp = fopen(fName, "rb");
			if(NULL == fp){
				errNum = errno;
				printf("Fail open %s reason = %s\n", fName, strerror(errNum));
				if(errno == 32){
					usleep(200000);
					return openFile(type);
				}
				else
					return -1;
			}
			fseek(fp, 0, SEEK_END);
			Len = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			Data = calloc(Len + 1, sizeof(char));
			fread(Data, 1, Len, fp);
			Data[Len]= '\0';
			root = czi_JSON_Parse(Data);

			if(!root)
			{
				czi_JSON_Delete(root);
				root = NULL;
	            printf("no json root:%s\n", Data);
				Result = -1;
			}
			else
			{
				Result = 0;
			}

			free(Data);
			fclose(fp);
			if(Result){
				usleep(200000);
				return openFile(type);
			}
			else
				return 0;
		}

		if(openFile("rb"))
			return -1;
#if 0
		int errNum = 0;
		fp = fopen(fName, "rb");
		if(NULL == fp)
		{
			errNum = errno;
            printf("Fail open %s reason = %s\n", fName, strerrno(errNum));
            free(pfName);
            pfName = NULL;
            return -1;
        }

		fseek(fp, 0, SEEK_END);
		Len = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		Data = calloc(Len + 1, sizeof(char));
		fread(Data, 1, Len, fp);
		Data[Len]= '\0';
		root = czi_JSON_Parse(Data);

		if(!root)
		{
			czi_JSON_Delete(root);
			root = NULL;
            printf("no json root:%s\n", Data);
			Result = -1;
		}
		else
		{
			Result = 0;
		}

		free(Data);
		fclose(fp);
#endif
	} // END if(New_Flag == 0)
	else
	{
		if(root)
		{
			czi_JSON_Delete(root);
			root = NULL;
		} // END if(!(!root))

		root = czi_JSON_CreateObject();
		Result = 0;
	} // END if(New_Flag == 0) else

	return Result;
} // END char CziJsonHandler_Open(char* fName)

/*Get Json String*/
char CziJsonHandler_GetRootString(char *outBuf, int bufLen){
    char *out;
    char Result = -1;
    out = czi_JSON_Print(root);
    if(strlen(out) > bufLen)
        Result = -1;
    else{
        strcpy(outBuf, out);
        Result = 0;
    }

    return Result;
}

/********************************************************************
功能描述：关闭配置文件函�?
输入参数：fName -- 将要写入的文件名
          Write_Flag -- 是否要更新文件处�? [ 1 -- 更新处理�? 0 -- 不更新处�? ]
输出参数：是否读取成�? [ 0 == 执行成功�?-1 == 执行失败 ]
********************************************************************/
char CziJsonHandler_Close(char isWrite)
{
	char *out;
	char Result = -1;
	FILE *fp = NULL;

    fileOpenCount--;

	if(0 != isWrite)
	{
		fp = fopen(pfName, "w+");

		if(!(NULL == fp))
		{
			out = czi_JSON_Print(root);
			fprintf(fp,"%s",out);
			fclose(fp);
			free(out);
			czi_JSON_Delete(root);
			root = NULL;
			Result = 0;
		} // END if(!(NULL == fp))
	} // END if(1 == Write_Flag)
	else
	{
		if(NULL != root)
		{
			czi_JSON_Delete(root);
			root = NULL;
		}
		Result = 0;
	}

    if(pfName != NULL)
    {
        free(pfName);
        pfName = NULL;
    }

	return Result;
} // END char CziJsonHandler_Close(char* fName, char Write_Flag)

/********************************************************************
功能描述：测试节点所在位置函�?
输入参数：Point -- 将要读取的节�?
          key -- 将要读取的配置变量名字符�?
输出参数：测试到的位�? [ -1 == 失败; 其它值为数据 ]
********************************************************************/
int cfg_read_Location(czi_JSON* Point, char* key)
{
	int Count;
	int Sum = 0;
	char Run = 0;
	int Res = -1;

	if(NULL != Point)
	{
		Sum = czi_JSON_GetArraySize(Point);
		if(Sum > 0)
		{
			for(Count = 0; ((Count < Sum) && (Run == 0)); Count++)
			{
				//if(memcpy((czi_JSON_GetArrayItem(Point,Count)->string), key, strlen((const char *)key)) == 0)
				if(memcmp((czi_JSON_GetArrayItem(Point,Count)->string), key, (strlen((const char *)key) + 1)) == 0)
				{
					Run = 1;
					Res = Count;
				} // END if(memcpy((czi_JSON_GetArrayItem(Point,Count)->string), *key, strlen((const char *)key)) == 0)
			} // END for(Count = 0; ((Count < Sum) && (Run == 0)); Count++)
		} // END if(Sum > 0)
	} // END if(NULL != Point)

	return Res;
} // END int cfg_read_Location(czi_JSON* Point, char* key)

/********************************************************************
功能描述：读取字符串配置参数函数
输入参数：Point -- 将要读取的节�?
          key -- 将要读取的配置变量名字符�?
输出参数：读取到的结�? [“F�? == 执行失败；否则为正确 ]
********************************************************************/
char* cfg_read_String(czi_JSON* Point, char* key)
{
	int Location;

	Location = cfg_read_Location(Point, key);

	if((-1 != Location) && ((czi_JSON_GetArrayItem(Point,Location)->type) == czi_JSON_String))
	{
		return (czi_JSON_GetArrayItem(Point,Location)->valuestring);
	} // END if((-1 != Location) && ((czi_JSON_GetArrayItem(Point,Location)->type) == czi_JSON_String))
	else
	{
		return (char*)Result_Error;
	} // END  // END if((-1 != Location) && ((czi_JSON_GetArrayItem(Point,Count)->type) == czi_JSON_String))
} // END char* cfg_read_String(czi_JSON* Point, char* key)

/********************************************************************
功能描述：读取数据类型配置参数函�?
输入参数：Point -- 将要读取的节�?
          key -- 将要读取的配置变量名字符�?
	  Result -- 将要返回的�?
输出参数：读取到的结�? [ -1 == 执行失败�?0 == 执行正确 ]
********************************************************************/
int cfg_read_Int(czi_JSON* Point, char* key, int* Result)
{
	int Location;
	int Res = -1;

	Location = cfg_read_Location(Point, key);

	if((-1 != Location) && ((czi_JSON_GetArrayItem(Point,Location)->type) == czi_JSON_Number))
	{
		*Result = czi_JSON_GetArrayItem(Point,Location)->valueint;
		Res = 0;
	} // END if((-1 != Location) && ((czi_JSON_GetArrayItem(Point,Count)->type) == czi_JSON_Number))

	return Res;
} // END int cfg_read_Int(czi_JSON* Point, char* key, int* Result)

/********************************************************************
功能描述：写入数据类型配置参数函�?
输入参数：Point -- 将要写入的节�?
          key -- 将要写入的配置变量名字符�?
	  value -- 将要写入的数�?
输出参数：读取到的结�? [ -1 == 执行失败�?0 == 执行正确 ]
********************************************************************/
int cfg_write_Int(czi_JSON* Point, char* key, int value)
{
	int Location;
	int Res = -1;
	Location = cfg_read_Location(Point, key);
	if(-1 != Location)   // 修改已有项目
	{
		if((czi_JSON_GetArrayItem(Point,Location)->type) == czi_JSON_Number)
		{
			czi_JSON_ReplaceItemInObject(Point, key, czi_JSON_CreateNumber(value));
			Res = 0;
		} // END if((czi_JSON_GetArrayItem(Point,Location)->type) == czi_JSON_Number)
	} // END if((-1 != Location)
	else // 添加项目
	{
		czi_JSON_AddNumberToObject(Point, key, value);
		Res = 0;
	}  // END if((-1 != Location) else
	return Res;
} // END int cfg_write_Int(czi_JSON* Point, char* key, int value)

/********************************************************************
功能描述：写入数据类型配置参数函�?
输入参数：Point -- 将要写入的节�?
          key -- 将要写入的配置变量名字符�?
	  value -- 将要写入的数�?
输出参数：写入到的结�? [ -1 == 执行失败�?0 == 执行正确 ]
********************************************************************/
int cfg_write_String(czi_JSON* Point, char* key, char* value)
{
	int Location;
	int Res = -1;

	Location = cfg_read_Location(Point, key);

	if(-1 != Location) // �޸�������Ŀ
	{
		if((czi_JSON_GetArrayItem(Point,Location)->type) == czi_JSON_String)
		{
 			czi_JSON_ReplaceItemInObject(Point, key, czi_JSON_CreateString(value));
			Res = 0;
		} // END ((czi_JSON_GetArrayItem(Point,Location)->type) == czi_JSON_String)
	} // END if((-1 != Location)
	else // ������Ŀ
	{
		czi_JSON_AddStringToObject(Point, key, value);
		Res = 0;
	}  // END if((-1 != Location) else

	return Res;
} // END int cfg_write_Int(czi_JSON* Point, char* key, int value)

/********************************************************************
功能描述：读取第一级字符串配置参数函数
输入参数：key -- 将要读取的配置变量名字符�?
输出参数：读取到的结�? [“F�? == 执行失败；否则为正确 ]
********************************************************************/
char* CziJsonHandler_ReadString(char* key)
{
	return cfg_read_String(root, key);
} // END char* CziJsonHandler_ReadString(char* key)

/********************************************************************
功能描述：读取第二级字符串配置参数函�?
输入参数：key -- 将要读取的配置节�?
          key2 -- 将要读取的配置变量名字符�?
输出参数：读取到的结�? [“F�? == 执行失败；否则为正确 ]
********************************************************************/
char* CziJsonHandler_Read2String(char* key, char* key2)
{
	int Location;
	char *Res;

	Location = cfg_read_Location(root, key);

	if(-1 == Location)
	{
		return (char*)Result_Error;
	} // END if(-1 == Location)
	else
	{
		node =czi_JSON_GetArrayItem(root, Location);
		Res = cfg_read_String(node, key2);
		return Res;
	} // END if(-1 == Location) else
} // END char* CziJsonHandler_Read2String(char* key, char* key2)

/********************************************************************
功能描述：读取第一级数据类型配置参数函�?
输入参数：key -- 将要读取的配置节�?
          value -- 将要读取的�?
输出参数：读取到的结�? [ -1 == 执行失败�?0 == 执行正确 ]
********************************************************************/
int CziJsonHandler_ReadInt(char* key, int* value)
{
	int Res_Value;
	int Res = cfg_read_Int(root, key, &Res_Value);
	*value = Res_Value;
	return Res;
} // END int CziJsonHandler_ReadInt(char* key, int* value);

/********************************************************************
功能描述：读取第二级数据类型配置参数函数
输入参数：key -- 将要读取的配置节�?
          value -- 将要读取的�?
输出参数：读取到的结�? [ -1 == 执行失败�?0 == 执行正确 ]
********************************************************************/
int CziJsonHandler_Read2Int(char* key, char* key2, int* value)
{
	int Location;
	int Res = -1;
	int Res_Value;

	Location = cfg_read_Location(root, key);

	if(-1 == Location)
	{
		return (-1);
	} // END if(-1 == Location)
	else
	{
		node =czi_JSON_GetArrayItem(root, Location);
		Res = cfg_read_Int(node, key2, &Res_Value);
		*value = Res_Value;
		return Res;
	} // END if(-1 == Location) else

} // END int CziJsonHandler_Read2Int(char* key, char* key2, int* value)

/********************************************************************
功能描述：写入第一级数据类型配置参数函�?
输入参数：key -- 将要写入的配置节�?
          value -- 将要写入的�?
输出参数：写入到的结�? [ -1 == 执行失败�?0 == 执行正确 ]
********************************************************************/
int CziJsonHandler_WriteInt(char* key, int value)
{
	return cfg_write_Int(root, key, value);
} // END int CziJsonHandler_WriteInt(char* key, int value)

/********************************************************************
 * 功能描述：写入第二级数据类型配置参数函数
 * 输入参数：key -- 将要写入的配置节�?
          key2 -- 将要写入的节�?
          value -- 将要写入的�?
 * 输出参数：写入到的结�? [ -1 == 执行失败�?0 == 执行正确 ]  
********************************************************************/
int CziJsonHandler_Write2Int(char* key, char* key2, int value)
{
	int Location;

	Location = cfg_read_Location(root, key);
	if(-1 == Location)
	{
		czi_JSON_AddItemToObject(root, key, node = czi_JSON_CreateObject());
	} // END if(-1 == Location)
	else
	{
		node =czi_JSON_GetArrayItem(root, Location);
	} // END if(-1 == Location) else
	return cfg_write_Int(node, key2, value);
} // END int CziJsonHandler_Write2Int(char* key, char* key2, int value)

/********************************************************************
 * 功能描述：写入第一级字符串配置参数函数
 * 输入参数：key -- 将要写入的配置节�?
          value -- 将要写入的�?
 * 输出参数：写入到的结�? [ -1 == 执行失败�?0 == 执行正确 ]  
********************************************************************/
int CziJsonHandler_WriteString(char* key, char* value)
{
	printf("CziJsonHandler_WriteString: key=%s, value=%s\n", key, value);
	return cfg_write_String(root, key, value);
} // END int CziJsonHandler_WriteString(char* key, char* value)

/********************************************************************
功能描述：写入第二级字符串配置参数函�?
输入参数：key -- 将要写入的配置节�?
          key2 -- 将要写入的节�?
          value -- 将要写入的�?
输出参数：写入到的结�? [ -1 == 执行失败�?0 == 执行正确 ]
********************************************************************/
int CziJsonHandler_Write2String(char* key, char* key2, char* value)
{
	int Location;

	Location = cfg_read_Location(root, key);

	if(-1 == Location)
	{
		czi_JSON_AddItemToObject(root, key, node = czi_JSON_CreateObject());
	} // END if(-1 == Location)
	else
	{
		node =czi_JSON_GetArrayItem(root, Location);
	} // END if(-1 == Location) else

	return cfg_write_String(node, key2, value);
}

czi_JSON* CziJsonHandler_GetRoot() {
    return root;
}
/********************** (C) COPYRIGHT CZZN *************************/

