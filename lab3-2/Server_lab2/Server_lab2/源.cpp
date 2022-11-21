#include <iostream>
#include <WINSOCK2.h>
#include <time.h>
#include <fstream>
#pragma comment(lib, "ws2_32.lib")
#define MAX_DIV_MESSAGE  10000000 //文件一次性读取的最大size
using namespace std;

const int MAXSIZE = 1024;//传输缓冲区最大长度
const unsigned char SYN = 0x1; //SYN = 1 ACK = 0
const unsigned char ACK = 0x2;//SYN = 0, ACK = 1
const unsigned char ACK_SYN = 0x3;//SYN = 1, ACK = 1
const unsigned char FIN = 0x4;//FIN = 1 ACK = 0
const unsigned char FIN_ACK = 0x5;//FIN = 1 ACK = 0
const unsigned char OVER = 0x7;//结束标志
double MAX_TIME = 0.5 * CLOCKS_PER_SEC;
//UDP头
struct HEADER
{
    u_short sum = 0;//校验和 16位
    u_short datasize = 0;//所包含数据长度 16位
    unsigned char flag = 0;
    //八位，使用后三位，排列是FIN ACK SYN 
    unsigned char SEQ = 0;
    //八位，传输的序列号，0~255，超过后mod
    HEADER() {
        sum = 0;//校验和 16位
        datasize = 0;//所包含数据长度 16位
        flag = 0;
        //八位，使用后四位，排列是FIN ACK SYN 
        SEQ = 0;
    }
};

