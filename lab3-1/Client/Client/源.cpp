#include <iostream>
#include <WINSOCK2.h>
#include <time.h>
#include <fstream>
#pragma comment(lib, "ws2_32.lib")
#define MAX_DIV_MESSAGE  10000000 //文件一次性读取的最大size
using namespace std;

const int MAXSIZE = 1024;//传输缓冲区最大长度
const unsigned char SYN = 0x1; //SYN = 1 ACK = 0
const unsigned char ACK = 0x2;//SYN = 0, ACK = 1，FIN = 0
const unsigned char ACK_SYN = 0x3;//SYN = 1, ACK = 1
const unsigned char FIN = 0x4;//FIN = 1 ACK = 0
const unsigned char FIN_ACK = 0x5;
const unsigned char OVER = 0x7;//结束标志
double MAX_TIME = 0.5 * CLOCKS_PER_SEC;


u_short checksum(u_short* mes, int size) {
    int count = (size + 1) / 2;
    u_short* buf = (u_short*)malloc(size + 1);
    memset(buf, 0, size + 1);
    memcpy(buf, mes, size);
    u_long sum = 0;
    while (count--) {
        sum += *buf++;
        if (sum & 0xffff0000) {
            sum &= 0xffff;
            sum++;
        }
    }
    return ~(sum & 0xffff);
}

struct HEADER
{
    u_short sum = 0;//校验和 16位
    u_short datasize = 0;//所包含数据长度 16位
    unsigned char flag = 0;
    //八位，使用后四位，排列是FIN ACK SYN 
    unsigned char SEQ = 0;
    //八位，传输的序列号，0~255，超过后mod
    HEADER() {
        sum = 0;//校验和 16位
        datasize = 0;//所包含数据长度 16位
        flag = 0;
        //八位，使用后三位，排列是FIN ACK SYN 
        SEQ = 0;
    }
};

