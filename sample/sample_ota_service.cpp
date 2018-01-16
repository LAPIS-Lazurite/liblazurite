/*!
  @file ota_service.cpp
  @brief about ota service program <br>
  sample code to serve OTA firmware update.

  @subsection how to use <br>

  parameter can be ommited.
  
  (ex)
  @code
  sample_ota_service test.bin 1 36 0xabcd 100 20
  sample_ota_service test.bin
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

//#define MANUAL_MODE		// uncomment, if manual mode

#define _SIGNATURE_SIZE				( 3 )
#define _PROGRAM_DATA_SIZE			( 128 )
#define _MAX_NAME_SIZE				( 16 )
#define _FILE_SIZE					( 0x10000 )
#define _BUILT_IN_KEYWORD_OFFSET	( 0x68 )
#define _HW_TYPE_OFFSET				( 0x68+4 )
#define _USER_PRGM_START			( 0x400 )
#define _USER_PRGM_END				( 0x6400-2 )
#define _DATA_REQ_EXPIRE_COUNT		( 10 )			// 10s
#define _REP_START_REQ_SIZE			( 23 )			// 3+1+1+16+2

using namespace lazurite;

/* --------------------------------------------------------------------------------
 * Please note that this OTA service uses SubGHz AES encrypted communication.
 * The below AES key must be changed apporpriately. For more infomation, please
 * refer to the document on Lazurite HP. ( http://www.lapis-semi.com/lazurite-jp/ )
 * -------------------------------------------------------------------------------- */
char key[] = "b64b729a3c362a2aee0063212f9bd317";

bool bStop;
int _read_file(uint8_t *path);
uint16_t _calc_end_addr(void);
void _reply_start_request(uint16_t panid, uint16_t addr);
void _reply_data_request(uint16_t panid, uint16_t addr);
void _begin_update(uint16_t panid);
void _timer_handler(int signum);
void _data_req_timer_start(void);
void _data_req_timer_stop(void);

typedef struct {
	uint8_t sig[_SIGNATURE_SIZE];
	uint8_t hw_type;
	uint8_t ver;
	uint8_t name[_MAX_NAME_SIZE];
	uint16_t addr;
	uint8_t bin[_PROGRAM_DATA_SIZE];
} __attribute__((__packed__)) TRX_DATA;

TRX_DATA trx_data;

uint8_t _file_buf[_FILE_SIZE];
uint8_t _file_ver=1;
uint8_t _hw_type=0;
uint16_t _end_addr=_USER_PRGM_END;
uint16_t _active_node_addr=0xffff;
int _timer_count=_DATA_REQ_EXPIRE_COUNT;
bool _timer_start=false;
bool _timer_stop=false;
bool _timer_running=false;
struct sigaction _act, _oldact;
timer_t	_timer_id;

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
	uint8_t ch=50;
	uint8_t rate=100;
	uint8_t pwr=20;
	uint16_t panid=0xabcd;
	uint8_t myaddr_be[8];
	uint8_t *fpath;

	// set Signal Trap
	setSignal(SIGINT);

	if(argc>1) {
		fpath = (uint8_t*)argv[1];
		printf("info : file name = %s\n",fpath);
	} else {
		printf("usage: ota_service file (ver,ch,panid,rate,pwr)\n");
		printf("\tfile - full path of bin file.\n\n");
		printf("optional:\n");
		printf("\tver - version number(1~255) of bin file.\n");
		printf("\tch,panid,rate,pwr - SubGHz parameters.\n");
		return -1;
	}

	//
	// file open and calculate end address
	//
	result = _read_file(fpath);
	if (result == -1) {
		printf("err  : file open failed. code:%d\n",result);
		return -1;
	} else if (result == -2) {
		printf("err  : file size is mismatch. code:%d\n",result);
		return -1;
	} else if (result == -3) {
		printf("err  : file is not build with ota profile. code:%d\n",result);
		return -1;
	} else if (result != _FILE_SIZE) {
		printf("err  : unknown error. code:%d\n",result);
		return -1;
	} else {
		// do nothing
	}

