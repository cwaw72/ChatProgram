/*
 * chat_server_win.c
 * VidualStudio���� console���α׷����� �ۼ��� multithread-DLL�� �����ϰ�
 *  ���϶��̺귯��(ws2_32.lib)�� �߰��� ��.
 */

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>

#define BUFSIZE 100

DWORD WINAPI ClientConn(void *arg);
void SendMSG(char* message, int len);
void ErrorHandling(char *message);

//Linked list
typedef struct Node {
	char name[20];
	struct Node* NextNode;
}node;

int clntNumber=0;
SOCKET clntSocks[10];			//10�� �ްڴ�.

node* pp = NULL;

HANDLE hMutex;					//�������� ��� ���ؽ� ���

//Linked list
node* SLL_CreateNode(char* mname) {
	node* NewNode = (node*)malloc(sizeof(node));
	
	strcpy(NewNode->name,mname);
	NewNode->NextNode = NULL;


	return NewNode;
}

void SLL_DestroyNode(node* n) {
	free(n);
}

void SLL_AppendNode(node** Head, node* NewNode) {
	//��� ��尡 NULL �̶�� ���ο� ��尡 Head
	if ((*Head) == NULL) {
		*Head = NewNode;
	}
	else {
		//������ ã�� NewNode�� ����
		node* Tail = (*Head);
		while(Tail->NextNode != NULL) {
			Tail = Tail->NextNode;
		}
		Tail->NextNode = NewNode;
	}
}

node* SSL_GetNodeAt(node* Head, char* sname) {
	node* Current = Head;

	//ã���� ����
	while (Current != NULL && strcmp(Current->name,sname)) {
		Current = Current->NextNode;
	}
	return Current;
}

void SLL_RemoveNode(node** Head, node* Remove) {

	if(Remove == NULL) return;

	if (*Head == Remove) {
		*Head = Remove->NextNode;
	}
	else {
		node* Current = *Head;
		while (Current != NULL && Current->NextNode != Remove) {
			Current = Current->NextNode;
		}

		if (Current != NULL)
			Current->NextNode = Remove->NextNode;
	}
}

int SSL_GetNodeCount(node* Head) {

	int count = 0;
	node* Current = Head;


	while (Current != NULL) {
		Current = Current->NextNode;
		count++;
	}
	return count;
}

int SSL_GetNodeSearch(node* Head, char* sname) {
	
	int count = 0;
	node* Current = Head;

	while (Current != NULL) {
		if (!strcmp(Current->name, sname))
			return count;
		Current = Current->NextNode;
		count++;
	}
	return -1;
}

int SSL_MadeNodeMemnerMessage(node* Head, char* fmessage) {

	int lenth = 0;
	node* Current = Head;
	lenth = sprintf(fmessage, " *���� ��� ���* \n");

	while (Current != NULL) {
		lenth += sprintf(fmessage + lenth, " [%s]\n", Current->name);
		Current = Current->NextNode;
	}
	return lenth;
}

int main(int argc, char **argv)
{
  WSADATA wsaData;
  SOCKET servSock;
  SOCKET clntSock;

  SOCKADDR_IN servAddr;
  SOCKADDR_IN clntAddr;
  int clntAddrSize;

  HANDLE hThread;
  DWORD dwThreadID;

  if(argc!=2){						  // ����üŷ!  
    printf("Usage : %s <port>\n", argv[0]);
    exit(1);
  }
  if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) /* Load Winsock 2.2 DLL */		//���̺귯�� �ʱ�ȭ ��ɾ� ���轺���� ������� ����
	  ErrorHandling("WSAStartup() error!");

  hMutex=CreateMutex(NULL, FALSE, NULL);		//���ؽ� ���� ���� �� �ʱ�ȭ
  if(hMutex==NULL){
	  ErrorHandling("CreateMutex() error");
  }

  servSock=socket(PF_INET, SOCK_STREAM, 0);		// ���� �Լ� �Ű����� (1.���ͳݰ迭�� ��������, 2.TCP �������� ����ϰڴ�. ä���� �����ؼ� ���������� Ŭ���ϰڴ�.) 
  if(servSock == INVALID_SOCKET)
    ErrorHandling("socket() error");
 
  memset(&servAddr, 0, sizeof(servAddr));		//���ͳ� �ּ� ����ü�� �ʱ�ȭ �ϴ� �κ� (�������� ���ͳ� ����)
  servAddr.sin_family=AF_INET;					//� ���ͳ� �йи��� ������ ���ͳݿ뵵�� ����ϰڴ�.
  servAddr.sin_addr.s_addr=htonl(INADDR_ANY);	
  servAddr.sin_port=htons(atoi(argv[1]));		//��Ʈ�ѹ� �Է� 9000�̶�� ���ڰ� ���⿡ �ش�ȴ�. htons ���� �����̳� : Host to network in shot
												//cpu�� �°� ���÷� ����ض�. �߿��ϴٳ׿�

  if(bind(servSock, (SOCKADDR*) &servAddr, sizeof(servAddr))==SOCKET_ERROR)
    ErrorHandling("bind() error");

  if(listen(servSock, 5)==SOCKET_ERROR)
    ErrorHandling("listen() error");

  while(1){
	  clntAddrSize=sizeof(clntAddr);
	  clntSock=accept(servSock, (SOCKADDR*)&clntAddr, &clntAddrSize);	//�� �մԸ��� ���� ������ ���� ������ < Ŭ���̾�Ʈ ���� 1:1 �� ����� >
	  
	  if(clntSock==INVALID_SOCKET)
		  ErrorHandling("accept() error");

	  WaitForSingleObject(hMutex, INFINITE);		//���ؽ��� �̿��ؼ� ��ȣ������Ѵ�!
	  //ũ��Ƽ�� ����!
	  clntSocks[clntNumber++]=clntSock;	// clntSock������ ����Ѵٴ� �̾߱� 


	  ReleaseMutex(hMutex);							//Ǯ�����

	  //ũ��Ƽ�� ����!
	  printf("���ο� ����, Ŭ���̾�Ʈ IP : %s \n", inet_ntoa(clntAddr.sin_addr));

	  hThread = (HANDLE)_beginthreadex(NULL, 0, ClientConn, (void*)clntSock, 0, (unsigned *)&dwThreadID);	//ClientConn  �� �Լ��� ���� �϶�
	  if(hThread == 0) {
		  ErrorHandling("������ ���� ����");
	  }
  }

  WSACleanup();
  return 0;
}

