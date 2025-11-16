#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef _WIN32
    #include <windows.h>
    typedef HANDLE serial_t;
#else
    typedef int serial_t;
#endif

typedef struct {
    serial_t handle;
    bool is_open;
    char port_name[256];
} SerialPort;

bool SerialOpen(SerialPort* port, const char* device, int baudrate);
void SerialClose(SerialPort* port);
int SerialWrite(SerialPort* port, const void* data, size_t len);
int SerialRead(SerialPort* port, void* buffer, size_t len);
void SerialFlush(SerialPort* port);
char** SerialListPorts(int* count);
void SerialFreePortList(char** ports, int count);
char* SerialFindCurveBug(void);

#endif