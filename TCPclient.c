/*
 * Function version
 * 
 * 1.�b Windows �ϥ� Socket �ݭn link Winsock Library�C[client��]
 *   link�覡�G
 *   �M�ת��u�ݩʡv->�u�պA�ݩʡv->�u�s�����v->�u��J�v->�u��L�̩ۨʡv�[�J wsock32.lib ��  Ws2_32.lib
 *   �]�i�H�b�{�����A�ϥΥH�U�覡�[�J
 *   #pragma comment(lib, "wsock32.lib") �� #pragma comment(lib, "Ws2_32.lib")
 * 
 * 2.wsock32.lib �M Ws2_32.lib ���ϧO�G
 *   wsock32.lib �O���ª� 1.1 �����AWs2_32.lib �O���s�� 2.0 �����C
 *   wsock32.lib �� winsock.h �@�_�ϥΡAWs2_32.lib �� WinSock2.h �@�_�ϥΡC
 *   winsock.h �M WinSock2.h ����P�ɨϥΡAWinSock2.h �O�ΨӨ��N winsock.h�A�Ӥ��O�X�i winsock.h�C 
 * ================================20160428==================================================  
 * 3. Max. 1460 bytes
 * 4. Add package_struct, and float,int,.etc array in structure can send and receive correctly
 *
 *
 */

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define TX_BUFLEN 900	//max 1460 but "Aurix:  " occupi 8 bytes
#define RX_BUFLEN 910
//#define DEFAULT_BUFLEN 50
#define SERVER_PORT "40050"
#define SERVER_IP "192.168.0.21"

struct package_struct
{
	int TOFarray[225];	
		
};

/********* Gloabal Variables *********/
WSADATA wsaData;
SOCKET ConnectSocket = INVALID_SOCKET;
struct addrinfo *result = NULL,
                *ptr = NULL,
                hints;
//char *sendbuf ;
char sendbuf[TX_BUFLEN];	
char recvbuf[RX_BUFLEN];
int iResult;
int recvbuflen = RX_BUFLEN;
/********* Gloabal Variables *********/

void TCPclientInit(void){
	
	memset(recvbuf,0,sizeof(recvbuf));
	memset(sendbuf,0,sizeof(sendbuf));

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
		system("pause");
        return 2;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;	
	
}


int main(void) 
{
	struct package_struct pkg_tx,pkg_rx; 
		
	int i;
     
	srand(time(NULL));
    	
while(1){
    // Resolve the server address and port
    iResult = getaddrinfo(SERVER_IP, SERVER_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
		system("pause");
        return 3;
    }

	

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
			system("pause");
            return 4;
        }

        // Connect to server.
        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server! %s:%s\n ",SERVER_IP,SERVER_PORT);
        WSACleanup();
		system("pause");
        return 5;
    }

	//random
	for(i=0;i<225;i++){
		
		//sendbuf[i]=(rand()%10)+0x30;	//rand() % (�̤j��-�̤p��+1) ) + �̤p��
		pkg_tx.TOFarray[i]=i*i;	
		//printf("%d ",pkg_tx.TOFarray[i]);
	
	}//for 
	
	memcpy(sendbuf,&pkg_tx,sizeof(pkg_tx));
	/*	
	for(i=0;i<TX_BUFLEN;i++){
		if(i%4 == 0)
			printf("num=%2d \n",i);
		printf("%02X ", sendbuf[i]);
	}
	printf("\n");
	*/
	
    // Send an initial buffer
    iResult = send( ConnectSocket, sendbuf, (int)sizeof(sendbuf), 0 );
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
		system("pause");
        return 6;
    }

    printf("Bytes sent: %ld size:%d \n", iResult,sizeof(sendbuf));


    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 7;
    }

    // Receive until the peer closes the connection
    do {

        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if ( iResult > 0 ){
			
			//Verify
			int diff_bytes=0;
			
			for(i=0;i<RX_BUFLEN;i++){
				recvbuf[i]=recvbuf[i+8];//shift "aurix: " out
				if( (recvbuf[i]!=sendbuf[i]) && (i<TX_BUFLEN) )	diff_bytes++;		
			}//for 
			
			printf("Bytes received: %d diff:%d \n", iResult,diff_bytes);
			printf("data:");
			/*
			for(i=0;i<RX_BUFLEN;i++){
				if(i%4 == 0)
					printf("num=%2d \n",i);
				printf("%02X ", recvbuf[i]);
			}
			printf("\n");
			*/
			memcpy(&pkg_rx,recvbuf,sizeof(pkg_rx));
	
			diff_bytes=0;
			for(i=0;i<225;i++){
				if(pkg_rx.TOFarray[i] != pkg_tx.TOFarray[i]) diff_bytes++;
				printf("%.4f ",pkg_rx.TOFarray[i]);
			}//for 
			printf("\nfloat diff:%d \n", diff_bytes);
	
		
		}//iResult > 0
        else if ( iResult == 0 )
            printf("Connection closed\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

    } while( iResult > 0 );

    // cleanup
    closesocket(ConnectSocket);
	
	
	system("pause");
}//while    

	WSACleanup();

	system("pause");
    return 0;
}
