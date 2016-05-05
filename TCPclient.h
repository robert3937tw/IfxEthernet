/*
 * Function version
 * 
 * 1.在 Windows 使用 Socket 需要 link Winsock Library。[client端]
 *   link方式：
 *   專案的「屬性」->「組態屬性」->「連結器」->「輸入」->「其他相依性」加入 wsock32.lib 或  Ws2_32.lib
 *   也可以在程式中，使用以下方式加入
 *   #pragma comment(lib, "wsock32.lib") 或 #pragma comment(lib, "Ws2_32.lib")
 * 
 * 2.wsock32.lib 和 Ws2_32.lib 的區別：
 *   wsock32.lib 是較舊的 1.1 版本，Ws2_32.lib 是較新的 2.0 版本。
 *   wsock32.lib 跟 winsock.h 一起使用，Ws2_32.lib 跟 WinSock2.h 一起使用。
 *   winsock.h 和 WinSock2.h 不能同時使用，WinSock2.h 是用來取代 winsock.h，而不是擴展 winsock.h。 
 * ================================20160428==================================================  
 * 3. Max. 1460 bytes
 * 4. Add package_struct, and float,int,.etc array in structure can send and receive correctly
 *
 *
 */



/********* Extern Variables *********/

/********* Extern Variables *********/

int TCPclientInit(void);
unsigned int TCPclientCommunication(unsigned char *sendbuf, int sendbufLen, unsigned char *recvdata);
int TCPtest(void);




