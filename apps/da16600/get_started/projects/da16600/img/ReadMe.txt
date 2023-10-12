DA16600 module has two types of flash.
To check the type of the flash on board, please follow below sequences using console command through uart0.
1. Enter "reset' for entering MROM state.
2. Enter "sflash info" in MROM prompt.
3. In case of W25Q32JW, below would be shown.
[MROM] sflash info
SFLASH:ef601615
Density:01ffffff
[MROM]
4. In case of AT25SL321, below would be shown.
[MROM] sflash info
SFLASH:1f421615
Density:01ffffff
[MROM]
5. the flash type of the target device can be selected once the macro is running.

