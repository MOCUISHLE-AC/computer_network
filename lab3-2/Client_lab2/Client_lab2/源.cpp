#pragma warning(disable:4996);
#include <iostream>
#include <WINSOCK2.h>
#include <time.h>
#include <fstream>
#include "Client_lab2.h"
#pragma comment(lib, "ws2_32.lib")
#define MAX_DIV_MESSAGE  10000000	//��������С
using namespace std;
//��ʱ�ش�
void Timeout_resend(SOCKET& socketClient, SOCKADDR_IN& servAddr, int servAddrlen, char* Buffer, Myheader header) {
	clock_t start = clock();
	setmode(socketClient, 1);
	while (true) {
		if (recvfrom(socketClient, Buffer, PACKEGESIZE, 0, (sockaddr*)&servAddr, &servAddrlen) > 0) {
			break;
		}
		else if (clock() - start > TIMEOUT) {
			//���ײ����뻺����
			memcpy(Buffer, &header, sizeof(header));
			sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
			start = clock();
			cout << "��ʱ�ش�ing~~~" << endl;
		}
	}
}
int Treetimes_Shake(SOCKET& socketClient, SOCKADDR_IN& servAddr, int servAddrlen) {
	Myheader header;
	//header.flag = SYN;
	char* buffer = new char[sizeof(header)];
	Header_init(header, SYN, 0, 0, 0, false);
	//1
	memcpy(buffer, &header, sizeof(header));
	if (sendto(socketClient, buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
	{
		return -1;
	}
	else {
		//��ӡflag��Ϣ
		header.flagprint();
	}
	//��ʱ�ش�
	Timeout_resend(socketClient, servAddr, servAddrlen, buffer, header);
	//2
	memcpy(&header, buffer, sizeof(header));
	u_short mysum =checksum((u_short*)&header, sizeof(header));
	if (check_flag(header,SYN|ACK,sizeof(header)))
	{
		header.flagprint();
	}
	else {
		cout << "�ڶ�������ʧ��" << endl;
		return -1;
	}
	Header_init(header, ACK, 0, 0, 0, false);
	if (sendto(socketClient, (char*)&header, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
	{
		return -1;
	}
	else {
		//��ӡflag��Ϣ
		header.flagprint();
	}
}
int Fourtimes_Wave(SOCKET& socketClient, SOCKADDR_IN& servAddr, int servAddrlen) {
	Myheader header;
	//header.flag = FIN;
	Header_init(header, FIN, 0, 0, 0, false);
	//1
	char* buffer = new char[sizeof(header)];
	memcpy(buffer, &header, sizeof(header));
	while (true) {
		if (sendto(socketClient, buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
		{
			cout << "send FIN=1 failed,resend" << endl;
		}
		else {
			header.flagprint();
			break;
		}
	}
	//2
	//��ʱ�ش�
	Timeout_resend(socketClient, servAddr, servAddrlen, buffer, header);
	memcpy(&header, buffer, sizeof(header));
	if (check_flag(header,ACK,sizeof(header)))
	{
		header.flagprint();
	}
	else {
		cout << "flag or checksum erro" << endl;
	}
	//3
	while (true)
	{
		if (recvfrom(socketClient, buffer, PACKEGESIZE, 0, (sockaddr*)&servAddr, &servAddrlen) == -1)
		{
			cout << "waiting FIN=1 ACK=1" << endl;
		}
		else
		{
			memcpy(&header, buffer, sizeof(header));
			if (check_flag(header,ACK|FIN,sizeof(header)))
			{
				header.flagprint();
				break;
			}
			else {
				cout << "flag wrong,waiting FIN=1 ACK=1" << endl;
			}
		}
	}
	//4
	//header.flag = ACK;
	Header_init(header, ACK, 0, 0, 0, false);
	memcpy(buffer, &header, sizeof(header));
	while (true) {
		if (sendto(socketClient, buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
		{
			cout << "send ACK=1 failed,resend" << endl;
		}
		else {
			header.flagprint();
			break;
		}
	}

	return 1;
}
void Go_backN(SOCKET& socketClient, SOCKADDR_IN& servAddr, char* message, int len) {
    Myheader header;
    char* buffer = new char[sizeof(header)];
	clock_t start=0;
	int sending = 0;
	int packagenum;
	if (float(len) / PACKEGESIZE > len / PACKEGESIZE)
	{
		packagenum = len / PACKEGESIZE + 1;
	}
	else
	{
		packagenum = len / PACKEGESIZE;
	}
	//goback_nʵ��
	for (int comfirmed = -1; comfirmed < packagenum - 1;)
	{
		int Addrlen = sizeof(servAddr);
		if (sending - comfirmed < header.windows && sending < packagenum)
		{
			if (sending == packagenum - 1)
			{
				int order = packagenum - 1;//����
				int offset = order * PACKEGESIZE;//ƫ��
				char* message_begin = message + offset;
				int lastsize = len - (packagenum - 1) * PACKEGESIZE;
				Myheader sendheader;
				Header_init(sendheader, SYN, lastsize, order % 256, 0, true);
				//header+���ݣ�1024��
				char* message_send = new char[PACKEGESIZE + sizeof(header)];
				//�����ݵ�У��͸���
				checksum_data(sendheader, message_begin, lastsize, message_send);
				//��������Сʣ��
				int bufferleft = MAX_DIV_MESSAGE - ((order)*PACKEGESIZE + lastsize);
				//����
				if (sendto(socketClient, message_send, lastsize + sizeof(header), 0, (sockaddr*)&servAddr, Addrlen) == -1)
				{
					cout << "div_send erro" << endl;
					continue;
				}
				//��ӡ��Ϣ
				fileprint(header, bufferleft);
				cout << "�ۻ�ȷ��\t"<<"�Ѿ�ȷ�ϣ�" << comfirmed << "\t���ڷ��ͣ�" << sending << endl;
				sending++;//����
				start = clock();//��ʱ
			}
			else if (sending >= 0 && sending < packagenum - 1)
			{
				int order = sending;//����
				int offset = order * PACKEGESIZE;//ƫ��
				char* message_begin = message + offset;
				int size = PACKEGESIZE;//��С
				Myheader sendheader;
				Header_init(sendheader, SYN, size, order % 256, 0, true);
				//header+���ݣ�1024��
				char* message_send = new char[PACKEGESIZE + sizeof(header)];
				//�����ݵ�У��͸���
				checksum_data(sendheader, message_begin, size, message_send);
				//��������Сʣ��
				int bufferleft = MAX_DIV_MESSAGE - ((order)*PACKEGESIZE + size);
				//����
				if (sendto(socketClient, message_send, size + sizeof(header), 0, (sockaddr*)&servAddr, Addrlen) == -1)
				{
					cout << "div_send erro" << endl;
					continue;
				}
				//��ӡ��Ϣ
				fileprint(header, bufferleft);
				cout <<"�ۻ�ȷ��\t" <<"�Ѿ�ȷ�ϣ�" << comfirmed << "\t���ڷ��ͣ�" << sending << endl;
				sending++;//����
				start = clock();//��ʱ
			}
		}
		//������
		setmode(socketClient, 1);
		int recieved_len = recvfrom(socketClient, buffer, PACKEGESIZE, 0, (sockaddr*)&servAddr, &Addrlen);
		if (recieved_len > 0) 
		{
			Myheader temp;//����ack
			memcpy(&temp, buffer, sizeof(header));
			if (check_flag(temp,ACK,sizeof(header)))
			{
				time_t nowtime;
				struct tm* p;
				time(&nowtime);
				p = localtime(&nowtime);
				//�жϴ��ص�ack�Ƿ����255
				char time_message[20] = { 0 };
				sprintf(time_message,"%02d:%02d:%02d\n", p->tm_hour, p->tm_min, p->tm_sec);
				//Խ�����⣬����comfirmed
				bounder_accross(temp, comfirmed,time_message);
				if (int(temp.ack) + 1 == sending)
				{
					start = start;
				}
				else
				{
					start = clock();
				}
			}
			else if ((temp.flag & ACK) != ACK)
			{
				time_t nowtime;
				struct tm* p;
				time(&nowtime);
				p = localtime(&nowtime);
				char time_message[20] = { 0 };
				sprintf(time_message, "%02d:%02d:%02d\n", p->tm_hour, p->tm_min, p->tm_sec);
				cout << time_message << endl;
				cout << "headerflag erro" << endl;
				sending = comfirmed + 1;
				continue;
			}
			else
			{
				time_t nowtime;
				struct tm* p;
				time(&nowtime);
				p = localtime(&nowtime);
				char time_message[20] = { 0 };
				sprintf(time_message, "%02d:%02d:%02d\n", p->tm_hour, p->tm_min, p->tm_sec);
				cout << time_message << endl;
				cout << "checksm erro" << endl;
				sending = comfirmed + 1;
				continue;
			}
		}
		else
		{
			if (clock() - start > TIMEOUT)
			{
				sending = comfirmed + 1;
				cout << "GobackN seq:"<<sending<<" resend"<<endl;
			}
		}
		setmode(socketClient, 0);
	}
	Header_init(header, OVER, 0, 0, 0, false);
	int Addrlen = sizeof(servAddr);
	sendto(socketClient, (char*)& header, sizeof(header), 0, (sockaddr*)&servAddr, Addrlen);
	//��ʱ�ش�
	Timeout_resend(socketClient, servAddr, Addrlen, buffer, header);
	//����over
	memcpy(&header, buffer, sizeof(header));
	if (check_flag(header,ACK|OVER,sizeof(header)))
	{
		 header.flagprint();
		 cout << "Send success" << endl;
	}
	setmode(socketClient, 0);
}
int main() {
	WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);
    SOCKADDR_IN server_addr;
    SOCKADDR_IN router_addr;
    SOCKET server;
	//socket
	socket_init(server_addr, router_addr);
    server = socket(AF_INET, SOCK_DGRAM, 0);
    int len = sizeof(server_addr);
    //��������
    if (Treetimes_Shake(server, router_addr, len) == -1)
    {
        return 0;
    }
    string filename;
    clock_t start = 0, end = 0;
    while (true) {
        cout << "�ļ�����:";
        cin >> filename;
        if (strcmp(filename.c_str(), "quit") == 0)
        {
            Go_backN(server, router_addr,(char*)(filename.c_str()), filename.length());
            cout << "�˳��ɹ�������" << endl;
            break;
        }
        //�ȴ����ļ���
        Go_backN(server, server_addr,(char*)(filename.c_str()), filename.length());
        ifstream fin(filename.c_str(), ifstream::binary);//�Զ����Ʒ�ʽ���ļ�
        cout << filename.c_str() << endl;
        char* buffer = new char[MAX_DIV_MESSAGE];
        int index = 0;
        long long size = 0;
        unsigned char temp = fin.get();
        while (fin)
        {
            buffer[index++] = temp;
            size++;
            temp = fin.get();
            if (index == MAX_DIV_MESSAGE) {
                if (start == 0)
                {
                    start = clock();
                    start++;
                }
                //���
				Go_backN(server, router_addr, buffer, index);
                char divide_flag[20] = "Notfinished";
				Go_backN(server, router_addr, divide_flag, strlen(divide_flag));
                index = 0;
                memset(buffer, '\0', strlen(buffer));
            }
        }
        cout << "�������һ��" << endl;
        fin.close();
        if (start == 0)
            start = clock();
		Go_backN(server, router_addr, buffer, index);
        char divide_flag[20] = "Finished";
		Go_backN(server, router_addr, divide_flag, strlen(divide_flag));
        end = clock();

        cout << "Total time:" << (end - start) / CLOCKS_PER_SEC << "s" << endl;
        delete[] buffer;
    }
	Fourtimes_Wave(server, router_addr, len);
}