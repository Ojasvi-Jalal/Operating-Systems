/*
 * CMOS Real-time Clock
 * SKELETON IMPLEMENTATION -- TO BE FILLED IN FOR TASK (3)
 */

/*
 * STUDENT NUMBER: s1612970
 */
#include <infos/drivers/timer/rtc.h>
#include <infos/util/lock.h>
#include <arch/x86/pio.h>

using namespace std;
using namespace infos::arch::x86;
using namespace infos::drivers;
using namespace infos::drivers::timer;
using namespace infos::util;

class CMOSRTC : public RTC {
public:
	static const DeviceClass CMOSRTCDeviceClass;

	#define	cmos_write 0x70
	#define	cmos_read	 0x71

	unsigned char second;
	unsigned char minute;
	unsigned char hour;
	unsigned char day;
	unsigned char month;
	unsigned int 	year;

	const DeviceClass& device_class() const override
	{
		return CMOSRTCDeviceClass;
	}

	int is_update_in_progress(){
		__outb(cmos_write, 0x0A);
		/*good idea to have a reasonable delay after selecting a CMOS register on Port
		//0x70, before reading/writing the value on Port 0x71
		*/
		//usleep(nanoseconds(100));
		//if bit 7 is set then update is in progress otherwise no.
		if(__inb(cmos_read) & 1<<7)
			return true;
		else
			return false;
	}

	bool is_bcd(){
		__outb(cmos_write, 0x0B);
		/*good idea to have a reasonable delay after selecting a CMOS register on Port
		//0x70, before reading/writing the value on Port 0x71
		*/
		//usleep(nanoseconds(100));
		uint8_t b = __inb(cmos_read);
		if(b & (1<<2))
				return false;
		else
				return true;
	}

	unsigned char read_register(int reg) {
		__outb(cmos_write,reg);
		uint8_t RTC_register = __inb(cmos_read);
		if(is_bcd()&&reg == 0x04)
				return ((RTC_register&0x0F) + (RTC_register&cmos_write/16)*10)|(RTC_register&0x80);
		if(is_bcd())
				return ((RTC_register&0x0F)+(RTC_register/16)*10);
		else
			return RTC_register;
	}
	/**
	 * Interrogates the RTC to read the current date & time.
	 * @param tp Populates the tp structure with the current data & time, as
	 * given by the CMOS RTC device.
	 */
	void read_timepoint(RTCTimePoint& tp) override
	{
		//make sure interrupts are disabled when  manipulating the run queue
		UniqueIRQLock l;

		unsigned char last_second;
    unsigned char last_minute;
    unsigned char last_hour;
    unsigned char last_day;
    unsigned char last_month;
    unsigned char last_year;
    unsigned char last_century;

		//wait for an update cycle to begin and an update cycle to complete
		while(is_update_in_progress());
		second	= read_register(0x00);
    minute	= read_register(0x02);
    hour		= read_register(0x04);
    day 		= read_register(0x07);
    month 	= read_register(0x08);
    year 		= read_register(0x09);

		// do {
    //         last_second = second;
    //         last_minute = minute;
    //         last_hour = hour;
    //         last_day = day;
    //         last_month = month;
    //         last_year = year;
		//
    //         while (is_update_in_progress());
    //         second	= read_register(0x00);
    //         minute	= read_register(0x02);
    //         hour		= read_register(0x04);
    //         day			= read_register(0x07);
    //         month		= read_register(0x08);
    //         year		= read_register(0x09);
    //   } while((last_second != second) || (last_minute != minute) || (last_hour != hour) ||
    //            (last_day != day) || (last_month != month) || (last_year != year));

		unsigned char registerB;
		registerB = read_register(0x0B);

		// if(!(registerB & 0x02) && (hour & 0x80)){
		// 	hour = ((hour & 0x7F) + 12)%24;
		// }

		tp.seconds					= (unsigned short) second;
		tp.minutes 					= (unsigned short) minute;
		tp.hours						= (unsigned short) hour;
		tp.day_of_month			= (unsigned short) day;
		tp.month						= (unsigned short) month;
		tp.year							= (unsigned short) year;
	}
};

const DeviceClass CMOSRTC::CMOSRTCDeviceClass(RTC::RTCDeviceClass, "cmos-rtc");

RegisterDevice(CMOSRTC);
