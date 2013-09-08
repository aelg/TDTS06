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

typedef std::pair<std::string, std::string> HeaderField;

class HttpConnection: private Connection {
public:
	HttpConnection(const Connection &conn);
	virtual ~HttpConnection();

	void setStatusLine(std::string *s);
	void sendStatusLine();
	void addHeaderField(HeaderField *header);
	void sendHeader();
	void addData(std::string *s);
	void sendData();

	HeaderField *getHeaderField();
	std::string *getStatusLine();
	std::string *getData();

private:
	std::string *rStausLine;
	std::queue<HeaderField*> rHeader;
	std::queue<std::string*> rData;
	std::string *sStatusLine;
};

#endif /* HTTPCONNECTION_H_ */
