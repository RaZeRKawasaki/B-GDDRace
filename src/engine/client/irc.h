/* IRC handle script by XXLTomate */
#ifndef GAME_CLIENT_IRC_H
#define GAME_CLIENT_IRC_H

#define IRC_BUFFER_SIZE	512
#define IRC_MAX_ARGS	256

#ifdef WIN32
#define vsnprintf _vsnprintf
#define snprintf  _snprintf
#endif


class IRC
{
public:

	struct CIRCData
	{
		char m_Server[32];
		short m_Port;
		char m_Channel[32];
		char m_ChannelKey[32];
		char m_Nick[32];
		char m_RealName[32];
	} m_IRCData;

	bool m_Connected;
	int m_NewMessages;

	const char *MainParser();
	const char *Init();
	void Leave();
	void Send(const char * pMsg);
	void Names();
	void Quit();
	void Topic();

private:
	int Connect(const char *remote_host_p, unsigned short remote_port);
	int SendLine(const char *format_p, ...);
	int RecvLine(char *line_p, unsigned int line_size);

	int	m_Sock;//the socket handle
    int m_ConnectAttempts;
    int m_ArgumentCount;
	char m_Buffer[512]; //the raw lines from IRC
    char m_From[56];
    char m_Nick[56];
    char *m_Sender;
    char *pArgument[IRC_MAX_ARGS]; //splited buffer by " "
    char *pToken;
};
#endif
