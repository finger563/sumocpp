#include "krpci/krpci.hpp"

using namespace std;

KRPCI::KRPCI(string name, string ip, int port, int streamPort) 
  : name_(name),
    ip_(ip),
    port_(port),
    streamPort_(streamPort),
    connected_(false)
{
  name_.resize(32);
  id_.resize(16);
  timeout_ = 1;
}

KRPCI::~KRPCI()
{
  Close();
}

void KRPCI::SetName(std::string name)
{
  name_ = name;
  name_.resize(32);
}

void KRPCI::SetIP(std::string ip)
{
  ip_ = ip;
}

void KRPCI::SetPort(int port)
{
  port_ = port;
}

void KRPCI::SetStreamPort(int streamPort)
{
  streamPort_ = streamPort;
}


bool KRPCI::Connect()
{
  /* socket: create the socket */
  if ((socket_ = socket(AF_INET, SOCK_STREAM, 0)) <0) {
    perror("ERROR opening socket");
    return false;
  }
  /* build the server's Internet address */
  struct sockaddr_in server_addr;    
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port_);
  if (inet_pton(AF_INET, ip_.c_str(), &(server_addr.sin_addr)) !=1) {
    perror("inet_pton");
    return false;
  }
  /* set the address to zero */
  memset(server_addr.sin_zero, 0, sizeof server_addr.sin_zero);
  /* connect: create a connection with the server */
  if (connect(socket_, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
    perror("ERROR connecting");
    return false;
  }
  /* set the timeout on the socket receive */
  struct timeval tv;
  tv.tv_sec = timeout_;
  tv.tv_usec = 0;
  if (setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
    perror("Couldn't set sockopts!");
    close(socket_);
    return false;
  }
  /* send the message line to the server */
  int sentbytes=0,numbytes=0;
  if ( numbytes = send(socket_, helloMessage, 12,0) == -1) {
    perror("send");
    return false;
  }
  /* send the client's connection name to the server */
  if ( numbytes = send(socket_, name_.c_str(), 32,0) == -1) {
    perror("send");
    return false;
  }
  /* receive the unique ID from server for this client */
  bool haveReceivedID = false;
  while (!haveReceivedID)
    {
      id_.clear();
      int bytesreceived =0;
      if ( (bytesreceived=recv(socket_,(void *)id_.data(),16,0)) <= 0) {
      }
      else {
	haveReceivedID = true;
      }
    }
  // ****************************
  // CONNECT TO THE STREAM SERVER
  // ****************************
  /* Open the stream socket */
  if ((streamSocket_ = socket(AF_INET, SOCK_STREAM, 0)) <0) {
    perror("ERROR opening stream socket");
    return false;
  }
  /* build the server's Internet address */
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(streamPort_);
  if (inet_pton(AF_INET, ip_.c_str(), &(server_addr.sin_addr)) !=1) {
    perror("stream inet_pton");
    return false;
  }
  /* set the address to zero */
  memset(server_addr.sin_zero, 0, sizeof server_addr.sin_zero);
  /* connect: create a connection with the server */
  if (connect(streamSocket_, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
    perror("ERROR connecting to stream");
    return false;
  }
  /* set the timeout on the socket receive */
  tv.tv_sec = timeout_;
  tv.tv_usec = 0;
  if (setsockopt(streamSocket_, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
    perror("Couldn't set sockopts!");
    Close();
    return false;
  }
  /* Send the stream Hello message */
  sentbytes=0,numbytes=0;
  if ( numbytes = send(streamSocket_, helloStreamMessage, 12,0) == -1) {
    perror("send");
    return false;
  }
  /* Send the client ID */
  if ( numbytes = send(streamSocket_, id_.c_str(), 16,0) == -1) {
    perror("send");
    return false;
  }
  /* Receive the stream ACK */
  bool haveReceivedAck = false;
  char ack[2];
  while (!haveReceivedAck)
    {
      memset(ack,0,2);
      int bytesreceived =0;
      if ( (bytesreceived=recv(streamSocket_,(void *)ack,2,0)) <= 0) {
      }
      else {
	if ( strncmp(ack,streamAck,2) == 0 )
	  haveReceivedAck = true;
      }
    }
  connected_ = true;
  boost::thread stream_thread(boost::bind(&KRPCI::streamThreadFunc, this));
  return true;
}

bool KRPCI::Close()
{
  close(socket_);
  close(streamSocket_);
  connected_ = false;
}

void KRPCI::streamThreadFunc()
{
  while (this->connected_)
    {
      this->getStreamResponsesFromStreamMessage();
    }
}

bool KRPCI::CreateStream(std::string streamName, krpc::Request req, boost::function<void (krpc::Response&)> fptr)
{
  if (!connected_)
    return false;
  krpc::Request streamReq;
  krpc::Response response;
  krpc::Argument* argument;
  bool retVal = true;

  streamReq.set_service("KRPC");
  streamReq.set_procedure("AddStream");

  argument = streamReq.add_arguments();
  argument->set_position(0);
  req.SerializeToString(argument->mutable_value());

  if (getResponseFromRequest(streamReq,response))
    {
      if ( response.has_error() )
	{
	  std::cout << "Response error: " << response.error() << endl;
	  return false;
	}
      uint64_t streamID;
      KRPCI::DecodeVarint(streamID, (char *)response.return_value().data(), response.return_value().size());
      KRPC_Stream *newStream = new KRPC_Stream(streamName,streamID,req,fptr);
      active_streams_[streamName] = newStream;
      id_to_stream_map_[streamID] = newStream;
    }
  else
    return false;

  return true;
}

bool KRPCI::RemoveStream(std::string streamName)
{
  if (!connected_)
    return false;
  krpc::Request request;
  krpc::Response response;
  krpc::Argument* argument;
  bool retVal = true;
  uint64_t streamID;
  std::map<std::string,KRPC_Stream*>::iterator it;
  
  it = active_streams_.find(streamName);
  if (it == active_streams_.end())
    return false;
  streamID = it->second->id;

  request.set_service("KRPC");
  request.set_procedure("RemoveStream");

  argument = request.add_arguments();
  argument->set_position(0);
  argument->mutable_value()->resize(10);
  CodedOutputStream::WriteVarint64ToArray(streamID, (unsigned char *)argument->mutable_value()->data());

  if (getResponseFromRequest(request,response))
    {
      if ( response.has_error() )
	{
	  std::cout << "Response error: " << response.error() << endl;
	  return false;
	}
      active_streams_.erase(streamName);
      id_to_stream_map_.erase(streamID);
    }
  else
    return false;

  return true;
}

bool KRPCI::createRequestString(krpc::Request req, std::string& str)
{
  uint64_t size;
  string msg;
  if ( req.SerializeToString(&msg) )
    {
      unsigned char msgLen[10] = {0};
      CodedOutputStream::WriteVarint64ToArray(msg.length(), msgLen);
      str = string((const char *)msgLen) + msg;
    } else
    {
      return false;
    }
  return true;
}

bool KRPCI::getResponseFromRequest(krpc::Request req, krpc::Response& res)
{
  if (!connected_)
    return false;
  string message;
  message.reserve(40);
  bool retVal = true;
  if ( createRequestString(req,message) )
    {
      int numBytes;
      if ( (numBytes = send(socket_, message.data(), message.length(), 0)) == -1 )
	{
	  perror("sending request");
	  return false;
	}
      char buf[maxBufferSize];
      memset(buf,0,maxBufferSize);
      int bytesreceived =0;
      if ( (bytesreceived=recv(socket_,buf,maxBufferSize-1,0)) <= 0) {
	perror("recv");
	return false;
      }
      //std::cout << "Socket received # bytes = " << bytesreceived << endl;
      ZeroCopyInputStream* raw_input = new ArrayInputStream(buf,maxBufferSize);
      CodedInputStream* coded_input = new CodedInputStream(raw_input);
      uint64_t size;
      coded_input->ReadVarint64(&size);
      //std::cout << "Received " << size << " bytes." << endl;
      if (!res.ParseFromCodedStream(coded_input))
	{
	  retVal = false;
	}
      delete coded_input;
      delete raw_input;
    } else
    {
      std::cerr << "Couldn't serialize request!" << std::endl;
      retVal = false;
    }
  return retVal;
}

bool KRPCI::getStreamResponsesFromStreamMessage()
{
  if (!connected_)
    return false;
  bool retVal = true;
  char buf[maxBufferSize];
  memset(buf,0,maxBufferSize);
  int bytesreceived =0;
  if ( (bytesreceived=recv(streamSocket_,buf,maxBufferSize-1,0)) <= 0) {
    //perror("get stream responses from stream message : stream socket receive");
    return false;
  }
  //std::cout << "Socket received # bytes = " << bytesreceived << endl;
  ZeroCopyInputStream* raw_input = new ArrayInputStream(buf,maxBufferSize);
  CodedInputStream* coded_input = new CodedInputStream(raw_input);
  uint64_t size;
  coded_input->ReadVarint64(&size);
  //std::cout << "Received " << size << " bytes." << endl;
  krpc::StreamMessage streamMessage;
  if (!streamMessage.ParseFromCodedStream(coded_input))
    {
      retVal = false;
    }
  for (int i=0;i<streamMessage.responses_size();i++)
    {
      krpc::StreamResponse streamResponse = streamMessage.responses(i);
      uint64_t streamID = streamResponse.id();
      krpc::Response response = streamResponse.response();
      std::map<uint64_t,KRPC_Stream*>::iterator it = id_to_stream_map_.find(streamID);
      if (it != id_to_stream_map_.end())
	{
	  it->second->response = response;
	  if (it->second->fptr != NULL)
	    it->second->fptr(it->second->response);
	}
    }
  delete coded_input;
  delete raw_input;
  return retVal;
}

/*
 * UTILITY FUNCTIONS
 * 
 * Source for the following functions:
 * https://github.com/Vivero/KNav/blob/master/KRPCI/src/KRPCI.cpp
 */

void KRPCI::PrintBytesHex(const char *buf, int size)
{
  for (int i = 0; i < size; i++)
    printf("0x%02X ", (unsigned char)buf[i]);
}

void KRPCI::EncodeVarint(uint32_t value, char *buf, int &size)
{
  size = google::protobuf::io::CodedOutputStream::VarintSize32(value);

  google::protobuf::io::ZeroCopyOutputStream *zcos = new google::protobuf::io::ArrayOutputStream(buf, size);
  google::protobuf::io::CodedOutputStream *cos = new google::protobuf::io::CodedOutputStream(zcos);
  cos->WriteVarint32(value);

  delete cos;
  delete zcos;
}

void KRPCI::EncodeVarint(uint64_t value, char *buf, int &size)
{
  size = google::protobuf::io::CodedOutputStream::VarintSize64(value);

  google::protobuf::io::ZeroCopyOutputStream *zcos = new google::protobuf::io::ArrayOutputStream(buf, size);
  google::protobuf::io::CodedOutputStream *cos = new google::protobuf::io::CodedOutputStream(zcos);
  cos->WriteVarint64(value);

  delete cos;
  delete zcos;
}

void KRPCI::DecodeVarint(uint32_t &value, char *buf, int size)
{
  // init coded stream
  google::protobuf::io::ZeroCopyInputStream *zcos = new google::protobuf::io::ArrayInputStream(buf, size);
  google::protobuf::io::CodedInputStream *cos = new google::protobuf::io::CodedInputStream(zcos);
  cos->ReadVarint32(&value);

  delete cos;
  delete zcos;
}

void KRPCI::DecodeVarint(uint64_t &value, char *buf, int size)
{
  // init coded stream
  google::protobuf::io::ZeroCopyInputStream *zcos = new google::protobuf::io::ArrayInputStream(buf, size);
  google::protobuf::io::CodedInputStream *cos = new google::protobuf::io::CodedInputStream(zcos);
  cos->ReadVarint64(&value);

  delete cos;
  delete zcos;
}

void KRPCI::DecodeString(string& str, char *buf, int size)
{
  google::protobuf::io::ZeroCopyInputStream *zcos = new google::protobuf::io::ArrayInputStream(buf, size);
  google::protobuf::io::CodedInputStream *cos = new google::protobuf::io::CodedInputStream(zcos);
  uint64_t len;
  cos->ReadVarint64(&len);
  cos->ReadString(&str,len);

  delete cos;
  delete zcos;
}

void KRPCI::EncodeTuple(double x, double y, double z, krpc::Tuple &tuple)
{ 
  string *x_str = tuple.add_items();
  string *y_str = tuple.add_items();
  string *z_str = tuple.add_items();

  x_str->assign((char *)&x, sizeof(x));
  y_str->assign((char *)&y, sizeof(y));
  z_str->assign((char *)&z, sizeof(z));
}

void KRPCI::DecodeTuple(krpc::Tuple tuple, double& x, double& y, double& z)
{
  memcpy(&x, tuple.items(0).data(), tuple.items(0).size());
  memcpy(&y, tuple.items(1).data(), tuple.items(0).size());
  memcpy(&z, tuple.items(2).data(), tuple.items(0).size());
}

