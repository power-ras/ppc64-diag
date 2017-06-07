diags_test_common_h_files = diags/encl_util.h \
			    diags/encl_common.h \
			    diags/diag_encl.h \
			    diags/test/test_utils.h \
			    common/utils.h

diags_test_bh_h_files = diags/bluehawk.h \
			$(diags_test_common_h_files)

diags_test_hr_h_files = diags/homerun.h \
			$(diags_test_common_h_files)

diags_test_slider_h_files = diags/slider.h \
			    $(diags_test_common_h_files)

diags_test_common_obj_files = diags/test/test_utils.c \
			      diags/encl_util.c \
			      common/utils.c

check_PROGRAMS += diags/test/bh_structs \
		  diags/test/hr_structs \
		  diags/test/slider_structs \
		  diags/test/bh_mk_healthy \
		  diags/test/hr_mk_healthy \
		  diags/test/slider_mk_healthy \
		  diags/test/bh_mk_hot_power \
		  diags/test/hr_mk_hot_power \
		  diags/test/slider_mk_hot_power \
		  diags/test/encl_dump_pg2 \
		  diags/test/encl_vpd

diags_test_bh_structs_SOURCES = diags/test/bh_structs.c \
				diags/encl_common.h \
				diags/bluehawk.h

diags_test_hr_structs_SOURCES = diags/test/hr_structs.c \
				diags/encl_common.h \
				diags/homerun.h

diags_test_slider_structs_SOURCES = diags/test/slider_structs.c \
				    diags/encl_common.h \
				    diags/slider.h

diags_test_encl_dump_pg2_SOURCES = diags/test/encl_dump_pg2.c \
				   diags/homerun.h \
				   diags/bluehawk.h \
				   diags/slider.h \
				   $(diags_test_common_obj_files) \
				   $(diags_test_common_h_files)

diags_test_bh_mk_healthy_SOURCES = diags/test/bh_mk_healthy.c \
				   diags/test/bh_healthy.c \
				   $(diags_test_common_obj_files) \
				   $(diags_test_bh_h_files)

diags_test_hr_mk_healthy_SOURCES = diags/test/hr_mk_healthy.c \
				   diags/test/hr_healthy.c \
				   $(diags_test_common_obj_files) \
				   $(diags_test_hr_h_files)

diags_test_slider_mk_healthy_SOURCES = diags/test/slider_mk_healthy.c \
				       diags/test/slider_healthy.c \
				       $(diags_test_common_obj_files) \
				       $(diags_test_slider_h_files)

diags_test_bh_mk_hot_power_SOURCES = diags/test/bh_mk_hot_power.c \
				     diags/test/bh_healthy.c \
				     $(diags_test_common_obj_files) \
				     $(diags_test_bh_h_files)

diags_test_hr_mk_hot_power_SOURCES = diags/test/hr_mk_hot_power.c \
				     diags/test/hr_healthy.c \
				     $(diags_test_common_obj_files) \
				     $(diags_test_hr_h_files)

diags_test_slider_mk_hot_power_SOURCES = diags/test/slider_mk_hot_power.c \
					 diags/test/slider_healthy.c \
					 $(diags_test_common_obj_files) \
					 $(diags_test_slider_h_files)

diags_test_encl_vpd_SOURCES = diags/test/encl_vpd.c \
			     diags/encl_util.c \
			     common/utils.c \
			     diags/homerun.h \
			     diags/bluehawk.h \
			     diags/slider.h \
			     $(diags_test_common_h_files)

EXTRA_DIST += diags/test/bluehawk.vpd \
	      diags/test/homerun.vpd \
	      diags/test/slider.vpd \
	      diags/test/run_tests \
	      diags/test/tests \
	      diags/test/tests-results \
	      diags/test/README
