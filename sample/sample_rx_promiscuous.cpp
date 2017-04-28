/*!
  @file test_rx.cpp
  @brief about test_raw <br>
  sample code to read raw data that is received.

  @subsection how to use <br>

  paramete can be ommited.
  
  (ex)
  @code
  test_raw 36 0xabcd 100 20
  test_raw 36 0xabcd
  @endcode

  when push Ctrl+C, process is quited.
  */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "../lib/liblazurite.h" 

using namespace lazurite;
bool bStop;

/*!
  signal handler <br>
  this process is executed, when Ctrl+C is pushed.
  */
void sigHandle(int sigName)
{
	bStop = true;
	printf("sigHandle = %d\n",sigName);
	return;
}
/*!
  set signal handler for Ctrl+C
  */

int setSignal(int sigName)
{
	if(signal(sigName,sigHandle)==SIG_ERR) return -1;
	return 0;
}
int main(int argc, char **argv)
{
	int result;
	char* en;
	uint8_t ch=36;
	uint8_t rate=100;
	uint8_t pwr=20;
	uint16_t panid=0xabcd;
	uint8_t myaddr_be[8];

	// set Signal Trap
	setSignal(SIGINT);

	timespec rxTime;

	if((result=lazurite_init())!=0) {
		printf("liblzgw_open fail = %d\n",result);
		return EXIT_FAILURE;
	}

	bStop = false;
	if(argc>1) {
		ch = strtol(argv[1],&en,0);
	}
	if(argc>2) {
		panid = strtol(argv[2],&en,0);
	}
	if(argc>3) {
		rate = strtol(argv[3],&en,0);
	}
	if(argc>4) {
		pwr = strtol(argv[4],&en,0);
	}

	printf("short address:: %04x\n",lazurite_getMyAddress());
	result = lazurite_getMyAddr64(myaddr_be);
	printf("mac address::   %02x%02x%02x%02x %02x%02x%02x%02x\n",
		myaddr_be[0],
		myaddr_be[1],
		myaddr_be[2],
		myaddr_be[3],
		myaddr_be[4],
		myaddr_be[5],
		myaddr_be[6],
		myaddr_be[7]
	);

	result = lazurite_begin(ch,panid,rate,pwr);
	if(result < 0) {
		printf("lazurite_begin fail = %d\n",result);
		return EXIT_FAILURE;
	}
	result = lazurite_setPromiscuous(true);
	if(result < 0) {
		printf("lazurite_promiscuous fail = %d\n",result);
		return EXIT_FAILURE;
	}
	result = lazurite_rxEnable();
	if(result < 0) {
		printf("lazurite_rxEnable fail = %d\n",result);
		return EXIT_FAILURE;
	}

	while(bStop == false)
	{
		uint16_t size;
		uint8_t rssi;
		SUBGHZ_MAC mac;
		char raw[256];
		memset(raw,0,sizeof(raw));
		result = lazurite_read(raw,&size);
		if(result > 0 ) {
			result = lazurite_decMac(&mac,raw,size);
			printf("%02x\t%04x\t%02x%02x%02x%02x%02x%02x%02x%02x\t%04x\t%02x%02x%02x%02x%02x%02x%02x%02x\t",
				mac.seq_num,
				mac.dst_panid,
				mac.dst_addr[7],
				mac.dst_addr[6],
				mac.dst_addr[5],
				mac.dst_addr[4],
				mac.dst_addr[3],
				mac.dst_addr[2],
				mac.dst_addr[1],
				mac.dst_addr[0],
				mac.dst_panid,
				mac.src_addr[7],
				mac.src_addr[6],
				mac.src_addr[5],
				mac.src_addr[4],
				mac.src_addr[3],
				mac.src_addr[2],
				mac.src_addr[1],
				mac.src_addr[0]
				);
			printf("%s\n", raw+mac.payload_offset);
		}
		usleep(100000);
	}

	if((result = lazurite_close()) !=0) {
		printf("fail to stop RF %d",result);
	}
	usleep(100000);
	if((result = lazurite_remove()) !=0) {
		printf("remove driver from kernel %d",result);
	}
	return 0;
}
