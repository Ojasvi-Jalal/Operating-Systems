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

	//to access a byte of CMOS memory
	#define	cmos_write 0x70
	//in order to read the value from registers
	#define	cmos_read	0x71

	//will store the values of the relevant CMOS registers for the real-time clock
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

	/*
	//interrogating register A to check if an update is in progress
	*/
	bool is_update_in_progress(){
		//the update in progress flag is stored in bit 7 of register A
		__outb(cmos_write, 0x0A);
		uint8_t test = __inb(cmos_read)&1<<7;

		//syslog.messagef(LogLevel::DEBUG, "IsUpdateInProgress: register A's 7 bit: %d,",test);
		//if bit 7 is set then update is in progress otherwise no.
		if(test){
			return true;
		}
		else{
			return false;
		}
	}

	/*
	//interrogating register B to check whether the value contained within the registers is in
	//binary or binary coded decimal
	*/
	bool is_bcd(){
		__outb(cmos_write, 0x0B);
		//check bit 2 of register B
		uint8_t bcd = __inb(cmos_read)&(1<<2);

		//if bit 2 of register B is set then the values are stored in binary else in bcd.
		if(bcd)
			return false;
		else
			return true;
	}

	/*
	//reads the byte at a given offset (reads the values of a register)
	*/
	unsigned short read_register(int reg) {
		//activate offset "reg"
		__outb(cmos_write,reg);
		//read data
		uint8_t RTC_register = __inb(cmos_read);
		/*check if the value stored in the register
		//is in binary or binary coded decimal,
		//if in BCD, return it's converted binary value
		*/
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

		//read the real time clock registers
		second	= read_register(0x00);
  	minute	= read_register(0x02);
  	hour	= read_register(0x04);
  	day 	= read_register(0x07);
  	month 	= read_register(0x08);
  	year 	= read_register(0x09);

		//filling in the fields of RTCTimePoint, the structure provided
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
