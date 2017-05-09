/*!
  @mainpage

  liblazurite
  @brief dynamic library to use LazDriver
  functions/APIs are symplified.

  @section general description
  documentation about liblazurite.so

  @section how to install
  @subsection install of LazDriver
  @code
  git clone git://github.com/LAPIS-Lazurite/LazDriver
  cd LazDriver
  make
  @endcode

  @subsection install of liblazurite
  @code
  git clone git://github.com/LAPIS-Lazurite/liblazurite
  cd liblazurite
  make
  @endcode

  then liblazurite is copied in /usr/lib

  @section about sample program
  sample code | build option | operation
  ------------| -------------| --------
  test_t      | tx           | sample of lazurite_write
  test_rx     | rx           | sample of lazurite_readPayload
  test_link   | link         | sample of lazurite_readLink
  test_raw    | raw          | sample of lazurite_read

 @date       Aug,20,2016
 @author     Naotaka Saito
 @par        Revision
 revision 0.01
 @par        Copyright
 Lapis Semiconductor
 *******************************************************************************
 @par        History
 - Preriminary Aug,22,2016 Naotaka Saito
 ******************************************************************************/

/*! @ingroup lazurite*/
/* @{ */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "drv-lazurite.h"
#include "liblazurite.h"
#include <unistd.h> 

#ifdef __cplusplus
/*! @class lazurite
  @brief  namespace of this library
  */
namespace lazurite
{
#endif
	static int fp;  /*< file descripter fo IOCTL */
	/*! @brief 
	  linkedAddr=0xffff:  receive from all device<br>
	  linkedAddr!=0xffff: receive from specific device<br>
	  */
	static uint16_t linkedAddr;
	/*! @brief
	  buffer for receiving data
	  */
	static char buf[256];

#define DEFAULT_CH		36 /*!< default channel */
#define DEFAULT_PANID	0xFFFF  /*!< default panid*/
#define DEFAULT_RX_ADDR	0xFFFF  /*!< default rx address*/
#define DEFAULT_RATE	100  /*!< default of bit rate*/
#define DEFAULT_PWR		20  /*!< default of tx power*/


	/*! @struct s_MAC_HEADER_BIT_ALIGNMENT
	  @brief  abstruct
	  internal use only
	  bit alightment of mac header
	  */
	typedef struct {
		uint8_t frame_type:3;
		uint8_t sec_enb:1;
		uint8_t pending:1;
		uint8_t ack_req:1;
		uint8_t panid_comp:1;
		uint8_t nop:1;
		uint8_t seq_comp:1;
		uint8_t ielist:1;
		uint8_t dst_addr_type:2;
		uint8_t frame_ver:2;
		uint8_t src_addr_type:2;
	} s_MAC_HEADER_BIT_ALIGNMENT;

	/*! @union u_MAC_HEADER
	  @brief  internal use only
	  mac header
	  data[] 8x2byte
	  header short type
	  alignment bit alignment
	  */
	typedef union {
		uint8_t data[2];
		uint16_t header;
		s_MAC_HEADER_BIT_ALIGNMENT alignment;
	} u_MAC_HEADER;

	/*! @struct SUBGHZ_MAC_PARAM
	  @brief internal use only
	  mac data format
	  */
	typedef struct
	{
		u_MAC_HEADER mac_header;
		uint8_t seq_num;
		uint8_t addr_type;
		uint16_t dst_panid;
		uint8_t  dst_addr[8];
		uint16_t src_panid;
		uint8_t  src_addr[8];
		char *raw;
		int16_t raw_len;
		char *payload;
		int16_t payload_len;
	} SUBGHZ_MAC_PARAM;

