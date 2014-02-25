/* test.h - MemTest-86  Version 3.2
 *
 * Released under version 2 of the Gnu Public License.
 * By Chris Brady
 * ----------------------------------------------------
 * MemTest86+ V2.00 Specific code (GPL V2.0)
 * By Samuel DEMEULEMEESTER, sdemeule@memtest.org
 * http://www.canardpc.com - http://www.memtest.org
 */

#ifndef _DWIPE_H_
#define _DWIPE_H_
#define E88     0x00
#define E801    0x04
#define E820NR  0x08           /* # entries in E820MAP */
#define E820MAP 0x0c           /* our map */
#define E820MAX 64             /* number of entries in E820MAP */
#define E820ENTRY_SIZE 20
#define MEMINFO_SIZE (E820MAP + E820MAX * E820ENTRY_SIZE)
#define MAX_DMI_MEMDEVS 16

#define NULL 0

#ifndef __ASSEMBLY__

#define E820_RAM        1
#define E820_RESERVED   2
#define E820_ACPI       3 /* usable as RAM once ACPI tables have been read */
#define E820_NVS        4

struct e820entry {
        unsigned long long addr;        /* start of memory segment */
        unsigned long long size;        /* size of memory segment */
        unsigned long type;             /* type of memory segment */
};

struct mem_info_t {
	unsigned long e88_mem_k;	/* 0x00 */
	unsigned long e801_mem_k;	/* 0x04 */
	unsigned long e820_nr;		/* 0x08 */
	struct e820entry e820[E820MAX];	/* 0x0c */
					/* 0x28c */
};

typedef unsigned long ulong;
#define SPINSZ		0x2000000
#define MOD_SZ		20
#define BAILOUT		if (bail) goto skip_test;
#define BAILR		if (bail) return;
#define NULL 0

#define DMI_SEARCH_START  0x0000F000
#define DMI_SEARCH_LENGTH 0x000F0FFF
#define MAX_DMI_MEMDEVS 16

#define RES_START	0xa0000
#define RES_END		0x100000
#define SCREEN_ADR	0xb8000
#define SCREEN_END_ADR  (SCREEN_ADR + 80*25*2)

#define TITLE_WIDTH	28
#define LINE_TIME	11
#define COL_TIME	0
#define LINE_TST	2
#define LINE_RANGE	3
#define LINE_CPU	1
#define COL_MID		30
#define LINE_PAT  4
#define COL_PAT		41
#define LINE_INFO	11
#define COL_CACHE_TOP   13
#define COL_RESERVED    22
#define COL_MMAP	29
#define COL_CACHE	40
#define COL_ECC		46
#define COL_TST		51
#define COL_PASS	56
#define COL_ERR		63
#define COL_ECC_ERR	72
#define LINE_HEADER	13
#define LINE_SCROLL	15
#define BAR_SIZE	(78-COL_MID-9)
#define LINE_MSG	18
#define COL_MSG		18

#define POP_W	30
#define POP_H	15
#define POP_X	16
#define POP_Y	8
#define POP2_W	74
#define POP2_H	21
#define POP2_X	3
#define POP2_Y	2
//#define NULL	0

/* memspeed operations */
#define MS_COPY		1
#define MS_WRITE	2
#define MS_READ		3

#define SZ_MODE_BIOS		1
#define SZ_MODE_BIOS_RES	2
#define SZ_MODE_PROBE		3

#define getCx86(reg) ({ outb((reg), 0x22); inb(0x23); })

/* lib functions */

struct eregs {
	ulong ss;
	ulong ds;
	ulong esp;
	ulong ebp;
	ulong esi;
	ulong edi;
	ulong edx;
	ulong ecx;
	ulong ebx;
	ulong eax;
	ulong vect;
	ulong code;
	ulong eip;
	ulong cs;
	ulong eflag;
};

void cprint(int y, int x, const char *text);
void dprint(int y, int x, ulong val, int len, int right);
void hprint(int y, int x, unsigned long val);
void inter(struct eregs *trap_regs);
void check_input(void);
int memcmp(const void *s1, const void *s2, ulong count);
void memcpy (void *dst, void *src, int len);
void hprint2(int y,int x, unsigned long val, int digits);
void itoa(char s[], int n);
void reverse(char s[]);
void set_cache(int val);
void *memmove(void *dest, const void *src, ulong n);
void memcpy (void *dst, void *src, int len);
void memset(void *dst, int c, int len);
void aprint(int y, int x, ulong page);

#define PRINTMODE_SUMMARY   1
#define PRINTMODE_ADDRESSES 0
#define PRINTMODE_PATTERNS  2
#define PRINTMODE_NONE      3
#define PRINTMODE_DMI      	4

