#pragma region Libraries

#include "handleSmartMeter.h"
#include "handleWorker.h"
#include "handleDistribution.h"
#pragma endregion

int main() {
#pragma region Initializing
	SOCKET listenSmartMeters = setListenSocket("5059"), listenWorkers = setListenSocket("4000");
	if (listenSmartMeters == INVALID_SOCKET || listenWorkers == INVALID_SOCKET) {
		WSACleanup();
		return 1;
	}
	SOCKET sendToDistro = INVALID_SOCKET;
	HANDLE distributioner = CreateThread(NULL, 0, &DistributionHandler, (LPVOID)&sendToDistro, 0, NULL);
	InitializeCriticalSection(&csOutput);
	InitializeCriticalSection(&csQ1);
	InitializeCriticalSection(&csQ2);
	InitializeCriticalSection(&csL);
	initSmartMeters();
	print("Initialization done.");
#pragma endregion

#pragma region Listening
	print("Press q to exit...\n");
	FD_SET set;
	timeval time = { 5, 0 };
	SOCKADDR_IN address;
	int addrlen = sizeof(address);
	while (true) {
		if (shuttingDown) continue;
		FD_ZERO(&set);
		FD_SET(listenSmartMeters, &set);
		FD_SET(listenWorkers, &set);

		if (_kbhit()) {
			if (_getch() == 'q') {
				print("Closing...");
				break;
			}
		}

		int result = select(0, &set, NULL, NULL, &time);
		if (result == SOCKET_ERROR) {
			print("select error: %d", WSAGetLastError());
			break;
		}
		else if (result == 0) {
			Sleep(10);
			continue;
		}
		else if (result > 0) {
			if (FD_ISSET(listenSmartMeters, &set)) {
				bool connected = false;
				for (int i = 0; i < MAX_SMART_METERS; i++) {
					if (!smart_meters[i].isUsed) {
						SOCKET socket = accept(listenSmartMeters, (sockaddr*)&address, &addrlen);
						if (socket == INVALID_SOCKET)
							break;

						HANDLE handle = CreateThread(NULL, 0, &smartMeterThread, &socket, 0, NULL);
						if (handle == NULL) {
							print("Client couldn't make thread");
							break;
						}

						smart_meters[i].handle = handle;
						smart_meters[i].socket = socket;
						smart_meters[i].isUsed = true;
						connected = true;
						break;
					}
				}

				if (connected) continue;

				print("Too much smart meters, can't connect");
				SOCKET socket = accept(listenWorkers, (sockaddr*)&address, &addrlen);
				if (socket == INVALID_SOCKET)
					break;

				shutdown(socket, SD_BOTH);
				closesocket(socket);
				continue;
			}
			else if (FD_ISSET(listenWorkers, &set)) {
				if (numOfWorkers >= 10) {
					print("Too much workers, can't connect");
					SOCKET socket = accept(listenWorkers, (sockaddr*)&address, &addrlen);
					if (socket == INVALID_SOCKET)
						break;

					shutdown(socket, SD_BOTH);
					closesocket(socket);
					continue;
				}

				SOCKET socket = accept(listenWorkers, (sockaddr*)&address, &addrlen);
				if (socket == INVALID_SOCKET) {
					print("Accept error: %d", WSAGetLastError());
					continue;
				}

				sendToDistro = socket;
				reorganize = true;
			}
		}
	}

#pragma endregion

#pragma region Closing
	shuttingDown = true;
	Sleep(100);
	clearQ1();
	clearQ2();
	CloseAllWorkers();
	closeAllSmartMeters();

	closesocket(listenSmartMeters);
	closesocket(listenWorkers);
	WSACleanup();

	DeleteCriticalSection(&csOutput);
	DeleteCriticalSection(&csQ1);
	DeleteCriticalSection(&csQ2);
	DeleteCriticalSection(&csL);
	SafeCloseHandle(distributioner);
#pragma endregion
	return 0;
}
