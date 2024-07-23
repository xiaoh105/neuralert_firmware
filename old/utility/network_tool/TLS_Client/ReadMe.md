README for TLS Client Application
====

## 1. Overview
The TLS client application is a counterpart application on Test PC to demonstrate DA16200 TLS server sample application. It's implemented base on [the Bouncy Castle Crypto APIs](http://www.bouncycastle.org/java.html/) with JAVA language.
The TLS client application periodically sends a message per 5 sec to TLS server after establishing TLS session.

## 2. Behavior
The behavior of the TLS client application is:
-    To establish DTLS session (Not able to send Certificate message during TLS handshake)
-    To send a message per 5 seconds to TLS server.

## 3. Building the Application
1. Open workspace for TLS client application.
Click File -> Import -> General -> Existing Projects into Workspace
2. Select directory of TLS client application.
3. Click Run As Java Application.
> *Notes:*
> You need the following tools to build the TLS client application:
> Eclipse IDE for Java Developers(https://www.eclipse.org/downloads/packages/)
> Version: 2020-09 (4.17.0) or latest

## 4. Usage
* tls_client_v1.0.exe [IP address] [Port]
    * Options
    IP address: IP address of TLS server. If no input, the default is 192.168.0.2.
    Port: Port number of TLS server. If no input, the default port number is 10197.

## 5. Release Notes
* Release Update - December, 09, 2020
Version 1.0, first offered to release on December, 09, 2020.
New - None
Fixed - None
Changed - None
Unresolved - None
