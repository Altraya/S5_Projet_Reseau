struct timeval tv;
tv.tv_sec = 0;
tv.tv_usec = 100000;
if (setsockopt(rcv_sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
    perror("Error");
}
