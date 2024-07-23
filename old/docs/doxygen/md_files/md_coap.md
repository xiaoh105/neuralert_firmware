The Constrained Application Protocol (CoAP) {#coap}
============================================================

## Overview

DA16200 provides CoAP(s) server & client features. The Constrained Application Protocol (CoAP) is a specialized web transfer protocol for use with constrained nodes and constrained networks in the Internet of Things. CoAP is designed for use between devices on the same constrained network (e.g., low-power, lossy networks), between devices and general nodes on the Internet, and between devices on different constrained networks both joined by an internet. CoAP is also being used via other mechanisms, such as SMS on mobile communication networks.

## Features

CoAP has the following main features:
- Web protocol fulfilling M2M requirements in constrained environments.
- UDP binding with optional reliability supporting unicast and multicast requests.
- Asynchronous message exchanges.
- Low header overhead and parsing complexity.
- URI and Content-type support.
- Security binding to Datagram Transport Layer Security (DTLS).

## APIs of CoAP client

CoAP client's APIs are defined in `coap_client.h` like following:
- Initialize & deinitialize CoAP client instance. The CoAP client instance should be initialized before operating CoAP client feature.
> @ref coap_client_init \n
> @ref coap_client_deinit \n
- Set CoAP-URI & Proxy-URI. The CoAP-URI should be setup before sending CoAP request.
> @ref coap_client_set_uri \n
> @ref coap_client_set_proxy_uri \n
- Send CoAP request. DA16200 CoAP Client supports 6 types CoAP requests(GET, PUT, POST, DELETE, PING, and Observe).
> @ref coap_client_request_get \n
> @ref coap_client_request_put \n
> @ref coap_client_request_post \n
> @ref coap_client_request_delete \n
> @ref coap_client_ping \n
> @ref coap_client_set_observe_notify \n
- Receive CoAP response. The CoAP response message will be received after sending CoAP request from DA16200. The following function will pass the CoAP response message to User.
> @ref coap_client_recv_response \n

## APIs of CoAP server

CoAP server's APIs are defined in `coap_server.h` like following:
- Start CoAP(s) server.
> @ref coap_server_start \n
> @ref coaps_server_start \n
- Stop CoAP(s) server.
> @ref coap_server_stop \n