int Treetimes_Shake(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen)//三次握手建立连接
{
    HEADER header;
    char* Buffer = new char[sizeof(header)];

    u_short sum;

    //进行第一次握手
    header.flag = SYN;
    header.sum = 0;//校验和置0
    u_short temp = checksum((u_short*)&header, sizeof(header));
    header.sum = temp;//计算校验和
    memcpy(Buffer, &header, sizeof(header));//将首部放入缓冲区
    if (sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
    {
        return -1;
    }
    else {
        cout << "Client发送Server的第一次握手：SYN=1" << endl;
    }
    clock_t start = clock(); //记录发送第一次握手时间

    u_long mode = 1;
    ioctlsocket(socketClient, FIONBIO, &mode);//非阻塞模式

    //接收第二次握手
    while (recvfrom(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
    {
        if (clock() - start > MAX_TIME)//超时，重新传输第一次握手
        {
            header.flag = SYN;
            header.sum = 0;//校验和置0
            header.sum = checksum((u_short*)&header, sizeof(header));//计算校验和
            memcpy(Buffer, &header, sizeof(header));//将首部放入缓冲区
            sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
            start = clock();
            cout << "第一次握手超时，正在进行重传" << endl;
        }
    }


    //进行校验和检验
    memcpy(&header, Buffer, sizeof(header));
    if (header.flag == ACK_SYN && checksum((u_short*)&header, sizeof(header) == 0))
    {
        cout << "Client接收Server的第二次握手: SYN=1 ACK=1" << endl;
    }
    else
    {
        cout << "连接发生错误，请重启客户端！" << endl;
        return -1;
    }

    //进行第三次握手
    header.flag = ACK;
    header.sum = 0;
    header.sum = checksum((u_short*)&header, sizeof(header));//计算校验和
    if (sendto(socketClient, (char*)&header, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
    {
        return -1;//判断客户端是否打开，-1为未开启发送失败
    }
    cout << "Client发送Server的第三次握手: ACK=1" << endl;
    return 1;
}


void send_package(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, char* message, int len, int& order)
{
    HEADER header;
    char* buffer = new char[MAXSIZE + sizeof(header)];
    header.datasize = len;
    header.SEQ = unsigned char(order);//序列号

    //buffer中记录header和message信息
    memcpy(buffer, &header, sizeof(header));
    memcpy(buffer + sizeof(header), message, sizeof(header) + len);
    u_short check = checksum((u_short*)buffer, sizeof(header) + len);//计算校验和
    header.sum = check;
    memcpy(buffer, &header, sizeof(header));
    sendto(socketClient, buffer, len + sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);//发送

    cout << "Send message " << len << " bytes!" << " flag:" << int(header.flag) << " SEQ:" << int(header.SEQ) << " SUM:" << int(header.sum) << endl;
    clock_t start = clock();//记录发送时间
    //接收ack等信息
    while (true)
    {
        u_long mode = 1;
        ioctlsocket(socketClient, FIONBIO, &mode);
        while (recvfrom(socketClient, buffer, MAXSIZE, 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
        {
            if (clock() - start > MAX_TIME)
            {
                header.datasize = len;
                header.SEQ = u_char(order);//序列号
                header.flag = u_char(0x0);
                memcpy(buffer, &header, sizeof(header));
                memcpy(buffer + sizeof(header), message, sizeof(header) + len);
                u_short check = checksum((u_short*)buffer, sizeof(header) + len);//计算校验和
                header.sum = check;
                memcpy(buffer, &header, sizeof(header));
                sendto(socketClient, buffer, len + sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);//发送
                cout << "TIME OUT! ReSend message " << len << " bytes! Flag:" << int(header.flag) << " SEQ:" << int(header.SEQ) << endl;
                start = clock();//记录发送时间
            }
        }
        memcpy(&header, buffer, sizeof(header));//缓冲区接收到信息，读取
        u_short check = checksum((u_short*)&header, sizeof(header));
        if (header.SEQ == u_short(order) && header.flag == ACK)
        {
            cout << "Send has been confirmed! Flag:" << int(header.flag) << " SEQ:" << int(header.SEQ) << endl;
            break;
        }
        else
        {
            continue;
        }
    }
    u_long mode = 0;
    ioctlsocket(socketClient, FIONBIO, &mode);//改回阻塞模式
}

void send(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, char* message, int len)
{
    int packagenum = len / MAXSIZE + (len % MAXSIZE != 0);
    int seqnum = 0;
    for (int i = 0; i < packagenum; i++)
    {
        send_package(socketClient, servAddr, servAddrlen, message + i * MAXSIZE, i == packagenum - 1 ? len - (packagenum - 1) * MAXSIZE : MAXSIZE, seqnum);
        seqnum++;
        if (seqnum > 255)
        {
            seqnum = seqnum - 256;
        }
    }
    //发送结束信息
    HEADER header;
    char* Buffer = new char[sizeof(header)];
    header.flag = OVER;
    header.sum = 0;
    u_short temp = checksum((u_short*)&header, sizeof(header));
    header.sum = temp;
    memcpy(Buffer, &header, sizeof(header));
    sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
    cout << "Send End!" << endl;
    clock_t start = clock();
    while (true)
    {
        u_long mode = 1;
        ioctlsocket(socketClient, FIONBIO, &mode);
        while (recvfrom(socketClient, Buffer, MAXSIZE, 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
        {
            if (clock() - start > MAX_TIME)
            {
                char* Buffer = new char[sizeof(header)];
                header.flag = OVER;
                header.sum = 0;
                u_short temp = checksum((u_short*)&header, sizeof(header));
                header.sum = temp;
                memcpy(Buffer, &header, sizeof(header));
                sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
                cout << "Time Out! ReSend End!" << endl;
                start = clock();
            }
        }
        memcpy(&header, Buffer, sizeof(header));//缓冲区接收到信息，读取
        u_short check = checksum((u_short*)&header, sizeof(header));
        if (header.flag == OVER)
        {
            cout << "对方已成功接收文件!" << endl;
            break;
        }
        else
        {
            continue;
        }
    }
    u_long mode = 0;
    ioctlsocket(socketClient, FIONBIO, &mode);//改回阻塞模式
}



int Fourtimes_Wave(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen)
{
    HEADER header;
    char* Buffer = new char[sizeof(header)];

    u_short sum;

    //进行第一次握手
    header.flag = FIN;
    header.sum = 0;//校验和置0
    u_short temp = checksum((u_short*)&header, sizeof(header));
    header.sum = temp;//计算校验和
    memcpy(Buffer, &header, sizeof(header));//将首部放入缓冲区
    if (sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
    {
        return -1;
    }
    clock_t start = clock(); //记录发送第一次挥手时间

    u_long mode = 1;
    ioctlsocket(socketClient, FIONBIO, &mode);

    //接收第二次挥手
    while (recvfrom(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
    {
        if (clock() - start > MAX_TIME)//超时，重新传输第一次挥手
        {
            header.flag = FIN;
            header.sum = 0;//校验和置0
            header.sum = checksum((u_short*)&header, sizeof(header));//计算校验和
            memcpy(Buffer, &header, sizeof(header));//将首部放入缓冲区
            sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
            start = clock();
            cout << "第一次挥手超时，正在进行重传" << endl;
        }
    }


    //进行校验和检验
    memcpy(&header, Buffer, sizeof(header));
    if (header.flag == ACK && checksum((u_short*)&header, sizeof(header) == 0))
    {
        cout << "收到第二次挥手信息" << endl;
    }
    else
    {
        cout << "连接发生错误，程序直接退出！" << endl;
        return -1;
    }

    //接收第三次挥手
    while (true) {
        if (recvfrom(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, &servAddrlen) > 0)
            break;
    }
    //进行校验和检验
    memcpy(&header, Buffer, sizeof(header));
    if (header.flag == FIN_ACK && checksum((u_short*)&header, sizeof(header) == 0))
    {
        cout << "收到第三次挥手信息" << endl;
    }
    else
    {
        cout << "连接发生错误，程序直接退出！" << endl;
        return -1;
    }

    //发送第四次挥手
    header.flag = ACK;
    header.sum = 0;//校验和置0
    temp = checksum((u_short*)&header, sizeof(header));
    header.sum = temp;//计算校验和
    memcpy(Buffer, &header, sizeof(header));//将首部放入缓冲区
    if (sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
    {
        return -1;
    }
    else {
        cout << "成功第四次挥手" << endl;
    }

}


int main()
{
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);

    SOCKADDR_IN server_addr;
    SOCKADDR_IN router_addr;
    SOCKET server;

    server_addr.sin_family = AF_INET;//使用IPV4
    server_addr.sin_port = htons(1234);
    server_addr.sin_addr.s_addr = htonl(2130706433);

    router_addr.sin_family = AF_INET;
    router_addr.sin_port = htons(4002);
    router_addr.sin_addr.s_addr = htonl(2130706434);// 127.0.0.2

    server = socket(AF_INET, SOCK_DGRAM, 0);
    int len = sizeof(server_addr);
    //建立连接
    if (Treetimes_Shake(server, router_addr, len) == -1)
    {
        return 0;
    }

    string filename;
    clock_t start=0, end=0;
    while (true) {
        cout << "请输入文件名称" << endl;
        cin >> filename;
        if (strcmp(filename.c_str(), "quit")==0)
        {
            send(server, router_addr, len, (char*)(filename.c_str()), filename.length());
            cout << "退出成功！！！" << endl;
            break;
        }
        //先传输文件名
        send(server, server_addr, len, (char*)(filename.c_str()), filename.length());

        ifstream fin(filename.c_str(), ifstream::binary);//以二进制方式打开文件
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
                //标记
                send(server, router_addr, len, buffer, index);
                char divide_flag[20] = "Notfinished";
                send(server, router_addr, len, divide_flag, strlen(divide_flag));
                index = 0;
                memset(buffer, '\0', strlen(buffer));
            }
        }
        cout << "进入最后一组" << endl;
        fin.close();
        if (start == 0)
            start = clock();
        send(server, router_addr, len, buffer, index);
        char divide_flag[20] = "Finished";
        send(server, router_addr, len, divide_flag, strlen(divide_flag));
        end = clock();

        cout << "传输总时间为:" << (end - start) / CLOCKS_PER_SEC << "s" << endl;
        cout << "吞吐率为:" << ((float)size) / ((end - start) / CLOCKS_PER_SEC) << "byte/s" << endl;
        delete[] buffer;
    }
    Fourtimes_Wave(server, router_addr, len);
}
