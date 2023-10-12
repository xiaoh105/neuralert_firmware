@echo Start mk_sboot_image_clean.bat
@echo off

del /q .\SBOOT\dmpublic\sboot_hbk1_cache_cert.bin
del /q .\SBOOT\dmpublic\sboot_hbk1_cache_cert_Cert.txt
del /q .\SBOOT\dmpublic\sboot_hbk1_ueboot_cert.bin
del /q .\SBOOT\dmpublic\sboot_hbk1_ueboot_cert_Cert.txt
del /q .\SBOOT\dmtpmcfg\*.*
del /q .\SBOOT\example\*.*
del /q .\SBOOT\public\*.img
del /q .\util\__temp_img_path.txt
exit
