
Programming DA16200 with SEGGER J-Link in Eclipse.

Introduction

- The following chapter introduces the way of programming DA16200 with SEGGER J-Flash.
- For support, please contact: alvin.park@diasemi.com

Requirements

- J-Link Lite or higher (https://www.segger.com/products/debug-probes/j-link/models/model-overview/)
- J-Link software V6.98 or later (https://www.segger.com/downloads/jlink/)
- Eclipse 2021-06 (4.20.0)

Preparation

- The J-Link setup procedure required in order to work with J-Flash is described in chapter 2 of the J-Link / J-Trace User Guide (UM08001).

Setup

- The Flash Loader installation
  In order to use the flash loader for DA16200, it should be installed with the following steps.
  
  1. Browse to the installation of the J-Link software package(ex. C:\Program Files (x86)\SEGGER\JLink). You should now see JFlash.exe, JLinkDevices.xml, ...

  2. Open JLinkDevices.xml in a text editor and add the device entry at the beginning, right after the <Database> opening tag.

  <!--                      -->
  <!-- Dialog Semiconductor -->
  <!--                      -->
  <Device>
   <ChipInfo Vendor="Dialog Semiconductor" Name="DA16200.SeggerES.4MB" Core="JLINK_CORE_CORTEX_M4" WorkRAMAddr="0x83000" WorkRAMSize="0x00020000" />
   <FlashBankInfo Name="QSPI Flash" BaseAddr="0x10000000" MaxSize="0x400000" Loader="Devices/Dialog/ES_DA16200_4MB.elf" LoaderType="FLASH_ALGO_TYPE_OPEN" />
  </Device>
  <Device>
   <ChipInfo Vendor="Dialog Semiconductor" Name="DA16200.SeggerES.2MB" Core="JLINK_CORE_CORTEX_M4" WorkRAMAddr="0x83000" WorkRAMSize="0x00020000" />
   <FlashBankInfo Name="QSPI Flash" BaseAddr="0x10000000" MaxSize="0x200000" Loader="Devices/Dialog/ES_DA16200_2MB.elf" LoaderType="FLASH_ALGO_TYPE_OPEN" />
  </Device>
  <Device>
   <ChipInfo Vendor="Dialog Semiconductor" Name="DA16200.eclipse.4MB" Core="JLINK_CORE_CORTEX_M4" WorkRAMAddr="0x83000" WorkRAMSize="0x00020000" />
   <FlashBankInfo Name="QSPI Flash" BaseAddr="0x10000000" MaxSize="0x400000" Loader="Devices/Dialog/DA16200_4MB.elf" LoaderType="FLASH_ALGO_TYPE_OPEN" />
  </Device>
  <Device>
   <ChipInfo Vendor="Dialog Semiconductor" Name="DA16200.eclipse.2MB" Core="JLINK_CORE_CORTEX_M4" WorkRAMAddr="0x83000" WorkRAMSize="0x00020000" />
   <FlashBankInfo Name="QSPI Flash" BaseAddr="0x10000000" MaxSize="0x200000" Loader="Devices/Dialog/DA16200_2MB.elf" LoaderType="FLASH_ALGO_TYPE_OPEN" />
  </Device>


  3. Copy the flash loader files (sdk_root\utility\j-link\flashloader\scripts\Devices\*.*), referenced in the JLinkDevices.xml entry, into the same directory where also the JLinkDevices.xml is located.

- The path of J-Link installation in Eclipse.
  1. Go to Menu > Window > Preferences > Run/Debug > String Substitution
  2. Select 'New...'
  3. Input the name as 'jlink_path' and the variable as the path of installation of the J-Link software (ex. C:\Program Files (x86)\SEGGER\JLink).   

- Import the 'scripts' project in Eclipse.
  1. The location of the project is {sdk_root}\utility\j-link\project
  
Programming

- Run program_qspi_jtag_win script in the External Tools
  1. Select a project in the Project Explorer to be programmed.
  2. Go to Menu > Run > External Tools.
  3. Select 'jlink_program_all_win'.
