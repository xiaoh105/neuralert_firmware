/**
 *****************************************************************************************
 * @file	messages_mosq.c
 * @brief	MQTT messages handler (memory, queue, retry)
 * @todo	File name : messages_mosq.c -> mqtt_client_message.c
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
#include "mosquitto_internal.h"
#include "memory_mosq.h"
#include "messages_mosq.h"
#include "send_mosq.h"
#include "util_mosq.h"
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
#include "mqtt_msg_tbl_presvd.h"
#endif

void _mosquitto_message_cleanup(struct mosquitto_message_all **message)
{
	struct mosquitto_message_all *msg;

	if (!message || !*message)
	{
		return;
	}

	msg = *message;

	if (msg->msg.topic)
	{
		_mosquitto_free(msg->msg.topic);
	}

	if (msg->msg.payload)
	{
		_mosquitto_free(msg->msg.payload);
	}

	_mosquitto_free(msg);
}

void _mosquitto_message_cleanup_all(struct mosquitto *mosq)
{
	struct mosquitto_message_all *tmp;

	while (mosq->in_messages)
	{
		tmp = mosq->in_messages->next;
		_mosquitto_message_cleanup(&mosq->in_messages);
		mosq->in_messages = tmp;
	}

	while (mosq->out_messages)
	{
		tmp = mosq->out_messages->next;
		_mosquitto_message_cleanup(&mosq->out_messages);
		mosq->out_messages = tmp;
	}
}

int mosquitto_message_copy(struct mosquitto_message *dst, const struct mosquitto_message *src)
{
	if (!dst || !src)
	{
		return MOSQ_ERR_INVAL;
	}

	dst->mid = src->mid;
	dst->topic = _mosquitto_strdup(src->topic);

	if (!dst->topic)
	{
		return MOSQ_ERR_NOMEM;
	}

	dst->qos = src->qos;
	dst->retain = src->retain;

	if (src->payloadlen)
	{
		dst->payload = _mosquitto_malloc(src->payloadlen);

		if (!dst->payload)
		{
			_mosquitto_free(dst->topic);
			return MOSQ_ERR_NOMEM;
		}

		memcpy(dst->payload, src->payload, src->payloadlen);
		dst->payloadlen = src->payloadlen;
	}
	else
	{
		dst->payloadlen = 0;
		dst->payload = NULL;
	}

	return MOSQ_ERR_SUCCESS;
}

int _mosquitto_message_delete(struct mosquitto *mosq, uint16_t mid, enum mosquitto_msg_direction dir)
{
	struct mosquitto_message_all *message;
	int rc;

	rc = _mosquitto_message_remove(mosq, mid, dir, &message);

	if (rc == MOSQ_ERR_SUCCESS)
	{
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
		mq_msg_tbl_presvd_del(message, dir);
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
	
		_mosquitto_message_cleanup(&message);
	}

	return rc;
}

void mosquitto_message_free(struct mosquitto_message **message)
{
	struct mosquitto_message *msg;

	if (!message || !*message)
	{
		return;
	}

	msg = *message;

	if (msg->topic)
	{
		_mosquitto_free(msg->topic);
	}

	if (msg->payload)
	{
		_mosquitto_free(msg->payload);
	}

	_mosquitto_free(msg);
}

int _mosquitto_message_queue(struct mosquitto *mosq, struct mosquitto_message_all *message, enum mosquitto_msg_direction dir)
{
	int rc = 0;

	if (dir == mosq_md_out)
	{
		mosq->out_queue_len++;
		message->next = NULL;

		if (mosq->out_messages_last)
		{
			mosq->out_messages_last->next = message;
		}
		else
		{
			mosq->out_messages = message;
		}

		mosq->out_messages_last = message;

		if (message->msg.qos > 0)
		{
			if (mosq->max_inflight_messages == 0 || mosq->inflight_messages < mosq->max_inflight_messages)
			{
				mosq->inflight_messages++;
			}
			else
			{
				rc = 1;
			}
		}
	}
	else
	{
		mosq->in_queue_len++;
		message->next = NULL;

		if (mosq->in_messages_last)
		{
			mosq->in_messages_last->next = message;
		}
		else
		{
			mosq->in_messages = message;
		}

		mosq->in_messages_last = message;
	}

	return rc;
}

void _mosquitto_messages_reconnect_reset(struct mosquitto *mosq)
{
	struct mosquitto_message_all *message;
	struct mosquitto_message_all *prev = NULL;

	ut_mqtt_sem_take_recur(&(mosq->in_message_mutex), portMAX_DELAY);

	message = mosq->in_messages;
	mosq->in_queue_len = 0;

	while (message)
	{
		mosq->in_queue_len++;
		message->timestamp = 0;

		if (message->msg.qos != 2)
		{
			if (prev)
			{
				prev->next = message->next;
				_mosquitto_message_cleanup(&message);
				message = prev;
			}
			else
			{
				mosq->in_messages = message->next;
				_mosquitto_message_cleanup(&message);
				message = mosq->in_messages;
			}
		}
		else
		{
			/* Message state can be preserved here because it should match
			* whatever the client has got. */
		}

		prev = message;
		message = message->next;
	}

	mosq->in_messages_last = prev;

	xSemaphoreGiveRecursive(mosq->in_message_mutex.mutex_info);
	ut_mqtt_sem_take_recur(&(mosq->out_message_mutex), portMAX_DELAY);

	mosq->inflight_messages = 0;
	message = mosq->out_messages;
	mosq->out_queue_len = 0;

	while (message)
	{
		mosq->out_queue_len++;
		message->timestamp = 0;

		if (mosq->max_inflight_messages == 0 || mosq->inflight_messages < mosq->max_inflight_messages)
		{
			if (message->msg.qos > 0)
			{
				mosq->inflight_messages++;
			}

			if (message->msg.qos == 1)
			{
				message->state = mosq_ms_wait_for_puback;
			}
			else if (message->msg.qos == 2)
			{
				/* Should be able to preserve state. */
			}
		}
		else
		{
			message->state = mosq_ms_invalid;
		}

		prev = message;
		message = message->next;
	}

	mosq->out_messages_last = prev;

	xSemaphoreGiveRecursive(mosq->out_message_mutex.mutex_info);
}

