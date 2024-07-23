/**
 *****************************************************************************************
 * @file		memory_mosq.c
 * @brief	Memory-releated APIs (malloc, calloc, realloc, free)
 * @todo		File name : memory_mosq.c -> mqtt_client_memory.c
 *****************************************************************************************
 */

/*
Copyright (c) 2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.

The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.

Contributors:
   Roger Light - initial implementation and documentation.

Copyright (c) 2019-2022 Modified by Renesas Electronics.
*/

#include "sdk_type.h"

#if defined (__SUPPORT_MQTT__)

#include "mqtt_client.h"
#include "memory_mosq.h"
#include "os.h"

#undef USE_HEAP_MEMORY

static inline void * mos_realloc(void *ptr, size_t size)
{
	void *result;

	if (ptr == NULL) {
		return os_zalloc(size);
	} else {
		result = os_zalloc(size);
		if (result == NULL)
			return NULL;
	}

	memcpy(result, ptr, size);
	
	vPortFree(ptr);
	
	return result;
}

static inline char * mos_strdup(const char *str)
{
	size_t len;
	char *result;

	// if str is null, allocate 1 byte and initialize with '\0'
	if (str == NULL)
	{
		len = 0;		
	}
	else
	{
		len = strlen(str);
	}

	result = pvPortMalloc(len + 1);

	if(result) {
		if (len == 0)
		{
			result[0] = '\0';
		}
		else
		{
			memcpy(result, str, len + 1);
		}
	}

	return result;
}

void *_mosquitto_calloc(size_t nmemb, size_t size)
{
	void *mem = os_calloc(nmemb, size);
	return mem;
}

void *_mosquitto_malloc(size_t size)
{
	void* mem = pvPortMalloc(size);
	return mem;
}

void *_mosquitto_realloc(void *ptr, size_t size)
{
	void *mem;
	
	mem = mos_realloc(ptr, size);
	return mem;
}

char *_mosquitto_strdup(const char *s)
{
	char* str = mos_strdup(s);

	return str;
}

void _mosquitto_free(void *mem)
{
	if (!mem)
	{
		return;
	}
	
	vPortFree(mem);		
}
#endif // (__SUPPORT_MQTT__)