DWORD WINAPI ClientConn(void *arg)
{
  SOCKET clntSock=(SOCKET)arg;
  int strLen=0;
  char message[BUFSIZE];
  int i;
  char strTemp[100];
  char fmessage[120];
  char* name = NULL;
  char* main1 = NULL;
  char* main2 = NULL;
  char* main3 = NULL;
  node* nNode = NULL;
  node* dNode = NULL;
  int lenth = 0;


  while ((strLen = recv(clntSock, message, BUFSIZE, 0)) != 0) {		// �޼����� ������ sendMSG�� ���� ��ε� �ɽ�Ʈ��
  
	  strcpy(strTemp,message);

	  //���ڿ� �տ��� �ڷ� ��ġ�Ǵ� ���� �˻�
	  name = strtok(strTemp, "[]");  //�г���		
	  main1 = strtok(NULL, " <>");	  //����1	
	  main2 = strtok(NULL, "<>");	  //����2
	  main3 = strtok(NULL, "<>");	  //����3

	  //printf("�̸� : %s ��ȭ :%s %d \n",name , strTemp2, strncmp(strTemp2, " @@", 2));
	  if(name != NULL)
		  if (!strncmp(main1, "@@join", 6) ){		  
		
			  if (name == NULL) continue;

			  //��ũ�� ����ũ ��� �߰�
			  nNode = SLL_CreateNode(name);
			  SLL_AppendNode(&pp, nNode);

			  //printf("���� %d , %s�� �߰��Ǿ����ϴ�.", strLen, pp->name);

			  lenth = sprintf(fmessage, " [%s] ���̵� ��ϵǾ����ϴ�.\n", name);		//���� �޼��� ����
			  SendMSG(fmessage, lenth);
		  }
		  else if (!strncmp(main1, "@@out", 5)) {
			  lenth = sprintf(fmessage, " [%s] ���̴ٰ� Ż���Ͽ����ϴ�.\n", name);	//���� �޼��� ����
		  
			  dNode = SSL_GetNodeAt(pp, name);
			  SLL_RemoveNode(&pp, dNode);
			  SLL_DestroyNode(dNode);		  

			  SendMSG(fmessage, lenth);
			  break;
		  }
		  else if (!strncmp(main1, "@@member", 8)) {

			  lenth = SSL_MadeNodeMemnerMessage(pp, &fmessage);
		
			  WaitForSingleObject(hMutex, INFINITE);
			  send(clntSock, fmessage, lenth, 0);
			  ReleaseMutex(hMutex);
		  }
		  else if (!strncmp(main1, "@@talk", 6) ){
			  //printf("%d ��° �ֳ׿�. \n", SSL_GetNodeSearch(pp, main2));
	
			  lenth = sprintf(fmessage, "<%s> <%s>\n", name,main3);		//���� �޼��� ����

			  WaitForSingleObject(hMutex, INFINITE);
			  send(clntSocks[SSL_GetNodeSearch(pp, main2)], fmessage, lenth, 0);
			  ReleaseMutex(hMutex);

		  }
		  else
			SendMSG(message, strLen);


  }

  WaitForSingleObject(hMutex, INFINITE);						//�޼��� �����°��� ������ �ֵ�
  for (i = 0; i<clntNumber; i++) {   // Ŭ���̾�Ʈ ���� �����		//������ ���� ������ ���� �����
	  if (clntSock == clntSocks[i]) {		//clntSocks[i] ��� ���� ����Ǿ�����
		  for (; i<clntNumber - 1; i++)
			  clntSocks[i] = clntSocks[i + 1];
		  break;
	  }
  }
  clntNumber--;
  ReleaseMutex(hMutex);

  closesocket(clntSock);
  return 0;
}

void SendMSG(char* message, int len)
{
	int i;
	WaitForSingleObject(hMutex, INFINITE); 
	for(i=0; i<clntNumber; i++)
		send(clntSocks[i], message, len, 0);
	ReleaseMutex(hMutex);	
}

void ErrorHandling(char *message)
{
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(1);
}
