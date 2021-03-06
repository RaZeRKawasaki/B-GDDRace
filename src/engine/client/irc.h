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

	void MainParser(char *pOut);
	const char *Init();
	void Leave();
	void Send(const char * pMsg);
	void Names();
	void Quit();
	void Topic();
    void Away_bot(); // Away bot ...
private:
	int Connect(const char *remote_host_p, unsigned short remote_port);
	int SendLine(const char *format_p, ...);
	int RecvLine(char *line_p, unsigned int line_size);
	void OutFormat(char* pOut, char* pArg[IRC_MAX_ARGS], int argumentCount, int offset, char* prefix);

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