	/******************************************************************************/
	/*! @brief ieee802154e mac decoder
	  @param[out]     *mac    result of decoding raw
	  @param[in]      *raw    raw data of ieee802154
	  @param[in]      raw_len length of raw
	  @par            Refer
	  - referene of grobal parameters none
	  @par            Modify
	  - modifying grobal parameterx none
	  @return         none
	  @exception      none
	 ******************************************************************************/
	static void subghz_decMac(SUBGHZ_MAC_PARAM *mac,void *raw,uint16_t raw_len)
	{
		int i;
		int16_t offset=0;
		uint8_t addr_type;
		char* buf = (char*) raw;
		mac->mac_header.data[0] = buf[offset],offset++;
		mac->mac_header.data[1] = buf[offset],offset++;
		if(!mac->mac_header.alignment.seq_comp) {
			mac->seq_num = buf[offset],offset++;
		}
		if(mac->mac_header.alignment.dst_addr_type) addr_type = 4;
		else addr_type = 0;
		if(mac->mac_header.alignment.src_addr_type) addr_type += 2;
		if(mac->mac_header.alignment.panid_comp) addr_type += 1;
		mac->addr_type = addr_type;
		//dst_panid
		switch(addr_type){
			case 1:
			case 4:
			case 6:
				mac->dst_panid = buf[offset+1];
				mac->dst_panid = (mac->dst_panid<<8) + buf[offset];
				offset+=2;
				break;
			default:
				mac->src_panid = 0xffff;
				break;
		}
		//dst_addr
		switch(mac->mac_header.alignment.dst_addr_type)
		{
			case 1:
				mac->dst_addr[0] = buf[offset],offset++;
				for(i=1;i<8;i++) {
					mac->dst_addr[i] = 0;
				}
				break;
			case 2:
				mac->dst_addr[0] = buf[offset],offset++;
				mac->dst_addr[1] = buf[offset],offset++;
				for(i=2;i<8;i++) {
					mac->dst_addr[i] = 0;
				}
				break;
			case 3:
				for(i=0;i<8;i++){
					mac->dst_addr[i] = buf[offset],offset++;
				}
			default:
				break;
		}
		// src_panid
		switch(mac->addr_type){
			case 2:
				mac->src_panid = buf[offset+1];
				mac->src_panid = (mac->src_panid<<8) + buf[offset];
				offset+=2;
				break;
			default:
				mac->src_panid = 0xffff;
				break;
		}
		//src_addr
		memset(mac->src_addr,0xffff,sizeof(mac->src_addr));
		switch(mac->mac_header.alignment.src_addr_type)
		{
			case 1:
				mac->src_addr[0] = buf[offset],offset++;
				for(i=1;i<8;i++) {
					mac->src_addr[i] = 0;
				}
				break;
			case 2:
				mac->src_addr[0] = buf[offset],offset++;
				mac->src_addr[1] = buf[offset],offset++;
				for(i=2;i<8;i++) {
					mac->src_addr[i] = 0;
				}
				break;
			case 3:
				for(i=0;i<8;i++){
					mac->src_addr[i] = buf[offset],offset++;
				}
			default:
				break;
		}
		mac->raw = buf;
		mac->raw_len = raw_len;
		mac->payload=buf+offset;
		mac->payload_len = raw_len-offset;
		return;
	}

	/******************************************************************************/
	/*! @brief get all data by text format
	  @param[out]     addr   linked address
	  @par            Refer
	  @return         0=success <br> 0 < fail
	  @exception      none
	 ******************************************************************************/
	extern "C" int lazurite_readStream(char* stream, uint16_t* size){
		char raw[256];
		uint16_t rawSize;
		int result;
		time_t sec;
		long nsec;
		uint8_t rssi;

		SUBGHZ_MAC mac;

		result = lazurite_read(raw,&rawSize);
		if(result <= 0) return result;

		raw[rawSize]=0;								// force to write NULL
		result = lazurite_getRxRssi();
		result = lazurite_getRxTime(&sec,&nsec);
		result = lazurite_decMac(&mac,raw,rawSize);
		sprintf(stream,"%d,%d,%d,%d,0x%04x,0x%04x,%s",
			sec,
			nsec,
			mac.seq_num,
			rssi,
			mac.src_panid,
			mac.src_addr,
			raw[mac.payload_offset]
			);
		*size = strnlen(stream,512);
		result = *size;

		return result;
	}

	/******************************************************************************/
	/*! @brief set linked address
	  addr = 0xffff		receiving all data
	  addr != 0xffff	receiving data of linked address only
	  @param[out]     addr   linked address
	  @par            Refer
	  @return         0=success <br> 0 < fail
	  @exception      none
	 ******************************************************************************/
	extern "C" int lazurite_link(uint16_t addr) {
		int result=0;
		linkedAddr = addr;
		return result;
	}
	/******************************************************************************/
	/*! @brief load LazDriver
	  @param      none
	  @return         0=success <br> 0 < fail
	  @exception  none
	  @todo  must be change folder name of lazdriver
	 ******************************************************************************/

	extern "C" int lazurite_init(void)
	{
		int result;
		int errcode = 0;

		// open device driver
		system("sudo insmod /home/pi/driver/LazDriver/lazdriver.ko");
		//system("sudo insmod /home/pi/driver/LazDriver/lazdriver.ko module_test=0xFFFF");
		system("sudo chmod 777 /dev/lzgw");
		fp = open("/dev/lzgw",O_RDWR),errcode--;
		if(fp<0) return -1;

		// initializing paramteters..
		linkedAddr = 0xffff;

		return 0;
	}

	/******************************************************************************/
	/*! @brief remove driver from kernel
	  @param     none
	  @return         0=success <br> 0 < fail
	  @exception none
	 ******************************************************************************/
	extern "C" int lazurite_remove(void) 
	{
		close(fp);
		usleep(100000);
		system("sudo rmmod lazdriver");
		return 0;
	}

