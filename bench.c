/*  Cyclops test - spews raw thrift agate add request
 *  to naively determine req/s
 *
 *  gcc -o bench bench.c -pthread && ./bench
 */

#include <stdlib.h>
#include <pthread.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <time.h>
#include <ev.h>

#define NOPS      1E5
#define NTHREADS  50
#define ADDLEN    84
#define ACKLEN    2

static unsigned long long z = 0;
char add[] = "\x00\x00\x00\x50\x80\x01\x00\x01\x00\x00\x00\x03\x61\x64\x64\x00"\
             "\x00\x00\x00\x0a\x00\x01\x00\x00\x00\x00\x53\x3b\x86\xb5\x0d\x00"\
             "\x02\x0b\x0f\x00\x00\x00\x01\x00\x00\x00\x0b\x74\x65\x73\x74\x5f"\
             "\x6d\x65\x74\x72\x69\x63\x0c\x00\x00\x00\x03\x08\x00\x01\x00\x00"\
             "\x00\x46\x00\x08\x00\x01\x00\x00\x00\x1e\x00\x08\x00\x01\x00\x00"\
             "\x00\x28\x00\x00";
int mode = 0;

void *work()
{
    int sockfd;
    struct sockaddr_in dest;
    char buffer[ACKLEN + 1];
    buffer[ACKLEN] = '\0';

    bzero(&dest, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(1998);
    if ( inet_aton("127.0.0.1", &dest.sin_addr.s_addr) == 0 ) {
        perror("127.0.0.1");
        exit(errno);
    }
    
    int was;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
         perror("Socket");
         exit(errno);
    }

    if ( connect(sockfd, (struct sockaddr*)&dest, sizeof(dest)) != 0 ) {
         printf("failed after %d successes.\n", z);
         perror("Connect ");
         return 1;
    }

    int recvd;
    while(z < NOPS){
        write(sockfd, add, ADDLEN);
        bzero(buffer, ACKLEN);
        recvd = 0;
        printf("%d ", sockfd);
        for(recvd=0; recvd < ACKLEN; recvd += recv(sockfd, buffer + recvd, ACKLEN-recvd, 0));
        //printf("buffer: %s\n", buffer);
        //printf("socket fd is %d\n", sockfd);
        //printf("received %d bytes\n", recvd);
        __sync_add_and_fetch(&z, 1);
    }
    close(sockfd);
    return 0;
}

int main(int argc, char **argv)
{
    pthread_t threads[NTHREADS];
    int i;
    FILE *addf, *getf;

    clock_t timer = clock();
    for (i=0; i<NTHREADS; ++i)
        pthread_create(&threads[i], NULL, work, NULL);
    for (i=0; i<NTHREADS; ++i)
        pthread_join(threads[i], NULL);
    timer = clock() - timer;

    float duration = ((float)timer)/CLOCKS_PER_SEC;
    float rate = z / duration;

    printf("%llu in %f seconds (%f/second)\n", z, duration, rate);

    return 0;
}
