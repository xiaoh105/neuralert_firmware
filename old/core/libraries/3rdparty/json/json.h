/**
 ******************************************************************************
 * @file     json.h
 * @brief    Defines and variable/function for JSON message parser for DA16XXX
 * @version  -
 * @date     -
 ******************************************************************************
 */

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

  Copyright (c) 2018-2022 Modified by Renesas Electronics.
*/


#ifndef __JSON_H__
#define	__JSON_H__

#include "sdk_type.h"

/* project version */
#define CJSON_VERSION_MAJOR 1
#define CJSON_VERSION_MINOR 3
#define CJSON_VERSION_PATCH 0

/* returns the version of cJSON as a string */
const char *cJSON_Version(void);

#include <stddef.h>

/* The cJSON structure: */

#define cJSON_Invalid	(0)
#define cJSON_False		(1 << 0)
#define cJSON_True		(1 << 1)
#define cJSON_NULL		(1 << 2)
#define cJSON_Number	(1 << 3)
#define cJSON_String	(1 << 4)
#define cJSON_Array		(1 << 5)
#define cJSON_Object	(1 << 6)
#define cJSON_Raw		(1 << 7) /* raw json */

#define cJSON_IsReference	256
#define cJSON_StringIsConst	512

/// DA16200 JSON main structure
typedef struct cJSON
{
	/// next allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem
	struct cJSON	*next;
	/// prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem
	struct cJSON	*prev;
	/// An array or object item will have a child pointer pointing to a chain of the items in the array/object
	struct cJSON	*child;

	/// The type of the item, as above.
	int				type;

	/// The item's string, if type==cJSON_String  and type == cJSON_Raw
	char			*valuestring;
	/// The item's number, if type==cJSON_Number
	int				valueint;
	/// The item's number, if type==cJSON_Number
	double			valuedouble;

	/// The item's name string, if this item is the child of, or is in the list of subitems of an object.
	char			*string;
} cJSON;

typedef struct cJSON_Hooks
{
	void *(*malloc_fn)(size_t sz);
	void (*free_fn)(void *ptr);
} cJSON_Hooks;

/* Supply malloc, realloc and free functions to cJSON */
void cJSON_InitHooks(cJSON_Hooks *hooks);


/**
 ****************************************************************************************
 * @brief Supply a block of JSON, and this returns a cJSON object you can interrogate.
 * Call cJSON_Delete when finished.
 * @param[in] value The input for string which user wants to parse to JSON
 * @return JSON output
 ****************************************************************************************
 */
cJSON*cJSON_Parse(const char *value);

/**
 ****************************************************************************************
 * @brief Render a cJSON entity to text for transfer/storage.
 * Free the char* when finished.
 * @param[in] item JSON structure input which user wants to print
 * @return JSON string output
 ****************************************************************************************
 */
char *cJSON_Print(const cJSON *item);

/**
 ****************************************************************************************
 * @brief Render a cJSON entity to text for transfer/storage without any formatting.
 * Free the char* when finished.
 * @param[in] item JSON structure input which user wants to print
 * @return JSON string output
 ****************************************************************************************
 */
char *cJSON_PrintUnformatted(const cJSON *item);

/**
 ****************************************************************************************
 * @brief Render a cJSON entity to text using a buffered strategy.
 * @param[in] item JSON structure input which user wants to print
 * @param[in] prebuffer a guess at the final size. guessing well reduces reallocation
 * @param[in] fmt fmt=0 gives unformatted, =1 gives formatted
 * @return JSON string output
 ****************************************************************************************
 */
char *cJSON_PrintBuffered(const cJSON *item, int prebuffer, int fmt);

/**
 ****************************************************************************************
 * @brief Render a cJSON entity to text using a buffer already allocated in memory
 * with length buf_len
 * @param[in] item JSON structure input which user wants to print
 * @param[out] buf JSON string output
 * @param[in] len memory size allocated
 * @param[in] fmt fmt=0 gives unformatted, =1 gives formatted
 * @return 1 on success and 0 on failure
 ****************************************************************************************
 */
int cJSON_PrintPreallocated(cJSON *item, char *buf, const int len, const int fmt);

/**
 ****************************************************************************************
 * @brief Delete a cJSON entity and all subentities.
 * @param[out] c JSON structure to delete
 ****************************************************************************************
 */
void cJSON_Delete(cJSON *c);

/**
 ****************************************************************************************
 * @brief Returns the number of items in an array (or object).
 * @param[in] array JSON structure input
 * @return the number of items in an array or object
 ****************************************************************************************
 */