	/******************************************************************************/
	/*! @brief set rx address to be sent
	  @param[out]     tmp_rxaddr   rxaddr to be sent
	  @return         0=success <br> 0 < fail
	  @exception      none
	 ******************************************************************************/
	extern "C" int lazurite_setRxAddr(uint16_t tmp_rxaddr)
	{
		int result;
		result = ioctl(fp,IOCTL_PARAM | IOCTL_SET_DST_ADDR0,tmp_rxaddr);
		if(result != tmp_rxaddr) {
			return -1;
		}
		return 0;
	}

	/******************************************************************************/
	/*! @brief set PANID for TX
	  @param[out]     txpanid    set PANID(Personal Area Network ID) for sending
	  @return         0=success <br> 0 < fail
	  @exception      none
	 ******************************************************************************/
	extern "C" int lazurite_setTxPanid(uint16_t txpanid)
	{
		int result;
		result = ioctl(fp,IOCTL_PARAM | IOCTL_SET_DST_PANID,txpanid);
		if(result != txpanid) return -1;
		return 0;
	}

	/******************************************************************************/
	/*! @brief setup lazurite module
	  @param[in]  ch (RF frequency)<br>
	  in case of 100kbps, ch is 24-31, 33-60<br>
	  in case of 50kbps, ch is 24-61
	  @param[in]  mypanid
	  set my PANID.
	  @param[in] rate 50 or 100<br>
	  100 = 100kbps<br>
	  50  = 50kbps 
	  @param[in] pwr 1 or 20<br>
	  1  = 1mW<br>
	  20 = 20mW
	  @return         0=success <br> 0 < fail
	  @exception  none
	 ******************************************************************************/
	extern "C" int lazurite_begin(uint8_t ch, uint16_t mypanid, uint8_t rate,uint8_t pwr)
	{
		int result;
		int errcode = 0;

		result = ioctl(fp,IOCTL_PARAM | IOCTL_SET_CH,ch), errcode--;
		if(result != ch) return errcode;

		result = ioctl(fp,IOCTL_PARAM | IOCTL_SET_MY_PANID,mypanid), errcode--;
		if(result != mypanid) return errcode;

		result = ioctl(fp,IOCTL_PARAM | IOCTL_SET_BPS,rate), errcode--;
		if(result != rate) return errcode;

		result = ioctl(fp,IOCTL_PARAM | IOCTL_SET_PWR,pwr), errcode--;
		if(result != pwr) return errcode;

		result = ioctl(fp,IOCTL_CMD | IOCTL_SET_BEGIN,0), errcode--;
		if(result != 0) return errcode;

		return 0;
	}

	/******************************************************************************/
	/*! @brief close driver (stop RF)
	  @param     none
	  @return         0=success <br> 0 < fail
	  @exception none
	 ******************************************************************************/
	extern "C" int lazurite_close(void)
	{
		int result;
		int errcode = 0;

		result = ioctl(fp,IOCTL_CMD | IOCTL_SET_CLOSE,0), errcode--;
		if(result != 0) return errcode;

		return 0;
	}

	/******************************************************************************/
	/*! @brief send data
	  @param[in]     rxpanid	panid of receiver
	  @param[in]     dst_be     8x8bit address pointer for 64bit MAC address(big endian)<br>
	  rxpanid & txaddr = 0xffff = broadcast <br>
	  others = unicast <br>
	  @param[in]     payload start poiter of data to be sent
	  @param[in]     length length of payload
	  @return         0=success=0 <br> -ENODEV = ACK Fail <br> -EBUSY = CCA Fail
	  @exception none
	 ******************************************************************************/
	extern "C" int lazurite_send64be(uint8_t *dst_be,const void* payload, uint16_t length)
	{
		int result;
		int errcode=-1;
		union {
			uint8_t a8[8];
			uint16_t a16[4];
		} uaddr;

		if(!dst_be) return errcode;
		for(int i=0;i<8;i++) {
			uaddr.a8[7-i] = dst_be[i];
		}

		result = ioctl(fp,IOCTL_PARAM | IOCTL_SET_DST_ADDR0,uaddr.a16[0]), errcode--;
		if(result != uaddr.a16[0]) return errcode;
		result = ioctl(fp,IOCTL_PARAM | IOCTL_SET_DST_ADDR1,uaddr.a16[1]), errcode--;
		if(result != uaddr.a16[1]) return errcode;
		result = ioctl(fp,IOCTL_PARAM | IOCTL_SET_DST_ADDR2,uaddr.a16[2]), errcode--;
		if(result != uaddr.a16[2]) return errcode;
		result = ioctl(fp,IOCTL_PARAM | IOCTL_SET_DST_ADDR3,uaddr.a16[3]), errcode--;
		if(result != uaddr.a16[3]) return errcode;

		result = write(fp,payload,length);
		return result;
	}

