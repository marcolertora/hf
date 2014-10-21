/*
******************************
*Company:Lumitek
*Data:2014-10-07
*Author:Meiyusong
******************************
*/

#include "../inc/lumitekConfig.h"

#ifdef CONFIG_LUMITEK_DEVICE
#include <hsf.h>
#include <string.h>
#include <stdio.h>

#include "../inc/itoCommon.h"
#include "../inc/asyncMessage.h"



//define data



//global data
static int g_udp_socket_fd = -1;
static hfthread_mutex_t g_udp_socket_mutex;


int USER_FUNC getUdpSocketFd(void)
{
	return g_udp_socket_fd;
}



static BOOL initUdpSockrtMutex(void)
{
	BOOL ret = TRUE;
	if((hfthread_mutext_new(&g_udp_socket_mutex)!= HF_SUCCESS))
	{
		HF_Debug(DEBUG_ERROR, "failed to create socketMutex");
		ret = FALSE;
	}
	return ret;
}


static void USER_FUNC udpCreateSocketAddr(struct sockaddr_in* addr, U32 socketIp)
{
	memset(addr, 0,  sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(UDP_SOCKET_PORT);
	addr->sin_addr.s_addr = socketIp;
}



static void USER_FUNC udpSocketInit(void)
{
	struct sockaddr_in socketAddr;
	U32 socketIp;

	while (1)
	{
		g_udp_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
		if (g_udp_socket_fd < 0)
		{
			msleep(1000);
			continue;
		}
		else
		{
			break;
		}
	}
	socketIp = htonl(INADDR_ANY);
	udpCreateSocketAddr(&socketAddr, socketIp);
	bind(g_udp_socket_fd, (struct sockaddr*)&socketAddr, sizeof(socketAddr));
	hfnet_set_udp_broadcast_port_valid(UDP_SOCKET_PORT-1, UDP_SOCKET_PORT);  //SDK Must used!
	u_printf("meiyusong===> g_udp_socket_fd = %d \n", g_udp_socket_fd);
}



static U8 USER_FUNC udpSockSelect(struct timeval* pTimeout)
{
	fd_set fdR;
	S32 ret;
	U8 sel= 0;

	FD_ZERO(&fdR);
	if (g_udp_socket_fd != -1)
	{
		FD_SET(g_udp_socket_fd,&fdR);
	}
	ret= select(g_udp_socket_fd+1,&fdR,NULL,NULL, pTimeout);
	if (ret<= 0)
	{
		return 0;
	}
	else if (FD_ISSET(g_udp_socket_fd, &fdR))
	{
		sel = 0x01;
	}
	return sel;
}



static S32 USER_FUNC udpSocketRecvData( S8 *buffer, S32 bufferLen, S32 socketFd, struct sockaddr_in *rm_add)
{
	S32 recvCount;
	U32 fromLen = sizeof(struct sockaddr);

	hfthread_mutext_lock(g_udp_socket_mutex);
	recvCount = recvfrom(socketFd, buffer, bufferLen, 0, (struct sockaddr *)rm_add, &fromLen);
	hfthread_mutext_unlock(g_udp_socket_mutex);
	//u_printf("meiyusong===> udpSocketRecvData:count=%d port=%d, ip=%X fromLen=%d\n", recvCount, rmaddr.sin_port, rmaddr.sin_addr.s_addr, fromLen);
	return recvCount;
}



static S32 USER_FUNC udp_send_data(U8 *SocketData, S32 bufferLen, S32 socketFd, struct sockaddr_in *tx_add)
{
	int sendCount;
    
	hfthread_mutext_lock(g_udp_socket_mutex);
	sendCount = sendto(socketFd, SocketData, bufferLen, 0, (struct sockaddr*)tx_add, sizeof(struct sockaddr));
	hfthread_mutext_unlock(g_udp_socket_mutex);
	return(sendCount);
}



static S8* USER_FUNC udpSocketGetData(U32* recvCount, struct sockaddr_in* pSocketAddr)
{
	S8* recvBuf;

	recvBuf = getUdpRecvBuf(TRUE);
	*recvCount= (U32)udpSocketRecvData(recvBuf, NETWORK_MAXRECV_LEN, g_udp_socket_fd, pSocketAddr);
	if(!checkRecvSocketData(*recvCount, recvBuf))
	{
		return NULL;
	}
	return recvBuf;
}



U32 USER_FUNC udpSocketSendData(U8* sendBuf, U32 dataLen, U32 socketIp)
{
	struct sockaddr_in socketAddr;


	udpCreateSocketAddr(&socketAddr, socketIp);
	return udp_send_data(sendBuf, (S32)dataLen, g_udp_socket_fd, &socketAddr);
}



void USER_FUNC deviceLocalUdpThread(void)
{
	U32 recvCount;
	S8* recvBuf;
	struct timeval timeout;
	struct sockaddr_in socketAddr;

	initUdpSockrtMutex();
	udpSocketInit();
	
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	memset(&socketAddr, 0, sizeof(struct sockaddr_in));
	
	hfthread_enable_softwatchdog(NULL, 30); //Start watchDog
	while(1)
	{

		//u_printf(" deviceLocalUdpThread \n");
		hfthread_reset_softwatchdog(NULL); //tick watchDog

		if(udpSockSelect(&timeout) > 0)
		{
			recvBuf = udpSocketGetData(&recvCount, &socketAddr);
			if(recvBuf != NULL)
			{
				insertSocketMsgToList(MSG_FROM_UDP, (U8*)recvBuf, recvCount, socketAddr.sin_addr.s_addr);
			}
		}
		msleep(100);
	}
}

#endif