#define BADRAM_MAXPATNS 10

struct pair {
       ulong adr;
       ulong mask;
};


static inline void cache_off(void)
{
        asm(
		"push %eax\n\t"
		"movl %cr0,%eax\n\t"
                "orl $0x40000000,%eax\n\t"  /* Set CD */
                "movl %eax,%cr0\n\t"
		"wbinvd\n\t"
		"pop  %eax\n\t");
}
static inline void cache_on(void)
{
        asm(
		"push %eax\n\t"
		"movl %cr0,%eax\n\t"
		"andl $0x9fffffff,%eax\n\t" /* Clear CD and NW */
		"movl %eax,%cr0\n\t"
		"pop  %eax\n\t");
}

static inline void reboot(void)
{
        asm(
		"movl %cr0,%eax\n\t"
		"andl  $0x00000011,%eax\n\t"
		"orl   $0x60000000,%eax\n\t"
		"movl  %eax,%cr0\n\t"
		"movl  %eax,%cr3\n\t"
		"movl  %cr0,%ebx\n\t"
		"andl  $0x60000000,%ebx\n\t"
		"jz    f\n\t"
		".byte 0x0f,0x09\n\t"	/* Invalidate and flush cache */
		"f: andb  $0x10,%al\n\t"
		"movl  %eax,%cr0\n\t"
		"movw $0x0010,%ax\n\t"
		"movw %ax,%ds\n\t"
		"movw %ax,%es\n\t"
		"movw %ax,%fs\n\t"
		"movw %ax,%gs\n\t"
		"movw %ax,%ss\n\t"
		"ljmp  $0xffff,$0x0000\n\t");
}

struct mmap {
	ulong pbase_addr;
	ulong *start;
	ulong *end;
};

struct pmap {
	ulong start;
	ulong end;
};

struct tseq {
	short cache;
	short pat;
	short iter;
	short errors;
	char *msg;
};

struct cpu_ident {
	char type;
	char model;
	char step;
	char fill;
	long cpuid;
	long capability;
	char vend_id[12];
	unsigned char cache_info[16];
	long pwrcap;
	long ext;
	long feature_flag;
	long dcache0_eax;
	long dcache0_ebx;
	long dcache0_ecx;
	long dcache0_edx;
	long dcache1_eax;
	long dcache1_ebx;
	long dcache1_ecx;
	long dcache1_edx;	
	long dcache2_eax;
	long dcache2_ebx;
	long dcache2_ecx;
	long dcache2_edx;	
	long dcache3_eax;
	long dcache3_ebx;
	long dcache3_ecx;
	long dcache3_edx;
};

struct xadr {
	ulong page;
	ulong offset;
};

struct err_info {
	struct xadr   low_addr;
	struct xadr   high_addr;
	unsigned long ebits;
	long	      tbits;
	short         min_bits;
	short         max_bits;
	unsigned long maxl;
	unsigned long eadr;
        unsigned long exor;
        unsigned long cor_err;
	short         hdr_flag;
};

#define X86_FEATURE_PAE		(0*32+ 6) /* Physical Address Extensions */
#define MAX_MEM_SEGMENTS E820MAX

/* Define common variables accross relocations of memtest86+ */
struct vars {
	volatile int test;
	int pass;
	unsigned long *eadr;
	unsigned long exor;
	int msg_line;
	int ecount;
	int ecc_ecount;
	int msegs;
	int testsel;
	int scroll_start;
	int rdtsc;
	int pae;
	int pass_ticks;
	int total_ticks;
	int pptr;
	int tptr;
	int beepmode;
	struct err_info erri;
	struct pmap pmap[MAX_MEM_SEGMENTS];
	struct mmap map[MAX_MEM_SEGMENTS];
	ulong plim_lower;
	ulong plim_upper;
	ulong clks_msec;
	ulong starth;
	ulong startl;
	ulong snaph;
	ulong snapl;
	ulong extclock;
	unsigned long imc_type;
	int printmode;
	int numpatn;
	struct pair patn [BADRAM_MAXPATNS];
	ulong test_pages;
	ulong selected_pages;
	ulong reserved_pages;
};

#define FIRMWARE_UNKNOWN   0
#define FIRMWARE_PCBIOS    1
#define FIRMWARE_LINUXBIOS 2

extern struct vars * const v;
extern unsigned char _start[], _end[], startup_32[];
extern unsigned char _size, _pages;
extern struct cpu_ident cpu_id;

/* CPU mode types */
#define CPM_SINGLE 1
#define CPM_RROBIN 2
#define CPM_SEQ    3

#endif /* __ASSEMBLY__ */
#endif /* _TEST_H_ */