	/******************************************************************************/
	/*! @brief send data
	  @param[in]     rxpanid	panid of receiver
	  @param[in]     dst_le     8x8bit address pointer for 64bit MAC address(big endian)<br>
	  rxpanid & txaddr = 0xffff = broadcast <br>
	  others = unicast <br>
	  @param[in]     payload start poiter of data to be sent
	  @param[in]     length length of payload
	  @return         0=success=0 <br> -ENODEV = ACK Fail <br> -EBUSY = CCA Fail
	  @exception none
	 ******************************************************************************/
	extern "C" int lazurite_send64le(uint8_t *dst_le,const void* payload, uint16_t length)
	{
		int result;
		int errcode=-1;
		union {
			uint8_t a8[8];
			uint16_t a16[4];
		} uaddr;

		if(!dst_le) return errcode;
		for(int i=0;i<7;i++) {
			uaddr.a8[i] = dst_le[i];
		}

		result = ioctl(fp,IOCTL_PARAM | IOCTL_SET_DST_ADDR0,uaddr.a16[0]), errcode--;
		if(result != uaddr.a16[0]) return errcode;
		result = ioctl(fp,IOCTL_PARAM | IOCTL_SET_DST_ADDR1,uaddr.a16[1]), errcode--;
		if(result != uaddr.a16[1]) return errcode;
		result = ioctl(fp,IOCTL_PARAM | IOCTL_SET_DST_ADDR2,uaddr.a16[2]), errcode--;
		if(result != uaddr.a16[2]) return errcode;
		result = ioctl(fp,IOCTL_PARAM | IOCTL_SET_DST_ADDR3,uaddr.a16[3]), errcode--;
		if(result != uaddr.a16[3]) return errcode;

		result = write(fp,payload,length);
		return result;
	}

	/******************************************************************************/
	/*! @brief send data
	  @param[in]     rxpanid	panid of receiver
	  @param[in]     txaddr     16bit short address of receiver<br>
	  rxpanid & txaddr = 0xffff = broadcast <br>
	  others = unicast <br>
	  @param[in]     payload start poiter of data to be sent
	  @param[in]     length length of payload
	  @return         0=success=0 <br> -ENODEV = ACK Fail <br> -EBUSY = CCA Fail
	  @exception none
	 ******************************************************************************/
	extern "C" int lazurite_send(uint16_t rxpanid,uint16_t rxaddr,const void* payload, uint16_t length)
	{
		int result;
		int errcode=0;

		result = ioctl(fp,IOCTL_PARAM | IOCTL_SET_DST_PANID,rxpanid), errcode--;
		if(result != rxpanid) return errcode;

		result = ioctl(fp,IOCTL_PARAM | IOCTL_SET_DST_ADDR0,rxaddr), errcode--;
		if(result != rxaddr) return errcode;

		result = write(fp,payload,length);
		return result;
	}

	/******************************************************************************/
	/*! @brief enable RX
	  @param     none
	  @return         0=success <br> 0 < fail
	  @exception none
	 ******************************************************************************/
	extern "C" int lazurite_rxEnable(void)
	{
		int result;
		int errcode=0;

		result = ioctl(fp,IOCTL_CMD | IOCTL_SET_RXON,0), errcode--;
		if(result != 0) return errcode;

		return 0;
	}

	/******************************************************************************/
	/*! @brief disable RX
	  @param     none
	  @return         0=success <br> 0 < fail
	  @exception none
	 ******************************************************************************/
	extern "C" int lazurite_rxDisable(void)
	{
		int result;
		int errcode=0;

		result = ioctl(fp,IOCTL_CMD | IOCTL_SET_RXOFF,0), errcode--;
		if(result != 0) return errcode;

		return 0;
	}

	/******************************************************************************/
	/*! @brief get my address
	  @param[out]     short pointer to return my address
	  @return         0=success <br> 0 < fail
	  @exception none
	 ******************************************************************************/