//	_prgm_name = _get_prgm_name();
//	printf("info : program name = %s\n",_prgm_name);

	_end_addr = _calc_end_addr();
	printf("info : end program addr = 0x%04X\n",_end_addr);

	timespec rxTime;

	if((result=lazurite_init())!=0) {
		printf("liblzgw_open fail = %d\n",result);
		return EXIT_FAILURE;
	}
	bStop = false;
	if(argc>2) {
		_file_ver = strtol(argv[2],&en,0);
	}
	if(argc>3) {
		ch = strtol(argv[3],&en,0);
	}
	if(argc>4) {
		panid = strtol(argv[4],&en,0);
	}
	if(argc>5) {
		rate = strtol(argv[5],&en,0);
	}
	if(argc>6) {
		pwr = strtol(argv[6],&en,0);
	}
	printf("info : host address = 0x%04X\n",lazurite_getMyAddress());

	result = lazurite_setKey(key);
	if(result < 0) {
		printf("err  : lazurite_setKey fail = %d\n",result);
		return EXIT_FAILURE;
	}

	result = lazurite_begin(ch,panid,rate,pwr);
	if(result < 0) {
		printf("err  : lazurite_begin fail = %d\n",result);
		return EXIT_FAILURE;
	}

	result = lazurite_rxEnable();
	if(result < 0) {
		printf("err  : lazurite_rxEnable fail = %d\n",result);
		return EXIT_FAILURE;
	}
#ifdef MANUAL_MODE
	printf("info : ctlr-c to break, or manual command mode.\n");
#else
	printf("info : ctlr-c to break.\n");
