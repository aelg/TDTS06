/*
 * HttpConnection.h
 *
 *  Created by: Linus Mellberg
 *  7 sep 2013
 */

#ifndef HTTPCONNECTION_H_
#define HTTPCONNECTION_H_

#include <string>
#include <queue>
#include <utility>

#include "Connection.h"
#include "ci_string.h"

typedef std::pair<std::string, std::string> HeaderField;

class HttpConnectionException : public std::logic_error {
 public:
  explicit HttpConnectionException(const std::string& what) throw() :
  	logic_error(what) {};
};

class HttpConnection: private Connection {
public:
	HttpConnection(const Connection &conn);
	virtual ~HttpConnection();

	enum ReceivedType{
		GET_REQUEST,
		NOT_IMPLEMENTED_REQUEST,
		RESPONSE
	};

	void setConnection(Connection &conn);

	static HeaderField *newHeaderField(const std::string &name, const std::string &value);

	void setStatusLine(std::string *&s);
	void setStatusLine(std::string *&&s);
	void addHeaderField(HeaderField *&header);
	void addHeaderField(const std::string &name, const std::string &value);
	void addData(std::string *&s);
	void addData(std::string *&&s);

	void addContentLength();

	void sendStatusLine();
	void sendHeader();
	void sendData();

	ReceivedType recvStatusLine();
	void recvHeader();
	void recvData(size_t length);
	bool recvChunk();

	HeaderField *getHeaderField();
	std::string *getStatusLine();
	std::string *getData();
	int getStatusCode();

private:
	std::string *rStatusLine;
	std::queue<HeaderField*> rHeader;
	std::string *rData;
	std::string *sStatusLine;
	std::queue<HeaderField*> sHeader;
	std::string *sData;
	int statusCode;
};

#endif /* HTTPCONNECTION_H_ */