	extern "C" int lazurite_getMyAddr64(uint8_t *addr)
	{
		int result;
		int errcode=-1;

		if(!addr) return errcode;
		result = ioctl(fp,IOCTL_PARAM | IOCTL_GET_MY_ADDR0,0), errcode--;
		if(result < 0) return errcode;
		else {
			addr[0] = (result >> 8 )&0x00FF;
			addr[1] = (result >> 0 )&0x00FF;
		}
		result = ioctl(fp,IOCTL_PARAM | IOCTL_GET_MY_ADDR1,0), errcode--;
		if(result < 0) return errcode;
		else {
			addr[2] = (result >> 8 )&0x00FF;
			addr[3] = (result >> 0 )&0x00FF;
		}
		result = ioctl(fp,IOCTL_PARAM | IOCTL_GET_MY_ADDR2,0), errcode--;
		if(result < 0) return errcode;
		else {
			addr[4] = (result >> 8 )&0x00FF;
			addr[5] = (result >> 0 )&0x00FF;
		}
		result = ioctl(fp,IOCTL_PARAM | IOCTL_GET_MY_ADDR3,0), errcode--;
		if(result < 0) return errcode;
		else {
			addr[6] = (result >> 8 )&0x00FF;
			addr[7] = (result >> 0 )&0x00FF;
		}

		return 0;
	}

	/******************************************************************************/
	/*! @brief get my address
	  @param[out]     short pointer to return my address
	  @return         0=success <br> 0 < fail
	  @exception none
	 ******************************************************************************/

	extern "C" long lazurite_getMyAddress(void)
	{
		int result;
		int errcode=0;
		uint16_t addr;

		result = ioctl(fp,IOCTL_PARAM | IOCTL_GET_MY_ADDR3,0), errcode--;
		if((addr = result )<0) return errcode;

		return addr;
	}

	/******************************************************************************/
	/*! @brief set my short address
	  @param[in]      my_addr(0x0000-0xfffe) last 2byte of 64bit MAC address is in default.
	  @param[out]     0: OK, -1:error (my_addr == 0xFFFF) 0xFFFF is reserved for broadcast
	 ******************************************************************************/
	extern "C" int lazurite_setMyAddress(uint16_t my_addr)
	{
		int result;
		int errcode=0;
		result = ioctl(fp,IOCTL_PARAM | IOCTL_SET_MY_SHORT_ADDR,my_addr), errcode--;
		if(result != 0) return errcode;
		return 0;
	}

	/******************************************************************************/
	/*! @brief send data via 920MHz
	  @param[in]     *payload     data 
	  @param[in]      size        data of length
	  @return         0=success=0 <br> -ENODEV = ACK Fail <br> -EBUSY = CCA Fail
	  @exception      none
	 ******************************************************************************/
	extern "C" int lazurite_write(const char* payload, uint16_t size)
	{
		int result;
		result = write(fp,payload,size);
		return result;
	}

	/******************************************************************************/
	/*! @brief decoding mac header for external function
	  @param[out]     *mac    result of decoding raw
	  @param[in]      *raw    raw data of ieee802154
	  @param[in]      raw_len length of raw
	  @return         length of raw data
	  @exception      none
	 ******************************************************************************/
	extern "C" int lazurite_decMac(SUBGHZ_MAC* mac,void* raw,uint16_t raw_size){
		int result;
		uint16_t tmp_size;
		SUBGHZ_MAC_PARAM param;
		subghz_decMac(&param,raw,raw_size);
		//lazurite_getRxRssi(&mac->rssi);
		//lazurite_getRxTime(&mac->tv_sec,&mac->tv_nsec);
		mac->header = param.mac_header.header;
		mac->frame_type = param.mac_header.alignment.frame_type;
		mac->sec_enb = param.mac_header.alignment.sec_enb;
		mac->pending = param.mac_header.alignment.pending;
		mac->ack_req = param.mac_header.alignment.ack_req;
		mac->panid_comp = param.mac_header.alignment.panid_comp;
		mac->seq_comp = param.mac_header.alignment.seq_comp;
		mac->ielist = param.mac_header.alignment.ielist;
		mac->src_addr_type = param.mac_header.alignment.src_addr_type;
		mac->frame_ver = param.mac_header.alignment.frame_ver;
		mac->dst_addr_type = param.mac_header.alignment.dst_addr_type;
		mac->seq_num = param.seq_num;
		mac->addr_type = param.addr_type;
		mac->dst_panid = param.dst_panid;
		memcpy(mac->dst_addr,param.dst_addr,8);
		mac->src_panid = param.src_panid;
		memcpy(mac->src_addr,param.src_addr,8);
		mac->payload_offset = param.raw_len - param.payload_len;
		mac->payload_len = param.payload_len;
		return param.raw_len;
	}

	/******************************************************************************/
	/*! @brief get size of receiving data
	  @param      none
	  @return     length of receiving packet
	  @exception  none
	 ******************************************************************************/
	extern "C" int lazurite_available(void)
	{
		int result;
		short tmp_size;
		result=read(fp,&tmp_size,2);
		if(result != 0) result = (int)tmp_size;
		return result;
	}

