/* IRC handle script by XXLTomate */
#include "irc.h"

#ifdef WIN32
#include <windows.h>
#include <winsock.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <malloc.h>
//#include <stdarg.h>

#include <base/system.h>
#include <engine/shared/config.h>

int IRC::Connect(const char *remote_host_p, unsigned short remote_port)
{
	struct sockaddr_in sin;    /* a structure which tells our socket where it's connecting to */
	struct hostent *hostent_p; /* a structure which will store results from the DNS query we do
								  for remote_host_p */

	/* perform a DNS query to find the IP address of remote_host_p */
	if (!(hostent_p = gethostbyname(remote_host_p)))
		return 0;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = PF_INET; /* IPv4 */
	sin.sin_addr.s_addr = *(long *) hostent_p->h_addr; /* take the IP address returned */
	sin.sin_port = htons(remote_port); /* convert remote_port to a network order byte */

	if (g_Config.m_IRCDebug)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), " - Resolved %s to %s\r\n", remote_host_p, inet_ntoa(sin.sin_addr));
		dbg_msg("IRC", aBuf);
	}

	//connect with socket
	if (connect(m_Sock, (struct sockaddr *) &sin, sizeof(sin)) == -1)
		return 0;

	return 1;
}

int IRC::SendLine(const char *format_p, ...)
{
	va_list args;
	char buffer[512] = {0};

	va_start(args, format_p);
	vsnprintf(buffer, sizeof(buffer), format_p, args);
	va_end(args);

	strncat(buffer, "\r\n", (sizeof(buffer) - strlen(buffer)));

	if (g_Config.m_IRCDebug)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), ">> %s", buffer);
		dbg_msg("IRC", aBuf);
	}

	return (send(m_Sock, buffer, strlen(buffer), 0));
}

int IRC::RecvLine(char *line_p, unsigned int line_size)
{
	char byte = 0;
	/* recv one byte at a time from the socket
	untill you reach a newline (\n) character */

	while (byte != '\n' && strlen(line_p) < line_size)
	{
		if (!recv(m_Sock, (char *) &byte, 1, 0))
			return 0;

		if (byte != '\r' && byte != '\n' && byte != '\0')
		{
			strncat(line_p, (char *) &byte, 1);
		}
	}

	if (g_Config.m_IRCDebug)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "<< %s", line_p);
		dbg_msg("IRC", aBuf);
	}

	return 1;
}

void IRC::Send(const char * pMsg)
{
	SendLine("PRIVMSG %s :%s", m_IRCData.m_Channel, pMsg);
}

void IRC::Names()
{
	SendLine("NAMES %s", m_IRCData.m_Channel);
}

void IRC::Quit()
{
	SendLine("QUIT");
}

void IRC::Topic()
{
	SendLine("TOPIC %s", m_IRCData.m_Channel);
}

