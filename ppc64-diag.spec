Name:	ppc64-diag
Version:	2.6.1
Release:	0
Summary: 	PowerLinux Platform Diagnostics
Group: 		System Environment/Base
License: 	Eclipse Public License (EPL) v1.0
Packager: 	IBM Corp. 
Vendor: 	IBM Corp.
ExclusiveArch:  ppc ppc64
BuildRequires:  libservicelog-devel, libvpd-devel, flex, perl, /usr/bin/yacc
BuildRequires:	ncurses-devel
Requires: 	servicelog, /usr/sbin/lsvpd, /sbin/chkconfig
# PRRN RTAS event notification handler depends on below librtas
# and powerpc-utils versions.
Requires:	librtas >= 1.3.8
Requires:	powerpc-utils >= 1.2.16
BuildRoot:	%{_tmppath}/%{name}-%{version}-build
Source0: 	ppc64-diag-%{version}.tar.gz

%description
This package contains various diagnostic tools for PowerLinux.
These tools captures the diagnostic events from Power Systems
platform firmware, SES enclosures and device drivers, and
write events to servicelog database. It also provides automated
responses to urgent events such as environmental conditions and
predictive failures, if appropriate modifies the FRUs fault
indicator(s) and provides event notification to system
administrators or connected service frameworks.

%prep
%setup -q

%build
make

%install
make install DESTDIR=$RPM_BUILD_ROOT
chmod 644 $RPM_BUILD_ROOT/etc/ppc64-diag/servevent_parse.pl
mkdir $RPM_BUILD_ROOT/etc/ppc64-diag/ses_pages
ln -sfv /usr/sbin/usysattn $RPM_BUILD_ROOT/usr/sbin/usysfault

%files
%defattr (-,root,root,-)
%doc /usr/share/doc/packages/ppc64-diag/COPYRIGHT
%doc /usr/share/man/man8/*
/usr/sbin/*
%dir /etc/ppc64-diag
%dir /etc/ppc64-diag/ses_pages
%config /etc/ppc64-diag/*
%config /etc/rc.powerfail
%config %attr(755,root,root) /etc/init.d/rtas_errd
%config %attr(744,root,root) /etc/ppc64-diag/prrn_hotplug

%post
# Post-install script --------------------------------------------------
/etc/ppc64-diag/ppc64_diag_setup --register >/dev/null
/etc/ppc64-diag/lp_diag_setup --register >/dev/null
if [ "$1" = "1" ]; then # first install
    /sbin/chkconfig --add rtas_errd
    /etc/init.d/rtas_errd start
elif [ "$1" = "2" ]; then # upgrade
    /etc/init.d/rtas_errd restart
fi

%preun
# Pre-uninstall script -------------------------------------------------
if [ "$1" = "0" ]; then # last uninstall
    /etc/init.d/rtas_errd stop
    /sbin/chkconfig --del rtas_errd
    /etc/ppc64-diag/ppc64_diag_setup --unregister >/dev/null
    /etc/ppc64-diag/lp_diag_setup --unregister >/dev/null
fi

%triggerin -- librtas
# trigger on librtas upgrades ------------------------------------------
if [ "$2" = "2" ]; then
    /etc/init.d/rtas_errd restart
fi

%changelog
* Fri Feb 08 2013 - Vasant Hegde <hegdevasant@in.ibm.com> - 2.6.1
- Handler to handle PRRN RTAS notification

* Tue Jan 29 2013 - Vasant Hegde <hegdevasant@in.ibm.com> - 2.6.0-3
- rtas_errd segfault fix in test path (bug #88056)

* Fri Jan 18 2013 - Vasant Hegde <hegdevasant@in.ibm.com> - 2.6.0-2
- Updated ELA catalog for e1000e and cxgb3 driver

* Fri Jan 11 2013 - Vasant Hegde <hegdevasant@in.ibm.com> - 2.6.0-1
- ppc64_diag_notify alignment fix

* Wed Dec 19 2012 - Vasant Hegde <hegdevasant@in.ibm.com> - 2.6.0
- Added Light Path Diagnostics code.

* Fri Nov 23 2012 - Vasant Hegde <hegdevasant@in.ibm.com> - 2.5.1
- 2.5.1 release

* Fri Sep 07 2012 - Vasant Hegde <hegdevasant@in.ibm.com> - 2.5.0
- Introduced new options to diag_encl command (Jim).
- Added bluehawk enclosure diagnostics support (Jim).
- Introduced new command "encl_led" to modify identify/fault indicators
  for SCSI enclosures (Jim).
- Fixed bug #78826.
- Added libvpd-devel as compilation dependency.

* Wed May 23 2012 - Vasant Hegde <hegdevasant@in.ibm.com> - 2.4.4
- Fixed bug #80220 and #81288.

* Tue Feb 14 2012 - Jim Keniston <jkenisto@us.ibm.com> - 2.4.3
- Added message catalogs for the ipr, ixgb, lpfc, and qla2xxx drivers
  (Anithra).  Fixed bugs #75118, #74636 and #74308.  Removed obsolete
  ppc64_diag_servagent script.

* Mon Jul 11 2011 - Anithra P Janakiraman <janithra@in.ibm.com> -2.4.2-1
- Minor modifications to GPFS catalog files and syslog_to_svclog.cpp 

* Wed Jun 29 2011 - Anithra P Janakiraman <janithra@in.ibm.com> -2.4.2-0
- Added gpfs files to the catalog, updated ppc64-diag-setup notification
  commands

* Wed Jun 15 2011 - Jim Keniston <jkenisto@us.ibm.com> - 2.4.1-0
- Changed Makefiles and rules.mk to build for the default architecture
  rather than -m32.

* Tue Feb 22 2011 - Anithra P Janakiraman <janithra@in.ibm.com> - 2.4.0-0
- Added ELA code to the package, made changes to the rules.mk and minor changes 
  to the spec file.	

* Fri Nov 19 2010 - Brad Peters <bpeters@us.ibm.com> - 2.3.5-0
- Bug fix adding in support for -e and -l, so that root users can
  be notified of serviceable events. Addresses bug #26192

* Wed Feb 24 2010 - Mike Mason <masonmik@us.ibm.com> - 2.3.4-2
- Added SIGCHLD handler to clean up servicelog notification scripts.

* Tue Sep 22 2009 - Brad Peters <bpeters@us.ibm.com> - 2.3.2-2
- Removed all absolute path references, specifically to /sbin/lsvpd and lsvpd

* Mon Sep 21 2009 - Brad Peters <bpeters@us.ibm.com> - 2.3.2-1
- Removed .spec references to aaa_base and insserv, which are SLES specific
