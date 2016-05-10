/*
 * chat_server_win.c
 * VidualStudio에서 console프로그램으로 작성시 multithread-DLL로 설정하고
 *  소켓라이브러리(ws2_32.lib)를 추가할 것.
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
SOCKET clntSocks[10];			//10명 받겠다.

node* pp = NULL;

HANDLE hMutex;					//세마포어 대신 뮤텍스 사용

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
	//헤드 노드가 NULL 이라면 새로운 노드가 Head
	if ((*Head) == NULL) {
		*Head = NewNode;
	}
	else {
		//테일을 찾아 NewNode를 연결
		node* Tail = (*Head);
		while(Tail->NextNode != NULL) {
			Tail = Tail->NextNode;
		}
		Tail->NextNode = NewNode;
	}
}

node* SSL_GetNodeAt(node* Head, char* sname) {
	node* Current = Head;

	//찾으면 멈춤
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
	lenth = sprintf(fmessage, " *현재 모든 멤버* \n");

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

  if(argc!=2){						  // 에러체킹!  
    printf("Usage : %s <port>\n", argv[0]);
    exit(1);
  }
  if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) /* Load Winsock 2.2 DLL */		//라이브러리 초기화 명령어 리룩스에선 사용하지 않음
	  ErrorHandling("WSAStartup() error!");

  hMutex=CreateMutex(NULL, FALSE, NULL);		//뮤텍스 변수 생성 및 초기화
  if(hMutex==NULL){
	  ErrorHandling("CreateMutex() error");
  }

  servSock=socket(PF_INET, SOCK_STREAM, 0);		// 소켓 함수 매개변수 (1.인터넷계열의 프로토콜, 2.TCP 프로토콜 사용하겠다. 채널을 개설해서 연속적으로 클릭하겠다.) 
  if(servSock == INVALID_SOCKET)
    ErrorHandling("socket() error");
 
  memset(&servAddr, 0, sizeof(servAddr));		//인터넷 주소 구조체를 초기화 하는 부분 (서버측의 인터넷 구조)
  servAddr.sin_family=AF_INET;					//어떤 인터넷 패밀리냐 소켓을 인터넷용도로 사용하겠다.
  servAddr.sin_addr.s_addr=htonl(INADDR_ANY);	
  servAddr.sin_port=htons(atoi(argv[1]));		//포트넘버 입력 9000이라는 숫자가 여기에 해당된다. htons 무슨 약자이냐 : Host to network in shot
												//cpu에 맞게 로컬로 사용해라. 중요하다네요

  if(bind(servSock, (SOCKADDR*) &servAddr, sizeof(servAddr))==SOCKET_ERROR)
    ErrorHandling("bind() error");

  if(listen(servSock, 5)==SOCKET_ERROR)
    ErrorHandling("listen() error");

  while(1){
	  clntAddrSize=sizeof(clntAddr);
	  clntSock=accept(servSock, (SOCKADDR*)&clntAddr, &clntAddrSize);	//그 손님만을 위한 소켓을 만들어서 보내줌 < 클라이언트 소켓 1:1 로 연결됨 >
	  
	  if(clntSock==INVALID_SOCKET)
		  ErrorHandling("accept() error");

	  WaitForSingleObject(hMutex, INFINITE);		//뮤텍스를 이용해서 보호해줘야한다!
	  //크리티컬 섹션!
	  clntSocks[clntNumber++]=clntSock;	// clntSock변수를 등록한다는 이야기 


	  ReleaseMutex(hMutex);							//풀러쥬기

	  //크리티컬 섹션!
	  printf("새로운 연결, 클라이언트 IP : %s \n", inet_ntoa(clntAddr.sin_addr));

	  hThread = (HANDLE)_beginthreadex(NULL, 0, ClientConn, (void*)clntSock, 0, (unsigned *)&dwThreadID);	//ClientConn  이 함수를 실행 하라
	  if(hThread == 0) {
		  ErrorHandling("쓰레드 생성 오류");
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


  while ((strLen = recv(clntSock, message, BUFSIZE, 0)) != 0) {		// 메세지가 들어오면 sendMSG를 통해 브로드 케스트함
  
	  strcpy(strTemp,message);

	  //문자열 앞에서 뒤로 일치되는 문자 검색
	  name = strtok(strTemp, "[]");  //닉네임		
	  main1 = strtok(NULL, " <>");	  //내용1	
	  main2 = strtok(NULL, "<>");	  //내용2
	  main3 = strtok(NULL, "<>");	  //내용3

	  //printf("이름 : %s 대화 :%s %d \n",name , strTemp2, strncmp(strTemp2, " @@", 2));
	  if(name != NULL)
		  if (!strncmp(main1, "@@join", 6) ){		  
		
			  if (name == NULL) continue;

			  //링크드 리스크 노드 추가
			  nNode = SLL_CreateNode(name);
			  SLL_AppendNode(&pp, nNode);

			  //printf("길이 %d , %s가 추가되었습니다.", strLen, pp->name);

			  lenth = sprintf(fmessage, " [%s] 아이디가 등록되었습니다.\n", name);		//보낼 메세지 설정
			  SendMSG(fmessage, lenth);
		  }
		  else if (!strncmp(main1, "@@out", 5)) {
			  lenth = sprintf(fmessage, " [%s] 아이다가 탈퇴하였습니다.\n", name);	//보낼 메세지 설정
		  
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
			  //printf("%d 번째 있네요. \n", SSL_GetNodeSearch(pp, main2));
	
			  lenth = sprintf(fmessage, "<%s> <%s>\n", name,main3);		//보낼 메세지 설정

			  WaitForSingleObject(hMutex, INFINITE);
			  send(clntSocks[SSL_GetNodeSearch(pp, main2)], fmessage, lenth, 0);
			  ReleaseMutex(hMutex);

		  }
		  else
			SendMSG(message, strLen);


  }

  WaitForSingleObject(hMutex, INFINITE);						//메세지 보내는것이 실패한 애들
  for (i = 0; i<clntNumber; i++) {   // 클라이언트 연결 종료시		//포문을 통해 나머지 소켓 때어넴
	  if (clntSock == clntSocks[i]) {		//clntSocks[i] 모든 소켓 저장되어있음
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
