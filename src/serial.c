#include "serial.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
    #include <windows.h>
    #include <setupapi.h>
    #include <devguid.h>
    #pragma comment(lib, "setupapi.lib")
    #define INVALID_SERIAL INVALID_HANDLE_VALUE
#else
    #include <fcntl.h>
    #include <termios.h>
    #include <unistd.h>
    #include <dirent.h>
    #define INVALID_SERIAL -1
#endif

bool SerialOpen(SerialPort* port, const char* device, int baudrate) {
    port->is_open = false;
    strncpy(port->port_name, device, sizeof(port->port_name) - 1);
    
#ifdef _WIN32
    port->handle = CreateFileA(
        device,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );
    
    if (port->handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    DCB dcb = {0};
    dcb.DCBlength = sizeof(dcb);
    
    if (!GetCommState(port->handle, &dcb)) {
        CloseHandle(port->handle);
        return false;
    }
    
    dcb.BaudRate = baudrate;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity = NOPARITY;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    
    if (!SetCommState(port->handle, &dcb)) {
        CloseHandle(port->handle);
        return false;
    }
    
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 500;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    SetCommTimeouts(port->handle, &timeouts);
    
#else
    port->handle = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    
    if (port->handle == -1) {
        return false;
    }
    
    struct termios tty;
    if (tcgetattr(port->handle, &tty) != 0) {
        close(port->handle);
        return false;
    }
    
    speed_t speed = B115200;
    if (baudrate == 9600) speed = B9600;
    else if (baudrate == 19200) speed = B19200;
    else if (baudrate == 38400) speed = B38400;
    else if (baudrate == 57600) speed = B57600;
    
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);
    
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 5;
    
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    
    if (tcsetattr(port->handle, TCSANOW, &tty) != 0) {
        close(port->handle);
        return false;
    }
#endif
    
    port->is_open = true;
    return true;
}

void SerialClose(SerialPort* port) {
    if (port->is_open) {
#ifdef _WIN32
        CloseHandle(port->handle);
#else
        close(port->handle);
#endif
        port->is_open = false;
    }
}

int SerialWrite(SerialPort* port, const void* data, size_t len) {
    if (!port->is_open) return -1;
    
#ifdef _WIN32
    DWORD written;
    if (WriteFile(port->handle, data, len, &written, NULL)) {
        return written;
    }
    return -1;
#else
    return write(port->handle, data, len);
#endif
}

int SerialRead(SerialPort* port, void* buffer, size_t len) {
    if (!port->is_open) return -1;
    
#ifdef _WIN32
    DWORD read;
    if (ReadFile(port->handle, buffer, len, &read, NULL)) {
        return read;
    }
    return -1;
#else
    return read(port->handle, buffer, len);
#endif
}

void SerialFlush(SerialPort* port) {
    if (!port->is_open) return;
    
#ifdef _WIN32
    PurgeComm(port->handle, PURGE_RXCLEAR | PURGE_TXCLEAR);
#else
    tcflush(port->handle, TCIOFLUSH);
#endif
}

char** SerialListPorts(int* count) {
    char** ports = malloc(sizeof(char*) * 32);
    *count = 0;
    
#ifdef _WIN32
    for (int i = 1; i <= 32; i++) {
        char port_name[16];
        snprintf(port_name, sizeof(port_name), "COM%d", i);
        
        HANDLE h = CreateFileA(port_name, GENERIC_READ | GENERIC_WRITE,
                               0, NULL, OPEN_EXISTING, 0, NULL);
        if (h != INVALID_HANDLE_VALUE) {
            CloseHandle(h);
            ports[*count] = malloc(strlen(port_name) + 1);
            strcpy(ports[*count], port_name);
            (*count)++;
        }
    }
#else
    DIR* dir = opendir("/dev");
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strstr(entry->d_name, "ttyUSB") || 
                strstr(entry->d_name, "ttyACM") ||
                strstr(entry->d_name, "cu.usb")) {
                char full_path[256];
                snprintf(full_path, sizeof(full_path), "/dev/%s", entry->d_name);
                ports[*count] = malloc(strlen(full_path) + 1);
                strcpy(ports[*count], full_path);
                (*count)++;
            }
        }
        closedir(dir);
    }
