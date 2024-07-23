README for DTLS Server Application
====

## 1. Overview
The DTLS server application is a counterpart application on Test PC to demonstrate DA16200 DTLS client sample application. It's implemented base on [the Bouncy Castle Crypto APIs](http://www.bouncycastle.org/java.html/) with JAVA language.
The DTLS server application periodically sends a message per 5 sec to DTLS client, after establishing DTLS session.

## 2. Behavior
The behavior of the DTLS server application is:
-    To allow DTLS session (Not send a CertificateRequest message during DTLS handshake)
-    To send a message per 5 seconds to DTLS client.

## 3. Building the Application
1. Open workspace for DTLS server application.
Click File -> Import -> General -> Existing Projects into Workspace
2. Select directory of DTLS server application.
3. Click Run As Java Application.
> *Notes:*
> You need the following tools to build the DTLS server application:
> Eclipse IDE for Java Developers(https://www.eclipse.org/downloads/packages/)
> Version: 2020-09 (4.17.0) or latest

## 4. Usage
* dtls_server_v1.0.exe [IP address] [Port]
    * Options
    IP address: IP address of DTLS socket. If no input, the default is local IP address.
    Port: Port number of DTLS socket. If no input, the default port number is 10199.

## 5. Release Notes
* Release Update - December, 09, 2020
Version 1.0, first offered to release on December, 09, 2020.
New - Allowed arguments to setup IP address and port number.
Fixed - None
Changed - None
Unresolved - None