	/******************************************************************************/
	/*! @brief read raw data
	  @param[out]     *raw
	  pointer to write received packet data.<br>
	  255 byte should be reserved.
	  @param[out]     size
	  size of raw data
	  @return     length of receiving packet
	  @exception  none
	 ******************************************************************************/
	extern "C" int lazurite_read(void* raw, uint16_t* size){
		int result;
		uint16_t tmp_size;
		SUBGHZ_MAC_PARAM mac;

		result = read(fp,&tmp_size,2);
		if(result == 0){
			*size=0;
			return 0;
		}
		result=read(fp,buf,tmp_size);
		memcpy(raw,buf,tmp_size);
		*size = tmp_size;
		return tmp_size;
	}
	/******************************************************************************/
	/*! @brief read only payload. header is abandoned.
	  @param[out]     *payload    memory for payload to be written. need to reserve 250 byte in maximum.
	  @param[out]     *size       size of payload
	  @return         size of payload
	  @exception      none
	  @note           about lazurite_readPayload:
	  mac header is abandoned.
	 ******************************************************************************/
	extern "C" int lazurite_readPayload(char* payload, uint16_t* size)
	{
		int result;
		uint16_t tmp_size;
		SUBGHZ_MAC_PARAM mac;

		result = read(fp,&tmp_size,2);
		if(result == 0){
			*size=0;
			return 0;
		}
		result=read(fp,buf,tmp_size);
		subghz_decMac(&mac,buf,tmp_size);
		memcpy(payload,mac.payload,mac.payload_len);
		*size = mac.payload_len;
		return mac.payload_len;
	}
	/******************************************************************************/
	/*! @brief read payload from linked address
	  @param[out]     *payload   pointer of payload
	  @param[out]     *size       length of payload
	  @return         size
	  @exception      none
	  @note  About linkedAddress mode:
	  The size is length of receiving packet in kernel driver.
	  When tx address is wrong in linked address mode, lazurite_readPayload or lazurite_read return 0.
	  mac header is abandoned in this mode.
	 ******************************************************************************/
	extern "C" int lazurite_readLink(char* payload, uint16_t* size)
	{
		int result;
		uint16_t tmp_size;
		int i;
		SUBGHZ_MAC_PARAM mac;
		for (i=0;i<16;i++) {
			result = read(fp,&tmp_size,2);
			if(result <= 0){
				*size=0;
				break;
			}
			result=read(fp,buf,tmp_size);
			subghz_decMac(&mac,buf,tmp_size);
			uint16_t *src_addr = (uint16_t*)mac.src_addr;
			if ((*src_addr == linkedAddr) || (linkedAddr == 0xFFFF))
			{
				*size = mac.payload_len;
				result = mac.payload_len;
				mac.payload[result]=0;
				memcpy(payload,mac.payload,*size);
				break;
			}
			else
			{
				continue;
			}
		}
		return result;
	}
	/******************************************************************************/
	/*! @brief get Receiving time
	  @param[out]     *tv_sec     32bit linux time data
	  @param[out]     *tv_nsec    32bit nsec time
	  @return         0=success <br> 0 < fail
	  @exception      none
	 ******************************************************************************/
	extern "C" int lazurite_getRxTime(time_t* tv_sec,long* tv_nsec)
	{
		time_t sec;
		long nsec;
		int result;
		int errcode=0;

		result = ioctl(fp,IOCTL_PARAM | IOCTL_GET_RX_SEC1,0),errcode--;
		if(result < 0) return errcode;
		sec = result;

		result = ioctl(fp,IOCTL_PARAM | IOCTL_GET_RX_SEC0,0),errcode--;
		if(result < 0) return errcode;
		sec = (sec << 16) + result;

		result = ioctl(fp,IOCTL_PARAM | IOCTL_GET_RX_NSEC1,0),errcode--;
		if(result < 0) return errcode;
		nsec = result;

		result = ioctl(fp,IOCTL_PARAM | IOCTL_GET_RX_NSEC0,0),errcode--;
		if(result < 0) return errcode;
		nsec = (nsec << 16) + result;

		*tv_sec = sec;
		*tv_nsec = nsec;

		return 0;
	}
	/******************************************************************************/
	/*! @brief get RSSI of last receiving packet
	  @param[out]     *rssi   value of RSSI.  0-255. 255 is in maxim
	  @return         0 > rssi <br> 0 < fail
	  @exception      none
	 ******************************************************************************/
	extern "C" int lazurite_getRxRssi(void)
	{
		int result;
		int errcode;
		result = ioctl(fp,IOCTL_PARAM | IOCTL_GET_RX_RSSI,0),errcode--;
		if(result < 0) return errcode;

		return result;
	}
	/******************************************************************************/
	/*! @brief get RSSI of ack in last tx packet
	  @param[out]     *rssi   value of RSSI.  0-255. 255 is in maxim
	  @return         Success=0, Fail<0
	  @return         0 > rssi <br> 0 < fail
	  @exception      none
	 ******************************************************************************/
	extern "C" int lazurite_getTxRssi(void)
	{
		int result;
		int errcode=0;
		result = ioctl(fp,IOCTL_PARAM | IOCTL_GET_TX_RSSI,0),errcode--;
		if(result < 0) return errcode;
		return result;
	}

