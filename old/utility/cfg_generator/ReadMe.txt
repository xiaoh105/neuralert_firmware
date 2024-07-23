
The launch configurations for the configuration generator in Eclipse.

Introduction
- The following chapter introduces how to add launch configurations for the configuration generator in Eclipse.

Requirements
- Eclipse 2021-06 (4.20.0) or later


Setup

- Import script project into workspace from {SDK root}\utility\j-link\project.
  1. Go to Menu > File > Import > Existing Projects into workspace
  2. Select "{SDK root}\utility\j-link\project" folder for root directory
  3. Check SDKJFlash > Finish
  *Caution: do not change the location of the cofiguration generator excutable file in SDK. 
  
- Import the launch configuration in Eclipse.
  1. The location of the launch configurations are {sdk_root}/utility/cfg_generator
  2. Go to Menu > File > Import > Run/Debug > Launch Configurations
  3. Browse the location of the launch configurations
  4. Check cfg_generator > Finish
  
Run Configuration Generator in the External tools
  1. Go to Menu > Run > External Tools.
  2. Select 'Generate Configuration for Window or Linux'.

For more information, see UM-WI-056-DA16200_DA16600_FreeRTOS_Getting_Started_Guide