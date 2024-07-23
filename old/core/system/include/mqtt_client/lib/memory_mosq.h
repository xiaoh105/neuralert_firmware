/**
 *****************************************************************************************
 * @file	memory_mosq.h
 * @brief	Header file - Memory-releated APIs
 * @todo	File name : memory_mosq.h -> mqtt_client_memory.h
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

#ifndef __MEMORY_MOSQ_H__
#define __MEMORY_MOSQ_H__

/**
 ****************************************************************************************
 * @brief calloc() for mqtt_client module
 ****************************************************************************************
 */
void *_mosquitto_calloc(size_t nmemb, size_t size);

/**
 ****************************************************************************************
 * @brief free() for mqtt_client module
 ****************************************************************************************
 */
void _mosquitto_free(void *mem);

/**
 ****************************************************************************************
 * @brief malloc() for mqtt_client module
 ****************************************************************************************
 */
void *_mosquitto_malloc(size_t size);

/**
 ****************************************************************************************
 * @brief realloc() for mqtt_client module
 ****************************************************************************************
 */
void *_mosquitto_realloc(void *ptr, size_t size);

/**
 ****************************************************************************************
 * @brief strdup() for mqtt_client module
 ****************************************************************************************
 */
char *_mosquitto_strdup(const char *s);

#endif	/* __MEMORY_MOSQ_H__ */
