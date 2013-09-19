/*
 * Proxy.h
 *
 *  Created by: Linus Mellberg
 *  12 sep 2013
 *
 */

#ifndef PROXY_H_
#define PROXY_H_

#include <stdexcept>
#include <string>
#include <vector>

#include "HttpConnection.h"

class ProxyException : public std::logic_error {
 public:
	enum Error{
		UNKNOWN_ERROR,
		HOST_HEADER_NOT_FOUND,
		SETUP_SERVER_CONNECTION_ERROR,
		BAD_BROWSER,
		BAD_SERVER
	};
  explicit ProxyException(const std::string& what, Error error) throw() :
  	logic_error(what), error_number(error) {};

  Error error_number;
};
class Proxy {
public:
	Proxy() = delete;
	Proxy(const Proxy&) = delete;
	Proxy(HttpConnection *browser, std::vector<ci_string> *filterWords);
	virtual ~Proxy();

	void run();

private:
	bool readBrowserRequest();
	bool transferBrowserRequest();
	bool setupServerConnection();
	bool sendBrowserRequest();
	bool sendBrowserRequestData();
	bool readServerResponse();
	bool transferServerResponseHeader();
	bool sendServerResponseHeader();
	bool transferData(HttpConnection* from, HttpConnection *to);
	bool sendServerResponseData();
	void closeServerConnection();

	HeaderField* filterHeaderFieldOut(HeaderField* h);
	HeaderField* filterHeaderFieldIn(HeaderField* h);

	bool filterData();

	void send501NotImplented();
	void send303SeeOther(std::string location);

	HttpConnection *browser;
	HttpConnection *server;

	std::string serverHostname;
	std::string serverPort;
	int contentLength;
	bool transferResponseData;
	bool transferRequestData;
	bool shouldBeFiltered;
	bool isCompressed;
	HeaderField *savedAcceptEncoding;
	std::string *filterBuffer;
	std::vector<ci_string> *filterWords;
};

#endif /* PROXY_H_ */
