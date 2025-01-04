/*
  Copyright (c) 2009 Dave Gamble
 
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
 
  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.
 
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef czi_JSON__h
#define czi_JSON__h

#ifdef __cplusplus
extern "C"
{
#endif

/* czi_JSON Types: */
#define czi_JSON_False  (1 << 0)
#define czi_JSON_True   (1 << 1)
#define czi_JSON_NULL   (1 << 2)
#define czi_JSON_Number (1 << 3)
#define czi_JSON_String (1 << 4)
#define czi_JSON_Array  (1 << 5)
#define czi_JSON_Object (1 << 6)
    
#define czi_JSON_IsReference 256
#define czi_JSON_StringIsConst 512

/* The czi_JSON structure: */
typedef struct czi_JSON {
    struct czi_JSON *next,*prev;    /* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
    struct czi_JSON *child;        /* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */

    int type;                    /* The type of the item, as above. */

    char *valuestring;            /* The item's string, if type==czi_JSON_String */
    int valueint;                /* The item's number, if type==czi_JSON_Number */
    double valuedouble;            /* The item's number, if type==czi_JSON_Number */

    char *string;                /* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
} czi_JSON;

typedef struct czi_JSON_Hooks {
      void *(*malloc_fn)(size_t sz);
      void (*free_fn)(void *ptr);
} czi_JSON_Hooks;

/* Supply malloc, realloc and free functions to czi_JSON */
extern void czi_JSON_InitHooks(czi_JSON_Hooks* hooks);


/* Supply a block of JSON, and this returns a czi_JSON object you can interrogate. Call czi_JSON_Delete when finished. */
extern czi_JSON *czi_JSON_Parse(const char *value);
/* Render a czi_JSON entity to text for transfer/storage. Free the char* when finished. */
extern char  *czi_JSON_Print(czi_JSON *item);
/* Render a czi_JSON entity to text for transfer/storage without any formatting. Free the char* when finished. */
extern char  *czi_JSON_PrintUnformatted(czi_JSON *item);
/* Render a czi_JSON entity to text using a buffered strategy. prebuffer is a guess at the final size. guessing well reduces reallocation. fmt=0 gives unformatted, =1 gives formatted */
extern char *czi_JSON_PrintBuffered(czi_JSON *item,int prebuffer,int fmt);
/* Delete a czi_JSON entity and all subentities. */
extern void   czi_JSON_Delete(czi_JSON *c);

/* Returns the number of items in an array (or object). */
extern int      czi_JSON_GetArraySize(czi_JSON *array);
/* Retrieve item number "item" from array "array". Returns NULL if unsuccessful. */
extern czi_JSON *czi_JSON_GetArrayItem(czi_JSON *array,int item);
/* Get item "string" from object. Case insensitive. */
extern czi_JSON *czi_JSON_GetObjectItem(czi_JSON *object,const char *string);
extern int czi_JSON_HasObjectItem(czi_JSON *object,const char *string);
/* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when czi_JSON_Parse() returns 0. 0 when czi_JSON_Parse() succeeds. */
extern const char *czi_JSON_GetErrorPtr(void);
    
/* These calls create a czi_JSON item of the appropriate type. */
extern czi_JSON *czi_JSON_CreateNull(void);
extern czi_JSON *czi_JSON_CreateTrue(void);
extern czi_JSON *czi_JSON_CreateFalse(void);
extern czi_JSON *czi_JSON_CreateBool(int b);
extern czi_JSON *czi_JSON_CreateNumber(double num);
extern czi_JSON *czi_JSON_CreateString(const char *string);
extern czi_JSON *czi_JSON_CreateArray(void);
extern czi_JSON *czi_JSON_CreateObject(void);

/* These utilities create an Array of count items. */
extern czi_JSON *czi_JSON_CreateIntArray(const int *numbers,int count);
extern czi_JSON *czi_JSON_CreateFloatArray(const float *numbers,int count);
extern czi_JSON *czi_JSON_CreateDoubleArray(const double *numbers,int count);
extern czi_JSON *czi_JSON_CreateStringArray(const char **strings,int count);

/* Append item to the specified array/object. */
extern void czi_JSON_AddItemToArray(czi_JSON *array, czi_JSON *item);
extern void    czi_JSON_AddItemToObject(czi_JSON *object,const char *string,czi_JSON *item);
extern void    czi_JSON_AddItemToObjectCS(czi_JSON *object,const char *string,czi_JSON *item);    /* Use this when string is definitely const (i.e. a literal, or as good as), and will definitely survive the czi_JSON object */
/* Append reference to item to the specified array/object. Use this when you want to add an existing czi_JSON to a new czi_JSON, but don't want to corrupt your existing czi_JSON. */
extern void czi_JSON_AddItemReferenceToArray(czi_JSON *array, czi_JSON *item);
extern void    czi_JSON_AddItemReferenceToObject(czi_JSON *object,const char *string,czi_JSON *item);

/* Remove/Detatch items from Arrays/Objects. */
extern czi_JSON *czi_JSON_DetachItemFromArray(czi_JSON *array,int which);
extern void   czi_JSON_DeleteItemFromArray(czi_JSON *array,int which);
extern czi_JSON *czi_JSON_DetachItemFromObject(czi_JSON *object,const char *string);
extern void   czi_JSON_DeleteItemFromObject(czi_JSON *object,const char *string);
    
/* Update array items. */
extern void czi_JSON_InsertItemInArray(czi_JSON *array,int which,czi_JSON *newitem);    /* Shifts pre-existing items to the right. */
extern void czi_JSON_ReplaceItemInArray(czi_JSON *array,int which,czi_JSON *newitem);
extern void czi_JSON_ReplaceItemInObject(czi_JSON *object,const char *string,czi_JSON *newitem);

/* Duplicate a czi_JSON item */
extern czi_JSON *czi_JSON_Duplicate(czi_JSON *item,int recurse);
/* Duplicate will create a new, identical czi_JSON item to the one you pass, in new memory that will
need to be released. With recurse!=0, it will duplicate any children connected to the item.
The item->next and ->prev pointers are always zero on return from Duplicate. */

/* ParseWithOpts allows you to require (and check) that the JSON is null terminated, and to retrieve the pointer to the final byte parsed. */
/* If you supply a ptr in return_parse_end and parsing fails, then return_parse_end will contain a pointer to the error. If not, then czi_JSON_GetErrorPtr() does the job. */
extern czi_JSON *czi_JSON_ParseWithOpts(const char *value,const char **return_parse_end,int require_null_terminated);

extern void czi_JSON_Minify(char *json);
extern int czi_JSON_IsArray(const czi_JSON *const item);
extern int czi_JSON_IsObject(const czi_JSON *const item);

/* Macros for creating things quickly. */
#define czi_JSON_AddNullToObject(object,name)        czi_JSON_AddItemToObject(object, name, czi_JSON_CreateNull())
#define czi_JSON_AddTrueToObject(object,name)        czi_JSON_AddItemToObject(object, name, czi_JSON_CreateTrue())
#define czi_JSON_AddFalseToObject(object,name)        czi_JSON_AddItemToObject(object, name, czi_JSON_CreateFalse())
#define czi_JSON_AddBoolToObject(object,name,b)    czi_JSON_AddItemToObject(object, name, czi_JSON_CreateBool(b))
#define czi_JSON_AddNumberToObject(object,name,n)    czi_JSON_AddItemToObject(object, name, czi_JSON_CreateNumber(n))
#define czi_JSON_AddStringToObject(object,name,s)    czi_JSON_AddItemToObject(object, name, czi_JSON_CreateString(s))

/* When assigning an integer value, it needs to be propagated to valuedouble too. */
#define czi_JSON_SetIntValue(object,val)            ((object)?(object)->valueint=(object)->valuedouble=(val):(val))
#define czi_JSON_SetNumberValue(object,val)        ((object)?(object)->valueint=(object)->valuedouble=(val):(val))

/* Macro for iterating over an array */
#define czi_JSON_ArrayForEach(pos, head)            for(pos = (head)->child; pos != NULL; pos = pos->next)

#ifdef __cplusplus
}
#endif

#endif
