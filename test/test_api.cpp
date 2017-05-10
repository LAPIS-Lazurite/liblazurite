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
	uint8_t myaddr_be[8];

	result=lazurite_test(0x4000);

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

	result = lazurite_begin(36,0xabcd,100,20);
	printf("result = 0x%04x,%04d\n",result,__LINE__);
	result = lazurite_setAckReq(true);
	printf("result = 0x%04x,%04d\n",result,__LINE__);
	result = lazurite_close();
	result = lazurite_setAckReq(false);
	printf("result = 0x%04x,%04d\n",result,__LINE__);
	result = lazurite_begin(36,0xabcd,100,20);
	result = lazurite_close();
	result = lazurite_setMyAddress(0x0000);
	printf("result = 0x%04x,%04d\n",result,__LINE__);
	result = lazurite_begin(36,0xabcd,100,20);
	result = lazurite_close();
	result = lazurite_getMyAddress();
	printf("result = 0x%04x,%04d\n",result,__LINE__);
	result = lazurite_setMyAddress(0xFFFe);
	printf("result = 0x%04x,%04d\n",result,__LINE__);
	result = lazurite_begin(36,0xabcd,100,20);
	result = lazurite_close();
	result = lazurite_getMyAddress();
	printf("result = 0x%04x,%04d\n",result,__LINE__);
	result = lazurite_setMyAddress(0xFFFF);
	printf("result = 0x%04x,%04d\n",result,__LINE__);
	result = lazurite_begin(36,0xabcd,100,20);
	result = lazurite_close();
	result = lazurite_getMyAddress();
	printf("result = 0x%04x,%04d\n",result,__LINE__);
	result = lazurite_setBroadcastEnb(false);
	printf("result = 0x%04x,%04d\n",result,__LINE__);
	result = lazurite_begin(36,0xabcd,100,20);
	result = lazurite_close();
	result = lazurite_setBroadcastEnb(true);
	printf("result = 0x%04x,%04d\n",result,__LINE__);
	result = lazurite_begin(36,0xabcd,100,20);
	result = lazurite_close();
	result = lazurite_setPromiscuous(false);
	printf("result = 0x%04x,%04d\n",result,__LINE__);
	result = lazurite_begin(36,0xabcd,100,20);
	result = lazurite_close();
	result = lazurite_setPromiscuous(true);
	printf("result = 0x%04x,%04d\n",result,__LINE__);
	result = lazurite_remove();
	return 0;
}

