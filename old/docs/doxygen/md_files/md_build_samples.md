Build {#build_samples}
----------------------

## 1. Overview

This section describes how to set up and run one of the examples that are included in the DA16200/600 FreeRTOS SDK. It also provides a description and details on how the example works. The example projects provide a quick and easy method to confirm the operation of specific features of the DA16200/600 before starting the development/implementation of a complete solution using the DA16200/600 FreeRTOS SDK.
The DA16200 SDK contains many examples which demonstrate how to use the features of the DA16200. The examples included are:

- Crypto: Examples showing how to use the cryptography and security capabilities.
- DPM: Examples showing how to use the various DPM low-power sleep modes.
- ETC: Examples showing how to get the current time, Access Point scan result.
- Network: Examples showing how to use various network protocols for either a client or server application.
- Peripheral: Examples showing how to use the peripherals such as GPIO, I2C, PWM, and so on.

Before using the examples, the Eclipse development environment must be set up. See the Getting Started Guide [2] for details on setting up Eclipse and importing the DA16200 SDK into that environment.
When the environment is set up, the examples can be found in the apps/common/examples directory. Each example directory has a similar structure and contains its own projects, one for da16200 and one for da16600, which can be imported into the Eclipse environment.

## 2. Building the Sample application
1. Import SDK source to the Eclipse IDE<br>
    1. Project Import: File -> Import -> Existing project into workspace<br>
        <img src="../image_files/Samples/Import_SDK_to_the_EclipseIDE_01.png" />
        <img src="../image_files/Samples/Import_SDK_to_the_EclipseIDE_02.png" /><br>
    2. Select sample directory by select folder window: Click Browse -> Select Folder<br>
        * Crypto AES sample directory: __{SDK source location}__\examples\Crypto\Crypto_AES\projects\da16200<br>
        <img src="../image_files/Samples/Import_SDK_to_the_EclipseIDE_03.png" /><br>
    3. Check all projects and Finish<br>
        <img src="../image_files/Samples/Import_SDK_to_the_EclipseIDE_04.png" /><br>
2. Build SDK<br>
    1. Build the sdk_main project: Right-click on the project and Build Project.<br>
        <img src="../image_files/Samples/Import_SDK_to_the_EclipseIDE_05.png" />
