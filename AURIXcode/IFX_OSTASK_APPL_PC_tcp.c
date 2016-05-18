/*
 * IFX_OSTASK_APPL_PC.c
 *
 *  Created on: 17.12.2013
 *      Author: fs
 */

#pragma GCC optimize "O0"

#include "Ifx_Types.h"
#include "os.h"

/** includes
 *########################################################################*/

//#include "time.h"
//#include "math.h"


#include "err.h"
#include "lwIP.h"
#include "api.h"
#include "tcp.h"
#include "udp.h"
#include "etharp.h"
#include "Appl_Task.h"

#include "ledCtrl.h"
#include "memp.h"

#include "EthLldInit.h"
// FIXME 2014-01-20 HZ debug support not available with this HW
#include "debugPin.h"


/** defines
 *########################################################################*/

#define UDP_PACKET_TOTAL_LEN (UDP_HLEN + IP_HLEN + SIZEOF_ETH_HDR)
#define SEND_DATA_LEN   1472
#define	RECV_PACKET_LEN	14641	//Map data

/** externs
 *########################################################################*/

/** globals
 *########################################################################*/
struct CARINFO{
	
	int x,y;
	float angle;	
	
};
struct CARINFO CarInfo; 
static u8_t map[RECV_PACKET_LEN];
static uint32 rxByte,txLen,i;


static err_t error = 0;

static uint32 appl_pc_task_led_count;

static struct ip_addr appl_pc_local_ip_addr;
static struct ip_addr appl_pc_remote_ip_addr;

static struct netconn *appl_pc_netconn_tcp_handle_ptr;
static struct netconn *appl_pc_netconn_tcp_data_ptr;

static u8_t ethApplPcBuffer[1536];  //KV ETH LLD works with 1536 bytes
static u8_t *pEthApplPcBuffer;


static struct netbuf *pPcTxNetBuf;
static struct netbuf *pPcRxNetBuf;
/*
struct netbuf {
  struct pbuf *p, *ptr;
  ip_addr_t addr;
  u16_t port;

struct pbuf {
  
  struct pbuf *next;	// next pbuf in singly linked pbuf chain 
   void *payload;		// pointer to the actual data in the buffer 
  
  //total length of this buffer and all next buffers in chain
  //belonging to the same packet.
  //For non-queue packet chains this is the invariant:
  //p->tot_len == p->len + (p->next? p->next->tot_len: 0)
  u16_t tot_len;

  u16_t len;			// length of this buffer 
  u8_t  type;//pbuf_type// pbuf_type as u8_t instead of enum to save space 
  u8_t flags;			// misc flags

  // the reference count always equals the number of pointers
  // that refer to this pbuf. This can be pointers from an application,
  // the stack itself, or pbuf->next pointers from a chain.
  u16_t ref;
};  
*/


//static uint32 ct3;
//static uint32 ctct3 = 0;

//#define SEND_DELAY 6000000


static uint8 aurixTxMod[8] = { 'A', 'u', 'r', 'i', 'x', ':', ' ', ' ' };

/** functions
 *########################################################################*/
void netconn_callback_tcp(struct netconn *, enum netconn_evt, u16_t len);

static uint32 locPcPort;
static uint32 remPcPort;
static uint32 rxFrameLen;
				