#endif
    
    return ports;
}

void SerialFreePortList(char** ports, int count) {
    for (int i = 0; i < count; i++) {
        free(ports[i]);
    }
    free(ports);
}

char* SerialFindCurveBug(void) {
#ifdef _WIN32
    HDEVINFO device_info = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, NULL, NULL, 
                                                 DIGCF_PRESENT);
    if (device_info == INVALID_HANDLE_VALUE) {
        return NULL;
    }
    
    SP_DEVINFO_DATA device_data;
    device_data.cbSize = sizeof(SP_DEVINFO_DATA);
    
    for (DWORD i = 0; SetupDiEnumDeviceInfo(device_info, i, &device_data); i++) {
        char hardware_id[256] = {0};
        char description[256] = {0};
        
        // Get hardware ID
        if (SetupDiGetDeviceRegistryPropertyA(device_info, &device_data, 
            SPDRP_HARDWAREID, NULL, (PBYTE)hardware_id, sizeof(hardware_id), NULL)) {
            
            // Check for CurveBug VID/PID
            if (strstr(hardware_id, "VID_16D0&PID_13F9")) {
                // Get COM port name
                HKEY key = SetupDiOpenDevRegKey(device_info, &device_data, 
                    DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
                if (key != INVALID_HANDLE_VALUE) {
                    char port_name[256];
                    DWORD size = sizeof(port_name);
                    if (RegQueryValueExA(key, "PortName", NULL, NULL, 
                        (LPBYTE)port_name, &size) == ERROR_SUCCESS) {
                        RegCloseKey(key);
                        SetupDiDestroyDeviceInfoList(device_info);
                        
                        char* result = malloc(strlen(port_name) + 1);
                        strcpy(result, port_name);
                        return result;
                    }
                    RegCloseKey(key);
                }
            }
        }
        
        // Also check description
        if (SetupDiGetDeviceRegistryPropertyA(device_info, &device_data, 
            SPDRP_DEVICEDESC, NULL, (PBYTE)description, sizeof(description), NULL)) {
            
            if (strstr(description, "CurveBug")) {
                HKEY key = SetupDiOpenDevRegKey(device_info, &device_data, 
                    DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
                if (key != INVALID_HANDLE_VALUE) {
                    char port_name[256];
                    DWORD size = sizeof(port_name);
                    if (RegQueryValueExA(key, "PortName", NULL, NULL, 
                        (LPBYTE)port_name, &size) == ERROR_SUCCESS) {
                        RegCloseKey(key);
                        SetupDiDestroyDeviceInfoList(device_info);
                        
                        char* result = malloc(strlen(port_name) + 1);
                        strcpy(result, port_name);
                        return result;
                    }
                    RegCloseKey(key);
                }
            }
        }
    }
    
    SetupDiDestroyDeviceInfoList(device_info);
    return NULL;
    
#else
    // On Linux/Mac, try common USB serial ports
    DIR* dir = opendir("/dev");
    if (!dir) return NULL;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, "ttyUSB") || 
            strstr(entry->d_name, "ttyACM") ||
            strstr(entry->d_name, "cu.usb")) {
            
            char full_path[256];
            snprintf(full_path, sizeof(full_path), "/dev/%s", entry->d_name);
            
            // Try to open and test
            int fd = open(full_path, O_RDWR | O_NOCTTY | O_NDELAY);
            if (fd != -1) {
                // Simple test - could be improved
                close(fd);
                closedir(dir);
                
                char* result = malloc(strlen(full_path) + 1);
                strcpy(result, full_path);
                return result;
            }
        }
    }
    
    closedir(dir);
    return NULL;
#endif
}