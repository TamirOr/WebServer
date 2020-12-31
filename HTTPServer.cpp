#include "WebServer.h"


int main()
{
	WSAData wsaData;
	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cout << "HTTP Server: Error at WSAStartup()\n";
		return -1;
	}

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (INVALID_SOCKET == listenSocket)
	{
		cout << "HTTP Server: Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return -1;
	}

	sockaddr_in serverService;
	serverService.sin_family = AF_INET;
	serverService.sin_addr.s_addr = INADDR_ANY;
	serverService.sin_port = htons(PORT);

	if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR *)&serverService, sizeof(serverService)))
	{
		cout << "HTTP Server: Error at bind(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return -1;
	}

	if (SOCKET_ERROR == listen(listenSocket, 5))
	{
		cout << "HTTP Server: Error at listen(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return -1;
	}
	
	addSocket(listenSocket, LISTEN);
	//time check, if connected nore then 180 will disconnected:
	timeOut.tv_sec = TIMEOUT_IN_SEC;
	timeOut.tv_usec = 0;

	while (true)
	{
		//check time:
		for (int i = 1; i < MAX_SOCKETS; i++) {
			time_t currDelay = time(0) - sockets[i].lastModify;
			if ((time(0) - sockets[i].lastModify > TIMEOUT_IN_SEC) && (sockets[i].lastModify != 0)) {
				cout << "HTTP Server: Client #" << sockets[i].id << " timed out." << endl;
				closesocket(sockets[i].id);
				removeSocket(i);
			}
		}

		fd_set waitRecv;
		FD_ZERO(&waitRecv);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE))
				FD_SET(sockets[i].id, &waitRecv);
		}

		fd_set waitSend;
		FD_ZERO(&waitSend);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].send == SEND)
				FD_SET(sockets[i].id, &waitSend);
		}


		int nfd;
		nfd = select(0, &waitRecv, &waitSend, NULL, &timeOut);

		if (nfd == SOCKET_ERROR)
		{
			cout << "HTTP Server: Error at select(): " << WSAGetLastError() << endl;
			WSACleanup();
			return -1;
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitRecv))
			{
				nfd--;
				switch (sockets[i].recv)
				{
				case LISTEN:
					acceptConnection(i);
					break;

				case RECEIVE:
					receiveMessage(i);
					break;
				}
			}
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitSend))
			{
				nfd--;
				switch (sockets[i].send)
				{
				case SEND:
					sendMessage(i);
					break;
				}
			}
		}
	}

	// Closing connections and Winsock.
	cout << "HTTP Server: Closing Connection.\n";
	closesocket(listenSocket);
	WSACleanup();

	return 1;
}

bool addSocket(SOCKET id, int what)
{
	unsigned long flag = 1;
	if (ioctlsocket(id, FIONBIO, &flag) != 0)
	{
		cout << "HTTP Server: Error at ioctlsocket(): " << WSAGetLastError() << endl;
	}

	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			sockets[i].id = id;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			sockets[i].len = 0;
			sockets[i].lastModify = time(0);
			
			socketsCount++;
			return (true);
		}
	}
	return (false);
}

void removeSocket(int index)
{
	cout << "Socket ID# " << sockets[index].id << " removed" << endl;
	sockets[index].id = EMPTY;
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	sockets[index].lastModify = 0;
	socketsCount--;
}

