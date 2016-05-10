/*
 * chat_client_win.c
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
#define NAMESIZE 20

DWORD WINAPI SendMSG(void *arg);
DWORD WINAPI RecvMSG(void *arg);
void ErrorHandling(char *message);

char name[NAMESIZE]="[Default]";
char message[BUFSIZE];
char fmessage[120];			//first message
int first = 0;

int main(int argc, char **argv)
{
  WSADATA wsaData;
  SOCKET sock;
  SOCKADDR_IN servAddr;
  
  HANDLE hThread1, hThread2;
  DWORD dwThreadID1, dwThreadID2;

  if(argc!=4){
    printf("Usage : %s <IP> <port> <name>\n", argv[0]);
    exit(1);
  }
  if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) /* Load Winsock 2.2 DLL */
	  ErrorHandling("WSAStartup() error!");


  //처음 접속시 아이디 입력
  printf(" 당신의 아이디를 입력하세요:");
  scanf("%s", argv[3]);
  sprintf(name, "[%s]", argv[3]); 
  sprintf(fmessage, "%s @@join <%s>", name, argv[3]);

  sock=socket(PF_INET, SOCK_STREAM, 0);
  if(sock == INVALID_SOCKET)
    ErrorHandling("socket() error");

  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family=AF_INET;
  servAddr.sin_addr.s_addr=inet_addr(argv[1]);
  servAddr.sin_port=htons(atoi(argv[2]));

  if(connect(sock, (SOCKADDR*)&servAddr, sizeof(servAddr))==SOCKET_ERROR)
	  ErrorHandling("connect() error");
  
  hThread1 = (HANDLE)_beginthreadex(NULL, 0, SendMSG, (void*)sock, 0, (unsigned *)&dwThreadID1); 
  hThread2 = (HANDLE)_beginthreadex(NULL, 0, RecvMSG, (void*)sock, 0, (unsigned *)&dwThreadID2);
  if(hThread1==0 || hThread2==0) {
	  ErrorHandling("쓰레드 생성 오류");
  }
  
  WaitForSingleObject(hThread1, INFINITE);
  WaitForSingleObject(hThread2, INFINITE);
 
  closesocket(sock);
  return 0;
}

DWORD WINAPI SendMSG(void *arg) // 메시지 전송 쓰레드 실행 함수
{
   SOCKET sock = (SOCKET)arg;
   char nameMessage[NAMESIZE+BUFSIZE];
   char* to = NULL;
   char* main = NULL;

   send(sock, fmessage, strlen(fmessage), 0);

   while(1) {
      fgets(message, BUFSIZE, stdin);
      sprintf(nameMessage,"%s %s", name, message);
 
	  if(!strncmp(message,"&quit",5)) {  // 'quit' 입력시 종료
		  sprintf(fmessage, "%s @@out", name);
		  send(sock, fmessage, strlen(fmessage), 0);
		  closesocket(sock);
         exit(0);		 
      }
	  else if(!strncmp(message, "&list",5)) {
		  sprintf(fmessage, "%s @@member", name);
		  send(sock, fmessage, strlen(fmessage), 0);
	  }
	  else if (!strncmp(message, "&p2p", 4)) {
		  strtok(message, "<>");
		  to = strtok(NULL, "<>");	  //닉네임
		  strtok(NULL, "<>");
		  main = strtok(NULL, "<>");	  //내용1
		

		  sprintf(fmessage, "%s @@talk <%s><%s>", name, to, main);
		  send(sock, fmessage, strlen(fmessage), 0);
	  }
	  else if(first)send(sock, nameMessage, strlen(nameMessage), 0);
	  first = 1;
   }
}

DWORD WINAPI RecvMSG(void *arg) /* 메시지 수신 쓰레드 실행 함수 */
{
  SOCKET sock = (SOCKET)arg;
  char nameMessage[NAMESIZE+BUFSIZE];
  int strLen;
  while(1){
    strLen = recv(sock, nameMessage, NAMESIZE+BUFSIZE-1, 0);
	if(strLen==-1) return 1;
	
    nameMessage[strLen]=0;
    fputs(nameMessage, stdout);
  }
}

void ErrorHandling(char *message)
{
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(1);
}