int cJSON_GetArraySize(const cJSON *array);

/**
 ****************************************************************************************
 * @brief Retrieve item number "item" from array "array".
 * @param[in] array JSON structure (array) input
 * @param[in] item array number
 * @return an item whose number "item" (NULL if unsuccessful)
 ****************************************************************************************
 */
cJSON *cJSON_GetArrayItem(const cJSON *array, int item);

/**
 ****************************************************************************************
 * @brief Get item "string" from object. Case insensitive.
 * @param[in] object JSON structure (object) input
 * @param[in] string parameter name
 * @return item value (NULL if unsuccessful)
 ****************************************************************************************
 */
cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string);

/**
 ****************************************************************************************
 * @brief Check the existence of the item "string"
 * @param[in] object JSON structure (object) input
 * @param[in] string parameter name
 * @return 1 or 0
 ****************************************************************************************
 */
int cJSON_HasObjectItem(const cJSON *object, const char *string);

/**
 ****************************************************************************************
 * @brief For analysing failed parses. This returns a pointer to the parse error.
 * You'll probably need to look a few chars back to make sense of it.
 * @return Defined when cJSON_Parse() returns 0. 0 when cJSON_Parse() succeeds.
 ****************************************************************************************
 */
const char *cJSON_GetErrorPtr(void);

cJSON *cJSON_CreateNull(void);
cJSON *cJSON_CreateTrue(void);
cJSON *cJSON_CreateFalse(void);
cJSON *cJSON_CreateBool(int b);
cJSON *cJSON_CreateNumber(double num);
cJSON *cJSON_CreateString(char *string);
cJSON *cJSON_CreateRaw(char *raw);

/**
 ****************************************************************************************
 * @brief Create a new JSON array
 * @return a JSON-formed array
 ****************************************************************************************
 */
cJSON *cJSON_CreateArray(void);

/**
 ****************************************************************************************
 * @brief Create a new JSON object 
 * @return a JSON-formed object
 ****************************************************************************************
 */
cJSON *cJSON_CreateObject(void);

/* These utilities create an Array of count items. */
cJSON *cJSON_CreateIntArray(int *numbers, int count);
cJSON *cJSON_CreateFloatArray(float *numbers, int count);
cJSON *cJSON_CreateDoubleArray(double *numbers, int count);
cJSON *cJSON_CreateStringArray(char **strings, int count);

/**
 ****************************************************************************************
 * @brief Append item to the specified array.
 * @param[out] array JSON-formed array
 * @param[in] item an array item to add
 ****************************************************************************************
 */
void cJSON_AddItemToArray(cJSON *array, cJSON *item);

/**
 ****************************************************************************************
 * @brief Append item to the specified object.
 * @param[out] object JSON-formed object
 * @param[in] string name to add
 * @param[in] item value to add
 ****************************************************************************************
 */
void cJSON_AddItemToObject(cJSON *object, char *string, cJSON *item);

/**
 ****************************************************************************************
 * @brief Use this when string is definitely const (i.e. a literal, or as good as),
 * and will definitely survive the cJSON object.
 * WARNING: When this function was used, make sure to always check that
 * (item->type & cJSON_StringIsConst) is zero before writing to `item->string
 * @param[out] object JSON-formed object
 * @param[in] string name to add
 * @param[in] item value to add
 ****************************************************************************************
 */
void cJSON_AddItemToObjectCS(cJSON *object, char *string, cJSON *item);

/**
 ****************************************************************************************
 * @brief Append reference to item to the specified array.
 * Use this when you want to add an existing cJSON to a new cJSON,
 * but don't want to corrupt your existing cJSON.
 * @param[out] array JSON-formed array
 * @param[in] item an array item to add
 ****************************************************************************************
 */
void cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item);

/**
 ****************************************************************************************
 * @brief Append reference to item to the specified object.
 * Use this when you want to add an existing cJSON to a new cJSON,
 * but don't want to corrupt your existing cJSON.
 * @param[out] object JSON-formed object
 * @param[in] string name to add
 * @param[in] item value to add
 ****************************************************************************************
 */
void cJSON_AddItemReferenceToObject(cJSON *object, char *string, cJSON *item);

/**
 ****************************************************************************************
 * @brief Detatch items from Arrays.
 * @param[in] array JSON-formed array
 * @param[in] which array number
 * @return Detatched item
 ****************************************************************************************
 */
cJSON *cJSON_DetachItemFromArray(cJSON *array, int which);

