diag_common_h_files = diags/encl_util.h \
		      diags/bluehawk.h \
		      diags/homerun.h \
		      diags/slider.h \
		      diags/encl_common.h \
		      common/platform.h \
		      common/utils.h

diag_encl_h_files = diags/diag_encl.h \
		    $(diag_common_h_files)

encl_led_h_files = diags/encl_led.h \
		   $(diag_common_h_files)

sbin_PROGRAMS += diags/diag_encl diags/encl_led

diags_diag_encl_SOURCES = diags/diag_encl.c \
			  diags/7031_D24_T24.c \
			  diags/bluehawk.c \
			  diags/encl_util.c \
			  diags/encl_common.c \
			  diags/homerun.c \
			  diags/slider.c \
			  common/platform.c \
			  common/utils.c \
			  $(diag_encl_h_files)
diags_diag_encl_LDADD = -lservicelog

diags_encl_led_SOURCES = diags/encl_led.c \
			 diags/encl_util.c \
			 diags/bluehawk_led.c \
			 diags/homerun_led.c \
			 diags/slider_led.c \
			 common/platform.c \
			 common/utils.c \
			 $(encl_led_h_files)

dist_man_MANS += diags/man/diag_encl.8 diags/man/encl_led.8