	/******************************************************************************/
	/*! @brief get address type
	  @param[in]     none 
	  @return         address type
	  type | rx_addr | src_addr | panid_comp | rx panid | src_panid
	  -----| --------| --------| ---------- | -------- | --------
	  0 | N | N | 0 | N | N
	  1 | N | N | 1 | Y | N
	  2 | N | Y | 0 | N | Y
	  3 | N | Y | 1 | N | N
	  4 | Y | N | 0 | Y | N
	  5 | Y | N | 1 | N | N
	  6 | Y | Y | 0 | Y | N
	  7 | Y | Y | 1 | N | N
	  @exception      none
	 ******************************************************************************/
	extern "C" int lazurite_getAddrType(void)
	{
		int result;
		int errcode=0;

		result = ioctl(fp,IOCTL_CMD | IOCTL_GET_SEND_MODE,0), errcode--;
		if(result != 0) return errcode;

		result = ioctl(fp,IOCTL_PARAM | IOCTL_GET_ADDR_TYPE,0), errcode--;
		if(result < 0 ) return errcode;

		return result;
	}

	/******************************************************************************/
	/*! @brief set address type
	  @param[in]      mac address type to send
	  @return         0=success <br> 0 < fail
	  @exception      none
	 ******************************************************************************/
	extern "C" int lazurite_setAddrType(uint8_t addr_type)
	{
		int result;
		int errcode=0;

		result = ioctl(fp,IOCTL_CMD | IOCTL_GET_SEND_MODE,0), errcode--;
		if(result != 0) return errcode;

		result = ioctl(fp,IOCTL_PARAM | IOCTL_SET_ADDR_TYPE,addr_type), errcode--;
		if(result != addr_type) return errcode;

		result = ioctl(fp,IOCTL_CMD | IOCTL_SET_SEND_MODE,0), errcode--;
		if(result != 0) return errcode;

		return 0;
	}

	/******************************************************************************/
	/*! @brief get CCA cycle Time
	  @param[in]     none 
	  @return         0 > senseTime <br> 0 < fail
	  @exception      none
	 ******************************************************************************/
	extern "C" int lazurite_getSenseTime(void)
	{
		int result;
		int errcode=0;

		result = ioctl(fp,IOCTL_CMD | IOCTL_GET_SEND_MODE,0), errcode--;
		if(result != 0) return errcode;

		result = ioctl(fp,IOCTL_PARAM | IOCTL_GET_SENSE_TIME,0), errcode--;
		if(result < 0 ) return errcode;

		return result;
	}

	/******************************************************************************/
	/*! @brief set cycle of CCA
	  @param[in]      CCA cycle 0-255(20 in default)
	  @return         0=success <br> 0 < fail
	  @exception      none
	 ******************************************************************************/
	extern "C" int lazurite_setSenseTime(uint8_t senseTime)
	{
		int result;
		int errcode=0;

		result = ioctl(fp,IOCTL_CMD | IOCTL_GET_SEND_MODE,0), errcode--;
		if(result != 0) return errcode;

		result = ioctl(fp,IOCTL_PARAM | IOCTL_SET_SENSE_TIME,senseTime), errcode--;
		if(result != senseTime) return errcode;

		result = ioctl(fp,IOCTL_CMD | IOCTL_SET_SEND_MODE,0), errcode--;
		if(result != 0) return errcode;

		return 0;
	}

	/******************************************************************************/
	/*! @brief get cycle time to resend, when tx is failed.
	  @param[in]      none
	  @return         0 > txretry <br> 0 < fail
	  @exception      none
	 ******************************************************************************/
	extern "C" int lazurite_getTxRetry(void)
	{
		int result;
		int errcode=0;

		result = ioctl(fp,IOCTL_CMD | IOCTL_GET_SEND_MODE,0), errcode--;
		if(result != 0) return errcode;

		result = ioctl(fp,IOCTL_PARAM | IOCTL_GET_TX_RETRY,0), errcode--;
		if(result < 0 ) return errcode;

		return result;
	}

