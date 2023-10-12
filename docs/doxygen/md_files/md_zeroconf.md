Zero-configuration Networking (Zeroconf) {#zeroconf}
============================================================

## Overview

Zero-configuration networking (zeroconf) is a set of technologies that automatically creates a usable computer network based on the Internet Protocol Suite (TCP/IP) when computers or network peripherals are interconnected. It does not require manual operator intervention or special configuration servers. Without zeroconf, a network administrator must set up network services, such as Dynamic Host Configuration Protocol (DHCP) and Domain Name System (DNS), or configure each computer's network settings manually.\n
Zeroconf is built on three core technologies: automatic assignment of numeric network addresses for networked devices, automatic distribution and resolution of computer hostnames, and automatic location of network services, such as printing devices.\n
Basically DA16200 provides mDNS, xmDNS, DNS-SD, and Local-link address features to provide Zero-configuration networking service.

## Features

- Local-link Address
- Multicast DNS(mDNS)
- eXtensible Multicast DNS(xmDNS)
- DNS-Based Service Discovery(DNS-SD)

## Definitions

DA16200 provides Zero-configuration networking service. The `__SUPPORT_ZERO_CONFIG__` definition is prerequisite condition to activate Zero-configuration networking service. When `__SUPPORT_ZERO_CONFIG__` definition is enabled, the mDNS service is active basically. If xmDNS and DNS-SD services are required, the definitions(`__SUPPORT_XMDNS__` and `__SUPPORT_DNS_SD__`) should be enabled.
- `__SUPPORT_ZERO_CONFIG__`
  + `__SUPPORT_XMDNS__`
  + `__SUPPORT_DNS_SD__`

## APIs of Multicast DNS

mDNS APIs are defined in `zero_config.h` like the following:
- Start and stop mDNS service.
> @ref zeroconf_start_mdns_service \n
> @ref zeroconf_stop_mdns_service \n
- Lookup mDNS service.
> @ref zeroconf_lookup_mdns_cmd \n
- Change hostname of mDNS service.
> @ref zeroconf_change_mdns_hostname_cmd \n
- Check mDNS service status. Registering mDNS service will take time for probing and announcement. The following API will be helpful to check mDNS service's status.
> @ref zeroconf_is_running_mdns_service \n

## APIs of eXtensible Multicast DNS

xmDNS APIs are defined in `zero_config.h` like the following:
- Start and stop xmDNS service.
> @ref zeroconf_start_xmdns_service \n
> @ref zeroconf_stop_xmdns_service \n
- Lookup xmDNS service.
> @ref zeroconf_lookup_xmdns_cmd \n
- Change hostname of xmDNS service.
> @ref zeroconf_change_xmdns_hostname_cmd \n

## APIs of DNS-Based Service Discovery

DNS-SD APIs are defined in `zero_config.h` like the following:
- Register and deregister a service in DNS-SD.
> @ref zeroconf_register_dns_sd_service \n
> @ref zeroconf_unregister_dns_sd_service \n
