#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

#define BACKLOG 1
#define MAXRECVLEN (1024)

static char test_buf[MAXRECVLEN];
static char *net_cmd_buf[2] = {
  //"net sta config tplink_2017 infotmmm",
  //"net sta config CY_WiFi_5B0F 12345678",
  "net sta config TP-LINK_5E87E6 12345678",
  //"net sta config infotm-codec infotminfotm",
  //"echo filesize=65536"
  "echo tid=7"
  //"echo tid=1 packetsize=5 msleep=33"
};
//static char net_cmd_buf[256] = "net sta config tplink_2017 infotmmm";

typedef enum _port_id {
  PORT_SPEED_TEST = 0,
  PORT_WAKEUP_DEVICE,
  PORT_RECV_HEART_BEAT,
  PORT_SEND_NET_CMD,
  PORT_MAX_NUM,
} t_port_id;

typedef int (*listen_callback)(int socket);

typedef struct _server_para {
  pthread_t listen_id;
  listen_callback callback;
  int port;
} t_server_para;

#define TIME_CYCLE_MAX_NUM 8
int cycle_index = 0;
int cycle_counter = 0;
static struct timeval tv_cycle[TIME_CYCLE_MAX_NUM];
static int packet_counter[TIME_CYCLE_MAX_NUM] = {0};

static int frame_counter = 0;
static struct timeval origion_tv = {0};
static struct timeval last_tv = {0};

int callback_speed_test(int socket)
{
  int iret = recv(socket, test_buf, MAXRECVLEN, MSG_WAITALL);
  if (iret > 0) {
    frame_counter ++;
  } else {
      close(socket);
      printf("speed test, recv failed and socket closed rejected!\n");
  }
  return iret;
}

int callback_recv_heart_beat(int socket)
{
  int iret = recv(socket, test_buf, 32, MSG_WAITALL);
  struct timeval tv;
  gettimeofday(&tv, NULL);
  if (iret>0) {
      frame_counter++;
      printf("recv %d heart beat at time(%ld.%03ld)!\n", frame_counter, tv.tv_sec, tv.tv_usec/1000);
  } else {
      close(socket);
      printf("recv heart beat failed and socket closed!\n");
  }
  return iret;
}
int callback_wakeup_device(int socket)
{
  int iret;
  struct timeval tv1;
  struct timeval tv2;
  gettimeofday(&tv1, NULL);

  iret = send(socket, test_buf, 32, MSG_WAITALL);

  gettimeofday(&tv2, NULL);
  frame_counter++;

  if (iret>0) {
      unsigned long delta = (tv2.tv_sec - tv1.tv_sec)*1000000 + (tv2.tv_sec - tv1.tv_sec);
      printf("Send:%d　%d heart duratio: %ld, at time(%ld.%03ld) to (%ld.%03ld)!\n", iret, frame_counter, delta, tv1.tv_sec, tv1.tv_usec/1000, tv2.tv_sec, tv2.tv_usec/1000);
      sleep(5);
  } else {
      close(socket);
      printf("speed test, recv failed and socket closed rejected!\n");
  }

  return iret;
}

int callback_send_net_cmd(int socket)
{
  static int index = 0;
  int iret = 0;
  if(index >= (sizeof(net_cmd_buf)/sizeof(char*))){
    //index = 0;
    iret = -1;
    return iret;
  }
  iret = send(socket, net_cmd_buf[index], 128, MSG_WAITALL);
  printf("send net cmd: %s\n", net_cmd_buf[index]);
  index++;
  
  return iret;
}

t_server_para server_array[PORT_MAX_NUM] = {
  {-1, callback_speed_test, 4321},
  {-1, callback_wakeup_device, 4323},
  {-1, callback_recv_heart_beat, 4325},
  {-1, callback_send_net_cmd, 4327},
};

