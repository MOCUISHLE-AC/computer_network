#pragma warning(disable:4996)
#include <winsock2.h> // winsock2的头文件
#include <WS2tcpip.h>
#include <iostream>
#pragma comment(lib, "ws2_32.lib")
#define max_num 10 //Client最大数量
#define port 6666 //默认端口号
using namespace std;

int total=0;//Client的数量
SOCKET cliSock[max_num] = {0};//客户端套接字 
SOCKADDR_IN cliAddr[max_num] = {0};//客户端地址
char cliname[max_num][20] = {0};

// stdcall的线程处理函数
DWORD WINAPI ThreadFun(LPVOID lpThreadParameter);
DWORD WINAPI WacthThread(LPVOID lpThreadParameter) {
	while (true) {
		char order[20] = { 0 };//服务器的命令
		cin.getline(order, 20);
		if (strcmp(order, "sudo ls") == 0) {
			if (total == 0) {
				cout << "聊天室人数：" << total << endl << endl;
			}
			else {
				cout << "聊天室人数：" << total << endl;
				for (int i = 0; i < total; i++) {
					cout << cliname[i] <<"\tsocket:"<< cliSock[i]<<endl;
				}
				cout << endl;
			}
		}
	}
	return 0;
}
int main()
{
	WSADATA wd;
	if (WSAStartup(MAKEWORD(2, 2), &wd) != 0)
	{
		cout << "WSAStartup Error:" << WSAGetLastError() << endl;
		return 0;
	}

	//创建流式套接字
	SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_socket == INVALID_SOCKET)
	{
		cout << "socket error:" << WSAGetLastError() << endl;
		return 0;
	}

	//绑定端口和ip
	sockaddr_in addr;
	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	cout << "默认端口号：" << port << endl;
	cout << "默认IP地址：" << "127.0.0.1" << endl;

	int len = sizeof(sockaddr_in);
	if (bind(server_socket, (SOCKADDR*)&addr, len) == SOCKET_ERROR)
	{
		cout << "bind Error:" << WSAGetLastError() << endl;
		return 0;
	}
	cout << "server建立成功" << endl << endl;
	//监听
	listen(server_socket, max_num);

	//命令行线程
	HANDLE Recv_thread = CreateThread(NULL, 0, WacthThread, (LPVOID)server_socket, 0, NULL);
	// 主线程循环接收客户端的连接
	while (true)
	{
		sockaddr_in addrClient;
		len = sizeof(sockaddr_in);
		// 接受成功返回与client通讯的Socket
		SOCKET accept_socket = accept(server_socket, (SOCKADDR*)&addrClient, &len);
		if (accept_socket != INVALID_SOCKET)
		{
			cliSock[total] = accept_socket;
			cliAddr[total] = addrClient;
			// 创建线程，并且传入与client通讯的套接字
			HANDLE Recv_thread = CreateThread(NULL, 0, ThreadFun, (LPVOID)accept_socket, 0, NULL);
			CloseHandle(Recv_thread); // 关闭对线程的引用
		}
	}

	// 6.关闭监听套接字
	closesocket(server_socket);

	// 清理winsock2的环境
	WSACleanup();

	return 0;
}

DWORD WINAPI ThreadFun(LPVOID lpThreadParameter)
{
	//与客户端通讯，发送或者接受数据
	total++;  
	SOCKET c = (SOCKET)lpThreadParameter;
	char temp[20];
	recv(c, temp, 20, 0);
	int index=-1;
	for (int i = 0; i < total; i++) {
		if (c == cliSock[i]) {
			index = i;
			break;
		}
	}
	
	cout << "欢迎" << temp << "进入聊天室！" <<endl;

	strcpy(cliname[index], temp);
	// 发送数据
	char welcome[100] = { 0 };
	sprintf(welcome, "欢迎 %s 进入聊天室！", cliname[index]);
	send(c, welcome, 100, 0);

	// 循环接收客户端数据
	int ret = 0;
	do
	{
		//接受Client发送的消息
		char receive[100] = { 0 };
		ret = recv(c, receive, 100, 0);
		char spread[100] = { 0 };
		//某些Client线程推出后，需要重新定位
		int index = -1;
		for (int i = 0; i < total; i++) {
			if (c == cliSock[i]) {
				index = i;
				break;
			}
		}
		sprintf(spread, "\n%s说：", cliname[index]);
		strcat(spread, receive);
		if (ret != 0 && ret != SOCKET_ERROR)
		{
			cout << spread;
			for (int i = 0; i < total; i++) {
				if (c != cliSock[i]) {
					send(cliSock[i], spread, 100, 0);
				}
			}
		}
		else {
			sprintf(spread, "\n%s退出了聊天!!!\n", cliname[index]);
			cout << endl << cliname[index] << "离开了聊天室!!!" << endl;
			for (int i = 0; i < total; i++) {
				if (c != cliSock[i]) {
					send(cliSock[i], spread, 100, 0);
				}
			}
		}
	} while (ret != SOCKET_ERROR && ret != 0);
	//某些Client线程推出后，需要重新定位
	index = -1;
	for (int i = 0; i < total; i++) {
		if (c == cliSock[i]) {
			index = i;
			break;
		}
	}
	//退出线程前，将Client数组向前移动
	for (int i = index; i < total; i++) {
		cliSock[i] = cliSock[i + 1];
		cliAddr[i] = cliAddr[i + 1];
		strcpy(cliname[i], cliname[i + 1]);
	}
	total--;
	return 0;
}