int _mosquitto_message_remove(struct mosquitto *mosq, uint16_t mid,
							  enum mosquitto_msg_direction dir,
							  struct mosquitto_message_all **message)
{
	struct mosquitto_message_all *cur, *prev = NULL;
	bool found = false;
	int rc;

	if (dir == mosq_md_out)
	{
		ut_mqtt_sem_take_recur(&(mosq->out_message_mutex), portMAX_DELAY);
		
		cur = mosq->out_messages;

		while (cur)
		{
			if (cur->msg.mid == mid)
			{
				if (prev)
				{
					prev->next = cur->next;
				}
				else
				{
					mosq->out_messages = cur->next;
				}

				*message = cur;
				mosq->out_queue_len--;

				if (cur->next == NULL)
				{
					mosq->out_messages_last = prev;
				}
				else if (!mosq->out_messages)
				{
					mosq->out_messages_last = NULL;
				}

				if (cur->msg.qos > 0)
				{
					mosq->inflight_messages--;
				}

				found = true;
				break;
			}

			prev = cur;
			cur = cur->next;
		}

		if (found)
		{
			cur = mosq->out_messages;

			while (cur)
			{
				if (mosq->max_inflight_messages == 0 || mosq->inflight_messages < mosq->max_inflight_messages)
				{
					if (cur->msg.qos > 0 && cur->state == mosq_ms_invalid)
					{
						mosq->inflight_messages++;

						if (cur->msg.qos == 1)
						{
							cur->state = mosq_ms_wait_for_puback;
						}
						else if (cur->msg.qos == 2)
						{
							cur->state = mosq_ms_wait_for_pubrec;
						}

						rc = _mosquitto_send_publish(mosq, cur->msg.mid, cur->msg.topic, cur->msg.payloadlen,
													 cur->msg.payload, cur->msg.qos, cur->msg.retain, cur->dup);
						if (rc)
						{
							xSemaphoreGiveRecursive(mosq->out_message_mutex.mutex_info);
							return rc;
						}
					}
				}
				else
				{
					xSemaphoreGiveRecursive(mosq->out_message_mutex.mutex_info);
					return MOSQ_ERR_SUCCESS;
				}

				cur = cur->next;
			}

			xSemaphoreGiveRecursive(mosq->out_message_mutex.mutex_info);
			return MOSQ_ERR_SUCCESS;
		}
		else
		{
			xSemaphoreGiveRecursive(mosq->out_message_mutex.mutex_info);
			return MOSQ_ERR_NOT_FOUND;
		}
	}
	else
	{	
		ut_mqtt_sem_take_recur(&(mosq->in_message_mutex), portMAX_DELAY);

		cur = mosq->in_messages;

		while (cur)
		{
			if (cur->msg.mid == mid)
			{
				if (prev)
				{
					prev->next = cur->next;
				}
				else
				{
					mosq->in_messages = cur->next;
				}

				*message = cur;
				mosq->in_queue_len--;

				if (cur->next == NULL)
				{
					mosq->in_messages_last = prev;
				}
				else if (!mosq->in_messages)
				{
					mosq->in_messages_last = NULL;
				}

				found = true;
				break;
			}

			prev = cur;
			cur = cur->next;
		}

		xSemaphoreGiveRecursive(mosq->in_message_mutex.mutex_info);

		if (found)
		{
			return MOSQ_ERR_SUCCESS;
		}
		else
		{
			return MOSQ_ERR_NOT_FOUND;
		}
	}
}