#endif

	//
	// wait for rx
	//
	while(1)
	{
		uint16_t size,src_addr;
		uint8_t raw[256];
		SUBGHZ_MAC mac;
		char tmp[100],*c;

		result = lazurite_read(raw,&size);
		if(result > 0 ) {
			lazurite_decMac(&mac,raw,size);
			src_addr = mac.src_addr[0] | mac.src_addr[1]<<8;
//for(int i=0;i<size;i++) printf("%02X ",raw[i]);
//printf("\n");
			memcpy(&trx_data,raw+mac.payload_offset,mac.payload_len);
			if (trx_data.hw_type == _hw_type) {
				if (strncmp((char*)trx_data.sig,"$R0",3)==0) {
					printf("RX   : 0x%04X hw_type = %02X\n",src_addr,trx_data.hw_type);
#ifndef MANUAL_MODE
					_reply_start_request(panid, src_addr);
#endif
				} else if (strncmp((char*)trx_data.sig,"$R1",3)==0) {
					printf("RX   : 0x%04X addr = %04X\n",src_addr,trx_data.addr);
#ifndef MANUAL_MODE
					usleep(10000);
					_reply_data_request(panid, src_addr);
#endif
				} else {
					// do nothing
				}
			} else {
				// do nothing
			}
		}
		if (_timer_start) {
			_timer_start = false;
			_data_req_timer_start();
			_timer_count=_DATA_REQ_EXPIRE_COUNT;		// timer reset
			_timer_running = true;
			printf("timer: start.\n");
		}
		if (_timer_stop || (_timer_count <= 0)) {
			_timer_stop = false;
			if (_timer_count <= 0) {
				printf("timer: 0x%04x node not responding, timer expired.\n",_active_node_addr);
				_active_node_addr = 0xffff;
			}
			_data_req_timer_stop();
			_timer_count=_DATA_REQ_EXPIRE_COUNT;		// timer reset
			_timer_running = false;
			printf("timer: stop.\n");
		}
		if (bStop) {
			bStop=false;
#ifdef MANUAL_MODE
			printf("input: q(quit),t(trigger),s/s_addr(start req),d(data req),c(clear active node)\n");
#else
			printf("input: q(quit),t(trigger)\n");
#endif
			fgets(tmp,100,stdin);
			c = strtok(tmp," ");
			if (*c=='q') {
				break;
			} else if (*c=='t') {
				_begin_update(panid);
				continue;
			} else if (*c=='s') {
				c = strtok(NULL, " ");
				if (c) {
					_reply_start_request(panid, strtol(c,0,0));
				} else {
					_reply_start_request(panid, src_addr);
				}
				continue;
			} else if (*c=='d') {
				_reply_data_request(panid, src_addr);
				continue;
			} else if (*c=='c') {
				_active_node_addr = 0xffff;
				printf("clear active ok node.\n");
				continue;
			} else {
				// do nothing
			}
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

void _reply_start_request(uint16_t panid, uint16_t src_addr)
{
	int result;

	if (src_addr == 0) return;
	if ((_active_node_addr == 0xffff) || (_active_node_addr == src_addr)) {
		strncpy((char*)trx_data.sig,"$A0",3);	// active ok
		printf("TX   : active ok 0x%04X.\n",src_addr);
	} else {
		strncpy((char*)trx_data.sig,"$A1",3);	// passive ok
		printf("TX   : passive ok 0x%04X.\n",src_addr);
	}
	trx_data.ver = _file_ver;
	trx_data.addr = _end_addr;
	result = lazurite_send(panid,src_addr,(uint8_t *)&trx_data,_REP_START_REQ_SIZE);
	if ((_active_node_addr == 0xffff) && (result >= 0)) {
		_active_node_addr = src_addr;
#ifndef MANUAL_MODE
		if (!_timer_running) _timer_start = true;
#endif
	}
}

void _reply_data_request(uint16_t panid, uint16_t src_addr)
{
	int result;
	uint16_t data_addr = trx_data.addr;

	if (src_addr == 0) return;
	if ((data_addr >= _USER_PRGM_START) && (data_addr <= _USER_PRGM_END) && \
		(_active_node_addr != 0xffff) && (_active_node_addr == src_addr) && \
		(trx_data.hw_type == _hw_type) && (trx_data.ver == _file_ver)) {
		strncpy((char*)trx_data.sig,"$A2",3);
		memcpy(&trx_data.bin,&_file_buf[data_addr],_PROGRAM_DATA_SIZE);
//for(int i=0;i<_PROGRAM_DATA_SIZE;i++) printf("%02X ",trx_data.bin[i]);
//printf("\n");
		result = lazurite_send(panid,0xffff,(uint8_t *)&trx_data,sizeof(trx_data));	// group cast
		printf("TX   : program data.\n");
		_timer_count=_DATA_REQ_EXPIRE_COUNT;			// timer reload
		if (data_addr >= _end_addr) {
			_active_node_addr = 0xffff;
#ifndef MANUAL_MODE
			if (_timer_running) _timer_stop = true;
#endif
		}
	}
}

void _begin_update(uint16_t panid)
{
	int result;
	uint8_t tx_data[] = "start";

	result = lazurite_send(panid,0xffff,tx_data,sizeof(tx_data));	// group cast
	printf("TX   : start trigger.\n");
}

int _read_file(uint8_t *path)
{
	int result = 0;
	FILE *fp;

	fp = fopen((char*)path, "rb");
	if (!fp) {
		result = -1;
	} else {
		result = fread(_file_buf,1,_FILE_SIZE,fp);
		fclose(fp);
		if (result != _FILE_SIZE) {
			result = -2;
		} else {
			if (strncmp((char*)&_file_buf[_BUILT_IN_KEYWORD_OFFSET],"$OTA",4) != 0) {
				result = -3;
			} else {
				_hw_type = _file_buf[_HW_TYPE_OFFSET];
				printf("info : built-in hw_type = 0x%02X\n",_hw_type);
			}
		}
	}
	return result;
}
 
uint16_t _calc_end_addr(void)
{
	int i;

	for (i=_USER_PRGM_END;i>_USER_PRGM_START;i--,i--) {
		if (*((uint16_t *)(_file_buf+i)) != 0xffff) break;
	}
	return (i & 0xff80);
}

void _timer_handler(int signum)
{
	_timer_count--;
	printf("timer: count %d\n", _timer_count);
}

void _data_req_timer_start(void)
{
	struct itimerspec itval;

    memset(&_act, 0, sizeof(struct sigaction));
    memset(&_oldact, 0, sizeof(struct sigaction));
 

    // register signal handler
    _act.sa_handler = _timer_handler;
    _act.sa_flags = 0;
    if(sigaction(SIGALRM, &_act, &_oldact) < 0) {
        perror("sigaction()");
        return;
    }
 
    // timer setting
    itval.it_value.tv_sec = 5;     // 1st time: 5s
    itval.it_value.tv_nsec = 0;
    itval.it_interval.tv_sec = 1;  // 2nd or later: 1s
    itval.it_interval.tv_nsec = 0;
 
    // create timer
    if(timer_create(CLOCK_REALTIME, NULL, &_timer_id) < 0) {
        perror("timer_create");
        return;
    }
 
    //  set timer
    if(timer_settime(_timer_id, 0, &itval, NULL) < 0) {
        perror("timer_settime");
        return;
	}
}

void _data_req_timer_stop(void)
{
	timer_delete(_timer_id);
	sigaction(SIGALRM, &_oldact, NULL);
}

