/* defs.h - MemTest-86 assembler/compiler definitions
 *
 * Released under version 2 of the Gnu Public License.
 * By Chris Brady, cbrady@sgi.com
 */

//#define __BIG_KERNEL__

#ifndef __BIG_KERNEL__
#define SETUPSECS	4		/* Number of setup sectors */
#else
#define SETUPSECS	8		/* Number of setup sectors */
#endif

/*
 * Caution!! There is magic in the build process.  Read
 * README.build-process before you change anything.
 * Unlike earlier versions all of the settings are in defs.h
 * so the build process should be more robust.
 */
#define LOW_TEST_ADR	0x00002000		/* Final adrs for test code */
#define HI_TEST_ADDR    0x00100000

#define BOOTSEG		0x07c0			/* Segment adrs for inital boot */
#define INITSEG		0x9000			/* Segment adrs for relocated boot */
#define SETUPSEG	(INITSEG+0x20)		/* Segment adrs for relocated setup */

#ifndef __BIG_KERNEL__
#define TSTLOAD		0x1000			/* Segment adrs for load of test */
#else
#define TSTLOAD		0x10000		    /* Segment adrs for load of test */
#endif

#define KERNEL_CS	0x10			/* 32 bit segment adrs for code */
#define KERNEL_DS	0x18			/* 32 bit segment adrs for data */
#define REAL_CS		0x20			/* 16 bit segment adrs for code */
#define REAL_DS		0x28			/* 16 bit segment adrs for data */


#define __PAGE_OFFSET  (0xC0000000)

#define BIOS_MAX_SECTOR_CNT  8
#define MAX_SECTOR_BUFFER_SIZE (BIOS_MAX_SECTOR_CNT * 512)

#define UTS_RELEASE  "1.0.0"
#define LINUX_COMPILE_BY "steel.mental"
#define LINUX_COMPILE_HOST "gmail.com"
#define UTS_VERSION "1.0.0"