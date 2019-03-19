#if !defined NETWORK_H
#define NETWORK_H

#include "u8_buffer.h"

SOCKET create_socket(bool use_tcp, u16 port, u32 address, bool reuse_address = true, bool enable_broadcast = false)
{
    SOCKET result;
    
    if (use_tcp)
        result = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    else
        result = socket(AF_INET, SOCK_DGRAM, 0);
    
    bool socket_is_valid = false;
    
    defer { if (!socket_is_valid) closesocket(result); };
    
    if (result == INVALID_SOCKET) {
        printf("could not create socket, error code: %d\n", WSAGetLastError());
        return INVALID_SOCKET;
    }
    
    if (reuse_address) {
        BOOL value = TRUE;
        if (setsockopt(result, SOL_SOCKET, SO_REUSEADDR, cast_p(char, &value), sizeof(value)) == SOCKET_ERROR)
        {
            printf("could not enable reuse address, error code: %d\n", WSAGetLastError());
            
            return INVALID_SOCKET;
        }
    }
    
    if (enable_broadcast) {
        BOOL value = TRUE;
        if (setsockopt(result, SOL_SOCKET, SO_BROADCAST, cast_p(char, &value), sizeof(value)) == SOCKET_ERROR)
        {
            printf("could not enable broadcast, error code: %d\n", WSAGetLastError());
            
            return INVALID_SOCKET;
        }
    }
    
    {
        sockaddr_in address_info = {};
        address_info.sin_family = AF_INET;
        address_info.sin_addr.s_addr = htonl(address);
        address_info.sin_port = htons(port);
        
        if (bind(result, cast_p(sockaddr, &address_info), sizeof(address_info)) == SOCKET_ERROR)
        {
            printf("could not bind sockt, error code: %d\n", WSAGetLastError());
            
            return INVALID_SOCKET;
        }
    }
    
    if (use_tcp)
        printf("created tcp socket %u\n", (u32)result);
    else
        printf("created udp socket %u\n", (u32)result);
    
    // prevent defer from closesocket
    socket_is_valid = true;
    
    return result;
}


bool send(SOCKET socket, u8_array buffer, sockaddr_in* address = NULL) {
    int bytes_send;
    if (address)
        bytes_send = sendto(socket, cast_p(char, buffer.data), buffer.count, 0, cast_p(sockaddr, address), sizeof(*address));
    else
        bytes_send = send(socket, cast_p(char, buffer.data), buffer.count, 0);
    
    if (bytes_send != buffer.count) {
        printf("error could not send data, error code: %d\n", WSAGetLastError());
        return false;
    } else {
        //printf("sending %u bytes\n", bytes_send);
    }
    
    return true;
}

bool read(SOCKET socket, u8_buffer *buffer, OPTIONAL OUTPUT sockaddr_in *out_address = null) {
    
    int byte_count;
    if (out_address) {
        int address_size = sizeof(*out_address);
        
        byte_count = recvfrom(socket, cast_p(char, buffer->data + buffer->count), buffer->capacity - buffer->count, 0, cast_p(sockaddr, out_address), &address_size);
    }
    else {
        byte_count = recv(socket, cast_p(char, buffer->data + buffer->count), buffer->capacity - buffer->count, 0);
    }
    
    if (byte_count == SOCKET_ERROR) {
        printf("recv faild with error code: %d\n", WSAGetLastError());
        return false;
    }
    
    buffer->count += byte_count;
    
    return true;
}

bool wait_for_reads(SOCKET *sockets, u32 socket_count, u64 timeout_in_us, bool *out_can_reads, bool *out_did_timeout)
{
    assert(socket_count <= 64);
    
    timeval timeout;
    timeout.tv_sec = timeout_in_us / 1000000;
    timeout.tv_usec = (timeout_in_us % 1000000);
    
    fd_set select_set;
    FD_ZERO(&select_set);
    
    s32 super_fd = 1;
    for (u32 i = 0; i < socket_count; ++i) {
        assert(sockets[i] && sockets[i] != INVALID_SOCKET);
        FD_SET(sockets[i], &select_set);
        
        super_fd = max(super_fd, sockets[i] + 1);
    }
    
    int select_state = select(super_fd, &select_set, null, null, &timeout); 
    
    switch (select_state) {
        // TIMEOUT
        case 0:
        *out_did_timeout = true;
        return true;
        
        case SOCKET_ERROR:
        printf("select failed with error code: %d\n", WSAGetLastError());
        for (u32 i = 0; i < socket_count; ++i)
            printf("socket %u\n", (u32)sockets[i]);
        return false;
        
        default: {
            assert(0 <= select_state && select_state <= socket_count);
            
            *out_did_timeout = false;
            
            for (u32 i = 0; i < socket_count; ++i)
                out_can_reads[i] = FD_ISSET(sockets[i], &select_set) != 0;
            
            return true;
        }
    }
    
    UNREACHABLE_CODE;
    return false;
}

bool wait_and_read(SOCKET socket, u8_buffer *buffer, u64 timeout_in_us, bool *out_did_timeout, sockaddr_in *out_address = null)
{
    bool can_read;
    if (!wait_for_reads(&socket, 1, timeout_in_us, &can_read, out_did_timeout))
        return false;
    
    if (*out_did_timeout)
        return true;
    
    assert(can_read);
    return read(socket, buffer, out_address);
}

#endif // NETWORK_H