void* ticks_handler(void* arg)
{
  struct timeval tv;
  struct timeval start;
  unsigned long long cur_time;
  unsigned long long delta_2s;
  unsigned long long speed_2s;
  unsigned long long delta_aver;
  unsigned long long speed_aver;

  while(1){
    sleep(1);

    gettimeofday(&tv, NULL);
    cur_time = (tv.tv_sec - origion_tv.tv_sec)*1000 + (tv.tv_usec - origion_tv.tv_usec)/1000;
    
    cycle_counter ++;
    cycle_index = cycle_counter%TIME_CYCLE_MAX_NUM;
    tv_cycle[cycle_index] = tv;
    packet_counter[cycle_index] = frame_counter;

    if(cycle_counter > 2){
      int last_index = (cycle_counter-1)%TIME_CYCLE_MAX_NUM;
      start = tv_cycle[last_index];
      delta_2s = (tv.tv_sec - start.tv_sec)*1000 + (tv.tv_usec - start.tv_usec)/1000;
      speed_2s =  ((packet_counter[cycle_index]-packet_counter[last_index])*1024*8/delta_2s);
    }else{
      delta_2s = 0;
      speed_2s = 0;
    }

    if(cycle_counter > TIME_CYCLE_MAX_NUM){
      int last_index = (cycle_counter+1-TIME_CYCLE_MAX_NUM)%TIME_CYCLE_MAX_NUM;
      start = tv_cycle[last_index];
      delta_aver = (tv.tv_sec - start.tv_sec)*1000 + (tv.tv_usec - start.tv_usec)/1000;
      speed_aver =  ((packet_counter[cycle_index]-packet_counter[last_index])*1024*8/delta_aver);
    }else{
      start = origion_tv;
      delta_aver = (tv.tv_sec - start.tv_sec)*1000 + (tv.tv_usec - start.tv_usec)/1000;
      speed_aver =  (MAXRECVLEN*frame_counter*8/delta_aver);
    }
    #if 1
    int prog_index = 0;
    char testbuf[1024];
    memset(testbuf, ' ', sizeof(testbuf));
    for(prog_index = 0;prog_index < (speed_2s/80);prog_index++){
    	testbuf[prog_index]='*';
    }
      
    if((cycle_counter%10) == 0){
      sprintf(&testbuf[150],"at time: %5d\n", cur_time/1000);
      sprintf(&testbuf[170],"\n");
      printf("%5d:%s ", (int)speed_2s, testbuf);
    }
    else{
      sprintf(&testbuf[prog_index],"\n");
      printf("%5d:%s", (int)speed_2s, testbuf);
    }
    #else
    if(speed_2s || speed_aver)
      printf("Total:%d K at: %lld.%03lld, aver:%lld.%03lldmbps in last %lldms, aver:%lld.%03lldmbps in last %lldms \n", frame_counter, cur_time/1000, cur_time%1000
          , speed_2s/1000, speed_2s%1000, delta_2s
          , speed_aver/1000, speed_aver%1000, delta_aver);
    else
      printf("*");
    #endif
  }
 
  return NULL;
}

void* listen_handler(void* arg)
{
    pthread_t tmp_tid;
    t_server_para *serv_entry = (t_server_para *)arg;
    int listenfd, connectfd;  /* socket descriptors */
    struct sockaddr_in server; /* server's address information */
    struct sockaddr_in client; /* client's address information */
    socklen_t addrlen;
    int iret;
    /* Create TCP socket */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        /* handle exception */
        perror("socket() error. Failed to initiate a socket");
        exit(1);
    }

    /* set socket option */
    int opt = SO_REUSEADDR;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bzero(&server, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(serv_entry->port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(listenfd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        /* handle exception */
        perror("Bind() error.");
        exit(1);
    }

    while(1){
      if(listen(listenfd,BACKLOG) == -1) {
        perror("listen() error. \n");
        exit(1);
      }

      addrlen = sizeof(client);
      if((connectfd = accept(listenfd,(struct sockaddr *)&client, &addrlen))==-1) {
        perror("accept() error. \n");
        exit(1);
      }

      struct timeval tv;
      gettimeofday(&tv, NULL);
      //printf("\nYou got a connection from client's ip %s, port %d at time %ld.%ld\n",inet_ntoa(client.sin_addr),htons(client.sin_port), tv.tv_sec,tv.tv_usec);

      cycle_counter = 0;
      tv_cycle[0] = tv;
      packet_counter[0] = 0;
      origion_tv.tv_sec = tv.tv_sec;
      origion_tv.tv_usec = tv.tv_usec;

      if(serv_entry->callback == callback_speed_test){
        iret = pthread_create(&tmp_tid, NULL, ticks_handler, NULL);
        if (iret != 0) {
          printf("pthread_create failed %d!\n", iret);
          return NULL;
        }
      }
      
      while(1)
      {
        if(serv_entry->callback){
          iret = serv_entry->callback(connectfd);
          if(iret <= 0) {
            //perror("socket error!\n");
            break;
          }
        }
      }
      close(connectfd);
    }
    close(listenfd); /* close listenfd */

    return 0;
}

int main(int argc, char *argv[])
{
    int i, ret;

    for (i = 0; i < PORT_MAX_NUM; i++) {
      ret = pthread_create(&server_array[i].listen_id, NULL, listen_handler, &server_array[i]);
      if (ret != 0) {
        printf("pthread_create failed %d!\n", i);
        return -1;
      }
    }    

    while(1){
      sleep(1);
    }

    return 0;
}
