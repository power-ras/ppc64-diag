[![Build Status](https://travis-ci.org/power-ras/ppc64-diag.svg)](https://travis-ci.org/power-ras/ppc64-diag)

PPC64 Diagnostics
=================
This package is amalgamation of various diagnostics tools for Power system
diagnosis and they can be broadly classified as below.

## PPC64 Events Diagnostics
The tools capture the diagnostic events/dumps from Power Systems platform
firmware, SES enclosures and logs serviceable event. These tools also provides
automated responses to urgent events such as environmental conditions and
predictive failures.

## Light Path Diagnostics
Light Path Diagnostics allows system engineers and administrators to easily
and quickly diagnose hardware problems on the IBM servers.

Light Path Diagnostics on PowerLinux constantly monitors for serviceable
events and enable/disable fault indicators as required.

## Error Log Analysis
These utilities are currently deprecated.

Tools Overview
==============
The below lists some of all these utilities and further documentation for each
of these utilities is available in their corresponding man pages.

***rtas_errd***: diagnose and handle platform events
Tool to handle the platform events on pSeries platform.

***opal_errd***: diagnose and handle platform events
Tool to handle the platform events on PowerNV platform.

***diag_encl***: diagnose SCSI enclosures
Tool to diagnose a limited number of SCSI enclosure types.

***encl_diag***: SCSI enclosure LED management
Tool to view or manipulate the indicators for the SCSI enclosures.

***extract_opal_dump***: Extract opal dump
Tool to collect the firmware dumps.

***lp_diag***: Light Path diagnostics
UI to view and modify the indicator status.

***usysident***: identification indicator utility
A utility to view the status of device identification indicators
(LEDs), and to turn the indicators on and off.

***usysattn*** , ***usysfault***:  attention indicator utility
A utility to view the status of the system attention and fault indicators
(LEDs), and to turn the indicators off after an event has been serviced.

## Source
https://github.com/power-ras/ppc64-diag

## License
See 'COPYING' file.

## Reporting issue
Create a GitHub issue if you have any request for change, assuming one does
not already exist. Clearly describe the issue including steps to reproduce
if it is a bug.

## Compilation dependencies
- C and C++ compiler (gcc, g++)
- GNU build tools (automake, autoconf, libtool, etc)
- ncurses-devel
- systemd-devel
- librtas-devel
- libvpd-devel
- libservicelog-devel

Note:
  Package name may differ slightly between Linux distributors. Ex: RedHat and
  SLES ships development packages as "-devel" while Ubuntu ships it as "-dev"
  package. Please check your linux distribution package naming convention and
  make sure you have installed right packages.

## Building
You can build on Power Linux System.

```bash
make
make install
```

### Building rpms
To build a tarball to feed to rpmbuild, do

`make dist-gzip

As an example, we use a command similar to the following:

`$ rpmbuild -ba <path-to-spec-file>`

## How to contribute
If you plan to submit the changes, submit a pull request based on top of
master. Include a descriptive commit message. Changes contributed should
focus on a single issue at a time to the extent possible.

### Hacking
The following workflow should work for you:
- Fork the repository on GitHub into your account.
- Create a topic branch from where you want to base your work.
  This is usually the master branch.
- Make sure you have added the necessary tests for your changes and make sure
  all tests pass.
- Push your changes to the topic branch in your fork of the repository.
- Include a descriptive commit message, and each commit should have
  linux-kernel style 'Signed-Off-By'.
- Submit a pull request to this repository.

You probably want to read the linux kernel Documentation/SubmittingPatches
as much of it applies to ppc64-diag.