	/******************************************************************************/
	/*! @brief set cycle to resend, when Tx is failed.
	  @param[in]      retry cycle 0-255(3 in default)
	  @return         0=success <br> 0 < fail
	  @exception      none
	 ******************************************************************************/
	extern "C" int lazurite_setTxRetry(uint8_t retry)
	{
		int result;
		int errcode=0;

		result = ioctl(fp,IOCTL_CMD | IOCTL_GET_SEND_MODE,0), errcode--;
		if(result != 0) return errcode;

		result = ioctl(fp,IOCTL_PARAM | IOCTL_SET_TX_RETRY,retry), errcode--;
		if(result != retry) return errcode;

		result = ioctl(fp,IOCTL_CMD | IOCTL_SET_SEND_MODE,0), errcode--;
		if(result != 0) return errcode;

		return 0;
	}

	/******************************************************************************/
	/*! @brief set tx interval until resend, when Tx is failed.
	  @param[in]      none
	  @return         txinterval(ms) >0 <br> 0 < fail
	  @exception      none
	 ******************************************************************************/
	extern "C" int lazurite_getTxInterval(void)
	{
		int result;
		int errcode=0;

		result = ioctl(fp,IOCTL_CMD | IOCTL_GET_SEND_MODE,0), errcode--;
		if(result != 0) return errcode;

		result = ioctl(fp,IOCTL_PARAM | IOCTL_GET_TX_INTERVAL,0), errcode--;
		if(result < 0 ) return errcode;

		return result;
	}

	/******************************************************************************/
	/*! @brief set interval to resend, when tx is failed.
	  @param[in]      txinterval 0(0ms) - 500(500ms), 500 in default
	  @return         0=success <br> 0 < fail
	  @exception      none
	 ******************************************************************************/
	extern "C" int lazurite_setTxInterval(uint16_t txinterval)
	{
		int result;
		int errcode=0;

		result = ioctl(fp,IOCTL_CMD | IOCTL_GET_SEND_MODE,0), errcode--;
		if(result != 0) return errcode;

		result = ioctl(fp,IOCTL_PARAM | IOCTL_SET_TX_INTERVAL,txinterval), errcode--;
		if(result != txinterval) return errcode;

		result = ioctl(fp,IOCTL_CMD | IOCTL_SET_SEND_MODE,0), errcode--;
		if(result != 0) return errcode;

		return 0;
	}

	/******************************************************************************/
	/*! @brief get backoff time
	  @param[in]      none
	  @return         0 > backoff time <br> 0 < fail <br>
	  backoff time = 320us * 2^cca_wait
	  @exception      none
	 ******************************************************************************/
	extern "C" int lazurite_getCcaWait(void)
	{
		int result;
		int errcode=0;

		result = ioctl(fp,IOCTL_CMD | IOCTL_GET_SEND_MODE,0), errcode--;
		if(result != 0) return errcode;

		result = ioctl(fp,IOCTL_PARAM | IOCTL_GET_TX_INTERVAL,0), errcode--;
		if(result < 0 ) return errcode;

		return result;
	}

	/******************************************************************************/
	/*! @brief set cca backoff time
	  @param[in]      ccawait (0 - 7), 7 in default <br>
	  backoff time = 320us * 2^cca_wait
	  @return         0=success <br> 0 < fail
	  @exception      none
	 ******************************************************************************/
	extern "C" int lazurite_setCcaWait(uint8_t ccawait)
	{
		int result;
		int errcode=0;

		result = ioctl(fp,IOCTL_CMD | IOCTL_GET_SEND_MODE,0), errcode--;
		if(result != 0) return errcode;

		result = ioctl(fp,IOCTL_PARAM | IOCTL_SET_CCA_WAIT,ccawait), errcode--;
		if(result != ccawait) return errcode;

		result = ioctl(fp,IOCTL_CMD | IOCTL_SET_SEND_MODE,0), errcode--;
		if(result != 0) return errcode;

		return 0;
	}

	/******************************************************************************/
	/*! @brief set promiscuous mode
	  @param[in]     true : promiscuous mode, false: normal mode
	  @exception      none
	 ******************************************************************************/
	extern "C" int lazurite_setPromiscuous(bool on)
	{
		int result;
		int errcode=0;
		result = ioctl(fp,IOCTL_PARAM | IOCTL_SET_PROMISCUOUS,on), errcode--;
		if(result != 0) return errcode;
		return 0;
	}

	/******************************************************************************/
	/*! @brief set ack request
	  @param[in]     true : ack requested, false: force to set non-ack
	  @exception      none
	 ******************************************************************************/
	extern "C" int lazurite_setAckReq(bool on)
	{
		int result;
		int errcode=0;
		result = ioctl(fp,IOCTL_PARAM | IOCTL_SET_ACK_REQ,on), errcode--;
		if(result != 0) return errcode;
		return 0;
	}

#ifdef __cplusplus
};
#endif



