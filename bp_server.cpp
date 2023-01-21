// Let's suppose you are building a server application that receives requests from a network 
// processes them and sends back results of the processing 
//
// suppose you have the following set of classes
// do not pay attention to the fact that there is no implementation
// whenever provided it gives you an outlook of what is happening inside. 

// reply packet

#include <stdio.h>
#include <netinet/in.h> 
#include <unistd.h> 
#include <string.h>
#include <errno.h>

#include <list>
#include <algorithm>

class Reply
{
public:
	char payload[1024] = {0}; 
  explicit Reply(const char* src) { strncpy(payload, src, 1023); } 
};

// request from network packet
class Request
{
	bool mLong;
public:
	// is it long or short in handling
	bool isShort() const { return !mLong; }
	bool islong() const { return mLong; }

	// creating reply packet (i.e. processing the request)
	Reply* process()
	{
		// if(mLong) sleep(3000);
		return new Reply(mLong? "L": "S");
	}
	Request(bool isLongRequest) : mLong(isLongRequest) {};
};

// single piece of network activity, either new connection or disconnect or new request
class NetworkActivity
{
public:
	enum Activity
	{
		newConnection,
		closeConnection,
		newRequest
	};

	Activity activity() const {return act_;} // what has happened
	int connection() const {return fd_;} // socket handle
	void setActivity(Activity act) { act_ = act; }

	static int acceptNewConnection(const int& listener_fd); // socket handle
	Request* request(); // returns new request if activity is newRequest
	void close() 
	{ 
		::close(fd_);
		fd_ = -1; 
	}
	
	bool connectionActive(const fd_set& fds) const { return fd_ >= 0 && FD_ISSET(fd_, &fds); } 
	bool connectionExist() const { return fd_ >= 0;}
  bool listener() const {return listener_;}
	explicit NetworkActivity(int fd = -1, bool listener = false):fd_(fd),listener_(listener){};
	~NetworkActivity() 
	{
		close(); 
	}
private:
	int fd_;
	Activity act_;
  bool listener_;
	const static int BUFLEN = 1024;
};

int NetworkActivity::acceptNewConnection(const int& listener_fd) 
{
	struct sockaddr_in new_addr;
	socklen_t addrlen;

	// the fd with event is on listener_fd
	// accept the new connection
	int new_fd = -1; ;
	if ((new_fd = accept(listener_fd, (struct sockaddr*)&new_addr, &addrlen)) < 0) 
	{
		fprintf(stderr, "accept failed [%s]\n", strerror(errno));
		return new_fd;
	}
	printf("new connection with fd: %d accepted\n", new_fd);
	return new_fd;
}

Request* NetworkActivity::request()
{
	char buf[NetworkActivity::BUFLEN] = {0};
	printf("data --> fd (%d) \n", fd_);
	int ret_val = recv(fd_, buf, NetworkActivity::BUFLEN, 0);
	if (ret_val == 0) 
	{
		printf("close fd:%d\n", fd_);
		close();
		return nullptr;
	}
	else if (ret_val == -1) 
	{
		printf("recv() failed for fd: %d [%s]\n", fd_, strerror(errno));
		return nullptr;
	}
	else // if (ret_val > 0)
	{ 
		printf("received data (len %d bytes) to fd %d): %s\n", ret_val, fd_, buf);
		return new Request(buf[0] == 'L'); // 'L' means request takes long time to process.   and there is 'S'
	}
}


// network layer
class Network
{
public:
	// select failed, 0;	new data comes 1; new connection 2;
	// new connection accept failed -1 
	int Select(unsigned timeout); // returns set of happened things
	void sendReply(int connection, Reply* reply); // sending reply to a request. no need to delete the reply after that
	bool shouldExit() const {return false;} // returns true if application should exit
	Network() 
	{
		if (createListenerSocket() == -1) 
		{
			throw("network failing");
		}
	}
	bool handleNewConnection();
	bool handleRequest(NetworkActivity& act);
	~Network() 
	{
		for(std::list<NetworkActivity>::iterator it = acts_.begin(); it != acts_.end(); ++it)
		{
			acts_.erase(it);
		}
	}
	std::list<NetworkActivity> acts_;
private: 
	fd_set read_fd_set_;
	int createListenerSocket();
	const int PORT = 7000;
};

void Network::sendReply(int fd, Reply* reply) 
{
	int ret_val = -1;
  ret_val = send(fd, reply->payload, 32, 0);
}

int Network::createListenerSocket() 
{
	int fd = -1;
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))== -1) 
	{
		fprintf(stderr, "socket failed [%s]\n", strerror(errno));
		return -1;
	}

	printf("Created a socket with fd: %d\n", fd);

	struct sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(PORT); 
	saddr.sin_addr.s_addr = INADDR_ANY; 

	int ret_val = 0;
	if ((ret_val = bind(fd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in))) != 0) 
	{
		fprintf(stderr, "bind failed [%s]\n", strerror(errno));
		close(fd);
		return -1;
	}

	ret_val = 0; // listen for incoming connections 
	if ((ret_val = listen(fd, 5)) != 0) 
	{
		fprintf(stderr, "listen failed [%s]\n", strerror(errno));
		close(fd);
		return -1;
	}
  else 
	{
		printf("listening on port 7000\n");
	}
	auto* p = new NetworkActivity(fd, true);
  acts_.push_back(*p);  // listener_fd is at the head of the list
	return fd;
}

