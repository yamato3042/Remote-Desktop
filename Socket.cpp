#include "Socket.h"

Socket::Socket(tcp::socket sock, std::function<void(SocketCallbackInfo)> callback)
	: sock(std::move(sock)), callback(std::move(callback))
{
	status = SOCKET_STATUS::PENDING_OPEN;

	threadOpen = std::thread(ThreadOpen, this); // obliger de passer par une fonction statique
	                                            // sinon ca compile pas
}

Socket::~Socket()
{
	SOCKET_STATUS oldStatus = status;

	status = SOCKET_STATUS::FINISH;

	if (oldStatus != SOCKET_STATUS::CLOSED)
	{
		asio::error_code er;

		sock.shutdown(asio::socket_base::shutdown_both, er);
		sock.close(er);
	}

	threadOpen.join(); // on ferme le thread pour open et on attend qu'il soit terminé 
	                   // si il est encore en cours d'execution
	
	asio::error_code er;
	sock.wait(asio::socket_base::wait_error, er); // on attend que les callback de recv et send soient
	                                              // terminés afin d'éviter que ces callback utilisent les données
	                                              // de l'objet courant alors qu'il est detruit
}

SOCKET_STATUS Socket::getStatus() const
{
	return status;
}

void Socket::ThreadOpen(Socket* This)
{
	bool opened = This->open();
	
	if (This->status == SOCKET_STATUS::FINISH)
		return;

	if (opened)
	{
		This->status = SOCKET_STATUS::OPEN;

		SocketCallbackInfo info;
		info.sockEvent = SOCKET_CALLBACK_EVENT::OPENED;

		This->callback(std::move(info));

		This->recv();
	}
	else
	{
		This->handleError(SOCKET_CALLBACK_EVENT::ERROR_OPEN);
	}
}

void Socket::handleError(SOCKET_CALLBACK_EVENT event)
{
	if (status == SOCKET_STATUS::CLOSED)
		return;

	asio::error_code er;

	sock.shutdown(asio::socket_base::shutdown_both, er);
	sock.close(er);

	status = SOCKET_STATUS::CLOSED;

	SocketCallbackInfo info;
	info.sockEvent = event;

	callback(std::move(info));
}

void Socket::recv()
{
	dataRecv.resize(sizeof(std::uint32_t));

	asio::async_read(sock, asio::buffer(dataRecv),
		[this](asio::error_code er, std::size_t size)
		{
			if (status == SOCKET_STATUS::FINISH)
				return;

			if (er || size > MAX_DATA_SIZE || dataRecv.size() != size)
			{
				handleError(SOCKET_CALLBACK_EVENT::ERROR_RECV);
				return;
			}
				
			std::uint32_t sizeData = reinterpret_cast<std::uint32_t&>(dataRecv[0]);

			if (!sizeData)
			{
				handleError(SOCKET_CALLBACK_EVENT::ERROR_RECV);
				return;
			}

			dataRecv.resize(sizeData);

			asio::async_read(sock, asio::buffer(dataRecv),
				[this](asio::error_code er, std::size_t size)
				{
					if (status == SOCKET_STATUS::FINISH)
						return;

					if (er || size > MAX_DATA_SIZE || dataRecv.size() != size)
					{
						handleError(SOCKET_CALLBACK_EVENT::ERROR_RECV);
						return;
					}
						
					recv();
						
					SocketCallbackInfo info;	
						
					info.sockEvent = SOCKET_CALLBACK_EVENT::DATA_AVAILABLE;
					info.data = dataRecv;

					callback(std::move(info));
						
				});
		});
}

void Socket::send(std::span<const char> data)
{
	assert(status == SOCKET_STATUS::OPEN && !data.empty() && data.size() <= MAX_DATA_SIZE);

	if (sending)
	{
		std::lock_guard<std::mutex> lock(mutexQueueSend);

		queueSend.push({ data.begin(), data.end() });
	}
	else
	{
		sending = true;

		dataSend.resize(sizeof(std::uint32_t) + data.size());

		reinterpret_cast<std::uint32_t&>(dataSend[0]) = static_cast<std::uint32_t>(data.size());
		std::copy(data.begin(), data.end(), dataSend.begin() + sizeof(std::uint32_t));

		internalSend();
	}
}
	

void Socket::internalSend()
{
	asio::async_write(sock, asio::buffer(dataSend),
		[this](asio::error_code er, std::size_t size)
		{
			if (status == SOCKET_STATUS::FINISH)
			{
				sending = false;

				return;
			}

			if (er || size > MAX_DATA_SIZE || dataSend.size() != size)
			{
				handleError(SOCKET_CALLBACK_EVENT::ERROR_SEND);

				return;
			}

			mutexQueueSend.lock();

			if (queueSend.empty())
			{
				mutexQueueSend.unlock();

				return;
			}

			std::vector<char>& newDataSend = queueSend.front();

			mutexQueueSend.unlock();

			dataSend.resize(sizeof(std::uint32_t) + newDataSend.size());

			reinterpret_cast<std::uint32_t&>(dataSend[0]) = static_cast<std::uint32_t>(newDataSend.size());
			std::copy(newDataSend.begin(), newDataSend.end(), dataSend.begin() + sizeof(std::uint32_t));

			mutexQueueSend.lock();
			queueSend.pop();
			mutexQueueSend.unlock();

			internalSend();
		});
}