//UDP检验和的计算方法是：
//后16位反码求和，运算，将结果取反写入校验和
u_short checksum(u_short* message, int size) { //size 为多少个字节
    //num   单位：u_short 16 bit
    int num = (size + 1) / 2;
    u_short* buf = (u_short*)malloc(size + 1);
    memset(buf, 0, size + 1);
    memcpy(buf, message, size);
    u_long sum = 0;

    //反码求和，最后取反
    while (num--) {
        sum += *buf++;
        if (sum & 0xffff0000) {
            sum &= 0xffff;
            sum++;
        }
    }
    return ~(sum & 0xffff);
}
//建立连接，三次握手
int Treetimes_Shake(SOCKET& sockServ, SOCKADDR_IN& ClientAddr, int& ClientAddrLen)
{
    HEADER header;
    char* buffer = new char[sizeof(header)];
    //接受SYN=1，第一次握手;
    while (true) {
        if (recvfrom(sockServ, buffer, sizeof(header), 0, (SOCKADDR*)&ClientAddr, &ClientAddrLen) == -1) {
            return -1;//接受SYN失效
        }
        memcpy(&header, buffer, sizeof(header));//sizeof(buffer)
        //SYN==1,并且校验和正确
        //所有的16位的数相加，如果低16位全为1，则没有出错。
        if (header.flag == SYN && checksum((u_short*)&header, sizeof(header)) == 0) {
            cout << "Server接受Client的第一次握手：SYN=1" << endl;
            break;
        }
    }
    //Server发送SYN==1，ACK==1
    header.flag = ACK_SYN;
    header.sum = 0;
    u_short temp = checksum((u_short*)&header, sizeof(header));
    header.sum = temp;//更新校验和
    //写入信息
    memcpy(buffer, &header, sizeof(header));
    if (sendto(sockServ, buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
    {
        return -1;//发送SYN、ACK失效
    }
    else
        cout << "Server发送Client的第二次握手：SYN=1 ACK=1" << endl;
    clock_t start = clock();//记录时间

    while (recvfrom(sockServ, buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, &ClientAddrLen) <= 0)
    {
        //超时重传
        if (clock() - start > MAX_TIME) {
            header.flag = ACK_SYN;
            header.sum = 0;
            u_short temp = checksum((u_short*)&header, sizeof(header));
            header.sum = temp;//更新校验和
            memcpy(buffer, &header, sizeof(header));
            if (sendto(sockServ, buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
            {
                return -1;//发送SYN、ACK失效
            }
            cout << "第二次握手失败，超时重传！！！" << endl;
        }
    }

    HEADER lastshake;
    memcpy(&lastshake, buffer, sizeof(lastshake));
    if (lastshake.flag = ACK && checksum((u_short*)&lastshake, sizeof(lastshake)) == 0) {
        cout << "Server接收Client的第三次握手：ACK=1" << endl;
    }
    else
    {
        cout << "第三次握手发生错误，请重启客户端" << endl;
    }
    return 1;//正常三次握手
}

//接收文件
int RecvMessage(SOCKET& sockServ, SOCKADDR_IN& ClientAddr, int& ClientAddrLen, char* message)
{
    long int all = 0;//文件长度
    HEADER header;
    char* buffer = new char[MAXSIZE + sizeof(header)];
    int seq = 0;
    int index = 0;
    while (true) {
        int length = recvfrom(sockServ, buffer, sizeof(header) + MAXSIZE, 0, (sockaddr*)&ClientAddr, &ClientAddrLen);//接收报文长度
        memcpy(&header, buffer, sizeof(header));
        //判断是否是结束
        if (header.flag == OVER && checksum((u_short*)&header, sizeof(header)) == 0)
        {
            cout << "文件接收完毕" << endl;
            break;
        }
        if (header.flag == unsigned char(0) && checksum((u_short*)buffer, length - sizeof(header))) {

            //判断是否是别的包
            if (seq != int(header.SEQ)) {
                //说明出了问题，返回ACK
                header.flag = ACK;
                header.datasize = 0;
                header.SEQ = (unsigned char)seq;
                header.sum = 0;
                u_short temp = checksum((u_short*)&header, sizeof(header));
                header.sum = temp;
                memcpy(buffer, &header, sizeof(header));
                //重发该包的ACK
                sendto(sockServ, buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, ClientAddrLen);
                cout << "Send to Clinet ACK:" << (int)header.SEQ << " SEQ:" << (int)header.SEQ << endl;
                continue;//丢弃该数据包
            }
            seq = int(header.SEQ);
            if (seq > 255)
            {
                seq = seq - 256;
            }
            //取出buffer中的内容
            cout << "Send message " << length - sizeof(header) << " bytes!Flag:" << int(header.flag) << " SEQ : " << int(header.SEQ) << " SUM:" << int(header.sum) << endl;
            char* temp = new char[length - sizeof(header)];
            memcpy(temp, buffer + sizeof(header), length - sizeof(header));
            //cout << "size" << sizeof(message) << endl;
            memcpy(message + all, temp, length - sizeof(header));
            all = all + int(header.datasize);

            //返回ACK
            header.flag = ACK;
            header.datasize = 0;
            header.SEQ = (unsigned char)seq;
            header.sum = 0;
            u_short temp1 = checksum((u_short*)&header, sizeof(header));
            header.sum = temp1;
            memcpy(buffer, &header, sizeof(header));
            //重发该包的ACK
            sendto(sockServ, buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, ClientAddrLen);
            cout << "Send to Clinet ACK:" << (int)header.SEQ << " SEQ:" << (int)header.SEQ << endl;
            seq++;
            if (seq > 255)
            {
                seq = seq - 256;
            }
        }

    }
    //over
    header.flag = OVER;
    header.sum = 0;
    u_short temp = checksum((u_short*)&header, sizeof(header));
    header.sum = temp;
    memcpy(buffer, &header, sizeof(header));
    if (sendto(sockServ, buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
    {
        return -1;
    }
    return all;
}
int Fourtimes_Wave(SOCKET& sockServ, SOCKADDR_IN& ClientAddr, int& ClientAddrLen) {
    HEADER header;
    char* Buffer = new char[sizeof(header)];
    while (true)
    {
        int length = recvfrom(sockServ, Buffer, sizeof(header) + MAXSIZE, 0, (sockaddr*)&ClientAddr, &ClientAddrLen);//接收报文长度
        memcpy(&header, Buffer, sizeof(header));
        if (header.flag == FIN && checksum((u_short*)&header, sizeof(header)) == 0)
        {
            cout << "Server接收Client第一次挥手: " << "FIN=1" << endl;
            break;
        }
    }
    //发送第二次挥手信息
    header.flag = ACK;
    header.sum = 0;
    u_short temp = checksum((u_short*)&header, sizeof(header));
    header.sum = temp;
    memcpy(Buffer, &header, sizeof(header));
    if (sendto(sockServ, Buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
    {
        return -1;
    }
    else {
        cout << "Server发送Client第二次挥手: " << "ACK=1" << endl;
    }

    //发送第三次挥手信息
    header.flag = FIN_ACK;
    header.sum = 0;
    temp = checksum((u_short*)&header, sizeof(header));
    header.sum = temp;
    memcpy(Buffer, &header, sizeof(header));
    if (sendto(sockServ, Buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
    {
        return -1;
    }
    else
        cout << "Server发送Client第三次挥手: " << "FIN=1 ACK=1" << endl;
    clock_t start = clock();
    while (recvfrom(sockServ, Buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, &ClientAddrLen) <= 0)
    {
        if (clock() - start > MAX_TIME)
        {
            header.flag = ACK;
            header.sum = 0;
            u_short temp = checksum((u_short*)&header, sizeof(header));
            header.sum = temp;
            memcpy(Buffer, &header, sizeof(header));
            if (sendto(sockServ, Buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
            {
                return -1;
            }
            cout << "第三次挥手超时，正在进行重传" << endl;
            start = clock();
        }
    }

    //接收第四次挥手信息
    HEADER last;
    memcpy(&last, Buffer, sizeof(header));
    if (last.flag == ACK && checksum((u_short*)&last, sizeof(last) == 0))
    {
        cout << "Server接收Client第四次挥手: " << "ACK=1" << endl;
    }
    else
    {
        cout << "发生错误,客户端关闭！" << endl;
        return -1;
    }
}
int main() {
    //定义socket
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);

    SOCKADDR_IN server_addr;
    SOCKADDR_IN router_addr;
    SOCKET server;
    //socket参数赋值
    server_addr.sin_family = AF_INET;//使用IPV4
    server_addr.sin_port = htons(1234);//端口
    server_addr.sin_addr.s_addr = htonl(2130706433);//地址

    router_addr.sin_family = AF_INET;
    router_addr.sin_port = htons(4002);
    router_addr.sin_addr.s_addr = htonl(2130706434);// 127.0.0.2

    server = socket(AF_INET, SOCK_DGRAM, 0);
    bind(server, (SOCKADDR*)&server_addr, sizeof(server_addr));//bind绑定本机端口
    cout << "Server进入bind状态，等待客户端上线" << endl;

    int len = sizeof(server_addr);
    //建立连接，三次握手
    Treetimes_Shake(server, router_addr, len);

    while (true) {
        char* name = new char[20];
        char* data = new char[MAX_DIV_MESSAGE];
        int namelen = RecvMessage(server, router_addr, len, name);
        //memcpy与strcpy不同，所以使用strcmp会出错
        string a;
        for (int i = 0; i < namelen; i++)
        {
            a = a + name[i];
        }
        ofstream fout(a.c_str(), ofstream::binary);
        if (strcmp(a.c_str(), "quit") == 0)
            break;
        while (true) {
            int datalen = RecvMessage(server, router_addr, len, data);
            cout << datalen;
            for (int i = 0; i < datalen; i++)
            {
                fout << data[i];
            }
            cout << "文件已成功下载到本地" << endl;
            memset(data, '\0', MAX_DIV_MESSAGE);

            datalen = RecvMessage(server, router_addr, len, data);
            string a;
            for (int i = 0; i < datalen; i++)
            {
                a = a + data[i];
            }
            if (strcmp(a.c_str(), "Finished") == 0) {
                cout << "报文分组已经结束" << endl;
                break;
            }
            memset(data, '\0', MAX_DIV_MESSAGE);
        }
        fout.close();
        delete[] name;
        delete[] data;
    }
    Fourtimes_Wave(server, router_addr, len);
    system("pause");
}