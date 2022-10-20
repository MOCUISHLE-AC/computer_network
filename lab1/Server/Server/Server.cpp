#pragma warning(disable:4996)
#include <winsock2.h> // winsock2��ͷ�ļ�
#include <WS2tcpip.h>
#include <iostream>
#pragma comment(lib, "ws2_32.lib")
#define max_num 10 //Client�������
#define port 6666 //Ĭ�϶˿ں�
using namespace std;

int total=0;//Client������
SOCKET cliSock[max_num] = {0};//�ͻ����׽��� 
SOCKADDR_IN cliAddr[max_num] = {0};//�ͻ��˵�ַ
char cliname[max_num][20] = {0};

// stdcall���̴߳�����
DWORD WINAPI ThreadFun(LPVOID lpThreadParameter);
DWORD WINAPI WacthThread(LPVOID lpThreadParameter) {
	while (true) {
		char order[20] = { 0 };//������������
		cin.getline(order, 20);
		if (strcmp(order, "sudo ls") == 0) {
			if (total == 0) {
				cout << "������������" << total << endl << endl;
			}
			else {
				cout << "������������" << total << endl;
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

	//������ʽ�׽���
	SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_socket == INVALID_SOCKET)
	{
		cout << "socket error:" << WSAGetLastError() << endl;
		return 0;
	}

	//�󶨶˿ں�ip
	sockaddr_in addr;
	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	cout << "Ĭ�϶˿ںţ�" << port << endl;
	cout << "Ĭ��IP��ַ��" << "127.0.0.1" << endl;

	int len = sizeof(sockaddr_in);
	if (bind(server_socket, (SOCKADDR*)&addr, len) == SOCKET_ERROR)
	{
		cout << "bind Error:" << WSAGetLastError() << endl;
		return 0;
	}
	cout << "server�����ɹ�" << endl << endl;
	//����
	listen(server_socket, max_num);

	//�������߳�
	HANDLE Recv_thread = CreateThread(NULL, 0, WacthThread, (LPVOID)server_socket, 0, NULL);
	// ���߳�ѭ�����տͻ��˵�����
	while (true)
	{
		sockaddr_in addrClient;
		len = sizeof(sockaddr_in);
		// ���ܳɹ�������clientͨѶ��Socket
		SOCKET accept_socket = accept(server_socket, (SOCKADDR*)&addrClient, &len);
		if (accept_socket != INVALID_SOCKET)
		{
			cliSock[total] = accept_socket;
			cliAddr[total] = addrClient;
			// �����̣߳����Ҵ�����clientͨѶ���׽���
			HANDLE Recv_thread = CreateThread(NULL, 0, ThreadFun, (LPVOID)accept_socket, 0, NULL);
			CloseHandle(Recv_thread); // �رն��̵߳�����
		}
	}

	// 6.�رռ����׽���
	closesocket(server_socket);

	// ����winsock2�Ļ���
	WSACleanup();

	return 0;
}

DWORD WINAPI ThreadFun(LPVOID lpThreadParameter)
{
	//��ͻ���ͨѶ�����ͻ��߽�������
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
	
	cout << "��ӭ" << temp << "���������ң�" <<endl;

	strcpy(cliname[index], temp);
	// ��������
	char welcome[100] = { 0 };
	sprintf(welcome, "��ӭ %s ���������ң�", cliname[index]);
	send(c, welcome, 100, 0);

	// ѭ�����տͻ�������
	int ret = 0;
	do
	{
		//����Client���͵���Ϣ
		char receive[100] = { 0 };
		ret = recv(c, receive, 100, 0);
		char spread[100] = { 0 };
		//ĳЩClient�߳��Ƴ�����Ҫ���¶�λ
		int index = -1;
		for (int i = 0; i < total; i++) {
			if (c == cliSock[i]) {
				index = i;
				break;
			}
		}
		sprintf(spread, "\n%s˵��", cliname[index]);
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
			sprintf(spread, "\n%s�˳�������!!!\n", cliname[index]);
			cout << endl << cliname[index] << "�뿪��������!!!" << endl;
			for (int i = 0; i < total; i++) {
				if (c != cliSock[i]) {
					send(cliSock[i], spread, 100, 0);
				}
			}
		}
	} while (ret != SOCKET_ERROR && ret != 0);
	//ĳЩClient�߳��Ƴ�����Ҫ���¶�λ
	index = -1;
	for (int i = 0; i < total; i++) {
		if (c == cliSock[i]) {
			index = i;
			break;
		}
	}
	//�˳��߳�ǰ����Client������ǰ�ƶ�
	for (int i = index; i < total; i++) {
		cliSock[i] = cliSock[i + 1];
		cliAddr[i] = cliAddr[i + 1];
		strcpy(cliname[i], cliname[i + 1]);
	}
	total--;
	return 0;
}
