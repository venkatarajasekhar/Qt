/*
 * libjingle
 * Copyright 2011, Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TALK_EXAMPLES_PEERCONNECTION_SERVER_DATA_SOCKET_H_
#define TALK_EXAMPLES_PEERCONNECTION_SERVER_DATA_SOCKET_H_
#pragma once

#ifdef WIN32
#include <winsock2.h>
typedef int socklen_t;
#else
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#define closesocket close
#endif

#include <string>

#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET  static_cast<int>(~0)
#endif

class SocketBase {
 public:
  SocketBase() : socket_(INVALID_SOCKET) { }
  explicit SocketBase(int socket) : socket_(socket) { }
  ~SocketBase() { Close(); }

  int socket() const { return socket_; }
  bool valid() const { return socket_ != INVALID_SOCKET; }

  bool Create();
  void Close();

 protected:
  int socket_;
};

// Represents an HTTP server socket.
class DataSocket : public SocketBase {
 public:
  enum RequestMethod {
    INVALID,
    GET,
    POST,
    OPTIONS,
  };

  explicit DataSocket(int socket)
      : SocketBase(socket),
        method_(INVALID),
        content_length_(0) {
  }

  ~DataSocket() {
  }

  static const char kCrossOriginAllowHeaders[];

  bool headers_received() const { return method_ != INVALID; }

  RequestMethod method() const { return method_; }

  const std::string& request_path() const { return request_path_; }
  std::string request_arguments() const;

  const std::string& data() const { return data_; }

  const std::string& content_type() const { return content_type_; }

  size_t content_length() const { return content_length_; }

  bool request_received() const {
    return headers_received() && (method_ != POST || data_received());
  }

  bool data_received() const {
    return method_ != POST || data_.length() >= content_length_;
  }

  // Checks if the request path (minus arguments) matches a given path.
  bool PathEquals(const char* path) const;

  // Called when we have received some data from clients.
  // Returns false if an error occurred.
  bool OnDataAvailable(bool* close_socket);

  // Send a raw buffer of bytes.
  bool Send(const std::string& data) const;

  // Send an HTTP response.  The |status| should start with a valid HTTP
  // response code, followed by a string.  E.g. "200 OK".
  // If |connection_close| is set to true, an extra "Connection: close" HTTP
  // header will be included.  |content_type| is the mime content type, not
  // including the "Content-Type: " string.
  // |extra_headers| should be either empty or a list of headers where each
  // header terminates with "\r\n".
  // |data| is the body of the message.  It's length will be specified via
  // a "Content-Length" header.
  bool Send(const std::string& status, bool connection_close,
            const std::string& content_type,
            const std::string& extra_headers, const std::string& data) const;

  // Clears all held state and prepares the socket for receiving a new request.
  void Clear();

 protected:
  // A fairly relaxed HTTP header parser.  Parses the method, path and
  // content length (POST only) of a request.
  // Returns true if a valid request was received and no errors occurred.
  bool ParseHeaders();

  // Figures out whether the request is a GET or POST and what path is
  // being requested.
  bool ParseMethodAndPath(const char* begin, size_t len);

  // Determines the length of the body and it's mime type.
  bool ParseContentLengthAndType(const char* headers, size_t length);

 protected:
  RequestMethod method_;
  size_t content_length_;
  std::string content_type_;
  std::string request_path_;
  std::string request_headers_;
  std::string data_;
};

// The server socket.  Accepts connections and generates DataSocket instances
// for each new connection.
class ListeningSocket : public SocketBase {
 public:
  ListeningSocket() {}

  bool Listen(unsigned short port);
  DataSocket* Accept() const;
};

#endif  // TALK_EXAMPLES_PEERCONNECTION_SERVER_DATA_SOCKET_H_
