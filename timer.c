#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include "timer.h"
#include "io.h"
#include "debug.h"
#include "lib.h"

#define TICKS	59659			/* Program counter to 50 ms = 59659 clks */

struct cpu_ident cpu_id;

static uint32_t extclock;
static uint32_t rdtsc;
static uint32_t pae;
static uint64_t tm_base_clk;

static uint32_t st_low, st_high;
static uint32_t end_low, end_high;

static int tsc_invariable = 0;

static unsigned long imc_type = 0;

#define getCx86(reg) ({ outb((reg), 0x22); inb(0x23); })

#define rdmsr(msr,val1,val2) \
	__asm__ __volatile__("rdmsr" \
			  : "=a" (val1), "=d" (val2) \
			  : "c" (msr))

#define X86_FEATURE_PAE		(0*32+ 6) /* Physical Address Extensions */


static uint32_t epoch = 1900;	/* year corresponding to 0x00	*/

char * cpu_type_str = "UNKNOWN";
int l1_cache;
int l2_cache;
int l3_cache;
uint32_t speed;


#define RTC_SECONDS             0
#define RTC_SECONDS_ALARM       1
#define RTC_MINUTES             2
#define RTC_MINUTES_ALARM       3
#define RTC_HOURS               4
#define RTC_HOURS_ALARM         5
#define RTC_DAY_OF_WEEK         6
#define RTC_DAY_OF_MONTH        7
#define RTC_MONTH               8
#define RTC_YEAR                9
#define RTC_FREQ_SELECT		10
# define RTC_UIP 0x80
# define RTC_DIV_CTL 0x70
/* This RTC can work under 32.768KHz clock only.  */
# define RTC_OSC_ENABLE 0x20
# define RTC_OSC_DISABLE 0x00
#define RTC_CONTROL     	11
# define RTC_SET 0x80
# define RTC_PIE 0x40
# define RTC_AIE 0x20
# define RTC_UIE 0x10
# define RTC_SQWE 0x08
# define RTC_DM_BINARY 0x04
# define RTC_24H 0x02
# define RTC_DST_EN 0x01

#define RTC_PORT(x)     (0x70 + (x))

