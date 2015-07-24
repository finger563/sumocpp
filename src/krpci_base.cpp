#include "sumoi/sumoi.hpp"

using namespace std;

SUMOI::SUMOI(string name, string ip, int port) 
  : name_(name),
    ip_(ip),
    port_(port),
    connected_(false)
{
  name_.resize(32);
  id_.resize(16);
  timeout_ = 1;
}

SUMOI::~SUMOI()
{
  Close();
}

void SUMOI::SetName(std::string name)
{
  name_ = name;
  name_.resize(32);
}

void SUMOI::SetIP(std::string ip)
{
  ip_ = ip;
}

void SUMOI::SetPort(int port)
{
  port_ = port;
}

bool SUMOI::Connect()
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
  connected_ = true;
  return true;
}

bool SUMOI::Close()
{
  close(socket_);
  connected_ = false;
}

bool SUMOI::createRequestString(krpc::Request req, std::string& str)
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

bool SUMOI::getResponseFromRequest(krpc::Request req, krpc::Response& res)
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

