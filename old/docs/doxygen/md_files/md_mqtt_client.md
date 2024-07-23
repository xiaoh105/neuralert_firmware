MQ Telemetry Transport (MQTT) {#mqtt_client}
============================================================

## Overview

MQTT (Message Queue Telemetry Transport) is an ISO standard (ISO/IEC PRF 20922) publish-subscribe-based messaging protocol. It works on top of the TCP/IP protocol. The publisher sends (PUBLISH) messages to the subscriber through the broker. The subscriber needs to keep the connection with the broker by TCP session while the publisher can disconnect the session with the broker after sending a message.
\n
once the broker receives a message with a specific topic it sends the message to subscribers which already registered with the topic. A subscriber can register with more than one topic. There can be many or no subscribers which registered with a specific topic.
\n
![mqtt_concept](../image_files/mqtt_concept.png) \n
The exchange of MQTT messages supports QoS (Quality of Service). QoS has 3 levels (0, 1 and 2) and the process of each QoS level is as below.
\n
The DA16200 supports both publisher and subscriber functions and allows simultaneous use. The subscriber function supports DPM mode. TLS is available for message encryption.
\n

## Features:

- Basically Use One TCP port including both Subscriber and Publisher
- Support TLS, QoS, Log-in, Keep-Alive, Client ID setting
- Support DPM with MQTT Auto-start
- If disconnected, Reconnection Procedure operates.
- Support one-port MQTT including Subscriber and Publisher, if user wants.

## APIs

- Start MQTT Client \n
> @ref mqtt_client_start \n
- Stop MQTT Client \n
> @ref mqtt_client_stop \n
- Check MQTT Client Connection \n
> @ref mqtt_client_check_conn \n
\n
- Send MQTT PUBLISH message \n
> @ref mqtt_client_send_message \n
- Send MQTT PUBLISH message with timeout check \n
> @ref mqtt_client_send_message_with_qos \n

\\<!--If __MQTT_ONE_PORT__ is undefined, the changed APIs are as follow:
- Start MQTT Subscriber \n
> @ref mqtt_client_start_sub \n
- Stop MQTT Subscriber \n
> @ref mqtt_client_stop_sub \n
- Check MQTT Subscriber Connection \n
> @ref mqtt_client_check_sub_conn \n
\n
- start MQTT Publisher \n
> @ref mqtt_client_start_pub \n
- Start MQTT Publisher \n
> @ref mqtt_client_stop_pub \n
- Check MQTT Publisher Connection \n
> @ref mqtt_client_check_pub_conn \n-->