/**
 ****************************************************************************************
 * @brief Delete items from Arrays.
 * @param[out] array JSON-formed array
 * @param[in] which array number
 ****************************************************************************************
 */
void cJSON_DeleteItemFromArray(cJSON *array, int which);

/**
 ****************************************************************************************
 * @brief Detatch items from Objects.
 * @param[in] object JSON-formed object
 * @param[in] string parameter name
 * @return Detatched item
 ****************************************************************************************
 */
cJSON *cJSON_DetachItemFromObject(cJSON *object, char *string);

/**
 ****************************************************************************************
 * @brief Delete items from Objects.
 * @param[out] object JSON-formed object
 * @param[in] string parameter name
 ****************************************************************************************
 */
void cJSON_DeleteItemFromObject(cJSON *object, char *string);

/**
 ****************************************************************************************
 * @brief Insert array items.
 * @param[out] array JSON-formed array
 * @param[in] which array number
 * @param[in] newitem new item to insert
 ****************************************************************************************
 */
void cJSON_InsertItemInArray(cJSON *array, int which, cJSON *newitem);

/**
 ****************************************************************************************
 * @brief Replace array items.
 * @param[out] array JSON-formed array
 * @param[in] which array number
 * @param[in] newitem new item to replace
 ****************************************************************************************
 */
void cJSON_ReplaceItemInArray(cJSON *array, int which, cJSON *newitem);

/**
 ****************************************************************************************
 * @brief Insert object items.
 * @param[out] object JSON-formed array
 * @param[in] string parameter name
 * @param[in] newitem new item to replace
 ****************************************************************************************
 */
void cJSON_ReplaceItemInObject(cJSON *object, char *string, cJSON *newitem);

/* Duplicate a cJSON item */
/**
 ****************************************************************************************
 * @brief Duplicate a cJSON item
 * @param[in] item JSON-formed item (array or object) to duplicate
 * @param[in] recurse With recurse!=0, it will duplicate any children connected to the
 * item.
 * @return the item duplicated ( The item->next and ->prev pointers are always zero
 * on return from Duplicate.)
 ****************************************************************************************
 */
cJSON *cJSON_Duplicate(const cJSON *item, int recurse);

/* Duplicate will create a new, identical cJSON item to the one you pass,
   in new memory that will need to be released. With recurse!=0,
   it will duplicate any children connected to the item. The item->next and
   ->prev pointers are always zero on return from Duplicate. */

/* ParseWithOpts allows you to require (and check) that the JSON is null
   terminated, and to retrieve the pointer to the final byte parsed. */
/* If you supply a ptr in return_parse_end and parsing fails,
   then return_parse_end will contain a pointer to the error.
   If not, then cJSON_GetErrorPtr() does the job. */
cJSON *cJSON_ParseWithOpts(const char *value, const char **return_parse_end, int require_null_terminated);

void cJSON_Minify(char *json);

/* Macros for creating things quickly. */
#define	cJSON_AddNullToObject(object,name)		cJSON_AddItemToObject(object, name,cJSON_CreateNull())
#define	cJSON_AddTrueToObject(object,name)		cJSON_AddItemToObject(object, name, cJSON_CreateTrue())
#define	cJSON_AddFalseToObject(object,name)		cJSON_AddItemToObject(object, name, cJSON_CreateFalse())
#define	cJSON_AddBoolToObject(object,name,b)	cJSON_AddItemToObject(object, name, cJSON_CreateBool(b))
#define	cJSON_AddNumberToObject(object,name,n)	cJSON_AddItemToObject(object, name, cJSON_CreateNumber(n))
#define	cJSON_AddStringToObject(object,name,s)	cJSON_AddItemToObject(object, name, cJSON_CreateString(s))
#define	cJSON_AddRawToObject(object,name,s)		cJSON_AddItemToObject(object, name, cJSON_CreateRaw(s))

/* When assigning an integer value, it needs to be propagated to valuedouble too. */
/* helper for the cJSON_SetNumberValue macro */
double cJSON_SetNumberHelper(cJSON *object, double number);
#define	cJSON_SetNumberValue(object, number)	((object) ? cJSON_SetNumberHelper(object, (double)number) : (number))
#define	cJSON_SetIntValue(object, number)		((object) ? (object)->valueint = (object)->valuedouble = (number) : (number))

/* Macro for iterating over an array */
#define cJSON_ArrayForEach(pos, head)			for(pos = (head)->child; pos != NULL; pos = pos->next)

#endif	/* __JSON_H__ */
