#pragma warning(disable:4996)
#include <winsock2.h>//winsock2��ͷ�ļ�
#include <iostream>
#include <windows.h>
#include <cstring>
using  namespace std;

//����������dll��lib
#pragma comment(lib, "ws2_32.lib")

DWORD WINAPI recvMsgThread(LPVOID IpParameter);
int  main()
{

	//����winsock2�Ļ���
	WSADATA  wd;
	if (WSAStartup(MAKEWORD(2, 2), &wd) != 0)
	{
		cout << "WSAStartup  error��" << GetLastError() << endl;
		return 0;
	}

	//������ʽ�׽���
	SOCKET  s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
	{
		cout << "socket  error��" << GetLastError() << endl;
		return 0;
	}

	//���ӷ�����
	sockaddr_in   addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(6666);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	cout << "connecting..." << endl;
	int len = sizeof(sockaddr_in);
	if (connect(s, (SOCKADDR*)&addr, len) == SOCKET_ERROR)
	{
		cout << "connect  error��" << GetLastError() << endl;
		return 0;
	}
	cout << "�����û�����";
	char name[20] = { 0 };
	cin.getline(name, 20);
	send(s, name, 20, 0);
	//���շ���˵���Ϣ
	char buf[100] = { 0 };
	recv(s, buf, 100, 0);
	cout << buf << endl;
	// �����̣߳����Ҵ�����clientͨѶ���׽���
	HANDLE hThread = CreateThread(NULL, 0, recvMsgThread, (LPVOID)s, 0, NULL);
	//��ʱ������˷���Ϣ
	int  ret = 0;
	while (true)
	{
		//��ȡ��������
		char chat[100] = { 0 };
		cout << ">";
		cin.getline(chat, 100);
		if (strlen(chat) != 0)
		{
			char quit[15] = "sudo quit";
			if (strcmp(chat, quit) == 0)
			{
				cout << "�˳��ɹ���" << endl;
				break;
			}
			//��ȡϵͳʱ��
			char time[100] = { 0 };
			SYSTEMTIME sys;
			GetLocalTime(&sys);
			sprintf(time, "\n%4d/%02d/%02d %02d:%02d:%02d\n", sys.wYear, sys.wMonth, sys.wDay, sys.wHour, sys.wMinute, sys.wSecond);
			//ƴ��
			strcat(chat, time);
			send(s, chat, 100, 0);
		}
		else {
			cout << "���벻��Ϊ�գ�����" << endl;
		}
	}
	CloseHandle(hThread); // �رն��̵߳�����
	//4.�رռ����׽���
	closesocket(s);

	//����winsock2�Ļ���
	WSACleanup();
	system("pause");
	return 0;
}
DWORD WINAPI recvMsgThread(LPVOID IpParameter)//������Ϣ���߳�
{
	SOCKET cliSock = (SOCKET)IpParameter;//��ȡ�ͻ��˵�SOCKET����
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


