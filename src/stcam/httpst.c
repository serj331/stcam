#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>

extern volatile uint8_t* jpegframe;
extern volatile uint32_t jpegframe_size;
extern volatile uint32_t jpegframe_num;

extern volatile pthread_mutex_t jpegframe_lock;
extern volatile pthread_cond_t jpegframe_wait_v;

int _readline(int fd, char* buf, int buf_size) {
    for(int i = 0; i < buf_size; i++) {
        char c;
        if(read(fd, &c, 1) != 1) {
            break;
        } else {
            if(c == '\n') {
                buf[i] = '\0';
                if(i>0) {
                    if(buf[i - 1] == '\r') {
                        buf[i - 1] = '\0';
                        return i - 1;
                    }
                }
                return i;
            } else {
                buf[i] = c;
            }
        }
    }
    return -1;
}

void* _http_req_th(void* arg) {
    int csockd = *(int* )arg;
    
    const char* resphead1 = 
            "HTTP/1.0 200 OK\r\n"
            "Connection: close\r\n"
            "Server: xxx\r\n"
            "Cache-Control: no-store, no-cache, must-revalidate, pre-check=0, post-check=0, max-age=0\r\n"
            "Pragma: no-cache\r\n"
            "Expires: Mon, 3 Jan 2000 12:34:56 GMT\r\n"
            "Content-Type: multipart/x-mixed-replace;boundary=\"%s\"\r\n"
            "\r\n";
            
    const char* resphead2 = 
                "--%s\r\n"
                "Content-Type: image/jpeg\r\n"
                "Content-Length: %d\r\n"
                "\r\n";

            
    char cbuf[1024];
    
    uint8_t* jpegsendbuf = malloc(1024000);
    if(jpegsendbuf == NULL) {
        perror("jpegsendbuf mem alloc error ");
        close(csockd);
        free(arg);
        return NULL;
    }
    
    int llen = _readline(csockd, cbuf, 1024);
    if(llen > 0) { 
        if((strncasecmp(cbuf, "GET / ", 6) == 0) || (strncasecmp(cbuf, "GET /index.", 11) == 0)) {
            printf("http [%d] http request header (1st line) -> %s\n", csockd, cbuf);
            
            //generate boundary
            char boundary[26] = {0};
            for(int i = 0; i < 25; i++) {
                switch(rand() % 3) {
                    case 0:
                    boundary[i] = (char)(rand() % 9) + 48;
                    break;
                    case 1:
                    boundary[i] = (char)(rand() % 25) + 65;
                    break;
                    case 2:
                    boundary[i] = (char)(rand() % 25) + 97;
                    break;
                }
            }
            printf("http [%d] start multipart response. generated boundary=%s\n", csockd, boundary);
            
            //send header
            snprintf(cbuf, 1024, resphead1, boundary);
            write(csockd, cbuf, strlen(cbuf));

            int jpegsend_num = 0, jpegsend_size = 0;
            
            //send body
            for(;;) {
//                //wait for new jpeg frame 
//                for(;;) {
//                    pthread_mutex_lock((pthread_mutex_t*)&jpegframe_lock);
//                    if(jpegframe_num != jpegsend_num) {
//                        jpegsend_size = jpegframe_size;
//                        memcpy(jpegsendbuf, (void*)jpegframe, jpegframe_size);
//                        jpegframe_num = jpegsend_num;
//                        pthread_mutex_unlock((pthread_mutex_t*)&jpegframe_lock);
//                        break;
//                    }
//                    pthread_mutex_unlock((pthread_mutex_t*)&jpegframe_lock);
//                    usleep(20000);
//                }
                
                //new logic
                pthread_mutex_lock((pthread_mutex_t*)&jpegframe_lock);
                pthread_cond_wait((pthread_cond_t*)&jpegframe_wait_v, (pthread_mutex_t*)&jpegframe_lock); 
                jpegsend_size = jpegframe_size;
                memcpy(jpegsendbuf, (void*)jpegframe, jpegframe_size);
                jpegframe_num = jpegsend_num;
                pthread_mutex_unlock((pthread_mutex_t*)&jpegframe_lock);
                

                snprintf(cbuf, 1024, resphead2, boundary, jpegsend_size);
                int nb = write(csockd, cbuf, strlen(cbuf));
                if(nb != strlen(cbuf)) {
                    break;
                }

                nb = write(csockd, jpegsendbuf, jpegsend_size);
                if(nb != jpegsend_size) {
                    break;
                }
            }
        }
    }

    close(csockd);
    free(jpegsendbuf);
    free(arg);
    printf("http [%d] client connection closed.\n", csockd);
    return NULL;
}

void* _accept_conn_th(void* arg) {
    int ssockd = *(int* )arg;
    for(;;) {
        struct sockaddr_in caddr;
        socklen_t caddr_size = sizeof(caddr);
        int csockd = accept(ssockd, (struct sockaddr *)&caddr, &caddr_size);
        if(csockd > 0) {
            printf("http [%d] accept new client connection from ip:%s\n", csockd, inet_ntoa(caddr.sin_addr));
            pthread_t tid;
            int* pcsockd = malloc(sizeof(csockd));
            *pcsockd = csockd;
            if(pthread_create(&tid, NULL, _http_req_th, pcsockd) != 0) {
                perror("http [] client request handler thread start failed. ");
                free(pcsockd);
                close(csockd);
            }
        } else {
            perror("http [] new connection accept error. ");
        }
    }
    return NULL;   
}

int HTTPStreamer_start(int port) {
    struct sockaddr_in saddr;
    saddr.sin_family = PF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = INADDR_ANY;

    int ssockd = socket(AF_INET, SOCK_STREAM, 0);
    if(ssockd > 0) {
        int enable = 1;
        if (setsockopt(ssockd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == 0) {
            if(bind(ssockd, (const struct sockaddr *)&saddr, sizeof(saddr)) == 0) {
                printf("http [] bind on port %d.\n", port);
                if(listen(ssockd, 5) == 0) {
                    printf("http [] start listen port %d.\n", port);
                    pthread_t tid;
                    int* pssockd = malloc(sizeof(ssockd));
                    *pssockd = ssockd;
                    if(pthread_create(&tid, NULL, _accept_conn_th, pssockd) == 0) {
                       return 0;
                    } else {
                        perror("http [] listening thread create failed. ");
                        free(pssockd);
                        close(ssockd);
                    }
                }
            }
        }
    }
    return -1;
}