#include <iostream>
#include <WINSOCK2.h>
#include <time.h>
#include <fstream>
#pragma comment(lib, "ws2_32.lib")
#define MAX_DIV_MESSAGE  10000000 //�ļ�һ���Զ�ȡ�����size
using namespace std;

const int MAXSIZE = 1024;//���仺������󳤶�
const unsigned char SYN = 0x1; //SYN = 1 ACK = 0
const unsigned char ACK = 0x2;//SYN = 0, ACK = 1
const unsigned char ACK_SYN = 0x3;//SYN = 1, ACK = 1
const unsigned char FIN = 0x4;//FIN = 1 ACK = 0
const unsigned char FIN_ACK = 0x5;//FIN = 1 ACK = 0
const unsigned char OVER = 0x7;//������־
double MAX_TIME = 0.5 * CLOCKS_PER_SEC;
//UDPͷ
struct HEADER
{
    u_short sum = 0;//У��� 16λ
    u_short datasize = 0;//���������ݳ��� 16λ
    unsigned char flag = 0;
    //��λ��ʹ�ú���λ��������FIN ACK SYN 
    unsigned char SEQ = 0;
    //��λ����������кţ�0~255��������mod
    HEADER() {
        sum = 0;//У��� 16λ
        datasize = 0;//���������ݳ��� 16λ
        flag = 0;
        //��λ��ʹ�ú���λ��������FIN ACK SYN 
        SEQ = 0;
    }
};

