using namespace std;

#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include "catalogs.h"
extern "C" {
#include "platform.c"
}

static const char *progname;

static void usage(void)
{
	cerr << "usage: " << progname << " [-C catalog_dir]" << endl;
	exit(1);
}

int main(int argc, char **argv)
{
	const char *catalog_dir = ELA_CATALOG_DIR;
	int c;
	int platform = 0;

	progname = argv[0];

	platform = get_platform();
	cout << progname << ": is not supported on the " <<
		__power_platform_name(platform) << " platform" << endl;
	exit(0);

	opterr = 0;
	while ((c = getopt(argc, argv, "C:")) != -1) {
		switch (c) {
		case 'C':
			catalog_dir = optarg;
			break;
		case '?':
			usage();
		}
	}
	if (optind != argc)
		usage();

	regex_text_policy = RGXTXT_WRITE;
	if (EventCatalog::parse(catalog_dir) != 0)
		exit(2);

	exit(0);
}
