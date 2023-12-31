#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <iostream>
#include <cstring>
#include <fcntl.h>

#define KB 1000
#define PKT_SIZE 1400
#define BUFFER_SIZE 400

#define FIN 0
#define DATA 1
#define ACK 2

using namespace std;

struct sockaddr_in si_me, si_other;
int s, slen;

typedef struct{
  int seq_num;
  int ack_num;
  int RSF;
  int datalen;
  char data[PKT_SIZE];
}pkt;

void diep(char *s) {
    perror(s);
    exit(1);
}


void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {
    
    slen = sizeof (si_other);


    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding\n");
    if (::bind(s, (struct sockaddr*) &si_me, sizeof (si_me)) == -1)
        diep("bind");

    
	/* Now receive data and send acknowledgements */    
    FILE* fp = fopen(destinationFile,"wb");
    
    pkt buffer[BUFFER_SIZE];
    char buf[sizeof(pkt)];
    int ack_status[BUFFER_SIZE];
    int ToBeFilledIdx = 0;
    int NextACK = 0;
    struct sockaddr_in sender_addr;
    socklen_t addrlen = sizeof(sender_addr);
    pkt pkt_in;
    pkt ack;
    int tmp_idx;
    int buffer_head = 0;
    int recvbytes;
    while(1){
    	if((recvbytes = recvfrom(s, buf, sizeof(pkt), 0, (struct sockaddr*)&sender_addr,&addrlen)) <= 0){
    		fprintf(stderr,"No Data");
    		exit(1);
    	}
    	memcpy(&pkt_in, buf, sizeof(pkt));
    	//cout << "pkt num: " << pkt_in.seq_num << " type: " << pkt_in.RSF << endl;
    	
    	if(pkt_in.RSF == FIN){				// sender close TCP connection
    		ack.seq_num = 0;
    		ack.ack_num = NextACK;
    		ack.RSF = FIN;
    		memcpy(buf,&ack,sizeof(pkt));
    		sendto(s, buf, sizeof(pkt), 0, (struct sockaddr*) &sender_addr, addrlen)
    		cout << "closed connection" << endl;
    		break;

    	}else if(pkt_in.RSF == DATA){                   //receive packet
    	   if(pkt_in.seq_num == NextACK){
    	   	memcpy(&buffer[ToBeFilledIdx], &pkt_in, sizeof(pkt));
    	   	fwrite(&pkt_in.data, sizeof(char), pkt_in.datalen,fp);
    	   	//cout << "written pkt" << pkt_in.seq_num << " bytes into file, toBeFilledIdx is now: "<< ToBeFilledIdx << endl;
    	   	//cout << "written " << pkt_in.datalen << " bytes into file" << endl;
    	   	
    	   	ToBeFilledIdx = (ToBeFilledIdx + 1) % BUFFER_SIZE;
    	   	if (ToBeFilledIdx == 0) buffer_head = buffer[BUFFER_SIZE-1].seq_num;
    	   	NextACK++;
    	  
    	   	
    	   	while(ack_status[ToBeFilledIdx] != 0){
    	   	   fwrite(&buffer[ToBeFilledIdx].data, sizeof(char), buffer[ToBeFilledIdx].datalen,fp);
    	   	   //cout << " write ahead data " << buffer[ToBeFilledIdx].seq_num << endl;
    	   	   ack_status[ToBeFilledIdx] = 0;
    	   	   ToBeFilledIdx = (ToBeFilledIdx + 1)%BUFFER_SIZE;
    	   	   //cout << "nexted buffed ack: " << ack_status[ToBeFilledIdx] << endl;
    	   	   if (ToBeFilledIdx == 0) buffer_head = buffer[BUFFER_SIZE-1].seq_num;
    	   	   cout << "NextAck: " << NextACK << endl;
    	   	   NextACK++;
    	   	   
    	   	}
    	   	
    	   }else if(pkt_in.seq_num > NextACK){
    	   	tmp_idx = pkt_in.seq_num - buffer_head;
    	   	if(tmp_idx <= BUFFER_SIZE){   //skipped data added to buffer
    	   	   ack_status[tmp_idx-1] = 1;
    	   	   memcpy(&buffer[tmp_idx-1], &pkt_in, sizeof(pkt));
    	   	  // cout << "queued seq_num: " << pkt_in.seq_num << endl;
    	   	  // cout << "tmp_idx-1: " << tmp_idx-1 << "buffer_head " << buffer_head <<endl;
    	   	}
    	   }else{
    	   	cout << "recieved duplicate packet seq: "<<pkt_in.seq_num << endl;	
    	   	
    	   }
    	
    	ack.seq_num = 0;
    	ack.ack_num = NextACK;
    	ack.RSF = ACK;
    	memcpy(buf,&ack,sizeof(pkt));
    	//cout << "sending ack" << NextACK << endl;
    	sendto(s, buf, sizeof(pkt), 0, (struct sockaddr*) &sender_addr, addrlen);
    	}else{
    	    cout << "incorrect file type" << endl;
    	    cout << "pkt seq" << pkt_in.seq_num << " pkt data" << pkt_in.data << endl;
    	    
    	
    	}

    	}
   
    close(s);
    printf("%s received.", destinationFile);
    return;
    
    
    
    
    }
	

/*
 * 
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;

    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);

    reliablyReceive(udpPort, argv[2]);
}
