Working directory:
let say test_dir = "c:\temp"

1. create ble image set for ota
find the document below
\docs\test_spec\DA16600 Combo Functional Test Report_Vx.x.x.xlsx

DA16600 Combo Functional Test Report.xlsx > tab: Pre-requisite > // How to generate a BLE FW image
copy 5 files to some folder : let say "c:\ota_img"

2. create a wifi baseline image for ota
img_set_1
	DA16600 Combo Functional Test Report.xlsx > tab: Pre-requisite > img_set_1
	
3. create wifi ota img sets
	DA16600 Combo Functional Test Report.xlsx > tab: Pre-requisite > OTA Test Image Sets	
	copy 5 files to "c:\ota_img"

4. copy img sets and tools to test_dir. test_dir directory structure looks like below
(ttl path: [SDK_ROOT]\1.UTILS\combo\test_script\ota_auto_test\)

test_dir\
		ota_img_dl.ttl
		ota_tc.ttl
		test_ota_main.ttl
		subroutine.ttl
		DA16600_FBOOT-GEN01-01-xxxxxxxxx-000000_W25Q32JW.img
		da14531_multi_part_proxr_A.img
		da14531_multi_part_proxr_B.img
		da14531_multi_part_proxr_C.img

		\img_set_1
			da14531_multi_part_proxr.img <------- This image is the (AAAAAAAA) in "DA16600 Combo Functional Test Report.xlsx > tab: Pre-requisite"
			DA16600_FBOOT-GEN01-01-xxxxxxxx-000000_W25Q32JW.img
			DA16600_FRTOS-GEN01-01-xxxxxxxx-000000.img

5. run http server
(before running Apache web server, please make sure no http server is running - e.g. IIS)
copy "c:\ota_img" to http server's doc folder (in case of Apache, C:\Apache24\htdocs\)
	pxr_sr_coex_ext_531_6_0_14_1114_1_single.img
	pxr_sr_coex_ext_531_6_0_14_1114_2_single.img
	pxr_sr_coex_ext_531_6_0_14_1114_3_single.img
	pxr_sr_coex_ext_531_6_0_14_1114_3_single_bsize.img
	pxr_sr_coex_ext_531_6_0_14_1114_3_single_wsig.img
	DA16600_FRTOS-GEN01-01-xxxxxxxxx-004990.img			
	DA16600_FRTOS-GEN01-01-xxxxxxxxx-004991.img			
	DA16600_FRTOS-GEN01-01-xxxxxxxxx-004992.img			
	DA16600_FRTOS-GEN01-02-xxxxxxxxx-004990.img					
	DA16600_FRTOS-GEN02-01-xxxxxxxxx-004990.img			

for Apache setup, refer
	DA16600 Combo Functional Test Report.xlsx > tab: Pre-requisite > Apache web server

run/stop Apatch server
	httpd -k stop
	httpd -k start

6. modify ttl
test_ota_main.ttl
: change version string
	search all with "FRTOS-" -> then all the files matched listed
	create a mapping table like ....

			FRTOS-GEN01-01-14145-000000 ---> existing in ttl
				FRTOS-GEN01-01-acfbc6be6-005006 ---> <<new>>

			RTOS-GEN01-01-14145-000001
				FRTOS-GEN01-01-cf41cc24c-005007

			FRTOS-GEN02-01-14145-000002
				FRTOS-GEN01-02-acfbc6be6-005006

			FRTOS-GEN01-01-14145-000002
				FRTOS-GEN01-01-fbd391916-005008

			FRTOS-GEN01-01-14145-000007
				FRTOS-GEN01-01-fbd391916-005111.img >> any non-existing file name is ok

			FRTOS-GEN01-02-14145-000002
				FRTOS-GEN01-02-acfbc6be6-005006

	for each, replace all with <<new>>
	check if replacement is ok by searching with "FRTOS-" again
	
	search with "FBOOT-"
	create a mapping table like ....
	
		DA16600_FBOOT-GEN01-01-14128-000000_W25Q32JW
			DA16600_FBOOT-GEN01-01-922f1e27d_W25Q32JW
	
	for each, replace all with <<new>>
	check if replacement is ok by searching with "FBOOT-" again
	
: change http server address if needed -> http://xxx.xxx.xxx.xxx 

7. DUT
make sure boot_idx is 0

factory > Y
setup > connect to an AP (without DPM ON) to which http server is reachable

8. enable teraterm log (timestamp ON)

9. DUT
power off / on

Run Teraterm > macro > test_ota_main.ttl
(this test covers tests from 1.3.2.1 ~ 1.3.4.6)