void acceptConnection(int index)
{
	SOCKET id = sockets[index].id;
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	sockets[index].lastModify = time(0);

	SOCKET msgSocket = accept(id, (struct sockaddr *)&from, &fromLen);
	if (INVALID_SOCKET == msgSocket)
	{
		cout << "HTTP Server: Error at accept(): " << WSAGetLastError() << endl;
		return;
	}
	cout << "HTTP Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << endl;
	if (addSocket(msgSocket, RECEIVE) == false)
	{
		cout << "\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
	return;
}

void receiveMessage(int index)
{
	SOCKET msgSocket = sockets[index].id;

	int len = sockets[index].len;
	int bytesRecv = recv(msgSocket, &sockets[index].buffer[len], sizeof(sockets[index].buffer) - len, 0);
	if (SOCKET_ERROR == bytesRecv)
	{
		cout << "HTTP Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}

	sockets[index].lastModify = time(0);
	if (bytesRecv == 0) {
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}
	else
	{
		sockets[index].buffer[len + bytesRecv] = '\0'; //add the null-terminating to make it a string
		cout << "HTTP Server: Recieved: " << bytesRecv << " bytes of \n\"" << &sockets[index].buffer[len] << "\" message.\n";

		sockets[index].len += bytesRecv;

		if (sockets[index].len > 0)
		{
			// Check what method is used for the request and act upon it (OPTIONS, GET, POST, HEAD, PUT, DELETE, TRACE)
			if (strncmp(sockets[index].buffer, GET_METHOD_STRING, strlen(GET_METHOD_STRING)) == 0) { receiveRequest(index, GET_METHOD, GET_METHOD_STRING); return; }
			else if (strncmp(sockets[index].buffer, HEAD_METHOD_STRING, strlen(HEAD_METHOD_STRING)) == 0) { receiveRequest(index, HEAD_METHOD, HEAD_METHOD_STRING); return; }
			else if (strncmp(sockets[index].buffer, OPTIONS_METHOD_STRING, strlen(OPTIONS_METHOD_STRING)) == 0) { receiveRequest(index, OPTIONS_METHOD, OPTIONS_METHOD_STRING); return; }
			else if (strncmp(sockets[index].buffer, PUT_METHOD_STRING, strlen(PUT_METHOD_STRING)) == 0) { receiveRequest(index, PUT_METHOD, PUT_METHOD_STRING); return; }
			else if (strncmp(sockets[index].buffer, POST_METHOD_STRING, strlen(POST_METHOD_STRING)) == 0) { receiveRequest(index, POST_METHOD, POST_METHOD_STRING); return; }
			else if (strncmp(sockets[index].buffer, DELETE_METHOD_STRING, strlen(DELETE_METHOD_STRING)) == 0) { receiveRequest(index, DELETE_METHOD, DELETE_METHOD_STRING); return; }
			else if (strncmp(sockets[index].buffer, TRACE_METHOD_STRING, strlen(TRACE_METHOD_STRING)) == 0) { receiveRequest(index, TRACE_METHOD, TRACE_METHOD_STRING); return; }
			else {
				sockets[index].send = SEND;
				sockets[index].sendMethod = NOT_SUPPORTED;

				closesocket(msgSocket);
				removeSocket(index);
				return;
			}
		}
	}

}

void sendMessage(int index)
{
	int bytesSent = 0;
	char sendBuff[BUFF_SIZE] = { 0 };
	ifstream reqURI;
	string response;
	stringstream filebuffer;
	string URILocation;
	string strHeaders;
	bool checkIfHead = false;
	SOCKET msgSocket = sockets[index].id;
	const char * retCode;
	int contentLen = 0;

	sockets[index].lastModify = time(0);

	if (!ParseMsg(URILocation, strHeaders, index)) {
		response = string("") + VERSION_STRING + " " + CODE_400_BAD_REQUEST + "\r\n";
		response += "Content-type: message/http\r\n";
		response += "Content-length: 0\r\n";
		response += "\r\n";// blank space
	}
	else {
		switch (sockets[index].sendMethod) {
		case (HEAD_METHOD):
			checkIfHead = true;
		case (GET_METHOD):
			reqURI.open(URILocation);

			if (!reqURI.is_open()) {
				cout << "HTTP Server: Couldn't retrieve URI" << endl;
				response = string("") + VERSION_STRING + " " + CODE_404_NOT_FOUND + "\r\n";
				response += "Content-type: message/http\r\n";
				response += "Content-length: 0\r\n";
				response += "\r\n"; // blank space

			}
			else {
				filebuffer << reqURI.rdbuf();
				contentLen = filebuffer.str().length();
				response = string("") + VERSION_STRING + " " + CODE_200_OK + "\r\n"; // initial reponse line (HTTP / 1.0 200 OK)
				response += "Content-type: text/html\r\n";
				response += string("") + "Content-length: " + to_string(contentLen) + "\r\n";
				response += "\r\n"; // blank space
				if (!checkIfHead)
				{
					response += filebuffer.str(); // message
				}
			}

			break;
		case (OPTIONS_METHOD):
			response = string("") + VERSION_STRING + " " + CODE_200_OK + "\r\n"; // initial reponse line (HTTP / 1.0 200 OK)
			response += "Allow: OPTIONS,GET,HEAD,PUT,POST,DELETE,TRACE\r\n"; // headers
			response += "Content-type: message/http\r\n";
			response += "Content-length: 0\r\n";
			response += "\r\n";// blank space

			break;
		case (PUT_METHOD):
			retCode = Put(index, URILocation, strHeaders, sockets[index].buffer, contentLen);

			response = string("") + VERSION_STRING + " " + retCode + "\r\n";
			response += "Content-type: text/html\r\n"; // headers
			response += "Content-length: 0\r\n";
			response += "\r\n"; // blank space

			break;
		case (DELETE_METHOD):

			if (remove(URILocation.c_str()) != 0) 
			{
				cout << "Resource failed to delete: " << URILocation.c_str() << endl;
			}

			response = string("") + VERSION_STRING + " " + CODE_200_OK + "\r\n";
			response += "Content-type: message/http\r\n"; // headers
			response += "Content-length: 0\r\n"; // headers
			response += "\r\n"; // blank space

			break;
		case (TRACE_METHOD):
			response = string("") + VERSION_STRING + " " + CODE_200_OK + "\r\n";
			response += "Content-type: message/http\r\n"; // headers
			response += string("") + "Content-length: " + to_string(sockets[index].len) + "\r\n"; // headers
			response += "\r\n"; // blank space
			response += string().assign(sockets[index].buffer, sockets[index].len);

			break;
		case (POST_METHOD):
			retCode = Post(index, URILocation, strHeaders, sockets[index].buffer, contentLen);
			response = string("") + VERSION_STRING + " " + retCode + "\r\n";
			response += "Content-type: text/html\r\n"; // headers
			response += "Content-length: 0\r\n";
			response += "\r\n"; // blank space
			break;
		default:
			response = string("") + VERSION_STRING + " " + CODE_501_NOT_IMPLEMENTED + "\r\n"; // initial reponse line (HTTP / 1.0 200 OK)
			response += ""; // headers
			response += "\r\n";// blank space
			break;
		}
	}

	response.copy(sendBuff, response.size(), 0);
	sendBuff[response.size()] = '\0';

	bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
	memset(sockets[index].buffer, 0, BUFF_SIZE);
	sockets[index].len = 0;

	if (SOCKET_ERROR == bytesSent)
	{
		cout << "HTTP Server: Error at send(): " << WSAGetLastError() << endl;
		return;
	}

	cout << "HTTP Server: Sent: " << bytesSent << "\\" << strlen(sendBuff) << " bytes of \"" << sendBuff << "\" message.\n";

	sockets[index].send = IDLE;
}



const char* Put(int index, string strURI, string sHeaders, char * msgBuffer, int& outputLen)
{
	const char* Code = CODE_200_OK;
	ofstream output;
	output.open(strURI, ios::in);

	if (!output.is_open()) {
		output.open(strURI, ios::trunc);
		Code = CODE_201_CREATED;
	}

	if (!output.is_open()) {
		cout << "HTTP Server: Couldn't create Resource" << endl;
		return CODE_500_SERVER_ERROR;
	}

	// write the file
	if (msgBuffer[0] == '\0')
	{
		Code = CODE_204_NO_CONTENT;
	}
	else {
		while (*msgBuffer != 0) {
			output << msgBuffer;
			outputLen += strlen(msgBuffer) + 1;
			msgBuffer += outputLen;
		}
	}

	output.close();

	return Code;
}

const char* Post(int index, string strURI, string sHeaders, char * msgBuffer, int& outputLen)
{
	const char* Code = CODE_200_OK;
	ofstream output;
	output.open(strURI, ios::app);


	if (!output.is_open()) {
		cout << "HTTP Server: Couldn't Find Resource" << endl;
		return CODE_500_SERVER_ERROR;
	}

	// write the file
	if (msgBuffer[0] == '\0') 
	{
		Code = CODE_204_NO_CONTENT;
	}
	else
	{
		while (*msgBuffer != 0)
		{
			output << msgBuffer;
			outputLen += strlen(msgBuffer) + 1;
			msgBuffer += outputLen;
		}
	}

	output.close();
	return Code;
}


void receiveRequest(int index, Method type, const char* methodName) 
{
	int len = strlen(methodName);
	sockets[index].send = SEND;
	sockets[index].sendMethod = type;
	memmove(sockets[index].buffer,
		&sockets[index].buffer[len + 1],
		sockets[index].len - (len + 1));
	sockets[index].len -= (len + 1);

	memset(&sockets[index].buffer[sockets[index].len], 0, (len + 1));
}

bool ParseMsg(string &URILocation, string &strHeaders, int index)
{
	char* spacePos = 0;
	int headers_len = 0, filename_len = 0;

	//main folder of path:
	URILocation = ROOT_DIR;
	strHeaders = "";

	// find URI name
	spacePos = strchr(sockets[index].buffer, ' ');

	// If there's no blank space
	if ((spacePos == NULL) || spacePos - sockets[index].buffer > sockets[index].len)
	{
		cout << "HTTP Server: Bad request" << endl;
		return false;
	}

	// Else, get the URI
	filename_len = spacePos - sockets[index].buffer;
	URILocation.append(sockets[index].buffer, filename_len);

	//check if there is query for hebrew page, and if html or text:
	//earse ".fileType", and add "-lang.fileType"
	char* position = strstr(sockets[index].buffer, HEB);
	string::size_type i = URILocation.find(HTML_FILE);
	//html file:
	if (i != string::npos)
	{
		URILocation.erase(i, strlen(".html"));

		if (position)
		{
			URILocation.append("-hebrew.html");
		}
		else
		{
			URILocation.append("-english.html");
		}
	}

	//txtfile:
	else
	{
		
		URILocation.erase((URILocation.find(TXT_FILE)), strlen(TXT_FILE));
		if (position!=nullptr)
		{
			URILocation.append("-hebrew.txt");
		}
		else
		{
			URILocation.append("-english.txt");
		}
	}

	spacePos = strstr(spacePos, "\r\n");
	if ((spacePos == nullptr) || spacePos - sockets[index].buffer > sockets[index].len) {
		cout << "HTTP Server: Bad request" << endl;
		return false;
	}

	spacePos += 2;

	int LineSize = spacePos - sockets[index].buffer;

	memcpy(sockets[index].buffer,&sockets[index].buffer[LineSize], sockets[index].len - (LineSize));
	sockets[index].len -= (LineSize);
	memset(&sockets[index].buffer[sockets[index].len], 0, (LineSize));

	spacePos = strstr(spacePos, "\r\n\r\n");

	// if there're headers
	if ((spacePos != NULL) &&
		spacePos - sockets[index].buffer <= sockets[index].len) {

		headers_len = spacePos - sockets[index].buffer + 4;
		strHeaders.append(sockets[index].buffer, headers_len);;

		memcpy(sockets[index].buffer,
			&sockets[index].buffer[headers_len],
			sockets[index].len - (headers_len));
		sockets[index].len -= (headers_len);
		memset(&sockets[index].buffer[sockets[index].len], 0, (headers_len));
	}

	return true;
}