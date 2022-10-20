#pragma warning(disable:4996)
#include <winsock2.h>//winsock2的头文件
#include <iostream>
#include <windows.h>
#include <cstring>
using  namespace std;

//勿忘，链接dll的lib
#pragma comment(lib, "ws2_32.lib")

DWORD WINAPI recvMsgThread(LPVOID IpParameter);
int  main()
{

	//加载winsock2的环境
	WSADATA  wd;
	if (WSAStartup(MAKEWORD(2, 2), &wd) != 0)
	{
		cout << "WSAStartup  error：" << GetLastError() << endl;
		return 0;
	}

	//创建流式套接字
	SOCKET  s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
	{
		cout << "socket  error：" << GetLastError() << endl;
		return 0;
	}

	//链接服务器
	sockaddr_in   addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(6666);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	cout << "connecting..." << endl;
	int len = sizeof(sockaddr_in);
	if (connect(s, (SOCKADDR*)&addr, len) == SOCKET_ERROR)
	{
		cout << "connect  error：" << GetLastError() << endl;
		return 0;
	}
	cout << "输入用户名：";
	char name[20] = { 0 };
	cin.getline(name, 20);
	send(s, name, 20, 0);
	//接收服务端的消息
	char buf[100] = { 0 };
	recv(s, buf, 100, 0);
	cout << buf << endl;
	// 创建线程，并且传入与client通讯的套接字
	HANDLE hThread = CreateThread(NULL, 0, recvMsgThread, (LPVOID)s, 0, NULL);
	//随时给服务端发消息
	int  ret = 0;
	while (true)
	{
		//获取聊天内容
		char chat[100] = { 0 };
		cout << ">";
		cin.getline(chat, 100);
		if (strlen(chat) != 0)
		{
			char quit[15] = "sudo quit";
			if (strcmp(chat, quit) == 0)
			{
				cout << "退出成功！" << endl;
				break;
			}
			//获取系统时间
			char time[100] = { 0 };
			SYSTEMTIME sys;
			GetLocalTime(&sys);
			sprintf(time, "\n%4d/%02d/%02d %02d:%02d:%02d\n", sys.wYear, sys.wMonth, sys.wDay, sys.wHour, sys.wMinute, sys.wSecond);
			//拼接
			strcat(chat, time);
			send(s, chat, 100, 0);
		}
		else {
			cout << "输入不能为空！！！" << endl;
		}
	}
	CloseHandle(hThread); // 关闭对线程的引用
	//4.关闭监听套接字
	closesocket(s);

	//清理winsock2的环境
	WSACleanup();
	system("pause");
	return 0;
}
DWORD WINAPI recvMsgThread(LPVOID IpParameter)//接收消息的线程
{
	SOCKET cliSock = (SOCKET)IpParameter;//获取客户端的SOCKET参数
	int ret = 0;
	while (1)
	{
		if (ret != SOCKET_ERROR)
		{
			char buf[100] = { 0 };
			ret = recv(cliSock, buf, 100, 0);
			cout << buf << endl;
		}
		else
		{
			break;
		}
	}
	return 0;
}


