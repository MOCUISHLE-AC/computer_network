#include <iostream>
#include <WINSOCK2.h>
#include <time.h>
#include <fstream>
#pragma comment(lib, "ws2_32.lib")
using namespace std;
const int PACKEGESIZE = 8192;       //包的长度
const unsigned char SYN = 0x1;      //SYN = 1 
const unsigned char ACK = 0x2;      //ACK = 1
const unsigned char FIN = 0x4;      //FIN = 1 
const unsigned char OVER = 0x8;     //结束标志
double TIMEOUT = 0.5 * CLOCKS_PER_SEC;//超时重传时间

class Myheader {
public:
	u_short Sum;		//校验和 16位
	u_short Datasize;	//所包含数据长度最大16位
	unsigned char flag;	//类型
	unsigned char seq;  //发送序列
	unsigned char ack;  //接收序列
    int windows = 10;   //窗口大小,默认为10
    bool isfile = 0;    //header 后面是否跟data
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

//计算校验和
u_short checksum(u_short* message, int size) {
    int count = (size + 1) / 2;
    u_short* buffer = new u_short[size + 1];
    memset(buffer, 0, size + 1);//将buffer初始化
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
    cout << "缓冲区剩余大小：" << bufferleft << endl;
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
        u_short mysum = checksum((u_short*)&header, sizeof(header));//mysum专门用于计算校验和
        header.Sum = mysum;
    }
    //后面还需跟数据
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
//header+data的校验和的计算
void checksum_data(Myheader& header, char* message, int len,char* message_send)
{
    //char* buffer = new char[PACKEGESIZE + sizeof(header)];//header+数据（1024）
    //buffer中记录header和message信息
    memcpy(message_send, &header, sizeof(header));
    memcpy(message_send + sizeof(header), message, len);
    header.Sum = checksum((u_short*)message_send, sizeof(header) + len);//计算校验和
    memcpy(message_send, &header, sizeof(header));//更新头部
}

//判断接受时是否跨越边界，更新窗口位置
void bounder_accross(Myheader& header, int& head, char* time_message) {
    //没有跨越
    cout << time_message << endl;
    if (int(header.ack) >= head % 256)
    {
        int up = head;
        int down = up + int(header.ack) - head % 256;
        head = down;//更新head
        cout << "Package Recieved!" << " ack:" << int(header.ack) << endl;
    }
    //会出现小于的情况只有跨越255
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
    //socket参数赋值
    server_addr.sin_family = AF_INET;//使用IPV4
    server_addr.sin_port = htons(1234);//端口
    server_addr.sin_addr.s_addr = htonl(2130706433);//地址

    router_addr.sin_family = AF_INET;
    router_addr.sin_port = htons(1235);
    router_addr.sin_addr.s_addr = htonl(2130706434);// 127.0.0.2
}