TASK(IFX_OSTASK_APPL_PC_TCP) {


	//srand(time(NULL));
	//TerminateTask();
	err_t err;

	//KV this code runs only the first time when Task_Appl is called

#if (BOARD==BOARD_1)
	appl_pc_local_ip_addr.addr = 0x1500A8C0UL;  //KV 192.168.0.21 = BOARD 1
	appl_pc_remote_ip_addr.addr = 0x1400A8C0UL;//KV 192,168.0.20 = PC
	locPcPort = 40050;
	remPcPort = 50001;//no use
#elif (BOARD==BOARD_2)
	appl_pc_local_ip_addr.addr = 0x1600A8C0UL;  //KV 192,168.0.22 = BOARD 2
	appl_pc_remote_ip_addr.addr = 0x1400A8C0UL;  //KV 192,168.0.20 = PC
	locPcPort = 40051;
	remPcPort = 50002;
#elif (BOARD==BOARD_3)
	appl_pc_local_ip_addr.addr = 0x1700A8C0UL;  //KV 192,168.0.22 = BOARD 2
	appl_pc_remote_ip_addr.addr = 0x1400A8C0UL;  //KV 192,168.0.20 = PC
	locPcPort = 40052;
	remPcPort = 50003;
#elif (BOARD==BOARD_4)
	appl_pc_local_ip_addr.addr = 0x1800A8C0UL;  //KV 192,168.0.22 = BOARD 2
	appl_pc_remote_ip_addr.addr = 0x1400A8C0UL;  //KV 192,168.0.20 = PC
	locPcPort = 40053;
	remPcPort = 50004;
#endif

	//KV create new netconn connection

	// FUCHS tcp:
	// FUCHS callback:
	//appl_pc_netconn_tcp_handle_ptr = netconn_new(NETCONN_TCP);
	appl_pc_netconn_tcp_handle_ptr = netconn_new_with_callback(NETCONN_TCP, netconn_callback_tcp);
	if (NULL == appl_pc_netconn_tcp_handle_ptr) {
		__asm__ volatile ("debug");
	}

	appl_pc_netconn_tcp_handle_ptr->flags = 0;

	//KV bind to the local ip_addr and port
	// FUCHS tcp:
	error = netconn_bind(appl_pc_netconn_tcp_handle_ptr, IP_ADDR_ANY,locPcPort);
	if (ERR_OK != error) {
		__asm__ volatile ("debug");
	}

	/* Tell connection to go into listening mode. */
	// FUCHS tcp:
	netconn_listen(appl_pc_netconn_tcp_handle_ptr);

	/* allocate a netbuf structure			*/
	pPcTxNetBuf = netbuf_new();
	if (NULL == pPcTxNetBuf) {
		__asm__ volatile ("debug");
	}

	//KV very important function JS: Indeed (...)
	/* FUCHS: allocate a pbuf structure, pointer to payload and frame length is set in 'echo server'	*/
	error = netbuf_ref(pPcTxNetBuf, 0, 0);
	if (ERR_OK != error) {
		__asm__ volatile ("debug");
	}

	//JS important: skip UDP and IP header length, otherwise we'll overwrite memory.
	pEthApplPcBuffer = ethApplPcBuffer + UDP_PACKET_TOTAL_LEN;

	//JS fill xBuf with bogus text
	// strcpy(pEthApplPcBuffer, "Hallo Welt, ich bin ein schoener Text der diesen Buffer hervorragend fuellt");
	
	CarInfo.angle=1;
	txLen=sizeof(CarInfo);
	pPcTxNetBuf->p->len = pPcTxNetBuf->p->tot_len = txLen;
	
	/* Grab new connection. */
	// FUCHS tcp:
	while (1) {
		err = netconn_accept(appl_pc_netconn_tcp_handle_ptr,&appl_pc_netconn_tcp_data_ptr);
		if (err != ERR_OK) {
			__asm__ volatile ("debug");
		}

		// toggle LED for heartbeat
		appl_pc_task_led_count++;

		// FUCHS tcp:
		while (ERR_OK== netconn_recv(appl_pc_netconn_tcp_data_ptr,&pPcRxNetBuf))
		{
			memcpy(map+rxFrameLen, pPcRxNetBuf->p->payload, pPcRxNetBuf->p->len);
			rxFrameLen += pPcRxNetBuf->p->len;
			netbuf_delete(pPcRxNetBuf);
										
			if (rxFrameLen >= RECV_PACKET_LEN) {
												
				CarInfo.x++;
				CarInfo.y = rxFrameLen;
				CarInfo.angle *=1.01;
												
				memcpy(pEthApplPcBuffer, &CarInfo, txLen);
				pPcTxNetBuf->p->payload = pEthApplPcBuffer;
				
				// FUCHS tcp:
				netconn_write(appl_pc_netconn_tcp_data_ptr, pPcTxNetBuf->p->payload, pPcTxNetBuf->p->len, NETCONN_COPY);
				//for(i=0;i<rxFrameLen;i++)
				//	RS232_PutCh(map[i]);	
								
				rxFrameLen= 0;
					
			}//if -> check receive whole packet 
								
		}	
			
		netconn_close(appl_pc_netconn_tcp_data_ptr);
		//netbuf_delete(pPcRxNetBuf);
		netconn_delete(appl_pc_netconn_tcp_data_ptr);
			
	}//while loop

	//netconn_disconnect(appl_pc_netconn_tcp_handle_ptr);
}

// FUCHS callback: Receive callback function
void netconn_callback_tcp(struct netconn *netc, enum netconn_evt evt, u16_t len) {
	if (evt == NETCONN_EVT_RCVPLUS) {
	}
	if (evt == NETCONN_EVT_RCVMINUS) {
	}
	if (evt == NETCONN_EVT_SENDPLUS) {
	}
}

/** end of module Appl_Task
 *########################################################################*/