void IRC::MainParser(char *pOut)
{
	char aBuf[1024];
	strcpy(pOut, "");
	memset(m_Buffer, 0, sizeof(m_Buffer));

	if (RecvLine(m_Buffer, sizeof(m_Buffer)) == 0)
		return;

	pToken = strtok(m_Buffer, " ");
	m_ArgumentCount = 0;

	while (pToken != NULL)
	{
		pArgument[m_ArgumentCount] = pToken;
		pToken = strtok(NULL, " ");
		m_ArgumentCount++;
	}

	if (m_ArgumentCount > 2)
	{
		if (strcmp(pArgument[1], "001") == 0)
		{
			SendLine("JOIN %s :%s", m_IRCData.m_Channel, m_IRCData.m_ChannelKey);
			str_format(aBuf, sizeof(aBuf), "*** Joined %s", m_IRCData.m_Channel);
			strcpy(pOut, aBuf);
			return;
		}
		else if (strcmp(pArgument[1], "433") == 0)
		{
			strncat(m_Nick, "_", 1);
			SendLine("NICK %s", m_Nick);
			strcpy(m_IRCData.m_Nick, m_Nick);
			return;
		}
		else if (strcmp(pArgument[1], "353") == 0)
		{
			char aBuf[1024];
			str_format(aBuf, sizeof(aBuf), "*** Users at %s: %s", m_IRCData.m_Channel, pArgument[5]+1);
			for(int i = 1; i < m_ArgumentCount-5; i++)
				str_format(aBuf, sizeof(aBuf), "%s %s", aBuf, pArgument[5+i]);
			strcpy(pOut, aBuf);
			return;
		}
		else if (strcmp(pArgument[1], "332") == 0)
		{
			char aBuf[1024];
			str_format(aBuf, sizeof(aBuf), "*** Topic at %s: %s", m_IRCData.m_Channel, pArgument[4]+1);
			for(int i = 1; i < m_ArgumentCount-4; i++)
				str_format(aBuf, sizeof(aBuf), "%s %s", aBuf, pArgument[4+i]);
			strcpy(pOut, aBuf);
			return;
		}
		else if (g_Config.m_IRCMotd && strcmp(pArgument[1], "372") == 0)
		{
			char aBuf[1024];
			str_format(aBuf, sizeof(aBuf), "*** %s", pArgument[3]+1);
			for(int i = 1; i < m_ArgumentCount-3; i++)
				str_format(aBuf, sizeof(aBuf), "%s %s", aBuf, pArgument[3+i]);
			strcpy(pOut, aBuf);
			return;
		}
	}

	if (m_ArgumentCount == 2)
	{
		if (strcmp(pArgument[0], "PING") == 0)
		{
			SendLine("PONG %s", pArgument[1]);
			return;
		}
	}

	if (m_ArgumentCount == 3)
	{
		if (strcmp(pArgument[1], "NICK") == 0)
		{
			m_Sender = strtok(pArgument[0], "!")+1;
			str_format(aBuf, sizeof(aBuf), "*** %s is now known as %s", m_Sender, pArgument[2]+1);
			strcpy(pOut, aBuf);
			return;
		}
	}

	if (m_ArgumentCount >= 4)
	{
		if (strcmp(pArgument[1], "PRIVMSG") == 0)
		{
			m_Sender = strtok(pArgument[0], "!")+1;
			//format whole message for output
			str_format(aBuf, sizeof(aBuf), "%s: %s", m_Sender, pArgument[3]+1);
			for(int i = 1; i< m_ArgumentCount-3; i++)
				str_format(aBuf, sizeof(aBuf), "%s %s", aBuf, pArgument[3+i]);
			strcpy(pOut, aBuf);
			return;
		}
	}
	return;
}

const char *IRC::Init()
{
	char aBuf[128];
#ifdef WIN32
	//setup winsockets
	WSADATA wsa_data;

	if (WSAStartup(MAKEWORD(2, 2), &wsa_data))
	{
		return "*** Fatal Error: failed to initialize winsock";
	}
#endif

	/* create a socket using the INET protocol family (IPv4), and make it a streaming TCP socket */
	if ((m_Sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		return "*** Fatal Error: socket() failed.";
	}

	if (g_Config.m_IRCDebug)
		dbg_msg("IRC", "*** Socket created");

	if (!Connect(m_IRCData.m_Server, m_IRCData.m_Port))
	{
		str_format(aBuf, sizeof(aBuf), "*** Error: Failed to connect to %s:%i", m_IRCData.m_Server, m_IRCData.m_Port);
		return aBuf;
	}

	if (g_Config.m_IRCDebug)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "*** Connected to %s:%i", m_IRCData.m_Server, m_IRCData.m_Port);
		dbg_msg("IRC", aBuf);
	}

	snprintf(m_Nick, sizeof(m_Nick), "%s", m_IRCData.m_Nick);

	SendLine("USER %s 127.0.0.1 localhost :%s", m_IRCData.m_Nick, m_IRCData.m_RealName);
	SendLine("NICK %s", m_Nick);
	return "";
}

void IRC::Leave()
{
#ifdef WIN32
	//clean up winsockets
	WSACleanup();
#endif
}