int _mosquitto_message_search_mid_in_rx_queue(struct mosquitto *mosq, uint16_t mid)
{
	struct mosquitto_message_all *cur = mosq->in_messages;
	bool found = false;

	while (cur) {
		if (cur->msg.mid == mid) {
			found = true;
			break;
		}
		cur = cur->next;
	}

	if (found) {
		return MOSQ_ERR_SUCCESS;
	} else {
		return MOSQ_ERR_NOT_FOUND;
	}
}

void _mosquitto_message_retry_check_actual(struct mosquitto *mosq, struct mosquitto_message_all *messages)
{
	da16x_time_t now = mosquitto_time();

	while (messages)
	{
		if (messages->timestamp + mosq->message_retry < now)
		{
			switch (messages->state)
			{
				case mosq_ms_wait_for_puback:
				case mosq_ms_wait_for_pubrec:
				{
					messages->timestamp	= now;
					messages->dup		= true;
					_mosquitto_send_publish(mosq, messages->msg.mid, messages->msg.topic, messages->msg.payloadlen, messages->msg.payload,
											messages->msg.qos, messages->msg.retain, messages->dup);
				} break;

				case mosq_ms_wait_for_pubrel:
				{
					messages->timestamp	= now;
					messages->dup		= true;
					_mosquitto_send_pubrec(mosq, messages->msg.mid);
				} break;

				case mosq_ms_wait_for_pubcomp:
				{
					messages->timestamp	= now;
					messages->dup		= true;
					_mosquitto_send_pubrel(mosq, messages->msg.mid);
				} break;

				default:
					break;
			}
		}
		messages = messages->next;
	}
}

void _mosquitto_message_retry_check(struct mosquitto *mosq)
{
	_mosquitto_message_retry_check_actual(mosq, mosq->out_messages);
	_mosquitto_message_retry_check_actual(mosq, mosq->in_messages);
}

void mosquitto_message_retry_set(struct mosquitto *mosq, unsigned int message_retry)
{
	if (mosq)
	{
		mosq->message_retry = message_retry;
	}
}

int _mosquitto_message_out_update(struct mosquitto *mosq, uint16_t mid,
								  enum mosquitto_msg_state state)
{
	struct mosquitto_message_all *message;

	ut_mqtt_sem_take_recur(&(mosq->out_message_mutex), portMAX_DELAY);

	message = mosq->out_messages;

	while (message)
	{
		if (message->msg.mid == mid)
		{
			message->state = state;
			message->timestamp = mosquitto_time();
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
			mq_msg_tbl_presvd_update(mosq_md_out, message->msg.mid, 
									message->state, message->timestamp);
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
			xSemaphoreGiveRecursive(mosq->out_message_mutex.mutex_info);
			return MOSQ_ERR_SUCCESS;
		}

		message = message->next;
	}

	xSemaphoreGiveRecursive(mosq->out_message_mutex.mutex_info);

	return MOSQ_ERR_NOT_FOUND;
}

int mosquitto_max_inflight_messages_set(struct mosquitto *mosq,	unsigned int max_inflight_messages)
{
	if (!mosq)
	{
		return MOSQ_ERR_INVAL;
	}

	mosq->max_inflight_messages = max_inflight_messages;

	return MOSQ_ERR_SUCCESS;
}
#endif // (__SUPPORT_MQTT__)
