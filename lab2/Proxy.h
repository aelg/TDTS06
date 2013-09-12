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

#include "HttpConnection.h"

class ProxyException : public std::logic_error {
 public:
	enum Error{
		UNKNOWN_ERROR,
		HOST_HEADER_NOT_FOUND,
		SETUP_SERVER_CONNECTION_ERROR
	};
  explicit ProxyException(const std::string& what, Error error) throw() :
  	logic_error(what), error_number(error) {};

  Error error_number;
};
class Proxy {
public:
	Proxy() = delete;
	Proxy(const Proxy&) = delete;
	Proxy(HttpConnection *browser);
	virtual ~Proxy();

	void run();

private:
	void readBrowserRequest();
	void transferBrowserRequest();
	void setupServerConnection();
	void sendBrowserRequest();
	void readServerResponse();
	void transferServerResponseHeader();
	void sendServerResponseHeader();
	bool transferServerResponseData();
	void sendServerResponseData();
	void closeServerConnection();

	void filterHeaderFieldOut(HeaderField* h);
	void filterHeaderFieldIn(HeaderField* h);

	HttpConnection *browser;
	HttpConnection *server;

	std::string serverHostname;
	int contentLength;

};

#endif /* PROXY_H_ */
