#ifndef SOCKET_H
#define SOCKET_H

#include <atomic>
#include <vector>
#include <thread>
#include <span>
#include <queue>
#include <mutex>
#include <functional>
#include <asio.hpp>

using asio::ip::tcp;

enum class SOCKET_STATUS
{
	OPEN,
	CLOSED,
	PENDING_OPEN,
	FINISH
};

enum class SOCKET_CALLBACK_EVENT
{
	OPENED,
	CLOSED,
	ERROR_OPEN,
	DATA_AVAILABLE,
	ERROR_RECV,
	ERROR_SEND,
};

struct SocketCallbackInfo
{
	SOCKET_CALLBACK_EVENT sockEvent;
	std::vector<char> data;
};

class Socket
{
public:
	Socket() = delete;
	Socket(const Socket& other) = delete;
	Socket(Socket&& other) = default;

	Socket& operator=(const Socket& other) = delete;
	Socket& operator=(Socket&& other) = default;

	Socket(tcp::socket sock, std::function<void(SocketCallbackInfo)> callback);

	~Socket();

	void send(std::span<const char> data);

	SOCKET_STATUS getStatus() const;

private:
	static constexpr std::size_t MAX_DATA_SIZE = 50'000'000;

	std::atomic<SOCKET_STATUS> status;

	std::vector<char> dataSend;
	std::vector<char> dataRecv;

	std::function<void(SocketCallbackInfo)> callback;

	std::thread threadOpen;

	std::queue<std::vector<char>> queueSend;
	std::mutex mutexQueueSend;
	std::atomic<bool> sending = false;

	static void ThreadOpen(Socket* This);
	void recv();
	void internalSend();

	void handleError(SOCKET_CALLBACK_EVENT event);

protected:
	tcp::socket sock;

	virtual bool open() = 0;
};

#endif