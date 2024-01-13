#pragma once

#ifndef HANDLESMARTMETER_H
#define HANDLESMARTMETER_H

#include "queue.h"

typedef struct smart_meter_st {
	HANDLE handle;
	SOCKET socket;
	bool isUsed;
} SMART_METER;

SMART_METER smart_meters[MAX_SMART_METERS];

void initSmartMeters() {
	for (int i = 0; i < MAX_SMART_METERS; i++) {
		smart_meters[i].socket = INVALID_SOCKET;
		smart_meters[i].isUsed = false;
	}
}

void closeSmartMeter(SOCKET socket) {
	for (int i = 0; i < MAX_SMART_METERS; i++) {
		if (smart_meters[i].socket == socket && smart_meters[i].isUsed) {
			smart_meters[i].isUsed = false;
			SafeCloseHandle(smart_meters[i].handle);
			closesocket(socket);
			smart_meters[i].socket = INVALID_SOCKET;
			return;
		}
	}
}

void closeAllSmartMeters() {
	for (int i = 0; i < MAX_SMART_METERS; i++) {
		if (smart_meters[i].isUsed) {
			smart_meters[i].isUsed = false;
			SafeCloseHandle(smart_meters[i].handle);
			shutdown(smart_meters[i].socket, SD_BOTH);
			closesocket(smart_meters[i].socket);
		}
	}
}

DWORD WINAPI smartMeterThread(LPVOID param) {
	SOCKET socket = *(SOCKET*)param;
	print("Smart meter %d connected", socket);
	FD_SET set;
	char buffer[BUFFER_SIZE];
	timeval time = { 1, 0 };

	while (true)
	{
		if (shuttingDown) {
			Sleep(10);
			continue;
		}
		FD_ZERO(&set);
		FD_SET(socket, &set);

		int result = select(0, &set, NULL, NULL, &time);
		if (result == SOCKET_ERROR) {
			closeSmartMeter(socket);
			break;
		}
		else if (result > 0) {
			int iResult = recv(socket, buffer, BUFFER_SIZE, 0);
			if (iResult > 0) {
				if (iResult < BUFFER_SIZE) {
					buffer[iResult] = '\0';
				}
				print("Received from smart meter %d: %s", socket, buffer);
				pushQ1(buffer);
			}
			else if (iResult == 0) {
				print("Smart meter %d disconnected", socket);
				closeSmartMeter(socket);
				break;
			}
			else {
				break;
			}
		}
	}
	return 0;
}

#endif // !HANDLESMARTMETER_H