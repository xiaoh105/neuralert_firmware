README for DTLS Client Application
====

## 1. Overview
The DTLS client application is a counterpart application on Test PC to demonstrate DA16200 DTLS server sample application. It's implemented base on [the Bouncy Castle Crypto APIs](http://www.bouncycastle.org/java.html/) with JAVA language.
The DTLS client application periodically sends a message per 5 sec to DTLS server after establishing DTLS session.

## 2. Behavior
The behavior of the DTLS client application is:
-    To establish DTLS session (Not able to send Certificate message during DTLS handshake)
-    To send a message per 5 seconds to DTLS server.

## 3. Building the Application
1. Open workspace for DTLS client application.
Click File -> Import -> General -> Existing Projects into Workspace
2. Select directory of DTLS client application.
3. Click Run As Java Application.
> *Notes:*
> You need the following tools to build the DTLS client application:
> Eclipse IDE for Java Developers(https://www.eclipse.org/downloads/packages/)
> Version: 2020-09 (4.17.0) or latest

## 4. Usage
* dtls_client_v1.0.exe [IP address] [Port]
    * Options
    IP address: IP address of DTLS server. If no input, the default is 192.168.0.2.
    Port: Port number of DTLS server. If no input, the default port number is 10199.

## 5. Release Notes
* Release Update - December, 09, 2020
Version 1.0, first offered to release on December, 09, 2020.
New - None
Fixed - None
Changed - None
Unresolved - None