//UDP����͵ļ��㷽���ǣ�
//��16λ������ͣ����㣬�����ȡ��д��У���
u_short checksum(u_short* message, int size) { //size Ϊ���ٸ��ֽ�
    //num   ��λ��u_short 16 bit
    int num = (size + 1) / 2;
    u_short* buf = (u_short*)malloc(size + 1);
    memset(buf, 0, size + 1);
    memcpy(buf, message, size);
    u_long sum = 0;

    //������ͣ����ȡ��
    while (num--) {
        sum += *buf++;
        if (sum & 0xffff0000) {
            sum &= 0xffff;
            sum++;
        }
    }
    return ~(sum & 0xffff);
}
//�������ӣ���������
int Treetimes_Shake(SOCKET& sockServ, SOCKADDR_IN& ClientAddr, int& ClientAddrLen)
{
    HEADER header;
    char* buffer = new char[sizeof(header)];
    //����SYN=1����һ������;
    while (true) {
        if (recvfrom(sockServ, buffer, sizeof(header), 0, (SOCKADDR*)&ClientAddr, &ClientAddrLen) == -1) {
            return -1;//����SYNʧЧ
        }
        memcpy(&header, buffer, sizeof(header));//sizeof(buffer)
        //SYN==1,����У�����ȷ
        //���е�16λ������ӣ������16λȫΪ1����û�г���
        if (header.flag == SYN && checksum((u_short*)&header, sizeof(header)) == 0) {
            cout << "Server����Client�ĵ�һ�����֣�SYN=1" << endl;
            break;
        }
    }
    //Server����SYN==1��ACK==1
    header.flag = ACK_SYN;
    header.sum = 0;
    u_short temp = checksum((u_short*)&header, sizeof(header));
    header.sum = temp;//����У���
    //д����Ϣ
    memcpy(buffer, &header, sizeof(header));
    if (sendto(sockServ, buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
    {
        return -1;//����SYN��ACKʧЧ
    }
    else
        cout << "Server����Client�ĵڶ������֣�SYN=1 ACK=1" << endl;
    clock_t start = clock();//��¼ʱ��

    while (recvfrom(sockServ, buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, &ClientAddrLen) <= 0)
    {
        //��ʱ�ش�
        if (clock() - start > MAX_TIME) {
            header.flag = ACK_SYN;
            header.sum = 0;
            u_short temp = checksum((u_short*)&header, sizeof(header));
            header.sum = temp;//����У���
            memcpy(buffer, &header, sizeof(header));
            if (sendto(sockServ, buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
            {
                return -1;//����SYN��ACKʧЧ
            }
            cout << "�ڶ�������ʧ�ܣ���ʱ�ش�������" << endl;
        }
    }

    HEADER lastshake;
    memcpy(&lastshake, buffer, sizeof(lastshake));
    if (lastshake.flag = ACK && checksum((u_short*)&lastshake, sizeof(lastshake)) == 0) {
        cout << "Server����Client�ĵ��������֣�ACK=1" << endl;
    }
    else
    {
        cout << "���������ַ��������������ͻ���" << endl;
    }
    return 1;//������������
}

//�����ļ�
int RecvMessage(SOCKET& sockServ, SOCKADDR_IN& ClientAddr, int& ClientAddrLen, char* message)
{
    long int all = 0;//�ļ�����
    HEADER header;
    char* buffer = new char[MAXSIZE + sizeof(header)];
    int seq = 0;
    int index = 0;
    while (true) {
        int length = recvfrom(sockServ, buffer, sizeof(header) + MAXSIZE, 0, (sockaddr*)&ClientAddr, &ClientAddrLen);//���ձ��ĳ���
        memcpy(&header, buffer, sizeof(header));
        //�ж��Ƿ��ǽ���
        if (header.flag == OVER && checksum((u_short*)&header, sizeof(header)) == 0)
        {
            cout << "�ļ��������" << endl;
            break;
        }
        if (header.flag == unsigned char(0) && checksum((u_short*)buffer, length - sizeof(header))) {

            //�ж��Ƿ��Ǳ�İ�
            if (seq != int(header.SEQ)) {
                //˵���������⣬����ACK
                header.flag = ACK;
                header.datasize = 0;
                header.SEQ = (unsigned char)seq;
                header.sum = 0;
                u_short temp = checksum((u_short*)&header, sizeof(header));
                header.sum = temp;
                memcpy(buffer, &header, sizeof(header));
                //�ط��ð���ACK
                sendto(sockServ, buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, ClientAddrLen);
                cout << "Send to Clinet ACK:" << (int)header.SEQ << " SEQ:" << (int)header.SEQ << endl;
                continue;//���������ݰ�
            }
            seq = int(header.SEQ);
            if (seq > 255)
            {
                seq = seq - 256;
            }
            //ȡ��buffer�е�����
            cout << "Send message " << length - sizeof(header) << " bytes!Flag:" << int(header.flag) << " SEQ : " << int(header.SEQ) << " SUM:" << int(header.sum) << endl;
            char* temp = new char[length - sizeof(header)];
            memcpy(temp, buffer + sizeof(header), length - sizeof(header));
            //cout << "size" << sizeof(message) << endl;
            memcpy(message + all, temp, length - sizeof(header));
            all = all + int(header.datasize);

            //����ACK
            header.flag = ACK;
            header.datasize = 0;
            header.SEQ = (unsigned char)seq;
            header.sum = 0;
            u_short temp1 = checksum((u_short*)&header, sizeof(header));
            header.sum = temp1;
            memcpy(buffer, &header, sizeof(header));
            //�ط��ð���ACK
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
        int length = recvfrom(sockServ, Buffer, sizeof(header) + MAXSIZE, 0, (sockaddr*)&ClientAddr, &ClientAddrLen);//���ձ��ĳ���
        memcpy(&header, Buffer, sizeof(header));
        if (header.flag == FIN && checksum((u_short*)&header, sizeof(header)) == 0)
        {
            cout << "Server����Client��һ�λ���: " << "FIN=1" << endl;
            break;
        }
    }
    //���͵ڶ��λ�����Ϣ
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
        cout << "Server����Client�ڶ��λ���: " << "ACK=1" << endl;
    }

    //���͵����λ�����Ϣ
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
        cout << "Server����Client�����λ���: " << "FIN=1 ACK=1" << endl;
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
            cout << "�����λ��ֳ�ʱ�����ڽ����ش�" << endl;
            start = clock();
        }
    }

    //���յ��Ĵλ�����Ϣ
    HEADER last;
    memcpy(&last, Buffer, sizeof(header));
    if (last.flag == ACK && checksum((u_short*)&last, sizeof(last) == 0))
    {
        cout << "Server����Client���Ĵλ���: " << "ACK=1" << endl;
    }
    else
    {
        cout << "��������,�ͻ��˹رգ�" << endl;
        return -1;
    }
}
int main() {
    //����socket
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);

    SOCKADDR_IN server_addr;
    SOCKADDR_IN router_addr;
    SOCKET server;
    //socket������ֵ
    server_addr.sin_family = AF_INET;//ʹ��IPV4
    server_addr.sin_port = htons(1234);//�˿�
    server_addr.sin_addr.s_addr = htonl(2130706433);//��ַ

    router_addr.sin_family = AF_INET;
    router_addr.sin_port = htons(4002);
    router_addr.sin_addr.s_addr = htonl(2130706434);// 127.0.0.2

    server = socket(AF_INET, SOCK_DGRAM, 0);
    bind(server, (SOCKADDR*)&server_addr, sizeof(server_addr));//bind�󶨱����˿�
    cout << "Server����bind״̬���ȴ��ͻ�������" << endl;

    int len = sizeof(server_addr);
    //�������ӣ���������
    Treetimes_Shake(server, router_addr, len);

    while (true) {
        char* name = new char[20];
        char* data = new char[MAX_DIV_MESSAGE];
        int namelen = RecvMessage(server, router_addr, len, name);
        //memcpy��strcpy��ͬ������ʹ��strcmp�����
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
            cout << "�ļ��ѳɹ����ص�����" << endl;
            memset(data, '\0', MAX_DIV_MESSAGE);

            datalen = RecvMessage(server, router_addr, len, data);
            string a;
            for (int i = 0; i < datalen; i++)
            {
                a = a + data[i];
            }
            if (strcmp(a.c_str(), "Finished") == 0) {
                cout << "���ķ����Ѿ�����" << endl;
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