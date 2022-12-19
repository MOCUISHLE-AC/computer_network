#pragma warning(disable:4996);
#include <iostream>
#include <WINSOCK2.h>
#include <time.h>
#include <fstream>
#include "Server_lab2.h"
#pragma comment(lib, "ws2_32.lib")
#define MAX_DIV_MESSAGE  10000000	//��������С
using namespace std;
//��ʱ�ش�
void Timeout_resend(SOCKET& sockServ, SOCKADDR_IN& ClientAddr,char* Buffer, Myheader header) {
    clock_t start = clock();
    setmode(sockServ, 1);
    while (true) {
        int ClientAddrLen = sizeof(ClientAddr);
        if (recvfrom(sockServ, Buffer, PACKAGESIZE, 0, (sockaddr*)&ClientAddr, &ClientAddrLen) > 0) {
            break;
        }
        else if (clock() - start > TIMEOUT) {
            //���ײ����뻺����
            ::memcpy(Buffer, &header, sizeof(header));
            sendto(sockServ, Buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, ClientAddrLen);
            start = clock();
            cout << "��ʱ�ش�ing~~~" << endl;
        }
    }
    setmode(sockServ, 0);
}
int Treetimes_Shake(SOCKET& sockServ, SOCKADDR_IN& ClientAddr)
{
    Myheader header;
    char* buffer = new char[sizeof(header)];
    //1
    while (true) {
        int ClientAddrLen = sizeof(ClientAddr);
        if (recvfrom(sockServ, buffer, sizeof(header), 0, (SOCKADDR*)&ClientAddr, &ClientAddrLen) == -1) 
        {
            return -1;//����SYNʧЧ
        }
        memcpy(&header, buffer, sizeof(header));//sizeof(buffer)
        if (check_flag(header,SYN,sizeof(header)))
        {
            header.flagprint();
            break;
        }
    }
    //2
    //header.flag = ACK | SYN;
    Header_init(header, ACK | SYN, 0, 0, 0, false);
    memcpy(buffer, &header, sizeof(header));
    int ClientAddrLen = sizeof(ClientAddr);
    if (sendto(sockServ, buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
    {
        return -1;//����SYN��ACKʧЧ
    }
    else
        header.flagprint();
    //��ʱ�ش�
    Timeout_resend(sockServ, ClientAddr,buffer, header);
    memcpy(&header, buffer, sizeof(header));
    if (check_flag(header,ACK,sizeof(header))) 
    {
        header.flagprint();
    }
    else
    {
        cout << "����ʧ��" << endl;
    }

}

int Fourtimes_Wave(SOCKET& sockServ, SOCKADDR_IN& ClientAddr)
{
    Myheader header;
    char* buffer = new char[sizeof(header)];
    //1
    int ClientAddrLen = sizeof(ClientAddr);
    while (true)
    {
        if (recvfrom(sockServ, buffer, PACKAGESIZE, 0, (sockaddr*)&ClientAddr, &ClientAddrLen) > 0)
        {
            memcpy(&header, buffer, sizeof(header));
            if (check_flag(header,FIN,sizeof(header)))
            {
               header.flagprint();
               break;
            }
            else
            {
                cout << "flag or checksum erro" << endl;
            }
        }
    }
    //2
   /* header.flag = ACK;*/
    Header_init(header, ACK, 0, 0, 0, false);
    memcpy(buffer, &header, sizeof(header));
    while (true){
        if (sendto(sockServ, buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, ClientAddrLen) > 0){
            header.flagprint();
            break;
        }
        else{
            cout << "send ACK=1 wrong,resend" << endl;
        }
    }
    //3
    /*header.flag = ACK|FIN;*/
    Header_init(header, ACK | FIN, 0, 0, 0, false);
    memcpy(buffer, &header, sizeof(header));
    while (true)
    {
        if (sendto(sockServ, buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, ClientAddrLen) > 0)
        {
            header.flagprint();
            break;
        }
        else
        {
            cout << "send ACK=1,FIN=1 wrong,resend" << endl;
        }
    }
    Timeout_resend(sockServ, ClientAddr,buffer, header);
    //4
    memcpy(&header, buffer, sizeof(header));
    if (check_flag(header,ACK,sizeof(header)))
    {
        header.flagprint();
    }
    else
    {
        cout << "ACK=1 ,erro";
        return -1;
    }
    return 1;
}
void time_show()
{
    time_t nowtime;
    struct tm* p;
    time(&nowtime);
    p = localtime(&nowtime);
    char time_message[20] = { 0 };
    sprintf(time_message, "%02d:%02d:%02d\n", p->tm_hour, p->tm_min, p->tm_sec);
    cout << time_message << endl;
}
//����
int Download(SOCKET& sockServ, SOCKADDR_IN& ClientAddr, char* message)
{
    Myheader header;
    char* file = new char[PACKAGESIZE + sizeof(header)];
    int expectedseq = 0;//server�˰�����ACK
    char* init = message; //��ʼλ��
    int sum=0;
    int bufferleft = MAX_DIV_MESSAGE;//������ʣ���С

    //socket 
    int ClientAddrLen = sizeof(ClientAddr);
    while (true)
    {
        recvfrom(sockServ, file, sizeof(header) + PACKAGESIZE, 0, (sockaddr*)&ClientAddr, &ClientAddrLen);
        memcpy(&header, file, sizeof(header));
        if (check_flag(header,SYN, header.Datasize + sizeof(header),file))
        {
            Myheader temp;//temp
            cout << "Client send "<<"seq:" << int(header.seq) << endl;
            if (expectedseq == header.seq) {
                //inorder��ʾ��ȷ����
                expectedseq++;
                bufferleft = bufferleft - header.Datasize;
                //����ACK
                Header_init(temp, ACK, 0,0, header.seq,false);
                //��־��header+������ʣ��
                fileprint(temp, bufferleft);
                time_show();
                //buffer��ǰ����Ϊ��¼��header
                memcpy(file, &temp, sizeof(temp));
                sendto(sockServ, file, sizeof(temp), 0, (sockaddr*)&ClientAddr, ClientAddrLen);
                //download��Ϣ
                char* write = new char[header.Datasize];
                memcpy(write, file + sizeof(header), header.Datasize);
                memcpy(message + sum, write, header.Datasize);
                sum += int(header.Datasize);
                delete[]write;
                //seq�Ƿ�Խ��
                bounder_over(expectedseq);
            }
            else
            {
                //�ظ�����ACK
                temp.Datasize = 0;
                if (expectedseq != 0)
                    temp.ack = expectedseq - 1;
                else
                    temp.ack = 255;
                temp.isfile = false;
                temp.flag = ACK;
                temp.Sum = checksum((u_short*)&temp, sizeof(header));
                //buffer��ǰ����Ϊ��¼��header
                memcpy(file, &temp, sizeof(temp));
                sendto(sockServ, file, sizeof(temp), 0, (sockaddr*)&ClientAddr, ClientAddrLen);
                cout << "Not in order" << endl;
                //seq�Ƿ�Խ��
                bounder_over(expectedseq);
            }
        }
        else if ((header.flag & OVER) == OVER && checksum((u_short*)&header, sizeof(header))==0)
        {
            Myheader temp;
            Header_init(header, ACK | OVER, 0, 0, 0, false);
            memcpy(file, &temp, sizeof(temp));
            sendto(sockServ, file, sizeof(temp), 0, (sockaddr*)&ClientAddr, ClientAddrLen);
            //����temp ��flag
            temp.flagprint();
            break;
        }
        else {
            cout << "flag or checksum erro" << endl;
        }
    }
    return sum+int(init-message);
}

int main() 
{
	//����socket
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);

    SOCKADDR_IN server_addr;
    SOCKADDR_IN router_addr;
    SOCKET server;
    //socket������ֵ
    socket_init(server_addr, router_addr);
    server = socket(AF_INET, SOCK_DGRAM, 0);
    bind(server, (SOCKADDR*)&server_addr, sizeof(server_addr));
    cout << "Server Listening~~~~~" << endl;
    int len = sizeof(server_addr);
    Treetimes_Shake(server, router_addr);
    //����data
    while (true)
    {
        char* name = new char[20];
        char* data = new char[MAX_DIV_MESSAGE];
        int namelen = Download(server, router_addr, name);
        cout <<"namelen:"<< namelen<<endl;
        //memcpy��strcpy��ͬ������ʹ��strcmp�����
        string a;
        for (int i = 0; i < namelen; i++)
        {
            a = a + name[i];
        }
        cout << a << endl;
        ofstream fout(a.c_str(), ofstream::binary);
        if (strcmp(a.c_str(), "quit") == 0)
            break;
        while (true) {
            int datalen = Download(server, router_addr, data);
            for (int i = 0; i < datalen; i++)
            {
                fout << data[i];
            }
            cout << datalen<<" of "<<a<<" �ɹ�����" << endl;
            ::memset(data, '\0', MAX_DIV_MESSAGE);

            datalen = Download(server, router_addr, data);
            string a;
            for (int i = 0; i < datalen; i++)
            {
                a = a + data[i];
            }
            if (strcmp(a.c_str(), "Finished") == 0) {
                cout << "�����������Ѿ�����" << endl;
                break;
            }
            ::memset(data, '\0', MAX_DIV_MESSAGE);
        }
        fout.close();
        delete[] name;
        delete[] data;
    }
    Fourtimes_Wave(server, router_addr);
}