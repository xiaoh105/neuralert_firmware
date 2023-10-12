rem How to use this batch file
rem 1. put the following files in a folder
rem 	[SDK_ROOT]\1.UTILS\combo\ble_img_creator\mkimage.exe
rem 	[SDK_ROOT]\1.UTILS\combo\ble_img_creator\for_ota_test\app_version_1.h
rem 	[SDK_ROOT]\1.UTILS\combo\ble_img_creator\for_ota_test\app_version_2.h
rem 	[SDK_ROOT]\1.UTILS\combo\ble_img_creator\for_ota_test\app_version_3.h
rem 	[SDK_ROOT]\1.UTILS\combo\ble_img_creator\for_ota_test\img_gen.bat
rem 	[SDK_ROOT]\1.UTILS\combo\ble_pre-built_img\DA14531_v6.0.14.1114\DA14531_1\pxr_sr_coex_ext_531_6_0_14_1114.bin
rem 	[SDK_ROOT]\1.UTILS\combo\ble_pre-built_img\DA14531_v6.0.14.1114\DA14531_2\pxm_coex_ext_531_6_0_14_1114.bin
rem 2. open command prompt and run the command below

rem 	img_gen.bat [base_version] [version_1_file] [version_2_file] [version_3_file]
rem 	e.g.) img_gen.bat 531_6_0_14_1114 app_version_1.h app_version_2.h app_version_3.h

rem ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

rem [base_version] used for 'naming" a img file <-- has nothing to do with the real version name embedded to .img file
rem [base_version]    		  :1 %1
rem [version_1_file]          :2 %2
rem [version_2_file]          :3 %3
rem [version_3_file]          :4 %4

set base_ver=%1
set ver_1=%2
set ver_2=%3
set ver_3=%4

set proxr_img_prefix=pxr_sr_coex_ext
rem set proxm_img_prefix=pxm_coex_ext
set proxr_bin=%proxr_img_prefix%_%1.bin
rem set proxm_bin=%proxm_img_prefix%_%1.bin
set proxr_out_multi=da14531_multi_part_proxr
rem set proxm_out_multi=da14531_multi_part_proxm
set out_folder=img_set

@echo on

mkdir %out_folder%

rem ------------------------------
rem 1. proxr multi-part image ....

mkimage single %proxr_bin% %ver_1% %proxr_img_prefix%_%base_ver%_1.img
timeout -t 1

mkimage single %proxr_bin% %ver_2% %proxr_img_prefix%_%base_ver%_2.img
timeout -t 1

mkimage multi spi %proxr_img_prefix%_%base_ver%_2.img 0x20 %proxr_img_prefix%_%base_ver%_1.img 0x8000 0xFF20 %proxr_out_multi%.img
timeout -t 1

copy .\%proxr_out_multi%.img	.\%out_folder%\
timeout -t 1

rem ------------------------------
rem 3. ota test image : v1, v2, and v3
mkimage single_ota %proxr_bin% %ver_1% %proxr_img_prefix%_%base_ver%_1_single.img
timeout -t 1

mkimage single_ota %proxr_bin% %ver_2% %proxr_img_prefix%_%base_ver%_2_single.img
timeout -t 1

mkimage single_ota %proxr_bin% %ver_3% %proxr_img_prefix%_%base_ver%_3_single.img
timeout -t 1

mkimage single_ota %proxr_bin% %ver_3% %proxr_img_prefix%_%base_ver%_3_single_wsig.img wsig
timeout -t 1

mkimage single_ota %proxr_bin% %ver_3% %proxr_img_prefix%_%base_ver%_3_single_bsize.img bsize
timeout -t 1

copy .\%proxr_img_prefix%_%base_ver%_1_single.img	.\%out_folder%\
timeout -t 1
copy .\%proxr_img_prefix%_%base_ver%_2_single.img	.\%out_folder%\
timeout -t 1
copy .\%proxr_img_prefix%_%base_ver%_3_single.img	.\%out_folder%\
timeout -t 1
copy .\%proxr_img_prefix%_%base_ver%_3_single_wsig.img	.\%out_folder%\
timeout -t 1
copy .\%proxr_img_prefix%_%base_ver%_3_single_bsize.img	.\%out_folder%\
timeout -t 1

rem ------------------------------
rem 4. test image : bank1_imageid=ff (max value), bank1_imageid=fe (max_value-1)
mkimage single %proxr_bin% %ver_1% %proxr_img_prefix%_%base_ver%_1.img
timeout -t 1

mkimage single %proxr_bin% %ver_2% %proxr_img_prefix%_%base_ver%_2.img
timeout -t 1

mkimage multi spi_bank1_ff %proxr_img_prefix%_%base_ver%_2.img 0x20 %proxr_img_prefix%_%base_ver%_1.img 0x8000 0xFF20 %proxr_out_multi%_A.img
timeout -t 1


rem ------------------------------
rem 5. test image : bank1=valid_img, bank2=invalid_img
mkimage single %proxr_bin% %ver_1% %proxr_img_prefix%_%base_ver%_1.img inval_flag
timeout -t 1

mkimage single %proxr_bin% %ver_2% %proxr_img_prefix%_%base_ver%_2.img
timeout -t 1

mkimage multi spi %proxr_img_prefix%_%base_ver%_2.img 0x20 %proxr_img_prefix%_%base_ver%_1.img 0x8000 0xFF20 %proxr_out_multi%_B.img
timeout -t 1


rem ------------------------------
rem 5. test image : bank1=invalid_img, bank2=valid_img
mkimage single %proxr_bin% %ver_1% %proxr_img_prefix%_%base_ver%_1.img
timeout -t 1

mkimage single %proxr_bin% %ver_2% %proxr_img_prefix%_%base_ver%_2.img inval_flag
timeout -t 1

mkimage multi spi %proxr_img_prefix%_%base_ver%_2.img 0x20 %proxr_img_prefix%_%base_ver%_1.img 0x8000 0xFF20 %proxr_out_multi%_C.img
timeout -t 1


copy .\%proxr_out_multi%_A.img	.\%out_folder%\
timeout -t 1
copy .\%proxr_out_multi%_B.img	.\%out_folder%\
timeout -t 1
copy .\%proxr_out_multi%_C.img	.\%out_folder%\
timeout -t 1

rem ------------------------------

del .\*.img

pause