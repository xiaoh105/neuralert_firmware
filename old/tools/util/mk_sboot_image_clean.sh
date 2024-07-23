#!/bin/bash
export LANG=C

echo Start mk_sboot_image_clean.sh

rm -f ./SBOOT/dmpublic/sboot_hbk1_cache_cert.bin
rm -f ./SBOOT/dmpublic/sboot_hbk1_cache_cert_Cert.txt
rm -f ./SBOOT/dmpublic/sboot_hbk1_ueboot_cert.bin
rm -f ./SBOOT/dmpublic/sboot_hbk1_ueboot_cert_Cert.txt
rm -f ./SBOOT/dmtpmcfg/*.*
rm -f ./SBOOT/example/*.*
rm -f ./SBOOT/public/*.img
