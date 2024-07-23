JavaScript Object Notation (JSON) {#json}
============================================================

## Overview

JSON (JavaScript Object Notation) is a lightweight data-interchange format. It is easy for humans to read and write. It is easy for machines to parse and generate. JSON is a text format that is completely language independent but uses conventions that are familiar to programmers of the C-family of languages, including C, C++, C#, Java, JavaScript, Perl, Python, and many others. These properties make JSON an ideal data-interchange language. These properties make JSON an ideal data-interchange language.
JSON communication structure on the DA16200 features are as in the following. \n

- Server & Client communication structure
- Server: JavaScript-based system
- Client: C-based system (DA16200)
- Data type: Periodical diagnosis data set from Client to Server (Generating) / JSON data from server to client processing (Parsing)
- DA16200 is using the cJSON module open source. (Ultra-lightweight JSON parser in ANSI C)

## Example

This document supposes that the DA16200 system (as below) sends data on JSON format to a server using JavaScript. \n
To server, the DA16200 sends these items on the JSON data format.
- MAC address
- Connection status (Connected or Disconnected)
- Connected AP SSID
- MAC address of connected AP (BSSID)
- Connection security (1: WPA or WPA2 / 0: OPEN)
- Network information array (IP Address / Netmask / Gateway IP Address)
\n\n
### JSON Object example
~~~{.c}
{
	"MAC": "AA:FF:02:35:20:01",
	"Status": "Connected",
	"SSID": "AP_WPA2_AES",
	"BSSID": "D0:95:C7:3C:60:E9",
	"Security": 1,
	"Network": ["192.168.0.5", "255.255.0.0", "192.168.0.1"]
}
~~~
* { } : Object (The series of {STR:VAL} pair)
* [ ] : Array (Series of VAL, minimum number is 0)
* STR - Name (String)
* VAL - Value (String, Number, Boolean, Object, Array, null)
* " " means a string (Number, Boolean, null do not have it)
* Array in an array / Array in an object / Object in an array / Object in an object formats are all possible.
* Comma (,) is needed between Arrays or Objects.
\n\n

### JSON Code example
~~~{.c}
#define CONNECTED 		1
#define DISCONNECTED 	0

#define SECURITY_WPA 	1
#define SECURITY_NONE	0

void cmd_json_sample(int argc, char *argv[])
{
	// C struct for JSON
	cJSON *root; 		// Main object
	cJSON *network; 	// Array = [IP, Netmask, GW]

	// Supplicant CLI
	extern int da16x_cli_reply(char *, char *, char *);
	char result_str[1024], *cft;

	// DA16200 MAC / SSID / BSSID / Connection & Security Status
	char string_da16x_mac[18];
	char string_da16x_ssid[32];
	char string_da16x_bssid[18];
	int  status, security;

	// Network Info. (IP, Netmask, Gateway)
	char *network_strings[3];

	// JSON text
	char *test_return;


	/* Memory Alloc. */
	memset(string_da16x_mac, 0, 18);
	memset(string_da16x_ssid, 0, 32);
	memset(string_da16x_bssid, 0, 18);

	for (int i = 0; i < 3; i++)
	{
		network_strings[i] = APP_MALLOC(32);
		memset(network_strings[i], 0, 32);
	}


	/* JSON Value loading */
	getMACAddrStr(1, string_da16x_mac);

	da16x_cli_reply("status", NULL, result_str);
	cft = strstr(result_str, "wpa_state=");
	cft += 10;
	if (strncmp(cft, "COMPLETED", 9) == 0)
	{
		status = CONNECTED;

		cft = strstr(result_str, "bssid=");
		cft += 6;
		strncpy(string_da16x_bssid, cft, 17);

		da16x_cli_reply("get_network 0 ssid", NULL, result_str);
		memcpy(string_da16x_ssid, result_str + 1, strlen(result_str) - 2);

		da16x_cli_reply("get_network 0 key_mgmt", NULL, result_str);
		if (strcmp(result_str, "NONE") == 0)
		{
			security = SECURITY_NONE;
		}
		else
		{
			security = SECURITY_WPA;
		}
	}
	else
	{
		status = DISCONNECTED;
	}

	get_ip_info(WLAN0_IFACE, GET_IPADDR, network_strings[0]);
	get_ip_info(WLAN0_IFACE, GET_SUBNET, network_strings[1]);
	get_ip_info(WLAN0_IFACE, GET_GATEWAY, network_strings[2]);

	/* JSON Generate */
	root = cJSON_CreateObject();	// Generating Main Object

	cJSON_AddStringToObject(root, "MAC", string_da16x_mac);

	if (status == CONNECTED)
	{
		cJSON_AddStringToObject(root, "Status", "Connected");
		cJSON_AddStringToObject(root, "SSID", string_da16x_ssid);
		cJSON_AddStringToObject(root, "BSSID", string_da16x_bssid);

		if (security == SECURITY_WPA)
		{
			cJSON_AddNumberToObject(root, "Security", 1);
		}
		else
		{
			cJSON_AddNumberToObject(root, "Security", 0);
		}
	}
	else
	{
		cJSON_AddStringToObject(root, "Status", "Disconnected");
	}

	network = cJSON_CreateStringArray(network_strings, 3); 		// Generating Network Conf. Array 
	cJSON_AddItemToObject(root, "Network", network); 		// Writing Network Conf. Array

	/* JSON Object Print */
	test_return = cJSON_Print(root);

	PRINTF("%s\n", test_return);

	/* Memory Free */
	cJSON_Delete(root);
	cJSON_Delete(network);
	APP_FREE(test_return);
	for (int i = 0; i < 3; i++)
	{
		APP_FREE(network_strings[i]);
	}
}
~~~