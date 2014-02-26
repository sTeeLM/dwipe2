/* defs.h - MemTest-86 assembler/compiler definitions
 *
 * Released under version 2 of the Gnu Public License.
 * By Chris Brady, cbrady@sgi.com
 */

#ifndef __DWIPE_DEFS_H__
#define __DWIPE_DEFS_H__

#define SETUPSECS	8		/* Number of setup sectors */

#define TSTLOAD		0x10000		    /* Segment adrs for load of test */

#define BOOTSEG		0x07c0			/* Segment adrs for inital boot */
#define INITSEG		0x9000			/* Segment adrs for relocated boot */
#define SETUPSEG	(INITSEG+0x20)		/* Segment adrs for relocated setup */

#define KERNEL_CS	0x10			/* 32 bit segment adrs for code */
#define KERNEL_DS	0x18			/* 32 bit segment adrs for data */
#define REAL_CS		0x20			/* 16 bit segment adrs for code */
#define REAL_DS		0x28			/* 16 bit segment adrs for data */

#define REAL_CODE_SEG 0x1000
#define REAL_CODE_OFFSET 0x0000
#define REAL_CODE_ADDR ((REAL_CODE_SEG << 4) + REAL_CODE_OFFSET)

#define BIOS_MAX_SECTOR_CNT  127
#define MAX_SECTOR_BUFFER_SIZE (BIOS_MAX_SECTOR_CNT * 512)

#define PROT_STACK_SIZE 8192

#define UTS_RELEASE  "1.0.0"
#define LINUX_COMPILE_BY "steel.mental"
#define LINUX_COMPILE_HOST "gmail.com"
#define UTS_VERSION "1.0.0"

#define __PAGE_OFFSET  (0xC0000000)

#endif
