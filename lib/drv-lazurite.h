/*
 * IEEE802.15.4e driver IO
 * 
 * File:  drv-802154e.c
 * 
 * Copyright 2015 Lapis Semiconductor Co.,Ltd.
 * Author: Naotaka Saito
 * 
 * The program is based on BP3596 driver by Communication Technology
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef _DRV_802154E_H_
#define _DRV_802154E_H_

#define	DRV_OK					0


// ioctl command list
#define	IOCTL_CMD				0x0000
#define	IOCTL_SET_BEGIN			0x11
#define	IOCTL_SET_RXON			0x13
#define	IOCTL_SET_RXOFF			0x15
#define	IOCTL_SET_CLOSE			0x17
#define	IOCTL_GET_SEND_MODE		0x18
#define	IOCTL_SET_SEND_MODE		0x19
#define	IOCTL_PARAM				0x1000
#define	IOCTL_GET_CH			0x02
#define	IOCTL_SET_CH			0x03
#define	IOCTL_GET_PWR			0x04
#define	IOCTL_SET_PWR			0x05
#define	IOCTL_GET_BPS			0x06
#define	IOCTL_SET_BPS			0x07
#define	IOCTL_GET_MY_PANID		0x08
#define	IOCTL_SET_MY_PANID		0x09
#define	IOCTL_GET_RX_PANID		0x0a
#define	IOCTL_SET_RX_PANID		0x0b
#define	IOCTL_GET_MY_ADDR0		0x0c
#define	IOCTL_SET_MY_ADDR0		0x0d
#define	IOCTL_GET_MY_ADDR1		0x0e
#define	IOCTL_SET_MY_ADDR1		0x0f
#define	IOCTL_GET_MY_ADDR2		0x10
#define	IOCTL_SET_MY_ADDR2		0x11
#define	IOCTL_GET_MY_ADDR3		0x12
#define	IOCTL_SET_MY_ADDR3		0x13
#define	IOCTL_GET_RX_ADDR0		0x14
#define	IOCTL_SET_RX_ADDR0		0x15
#define	IOCTL_GET_RX_ADDR1		0x16
#define	IOCTL_SET_RX_ADDR1		0x17
#define	IOCTL_GET_RX_ADDR2		0x18
#define	IOCTL_SET_RX_ADDR2		0x19
#define	IOCTL_GET_RX_ADDR3		0x1a
#define	IOCTL_SET_RX_ADDR3		0x1b
#define	IOCTL_GET_ADDR_TYPE		0x1c
#define	IOCTL_SET_ADDR_TYPE		0x1d
#define	IOCTL_GET_DRV_MODE		0x20
#define	IOCTL_SET_DRV_MODE		0x21
#define	IOCTL_GET_SENSE_TIME	0x22
#define	IOCTL_SET_SENSE_TIME	0x23
#define	IOCTL_GET_TX_RETRY		0x24
#define	IOCTL_SET_TX_RETRY		0x25
#define	IOCTL_GET_TX_INTERVAL	0x26
#define	IOCTL_SET_TX_INTERVAL	0x27
#define	IOCTL_GET_CCA_WAIT		0x28
#define	IOCTL_SET_CCA_WAIT		0x29
#define	IOCTL_GET_RX_SEC0		0x2A
#define	IOCTL_GET_RX_SEC1		0x2C
#define	IOCTL_GET_RX_NSEC0		0x2E
#define	IOCTL_GET_RX_NSEC1		0x30
#define	IOCTL_GET_RX_RSSI		0x32
#define	IOCTL_GET_TX_RSSI		0x34
#define	IOCTL_RF				0x2000
#define	IOCTL_EEPROM			0x3000
#define	IOCTL_LED				0x4000

#endif // _DRV_802154E_H_