int Network::Select(unsigned timeout_micro) 
{
	struct timeval timeout
	{
		.tv_sec = 0,
		.tv_usec = timeout_micro*1000
	};

	// Set the fd_set before passing it to the select call 
	FD_ZERO( &read_fd_set_ );
	for(auto const& act: acts_)
	{
		if (act.connectionExist())
			FD_SET(act.connection(), &read_fd_set_);
	}

	// select() and wait
	printf("select()\n");
	int desc_ready_count = -1;
	// if ((ret_val = select(FD_SETSIZE, &read_fd_set_, NULL, NULL, &timeout)) < 0) 
	if ((desc_ready_count = select(FD_SETSIZE, &read_fd_set_, NULL, NULL, NULL)) < 0) 
	{
		fprintf(stderr, "select failed [%s]\n", strerror(errno));
		return 0;
	}

  return desc_ready_count;
}

bool Network::handleNewConnection()
{
	int new_fd = -1;
	auto& listener = acts_.front();
	if (!listener.connectionActive(read_fd_set_)) return false;

  int listener_fd = listener.connection(); 
	auto* na =	new NetworkActivity(new_fd = NetworkActivity::acceptNewConnection(listener_fd));
  if (!na) return false;

	for(auto & act: acts_)
	{
		if (act.listener() || act.connectionExist()) continue;
    auto& a = *na;
		std::swap(act, a);
		return true;
	}
	acts_.push_back(*na);
	return true;
}

bool Network::handleRequest(NetworkActivity& act)
{
	if (act.listener() || !act.connectionActive(read_fd_set_)) return false; 	
	Request* req = act.request();	
	if (req)
	{
		Reply* rpl = req->process(); // handle the request
		this->sendReply(act.connection(), rpl);
		delete rpl;
	}
	delete req;
	return true;
}

/*
	The application should call Network::Select in a loop to get a list of network activities happened
	The Select() blocks control and sits inside till anything happens or the timeout expires, then returns the list.

	A client connection is identified by an integer handle.	For every handle (client) the list returned from Network::Select() will have at most one activity.

	Any request (i.e. when Request::activity() returns newRequest for it) has to be processed (i.e. the Request::process has to be called)
	and the reply should be sent back to the client.

	Requests are of two types: long and short. 
	Short requests do not take much time for processing. Let's suppose that we have just a single form of short requests - ping-pong. 
	Long requests take considerably longer time to process. 

	The method Network::sendReply is non-blocking but requires valid connection (connection handle).

	The Network implementation is considered as not thread safe.

	In the simplest way the server might look like this:
*/

int main()
{
	Network network;
	while(!network.shouldExit())
	{
		int ndesc = network.Select(100); 
    if (ndesc <= 0) continue; 
		if (network.handleNewConnection()) --ndesc;
    if (!ndesc) continue;

		for(auto & act: network.acts_) 
		{
			if (network.handleRequest(act)) { --ndesc; }
			if (!ndesc) continue;
		}
	}
  return 0;
}

/*
 This is a kind of working implementation. But it is non responsive. It hangs up while processing long request(s).

 The aim is to make the server more responsive in general and especially in processing short requests.
 I.e. short requests should be processed as soon as possible. 
 It is obviously (at least to me) that a parallel working is needed to achieve the aim.

 There is no need to show how to work with network (a la FD_SET and other socket related stuff). Just stay with the set of classes provided.
 
 Requirements: 
 1. Short requests should be handled quickly.
 2. While calling sendReply the connection parameter has to be valid. A client connection handle is valid only while the client is connected.
	The client disconnect is detected (i.e. the connection handle invalidates) inside the Network::Select call. 
	Connection handles are guaranteed to be unique while they represent live connections. 
	But once a client disconnects and the corresponding activity is delivered to an application and another Network::Select is called, 
	the handle might be reused for a newly connected client.
 3. All request for live clients should be handled and reply has to be sent back to them.
 4. You may not send a reply supposed to be for one client to another client. This is the situation when connection handle is 
		reused. For example, a client sends a request then disconnects and then another client connects and takes the same handle as the previous one.
	If this happens while you process the request, the connection handle associated with the request will be still valid (identifies live connection)
	but identifies different client.

 I will be looking at an architecture and a synchronization.
 The architecture is a set of classes/objects needed to achieve the aim.
 The sycnronization should be done in a way you usually do. I assume that you always try to do it up to your best abilities.
 The solution has not to be compileable or 100% syntactically correct - I will understand (I hope) what you are trying to achieve.
 Even if it will not be working at all, it will give me outlook at the way of your thinking.
 I expect up to two-three pages of code. I guess the task is not so demanding for more than that.
 Good luck.
*/