/*
Register  Contents
 0x00      Seconds
 0x02      Minutes
 0x04      Hours
 0x06      Weekday
 0x07      Day of Month
 0x08      Month
 0x09      Year
 0x32      Century (maybe)
 0x0A      Status Register A
 0x0B      Status Register B
*/
struct rtc_time {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

static struct rtc_time global_rtc_tm;

unsigned char rtc_cmos_read(unsigned char addr)
{
    unsigned char val;

    outb(addr, RTC_PORT(0));
    val = inb(RTC_PORT(1));

    return val;
}

#define CMOS_READ(addr) rtc_cmos_read(addr)

static inline unsigned char rtc_is_updating(void)
{
	unsigned char uip;
	uip = (CMOS_READ(RTC_FREQ_SELECT) & RTC_UIP);
	return uip;
}

static unsigned char bcd2bin(unsigned char val)
{
	return (val & 0x0f) + (val >> 4) * 10;
}

static void rtc_get_rtc_time(struct rtc_time *rtc_tm)
{
	unsigned char ctrl;

	/*
	 * read RTC once any update in progress is done. The update
	 * can take just over 2ms. We wait 20ms. There is no need to
	 * to poll-wait (up to 1s - eeccch) for the falling edge of RTC_UIP.
	 * If you need to know *exactly* when a second has started, enable
	 * periodic update complete interrupts, (via ioctl) and then
	 * immediately read /dev/rtc which will block until you get the IRQ.
	 * Once the read clears, read the RTC time (again via ioctl). Easy.
	 */

	while (rtc_is_updating() != 0);

	/*
	 * Only the values that we read from the RTC are set. We leave
	 * tm_wday, tm_yday and tm_isdst untouched. Note that while the
	 * RTC has RTC_DAY_OF_WEEK, we should usually ignore it, as it is
	 * only updated by the RTC when initially set to a non-zero value.
	 */
	rtc_tm->tm_sec = CMOS_READ(RTC_SECONDS);
	rtc_tm->tm_min = CMOS_READ(RTC_MINUTES);
	rtc_tm->tm_hour = CMOS_READ(RTC_HOURS);
	rtc_tm->tm_mday = CMOS_READ(RTC_DAY_OF_MONTH);
	rtc_tm->tm_mon = CMOS_READ(RTC_MONTH);
	rtc_tm->tm_year = CMOS_READ(RTC_YEAR);
	/* Only set from 2.6.16 onwards */
	rtc_tm->tm_wday = CMOS_READ(RTC_DAY_OF_WEEK);

	ctrl = CMOS_READ(RTC_CONTROL);

	if (!(ctrl & RTC_DM_BINARY)) {
		rtc_tm->tm_sec = bcd2bin(rtc_tm->tm_sec);
		rtc_tm->tm_min = bcd2bin(rtc_tm->tm_min);
		rtc_tm->tm_hour = bcd2bin(rtc_tm->tm_hour);
		rtc_tm->tm_mday = bcd2bin(rtc_tm->tm_mday);
		rtc_tm->tm_mon = bcd2bin(rtc_tm->tm_mon);
		rtc_tm->tm_year = bcd2bin(rtc_tm->tm_year);
		rtc_tm->tm_wday = bcd2bin(rtc_tm->tm_wday);
	}

	/*
	 * Account for differences between how the RTC uses the values
	 * and how they are defined in a struct rtc_time;
	 */
	rtc_tm->tm_year += epoch - 1900;
	if (rtc_tm->tm_year <= 69)
		rtc_tm->tm_year += 100;

	rtc_tm->tm_mon--;
}

static uint32_t correct_tsc(uint32_t el_org)
{
	
	float coef_now, coef_max;
	int msr_lo, msr_hi, is_xe;
	
	rdmsr(0x198, msr_lo, msr_hi);
	is_xe = (msr_lo >> 31) & 0x1;		
	
	if(is_xe){
		rdmsr(0x198, msr_lo, msr_hi);
		coef_max = ((msr_hi >> 8) & 0x1F);	
		if ((msr_hi >> 14) & 0x1) { coef_max = coef_max + 0.5f; }
	} else {
		rdmsr(0x17, msr_lo, msr_hi);
		coef_max = ((msr_lo >> 8) & 0x1F);
		if ((msr_lo >> 14) & 0x1) { coef_max = coef_max + 0.5f; }
	}
	
	if((cpu_id.feature_flag >> 7) & 1) {
		rdmsr(0x198, msr_lo, msr_hi);
		coef_now = ((msr_lo >> 8) & 0x1F);
		if ((msr_lo >> 14) & 0x1) { coef_now = coef_now + 0.5f; }
	} else {
		rdmsr(0x2A, msr_lo, msr_hi);
		coef_now = (msr_lo >> 22) & 0x1F;
	}
		
	if(coef_max && coef_now) { el_org = (uint32_t)(el_org * coef_now / coef_max); }
	
	return el_org;

}

/* Returns CPU clock in khz */
static int cpuspeed(void)
{
	int loops;

	/* Setup timer */
	outb((inb(0x61) & ~0x02) | 0x01, 0x61);
	outb(0xb0, 0x43);
	outb(TICKS & 0xff, 0x42);
	outb(TICKS >> 8, 0x42);

	asm __volatile__ ("rdtsc":"=a" (st_low),"=d" (st_high));

	loops = 0;
	do {
		loops++;
	} while ((inb(0x61) & 0x20) == 0);

	asm __volatile__ (
		"rdtsc\n\t" \
		"subl st_low,%%eax\n\t" \
		"sbbl st_high,%%edx\n\t" \
		:"=a" (end_low), "=d" (end_high)
	);

	/* Make sure we have a credible result */
	if (loops < 4 || end_low < 50000) {
		return(-1);
	}

	if(tsc_invariable){ end_low = correct_tsc(end_low);	}

	return end_low/50;	
}



static void cpu_type(void)
{
	int i, off=0;

    SDBG("cpu_id.type %d", cpu_id.type);
    SDBG("cpu_id.model %d", cpu_id.model);
    SDBG("cpu_id.cpuid %d", cpu_id.cpuid);

	/* If the CPUID instruction is not supported then this is */
	/* a 386, 486 or one of the early Cyrix CPU's */
	if (cpu_id.cpuid < 1) {
		switch (cpu_id.type) {
		case 2:
			/* This is a Cyrix CPU without CPUID */
			i = getCx86(0xfe);
			i &= 0xf0;
			i >>= 4;
			switch(i) {
			case 0:
			case 1:
                cpu_type_str = "Cyrix Cx486";
				break;
			case 2:
				cpu_type_str = "Cyrix 5x86";
				break;
			case 3:
				cpu_type_str = "Cyrix 6x86";
				break;
			case 4:
				cpu_type_str = "Cyrix MediaGX";
				break;
			case 5:
				cpu_type_str = "Cyrix 6x86MX";
				break;
			case 6:
				cpu_type_str = "Cyrix MII";
				break;
			default:
				cpu_type_str = "Cyrix ???";
				break;
			}
			break;
		case 3:
			cpu_type_str = "386";
			break;

		case 4:
			cpu_type_str = "486";
			l1_cache = 8;
			break;
		}
		return;
	}

	/* We have cpuid so we can see if we have pae support */
	if (cpu_id.capability & (1 << X86_FEATURE_PAE)) {
		pae = 1;
	}
	switch(cpu_id.vend_id[0]) {
	/* AMD Processors */
	case 'A':
		switch(cpu_id.type) {
		case 4:
			switch(cpu_id.model) {
			case 3:
				cpu_type_str = "AMD 486DX2";
				break;
			case 7:
				cpu_type_str = "AMD 486DX2-WB";
				break;
			case 8:
				cpu_type_str = "AMD 486DX4";
				break;
			case 9:
				cpu_type_str = "AMD 486DX4-WB";
				break;
			case 14:
				cpu_type_str = "AMD 5x86-WT";
				break;
			}
			/* Since we can't get CPU speed or cache info return */
			return;
		case 5:
			switch(cpu_id.model) {
			case 0:
			case 1:
			case 2:
			case 3:
				cpu_type_str = "AMD K5";
				l1_cache = 8;
				off = 6;
				break;
			case 6:
			case 7:
				cpu_type_str = "AMD K6";
				off = 6;
				l1_cache = cpu_id.cache_info[3];
				l1_cache += cpu_id.cache_info[7];
				break;
			case 8:
				cpu_type_str = "AMD K6-2";
				off = 8;
				l1_cache = cpu_id.cache_info[3];
				l1_cache += cpu_id.cache_info[7];
				break;
			case 9:
				cpu_type_str = "AMD K6-III";
				off = 10;
				l1_cache = cpu_id.cache_info[3];
				l1_cache += cpu_id.cache_info[7];
				l2_cache = (cpu_id.cache_info[11] << 8);
				l2_cache += cpu_id.cache_info[10];
				break;
			case 10:
				cpu_type_str = "AMD Geode LX";
				off = 12;
				l1_cache = cpu_id.cache_info[3];
				l1_cache += cpu_id.cache_info[7];
				l2_cache = (cpu_id.cache_info[11] << 8);
				l2_cache += cpu_id.cache_info[10];
				break;
			case 13:
				cpu_type_str = "AMD K6-III+";
				off = 11;
				l1_cache = cpu_id.cache_info[3];
				l1_cache += cpu_id.cache_info[7];
				l2_cache = (cpu_id.cache_info[11] << 8);
				l2_cache += cpu_id.cache_info[10];
				break;
			}
			break;
		case 6:
			switch(cpu_id.model) {
			case 1:
				cpu_type_str = "AMD Athlon (0.25)";
				off = 17;
				l2_cache = (cpu_id.cache_info[11] << 8);
				l2_cache += cpu_id.cache_info[10];
				break;
			case 2:
			case 4:
				cpu_type_str = "AMD Athlon (0.18)";
				off = 17;
				l2_cache = (cpu_id.cache_info[11] << 8);
				l2_cache += cpu_id.cache_info[10];
				break;
			case 6:
				l2_cache = (cpu_id.cache_info[11] << 8);
				l2_cache += cpu_id.cache_info[10];
				if (l2_cache == 64) {
					cpu_type_str = "AMD Duron (0.18)";
				} else {
					cpu_type_str = "Athlon XP (0.18)";
				}
				off = 16;
				break;
			case 8:
			case 10:
				l2_cache = (cpu_id.cache_info[11] << 8);
				l2_cache += cpu_id.cache_info[10];
				if (l2_cache == 64) {
					cpu_type_str = "AMD Duron (0.13)";
				} else {
					cpu_type_str = "Athlon XP (0.13)";
				}
				off = 16;
				break;
			case 3:
			case 7:
				cpu_type_str = "AMD Duron";
				off = 9;
				/* Duron stepping 0 CPUID for L2 is broken */
				/* (AMD errata T13)*/
				if (cpu_id.step == 0) { /* stepping 0 */
					/* Hard code the right size*/
					l2_cache = 64;
				} else {
					l2_cache = (cpu_id.cache_info[11] << 8);
					l2_cache += cpu_id.cache_info[10];
				}
				break;
			}
			l1_cache = cpu_id.cache_info[3];
			l1_cache += cpu_id.cache_info[7];
			break;
		case 15:
			l1_cache = cpu_id.cache_info[3];
			l2_cache = (cpu_id.cache_info[11] << 8);
			l2_cache += cpu_id.cache_info[10];
			imc_type = 0x0100;
			if(((cpu_id.ext >> 16) & 0xFF) < 0x10) {
			// Here if CPUID.EXT < 0x10h	(old K8/K10)
				switch(cpu_id.model) {
				default:
					cpu_type_str = "AMD K8";
					off = 6;
					break;
				case 1:
				case 5:
					if (((cpu_id.ext >> 16) & 0xF) != 0) {
						cpu_type_str = "AMD Opteron (0.09)";				
					} else {
						cpu_type_str = "AMD Opteron (0.13)";
					}
					off = 18;
					break;
				case 3:
				case 11:
					cpu_type_str = "Athlon 64 X2";
					off = 12;				
					break;			
				case 8:
					cpu_type_str = "Turion 64 X2";
					off = 12;						
					break;
				case 4:
				case 7:
				case 12:
				case 14:
				case 15:
					if (((cpu_id.ext >> 16) & 0xF) != 0) {
						if (l2_cache > 256) {
							cpu_type_str = "Athlon 64 (0.09)";
						} else {
							cpu_type_str = "Sempron (0.09)";						
						}
					} else {
						if (l2_cache > 256) {
							cpu_type_str = "Athlon 64 (0.13)";
						} else {
							cpu_type_str = "Sempron (0.13)";						
						}		
					}
					off = 16;
					break;			
				}
				break;
			} else {
			 // Here if CPUID.EXT >= 0x10h	(new K10)
				l3_cache = (cpu_id.cache_info[15] << 8);
				l3_cache += (cpu_id.cache_info[14] >> 2);
				l3_cache *= 512;
				switch(cpu_id.model) {
				case 1:
					imc_type = 0x0102;
					cpu_type_str = "AMD Fusion @";
					off = 12;				
					break;
				default:			 
				case 2:
					imc_type = 0x0101;
					cpu_type_str = "AMD K10 (65nm) @";
					off = 16;				
					break;	
				case 4:
					imc_type = 0x0101;
					cpu_type_str = "AMD K10 (45nm) @";
					off = 16;				
					break;				
				case 9:
					imc_type = 0x0101;
					cpu_type_str = "AMD Magny-Cours";
					off = 15;				
					break;			
				}
				break;		  
			}
		}
		break;

	/* Intel or Transmeta Processors */
	case 'G':
		if ( cpu_id.vend_id[7] == 'T' ) {	/* GenuineTMx86 */
			if (cpu_id.type == 5) {
				cpu_type_str = "TM 5x00";
				off = 7;
			} else if (cpu_id.type == 15) {
				cpu_type_str = "TM 8x00";
				off = 7;
			}
			l1_cache = cpu_id.cache_info[3] + cpu_id.cache_info[7];
			l2_cache = (cpu_id.cache_info[11]*256) + cpu_id.cache_info[10];
		} else {				/* GenuineIntel */
			if (cpu_id.type == 4) {
				switch(cpu_id.model) {
				case 0:
				case 1:
					cpu_type_str = "Intel 486DX";
					off = 11;
					break;
				case 2:
					cpu_type_str = "Intel 486SX";
					off = 11;
					break;
				case 3:
					cpu_type_str = "Intel 486DX2";
					off = 12;
					break;
				case 4:
					cpu_type_str = "Intel 486SL";
					off = 11;
					break;
				case 5:
					cpu_type_str = "Intel 486SX2";
					off = 12;
					break;
				case 7:
					cpu_type_str = "Intel 486DX2-WB";
					off = 15;
					break;
				case 8:
					cpu_type_str = "Intel 486DX4";
					off = 12;
					break;
				case 9:
					cpu_type_str = "Intel 486DX4-WB";
					off = 15;
					break;
				}
				/* Since we can't get CPU speed or cache info return */
				return;
			}

			/* Get the cache info */
			for (i=0; i<16; i++) {
#ifdef CPUID_DEBUG
				dprint(12,i*3,cpu_id.cache_info[i],2,1);
#endif
				switch(cpu_id.cache_info[i]) {
				case 0x6:
				case 0xa:
				case 0x66:
					l1_cache = 8;
					break;
				case 0x8:
				case 0xc:
				case 0x67:
				case 0x60:
					l1_cache = 16;
					break;
				case 0x9:
				case 0xd:
				case 0x68:
				case 0x2c:
				case 0x30:
					l1_cache = 32;
					break;
				case 0x40:
					l2_cache = 0;
					break;
				case 0x41:
				case 0x79:
				case 0x39:
				case 0x3b:
					l2_cache = 128;
					break;
				case 0x3a:
					l2_cache = 192;
					break;
				case 0x21:
				case 0x42:
				case 0x7a:
				case 0x82:
				case 0x3c:
				case 0x3f:
					l2_cache = 256;
					break;
				case 0x3d:
					l2_cache = 384;
					break;				
				case 0x43:
				case 0x7b:
				case 0x83:
				case 0x86:
				case 0x3e:
				case 0x7f:
				case 0x80:
					l2_cache = 512;
					break;
				case 0x44:
				case 0x7c:
				case 0x84:
				case 0x87:
				case 0x78:
					l2_cache = 1024;
					break;
				case 0x45:
				case 0x7d:
				case 0x85:
					l2_cache = 2048;
					break;
				case 0x48:
					l2_cache = 3072;
					break;
				case 0x49:
					l2_cache = 4096;
					break;
				case 0x4e:
					l2_cache = 6144;
					break;
				case 0x22:
				case 0xd0:
					l3_cache = 512;
				case 0x23:
				case 0xd1:
				case 0xd6:
					l3_cache = 1024;
					break;
				case 0xdc:
					l3_cache = 1536;
					break;
				case 0x25:
				case 0xd2:
				case 0xd7:
				case 0xe2:
					l3_cache = 2048;
					break;
				case 0xdd:
					l3_cache = 3072;
					break;				
				case 0x29:
				case 0x46:
				case 0xd8:
				case 0xe3:
					l3_cache = 4096;
					break;
				case 0x4a:
				case 0xde:
					l3_cache = 6144;
					break;
				case 0x47:
				case 0x4b:
				case 0xe4:
					l3_cache = 8192;
					break;				
				case 0x4c:
				case 0xea:
					l3_cache = 12288;
					break;	
				case 0x4d:
					l3_cache = 16374;
					break;						
				case 0xeb:
					l3_cache = 18432;
					break;
				case 0xec:
					l3_cache = 24576;
					break;		
				}
			}
		
		// If no cache found, check if deterministic cache info are available
		if(l1_cache == 0 && ((cpu_id.dcache0_eax >> 5) & 7) == 1) 
			{
		
			long dcache[] = { cpu_id.dcache0_eax, cpu_id.dcache0_ebx, cpu_id.dcache0_ecx, cpu_id.dcache0_edx,
												cpu_id.dcache1_eax, cpu_id.dcache1_ebx, cpu_id.dcache1_ecx, cpu_id.dcache1_edx,
												cpu_id.dcache2_eax, cpu_id.dcache2_ebx, cpu_id.dcache2_ecx, cpu_id.dcache2_edx,
												cpu_id.dcache3_eax, cpu_id.dcache3_ebx, cpu_id.dcache3_ecx, cpu_id.dcache3_edx
											};
			
			for(i=0; i<4; i++)
			{
				switch((dcache[i*4] >> 5) & 7)
				{
					case 1:
						// We don't want L1 I-Cache, only L1 D-Cache
						if((dcache[i*4] & 3) != 2)
						{
							l1_cache = (((dcache[i*4+1] >> 22) & 0x3FF) + 1) * (((dcache[i*4+1] >> 12) & 0x3FF) + 1);
							l1_cache *= ((dcache[i*4+1] & 0xFFF) + 1) * (dcache[i*4+2] + 1) / 1024;
						}
						break;
					case 2:
						l2_cache = (((dcache[i*4+1] >> 22) & 0x3FF) + 1) * (((dcache[i*4+1] >> 12) & 0x3FF) + 1);
						l2_cache *= ((dcache[i*4+1] & 0xFFF) + 1) * (dcache[i*4+2] + 1) / 1024;
						break;			
					case 3:
						l3_cache = (((dcache[i*4+1] >> 22) & 0x3FF) + 1) * (((dcache[i*4+1] >> 12) & 0x3FF) + 1);
						l3_cache *= ((dcache[i*4+1] & 0xFFF) + 1) * (dcache[i*4+2] + 1) / 1024;
						break;						
				}	
			}
		}


			switch(cpu_id.type) {
			case 5:
				switch(cpu_id.model) {
				case 0:
				case 1:
				case 2:
				case 3:
				case 7:
					cpu_type_str = "Pentium";
					if (l1_cache == 0) {
						l1_cache = 8;
					}
					off = 7;
					break;
				case 4:
				case 8:
					cpu_type_str = "Pentium-MMX";
					if (l1_cache == 0) {
						l1_cache = 16;
					}
					off = 11;
					break;
				}
				break;
			case 6:
				switch(cpu_id.model) {
				case 0:
				case 1:
					cpu_type_str = "Pentium Pro";
					off = 11;
					break;
				case 3:
					cpu_type_str = "Pentium II";
					off = 10;
					break;
				case 5:
				if ((cpu_id.ext >> 16) & 0xF) {
					if(((cpu_id.ext >> 16) & 0xF) > 1) {
						cpu_type_str = "Intel Core i3/i5";
						tsc_invariable = 1;
						imc_type = 0x0003;
						off = 16;
					} else {
						cpu_type_str = "Intel EP80579";
						if (l2_cache == 0) { l2_cache = 256; }
						off = 13;						
					}
				} else {
					if (l2_cache == 0) {
						cpu_type_str = "Celeron";
						off = 7;
					} else {
						cpu_type_str = "Pentium II";
						off = 10;
					}
				 }
					break;
				case 6:
						if (l2_cache == 128) {
							cpu_type_str = "Celeron";
							off = 7;
						} else {
							cpu_type_str = "Pentium II";
							off = 10;
						}
					break;
				case 7:
				case 8:
				case 11:
					if (((cpu_id.ext >> 16) & 0xF) != 0) {
						tsc_invariable = 1;
						if (l2_cache < 1024) {
							cpu_type_str = "Celeron";
							off = 7;
						} else {
							cpu_type_str = "Intel Core 2";
							off = 12;
						}					
					} else {
						if (l2_cache == 128) {
							cpu_type_str = "Celeron";
							off = 7;
						} else {
							cpu_type_str = "Pentium III";
							off = 11;
						}
					}
					break;
				case 9:
					if (l2_cache == 512) {
						cpu_type_str = "Celeron M (0.13)";
					} else {
						cpu_type_str = "Pentium M (0.13)";
					}
					off = 16;
					break;
				case 10:
					if (((cpu_id.ext >> 16) & 0xF) != 0) {
							tsc_invariable = 1;
							if(((cpu_id.ext >> 16) & 0xF) > 1) {
								cpu_type_str = "Intel Core Gen2";
								imc_type = 0x0004;
								off = 15;
							} else {
							  imc_type = 0x0001;
								cpu_type_str = "Intel Core i7";
								off = 13;
							}
						} else {
							cpu_type_str = "Pentium III Xeon";
							off = 16;
						}					
					break;
				case 12:
					if (((cpu_id.ext >> 16) & 0xF) > 1) {					
						cpu_type_str = "Core i7 (32nm)";
						tsc_invariable = 1;
						imc_type = 0x0002;
						off = 14;
					} else {
						l1_cache = 24;
						cpu_type_str = "Atom (0.045)";
						off = 12;						
					}
					break;					
				case 13:
					if (l2_cache == 1024) {
						cpu_type_str = "Celeron M (0.09)";
					} else {
						cpu_type_str = "Pentium M (0.09)";
					}
					off = 16;
					break;
				case 14:
					if (((cpu_id.ext >> 16) & 0xF) != 0) {
						tsc_invariable = 1;
						imc_type = 0x0001;
						SINF("Intel Core i5/i7");
						off = 16;
					} else {
						cpu_type_str = "Intel Core";
						off = 10;
					}
					break;				
				case 15:
					if (l2_cache == 1024) {
						cpu_type_str = "Pentium E";
						off = 9;
					} else {
						cpu_type_str = "Intel Core 2";
						off = 12;	
					}				
					tsc_invariable = 1;
					break;
				}
				break;
			case 15:
				switch(cpu_id.model) {
				case 0:
				case 1:
					if (l2_cache == 128) {
						cpu_type_str = "Celeron (0.18)";
						off = 14;
					} else if (cpu_id.pwrcap == 0x0B) {
						cpu_type_str = "Xeon DP (0.18)";
						off = 14;
					} else if (cpu_id.pwrcap == 0x0C) {
						cpu_type_str = "Xeon MP (0.18)";
						off = 14;
					} else {
						cpu_type_str = "Pentium 4 (0.18)";
						off = 16;
					}
					break;
				case 2:
					if (l2_cache == 128) {
						cpu_type_str = "Celeron (0.13)";
						off = 14;
					} else if (cpu_id.pwrcap == 0x0B) {
						cpu_type_str = "Xeon DP (0.13)";
						off = 14;
					} else if (cpu_id.pwrcap == 0x0C) {
						cpu_type_str = "Xeon MP (0.13)";
						off = 14;
					} else {
						cpu_type_str = "Pentium 4 (0.13)";
						off = 16;
					}
					break;
				case 3:
				case 4:
					if (l2_cache == 256) {
						cpu_type_str = "Celeron (0.09)";
						off = 14;
					} else if (cpu_id.pwrcap == 0x0B) {
						cpu_type_str = "Xeon DP (0.09)";
						off = 14;
					} else if (cpu_id.pwrcap == 0x0C) {
						cpu_type_str = "Xeon MP (0.09)";
						off = 14;
					} else if ((cpu_id.step == 0x4 || cpu_id.step == 0x7) && cpu_id.model == 0x4) {
						cpu_type_str = "Pentium D (0.09)";
						off = 16;
					} else {
						cpu_type_str = "Pentium 4 (0.09)";
						off = 16;
					}
					break;
				case 6:
					cpu_type_str = "Pentium D (65nm)";
					off = 16;		
					break;
				default:
					cpu_type_str = "Unknown Intel";
					off = 13;		
					break;													
				}
				break;
			}
		}
		break;

	/* VIA/Cyrix/Centaur Processors with CPUID */
	case 'C':
		if ( cpu_id.vend_id[1] == 'e' ) {	/* CentaurHauls */
			l1_cache = cpu_id.cache_info[3] + cpu_id.cache_info[7];
			l2_cache = cpu_id.cache_info[11];
			switch(cpu_id.type){
			case 5:
				cpu_type_str = "Centaur 5x86";
				off = 12;
				break;
			case 6: // VIA C3
				switch(cpu_id.model){
					default:
						if (cpu_id.step < 8) {
							cpu_type_str = "VIA C3 Samuel2";
							off = 14;
						} else {
							cpu_type_str = "VIA C3 Eden";
							off = 11;
						}
						break;
					case 10:
						cpu_type_str = "VIA C7 (C5J)";
						l1_cache = 64;
						l2_cache = 128;
						off = 16;
						break;
					case 13:
						cpu_type_str = "VIA C7 (C5R)";
						l1_cache = 64;
						l2_cache = 128;
						off = 12;
						break;
					case 15:
						cpu_type_str = "VIA Isaiah (CN)";
						l1_cache = 64;
						l2_cache = 1024;
						off = 15;
						break;
				}
			}
		} else {				/* CyrixInstead */
			switch(cpu_id.type) {
			case 5:
				switch(cpu_id.model) {
				case 0:
					cpu_type_str = "Cyrix 6x86MX/MII";
					off = 16;
					break;
				case 4:
					cpu_type_str = "Cyrix GXm";
					off = 9;
					break;
				}
				return;

			case 6: // VIA C3
				switch(cpu_id.model) {
				case 6:
					cpu_type_str = "Cyrix III";
					off = 9;
					break;
				case 7:
					if (cpu_id.step < 8) {
						cpu_type_str = "VIA C3 Samuel2";
						off = 14;
					} else {
						cpu_type_str = "VIA C3 Ezra-T";
						off = 13;
					}
					break;
				case 8:
					cpu_type_str = "VIA C3 Ezra-T";
					off = 13;
					break;
				case 9:
					cpu_type_str = "VIA C3 Nehemiah";
					off = 15;
					break;
				}
				// L1 = L2 = 64 KB from Cyrix III to Nehemiah
				l1_cache = 64;
				l2_cache = 64;
				break;
			}
		}
		break;

	/* Unknown processor */
	default:
		off = 3;
		/* Make a guess at the family */
		switch(cpu_id.type) {
		case 5:
			cpu_type_str = "586";
			return;
		case 6:
			cpu_type_str = "686";
			return;
		}
	}

	/* We are here only if the CPU type supports the rdtsc instruction */

	/* Print CPU speed */
	if ((speed = cpuspeed()) > 0) {
		if (speed < 1000000-50) {
			speed += 50; /* for rounding */
            SINF("Speed: %d.%d MHz", speed/1000,(speed/100)%10 );
		} else {
			speed += 500; /* for rounding */
            SINF("Speed: %d MHz", speed/1000);
		}
		extclock = speed;
	}

	/* Print out L1 cache info */
	/* To measure L1 cache speed we use a block size that is 1/4th */
	/* of the total L1 cache size since half of it is for instructions */
	if (l1_cache) {
        SINF("L1 Cache: %d K", l1_cache);
	}

	/* Print out L2 cache info */
	/* We measure the L2 cache speed by using a block size that is */
	/* the size of the L1 cache.  We have to fudge if the L1 */
	/* cache is bigger than the L2 */
	if (l2_cache) {
        SINF("L2 Cache: %d K", l2_cache);
	}

	/* Print out L3 cache info */
	/* We measure the L3 cache speed by using a block size that is */
	/* the size of the L2 cache. */

	if (l3_cache) {
        SINF("L3 Cache: %d K", l3_cache);
	}


	rdtsc = 1;
	if (l1_cache == 0) { l1_cache = 66; }
	if (l2_cache == 0) { l1_cache = 666; }

}

static unsigned long my_mktime(const unsigned int year0, const unsigned int mon0,
       const unsigned int day, const unsigned int hour,
       const unsigned int min, const unsigned int sec)
{
	unsigned int mon = mon0, year = year0;

	/* 1..12 -> 11,12,1..10 */
	if (0 >= (int) (mon -= 2)) {
		mon += 12;	/* Puts Feb last since it has leap day */
		year -= 1;
	}

	return ((((unsigned long)
		  (year/4 - year/100 + year/400 + 367*mon/12 + day) +
		  year*365 - 719499
	    )*24 + hour /* now have hours */
	  )*60 + min /* now have minutes */
	)*60 + sec; /* finally seconds */
}

static unsigned long get_timer_base_usec(void)
{
    return 0;
}

static unsigned long get_timer_offset_usec(void)
{
    static uint32_t curl;
    static uint32_t curh;
    static uint64_t cur;
    asm __volatile__ ("rdtsc":"=a" (curl),"=d" (curh));
    cur = curh;
    cur <<= 32;
    cur += curl;
    
    cur -= tm_base_clk;
    
    /* extclock is kHZ */
    return (cur / extclock) * 1000;
}

static unsigned long get_timer_base_sec(void)
{
    return my_mktime(
        global_rtc_tm.tm_year + epoch, 
        global_rtc_tm.tm_mon + 1, 
        global_rtc_tm.tm_mday,
        global_rtc_tm.tm_hour,
        global_rtc_tm.tm_min,
        global_rtc_tm.tm_sec);
}

void do_gettimeofday(struct timeval *tv)
{
	unsigned long usec, sec;

    usec = get_timer_base_usec();
	sec = get_timer_base_sec();
	usec += get_timer_offset_usec();

	/* usec may have gone up a lot: be safe */
	while (usec >= 1000000) {
		usec -= 1000000;
		sec++;
	}
    if(tv) {
        tv->tv_sec = sec;
        tv->tv_usec = usec;
    }
}

void timer_init(void)
{
    struct timeval tv;
    int ret, len;
    struct tm *tmp;
    char buffer[1024];

    uint32_t startl;
    uint32_t starth;
    cpu_type();
    rtc_get_rtc_time(&global_rtc_tm);

	/* Record the starting time */
	asm __volatile__ ("rdtsc":"=a" (startl),"=d" (starth));
    tm_base_clk = starth; tm_base_clk <<= 32;
    tm_base_clk += startl;

    ret = gettimeofday(&tv, NULL);
    SDBG("ret = %d", ret);
    SDBG("tv.tv_sec = %d", tv.tv_sec);
    SDBG("tv.tv_usec = %d", tv.tv_usec);

    len = 0;
    tmp = localtime(&tv.tv_sec);
    len += strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tmp);
    buffer[len] = 0;
    SDBG("current time is %s", buffer);
}
