#include <iostream>
#include <WINSOCK2.h>
#include <time.h>
#include <fstream>
#pragma comment(lib, "ws2_32.lib")
using namespace std;
const int PACKEGESIZE = 8192;       //���ĳ���
const unsigned char SYN = 0x1;      //SYN = 1 
const unsigned char ACK = 0x2;      //ACK = 1
const unsigned char FIN = 0x4;      //FIN = 1 
const unsigned char OVER = 0x8;     //������־
double TIMEOUT = 0.5 * CLOCKS_PER_SEC;//��ʱ�ش�ʱ��

class Myheader {
public:
	u_short Sum;		//У��� 16λ
	u_short Datasize;	//���������ݳ������16λ
	unsigned char flag;	//����
	unsigned char seq;  //��������
	unsigned char ack;  //��������
    int windows = 10;   //���ڴ�С,Ĭ��Ϊ10
    bool isfile = 0;    //header �����Ƿ��data
	Myheader();
    void flagprint();
};

Myheader::Myheader() {
	Sum = 0;
	Datasize = 0;
	flag = 0;
	seq = 0;
	ack = 0;
}

//����У���
u_short checksum(u_short* message, int size) {
    int count = (size + 1) / 2;
    u_short* buffer = new u_short[size + 1];
    memset(buffer, 0, size + 1);//��buffer��ʼ��
    memcpy(buffer, message, size);
    u_long sum = 0;
    for (int i = 0; i < count; i++)
    {
        sum += *buffer++;
        if (sum & 0xffff0000) {
            sum &= 0xffff;
            sum++;
        }
    }
    return ~(sum & 0xffff);
}

void Myheader::flagprint() {
    int SYN_flag, ACK_flag, FIN_flag,Over_flag;
    SYN_flag = (flag & 0x1)==SYN ? 1 : 0;
    ACK_flag = (flag & 0x2)==ACK ? 1 : 0;
    FIN_flag = (flag & 0x4)==FIN ? 1 : 0;
    Over_flag = (flag & 0x8)==OVER ? 1 : 0;
    cout << "isfile="<<isfile<<"\t SYN=" << SYN_flag << "\t ACK=" << ACK_flag << "\t FIN=" << FIN_flag << "\t OVER=" << Over_flag << endl;
}

void fileprint(Myheader temp,int bufferleft) {
    temp.flagprint();
    cout << "seq=" << int(temp.seq) << "\t ack=" << int(temp.ack) << "\t Sum=" << temp.Sum << "\t DataSize=" << temp.Datasize << endl;
    cout << "������ʣ���С��" << bufferleft << endl;
}

void setmode(SOCKET& socketClient, int flag)
{
    if (flag == 0)
    {
        u_long mode = 0;
        ioctlsocket(socketClient, FIONBIO, &mode);
    }
    else
    {
        u_long mode = 1;
        ioctlsocket(socketClient, FIONBIO, &mode);
    }
}
void Header_init(Myheader& header, unsigned char myflag, u_short size, unsigned char myseq, unsigned char myack, bool myisfile)
{
    if (!myisfile)
    {
        header.ack = myack;
        header.seq = myseq;
        header.isfile = myisfile;
        header.flag = myflag;
        header.Sum = 0;
        header.Datasize = size;
        u_short mysum = checksum((u_short*)&header, sizeof(header));//mysumר�����ڼ���У���
        header.Sum = mysum;
    }
    //���滹�������
    else
    {
        header.ack = myack;
        header.seq = myseq;
        header.isfile = myisfile;
        header.flag = myflag;
        header.Sum = 0;
        header.Datasize = size;
    }
}
//header+data��У��͵ļ���
void checksum_data(Myheader& header, char* message, int len,char* message_send)
{
    //char* buffer = new char[PACKEGESIZE + sizeof(header)];//header+���ݣ�1024��
    //buffer�м�¼header��message��Ϣ
    memcpy(message_send, &header, sizeof(header));
    memcpy(message_send + sizeof(header), message, len);
    header.Sum = checksum((u_short*)message_send, sizeof(header) + len);//����У���
    memcpy(message_send, &header, sizeof(header));//����ͷ��
}

//�жϽ���ʱ�Ƿ��Խ�߽磬���´���λ��
void bounder_accross(Myheader& header, int& head, char* time_message) {
    //û�п�Խ
    cout << time_message << endl;
    if (int(header.ack) >= head % 256)
    {
        int up = head;
        int down = up + int(header.ack) - head % 256;
        head = down;//����head
        cout << "Package Recieved!" << " ack:" << int(header.ack) << endl;
    }
    //�����С�ڵ����ֻ�п�Խ255
    else if (int(header.ack) < head % 256)
    {
        int up = head;
        int down = up + int(header.ack) - head % 256 + 256;
        head = down;
        cout << "Package Recieved!" << " ack:" << int(header.ack) << endl;
    }
}

bool check_flag(Myheader& header, unsigned char flag, int size, char* message = nullptr)
{
    if (message == nullptr)
    {
        if (header.flag == flag && checksum((u_short*)&header, size) == 0)
        {
            return true;
        }
        else
            return false;
    }
    else
    {
        if (header.flag == flag && checksum((u_short*)message, size) == 0)
        {
            return true;
        }
        else
            return false;
    }
}
void socket_init(SOCKADDR_IN& server_addr, SOCKADDR_IN& router_addr)
{
    //socket������ֵ
    server_addr.sin_family = AF_INET;//ʹ��IPV4
    server_addr.sin_port = htons(1234);//�˿�
    server_addr.sin_addr.s_addr = htonl(2130706433);//��ַ

    router_addr.sin_family = AF_INET;
    router_addr.sin_port = htons(1235);
    router_addr.sin_addr.s_addr = htonl(2130706434);// 127.0.0.2
}


