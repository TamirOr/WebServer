#pragma once


#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <fstream>
#include <sstream>



const char * VERSION_STRING = "HTTP/1.1";
const char * ROOT_DIR = "C:/webServer";
const int BUFF_SIZE = 2048;
const int TIMEOUT_IN_SEC = 180;

//socket struct:
struct SocketState
{
	SOCKET id;			// Socket handle
	int	recv;			// Receiving?
	int	send;			// Sending?
	int sendMethod;	// Sending sub-type
	char buffer[BUFF_SIZE];
	int len;
	time_t lastModify;
};

const int PORT = 8080;
const int MAX_SOCKETS = 60;
const int EMPTY = 0;
const int LISTEN = 1;
const int RECEIVE = 2;
const int IDLE = 3;
const int SEND = 4;

//Enum and const for Method:
typedef enum
{
	GET_METHOD,
	OPTIONS_METHOD,
	HEAD_METHOD,
	PUT_METHOD,
	POST_METHOD,
	DELETE_METHOD,
	TRACE_METHOD,
	NOT_SUPPORTED
} Method;


const char* const GET_METHOD_STRING = "GET";
const char* const OPTIONS_METHOD_STRING = "OPTIONS";
const char* const HEAD_METHOD_STRING = "HEAD";
const char* const PUT_METHOD_STRING = "PUT";
const char* const POST_METHOD_STRING = "POST";
const char* const DELETE_METHOD_STRING = "DELETE";
const char* const TRACE_METHOD_STRING = "TRACE";


//const for CODE:
const char* const CODE_200_OK = "200 OK";
const char* const CODE_201_CREATED = "201 Created";
const char* const CODE_204_NO_CONTENT = "204 No Content";
const char* const CODE_301_MOVED_PERMANENTLY = "301 Moved Permanently";
const char* const CODE_400_BAD_REQUEST = "400 Bad Request";
const char* const CODE_404_NOT_FOUND = "404 Not Found";
const char* const CODE_412_PRECONDITION_FAILED = "412 Precondition failed";
const char* const CODE_500_SERVER_ERROR = "500 Server Error";
const char* const CODE_501_NOT_IMPLEMENTED = "501 Not Implemented";


const char* const TXT_FILE = ".txt";
const char* const HTML_FILE = ".html";
const char* const HEB = "he-IL";
const char* const ENG = "en-US";


bool addSocket(SOCKET id, int what);
void removeSocket(int index);
void acceptConnection(int index);
void receiveMessage(int index);
void sendMessage(int index);
void receiveRequest(int index, Method type, const char* string);
bool ParseMsg(string &URILocation, string &strHeaders, int index);
const char* Put(int index, string strURI, string sHeaders, char * msgBuf, int& outputLen);
const char* Post(int index, string strURI, string sHeaders, char * msgBuf, int& outputLen);

struct SocketState sockets[MAX_SOCKETS] = { 0 };
int socketsCount = 0;
struct timeval timeOut;