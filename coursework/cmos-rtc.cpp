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
#include <infos/kernel/log.h>

using namespace std;
using namespace infos::arch::x86;
using namespace infos::drivers;
using namespace infos::drivers::timer;
using namespace infos::util;
using namespace infos::kernel;

class CMOSRTC : public RTC {
public:
	static const DeviceClass CMOSRTCDeviceClass;

	#define	cmos_write	0x70
	#define	cmos_read	0x71

	unsigned short second;
	unsigned short minute;
	unsigned short hour;
	unsigned short day;
	unsigned short month;
	unsigned short year;

	const DeviceClass& device_class() const override
	{
		return CMOSRTCDeviceClass;
	}

	bool is_update_in_progress(){
		__outb(cmos_write, 0x0A);
		/*good idea to have a reasonable delay after selecting a CMOS register on Port
		//0x70, before reading/writing the value on Port 0x71
		*/
		//usleep(nanoseconds(100));
		//if bit 7 is set then update is in progress otherwise no.
		uint8_t test = __inb(cmos_read)&1<<7; //& 1<<6;//
		//uint8_t test = __inb(cmos_read) & //1<<7;
		//syslog.messagef(LogLevel::DEBUG, "IsUpdateInProgress: register A's 7 bit: %d,",test);
		if(test){
			syslog.messagef(LogLevel::DEBUG, "IsUpdateInProgress: register A's 7 bit is set to 1: %d,", test);
			return true;
		}		
		else{
			//syslog.messagef(LogLevel::DEBUG, "IsUpdateInProgress: register A's 7 bit is set to 0");
			return false;
		}
			
	}

	bool is_bcd(){
		__outb(cmos_write, 0x0B);
		/*good idea to have a reasonable delay after selecting a CMOS register on Port
		//0x70, before reading/writing the value on Port 0x71
		*/
		//usleep(nanoseconds(100));
		uint8_t bcd = __inb(cmos_read)&(1<<2);
		if(bcd)
				return false;
		else
				return true;
	}

	unsigned short read_register(int reg) {
		__outb(cmos_write,reg);
		uint8_t RTC_register = __inb(cmos_read);
		// if(is_bcd()&&reg == 4)
		// 		return ((RTC_register&0x0F) + (RTC_register&cmos_write/16)*10)|(RTC_register&0x80);
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

		//wait for an update cycle to begin and an update cycle to complete	
		if(!is_update_in_progress()){
			while(!is_update_in_progress());
			while(is_update_in_progress());
		}
	
		second	= read_register(0x00);
    	minute	= read_register(0x02);
    	hour	= read_register(0x04);
    	day 	= read_register(0x07);
    	month 	= read_register(0x08);
    	year 	= read_register(0x09);

		// unsigned short registerB;
		// registerB = read_register(0x0B);

		// if(!(registerB & 0x02) && (hour & 0x80)){
		// 	hour = ((hour & 0x7F) + 12)%24;
		// }

		tp.seconds			=  second;
		tp.minutes 			=  minute;
		tp.hours			=  hour;
		tp.day_of_month		=  day;
		tp.month			=  month;
		tp.year				=  year;
	}
};

const DeviceClass CMOSRTC::CMOSRTCDeviceClass(RTC::RTCDeviceClass, "cmos-rtc");

RegisterDevice(CMOSRTC);
