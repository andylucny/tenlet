#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32

#include <winsock2.h>
#pragma message ("adding winsock to project")
#pragma comment (lib,"ws2_32.lib")

#include <conio.h>
#define getch _getch
#define getche _getche
#define kbhit _kbhit

#include <windows.h>

#else 

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>

int kbhit()
{
    struct termios term;
    tcgetattr(0, &term);

    struct termios term2 = term;
    term2.c_lflag &= ~ICANON;
    tcsetattr(0, TCSANOW, &term2);

    int byteswaiting;
    ioctl(0, FIONREAD, &byteswaiting);

    tcsetattr(0, TCSANOW, &term);

    return byteswaiting > 0;
}

int getch(void)
{
    struct termios oldattr, newattr;
    int ch;
    tcgetattr( STDIN_FILENO, &oldattr );
    newattr = oldattr;
    newattr.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newattr );
    ch = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldattr );
    return ch;
}

int getche(void)
{
    struct termios oldattr, newattr;
    int ch;
    tcgetattr( STDIN_FILENO, &oldattr );
    newattr = oldattr;
    newattr.c_lflag &= ~( ICANON );
    tcsetattr( STDIN_FILENO, TCSANOW, &newattr );
    ch = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldattr );
    return ch;
}

#endif // _WIN32

char ibuf [1024], obuf[1024];

main(int argc, char *argv[])
{
    int sock, length;
    struct sockaddr_in server;
    int msgsock;
    int rval, go;
	char *logfile = NULL;

#ifdef _WIN32
	int mode = 1;  // 1 to enable non-blocking socket
	WSADATA wsaData;
	WSAStartup(0x202, &wsaData);
#endif // _WIN32

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
		printf("%d %d\n",sock,errno);
        perror("opening stream socket");
        exit(1);
    }

	if (argc>2) logfile = argv[2];
    
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(atoi(argv[1]));
    if (bind(sock, (struct sockaddr *)&server, sizeof(server))) {
            perror("binding stream socket");
            exit(1);
    }
    length = sizeof(server);
    if (getsockname(sock, (struct sockaddr *)&server, &length)) {
        perror("getting socket name");
        exit(1);
    }
    printf("Socket has port #%d\n", ntohs(server.sin_port));
    
    listen(sock, 5);
    do {
            msgsock = accept(sock, 0, 0);
            if (msgsock == -1) {
                perror("on accept");
                exit(1);
            }
#ifdef _WIN32
			ioctlsocket(msgsock, FIONBIO, &mode);
#else
			fcntl(msgsock, F_SETFL, O_NONBLOCK);
#endif // _WIN32
            printf("client connected\n");
    
            for (go=1;go>0;) {
                int len = 0;
                while (kbhit()) {
                    int c = ibuf[len++] = getch();
                    if (c==13) {
                        printf("\n");
                        ibuf[len++] = '\n';
                    }
                    if (c==27) go = -1;
                }
                if (len>0) {
                    ibuf[len] = 0;
					if (send (msgsock, ibuf, len ,0) < 0) go = 0;
                    //if (write(msgsock, ibuf, len) < 0) go = 0;
                    len = 0;
                }
    
                memset(obuf, 0, sizeof(obuf));
                rval  = recv(msgsock, obuf,  1024, 0);
				//rval  = read(msgsock, obuf,  1024);
                if (rval > 0) {
                    printf("%s",obuf);
					if (logfile) {
#ifdef _WIN32
						FILE *f = fopen(logfile,"wt");
#else 
						FILE *f = fopen(logfile,"w");
#endif
						if (f) {
							fprintf(f,"%s",obuf);
							fclose(f);
						}
					}
                }
                else {
                    if (rval == 0) go = 0;
                    errno = 0;
                }
            }
            printf("\nclosing socket\n");
#ifdef _WIN32
            closesocket(msgsock);
#else
			close(msgsock);
#endif // _WIN32
    } while (go >= 0);
#ifdef _WIN32
    close(sock);
	WSACleanup();
#else
    close(sock);
#endif // _WIN32
}


