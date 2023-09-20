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
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <iostream>
#include <cmath>
#include <queue>
#include <errno.h>
#include <deque>
#include <chrono>


using namespace std;

using namespace chrono;

struct sockaddr_in si_other;
int s, slen;
int ssthres = 64;



#define KB 1000
#define PKT_SIZE 1400

#define FIN 0
#define DATA 1
#define ACK 2
#define TRUE 1
#define FALSE 0


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

int cogctrl(int cwnd, int multiACK, bool timeout){
	if(timeout){  //case1: timeout occured -> threadhold/=2, cwnd =1
		cwnd = 1;
		ssthres *= 0.5;
		cout << "timeout occur" << endl;
	}
	if(multiACK == 3){  //case2: 3 dup acks -> cwnd /= 2
		cwnd /= 2;
		ssthres = cwnd;
		cout << "3 dup acks received" << endl;
	}
	else if(cwnd < ssthres){ //case3: slow start -> cwnd *= 2
		cwnd *=2;
	}else{ //case4: congestion avoidance -> cwnd ++
		cwnd ++;
	}
	return cwnd;

}
void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer) {
    //Open the file
    FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Could not open file to send.");
        exit(1);
    }
    
      /* setting up packet buffer      */
    int TOTAL_BUFF_SIZE = 1000;
    int finalPKTNum = ceil(static_cast<double>(bytesToTransfer)/PKT_SIZE);
    int bytes_left = bytesToTransfer;
    
    //pkt dataBuffer[TOTAL_BUFF_SIZE];

    
	/* Determine how many bytes to transfer */

    slen = sizeof (si_other);

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_other, 0, sizeof (si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(hostUDPport);
    if (inet_aton(hostname, &si_other.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

	/*initialize time out*/
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 30*1000;
    if(setsockopt(s,SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))<0){
    	cout << "Error Setting TimeOut" << strerror(errno) << endl;
    	exit(1);
    
    }
    
    
	/* Send data and receive acknowledgements on s*/
    int numbytes;
    int seqNumToSend = 0;
    int cwnd = 1;
    int bufferHead = 0;
    char rcv_buffer[sizeof(pkt)];
    char snd_buffer[sizeof(pkt)];
    char tmp_buffer[sizeof(pkt)];
    int multiACK = 0;
    
    int pktToSend = 0;
    int latestSeqNum = 0;  //????????????????????
    int bytesRead, startval;
    deque<time_point<high_resolution_clock>> timestamps;
    deque<pkt> NotYetACK;
    
    while(TRUE){
    		pktToSend = min(cwnd - (int)NotYetACK.size(),finalPKTNum - latestSeqNum);
    		pktToSend = max(0,pktToSend);
    		//cout << "arg1: " << cwnd - (int)NotYetACK.size() <<"arg2: "<< finalPKTNum - latestSeqNum + 1 <<endl;
    		//cout << "pkttosend: " << pktToSend << " latestseqn: " << latestSeqNum << "finalpktnum: " << finalPKTNum << endl;
    		//sending pkts:
    		startval = (int)NotYetACK.size();
    		for(int i = startval; i < startval + pktToSend; i++){
    		   //read from file
    		   	pkt tmp;
    			if(bytes_left > PKT_SIZE){
			    	  bytesRead = fread(tmp_buffer, sizeof(char),PKT_SIZE,fp);
			    	  bytes_left -= bytesRead;
		  	}else{
			    	
			    	 bytesRead = fread(tmp_buffer, sizeof(char),bytes_left,fp);
			    	 bytes_left -= bytesRead;
			    	 cout << "final bytes Read: " << bytesRead << endl;
			    	 cout << "read buff content: " << tmp_buffer << endl;
			    
			    	}
			  tmp.seq_num = latestSeqNum;
			  latestSeqNum ++;
			  tmp.ack_num = -1;
			  tmp.RSF = DATA;
			  tmp.datalen = bytesRead;
			  memcpy(tmp.data, &tmp_buffer, sizeof(char)*bytesRead);
			  //save a copy to NotYetACK
			  NotYetACK.push_back(tmp);
			  timestamps.push_back(high_resolution_clock::now());    //keep track of current timestamp
			  //send current pkt 
			   memcpy(snd_buffer, &tmp, sizeof(pkt));
		    	   if((numbytes = sendto(s, snd_buffer, sizeof(pkt),0,(struct sockaddr*)&si_other,slen))== -1){  //  sendto?????
		    	   	cout<<"fail to send seq_num: " << tmp.seq_num << endl;
		    	   	exit(1);
		    	   }else{ }//cout<<"sent seq_num: " << tmp.seq_num <<endl;}
    		}
    		//receiving pkt
    		if((numbytes = recvfrom(s,rcv_buffer,sizeof(pkt),0,(struct sockaddr*)&si_other,(socklen_t*)&slen))==-1){   //rcv timeout
    		
    			
	    		if(errno == EAGAIN || errno == EWOULDBLOCK){   //timeout occured
	    			//check for which packet has timed out 
	    			for(int i = 0; i < timestamps.size(); i++){
	    				auto currTime = high_resolution_clock::now();
	    				auto diff = duration_cast<milliseconds>(currTime - timestamps[i]);
	    				int elapsedTime = diff.count();
	    				timestamps[i] = high_resolution_clock::now(); //reset timeout value
	    				if(elapsedTime > 30){   //timeout occured, ressend packet
	    					memcpy(snd_buffer, &NotYetACK[i], sizeof(pkt));
			    		 	if((numbytes = sendto(s, snd_buffer, sizeof(pkt),0,(struct sockaddr*)&si_other,slen))== -1){
			    		 		cout << "Failed to re-send seq_num: " << NotYetACK[i].seq_num << endl;
			    		 		exit(1);
			    		 	}else{
			    		 	        pkt rnd;
			    		 	        memcpy(&rnd,snd_buffer, sizeof(pkt));
			    		 	
			    		 		cout << "resend timed out pkt_num: " << NotYetACK[i].seq_num << endl;
			    		 		cout << "timeout packet " << rnd.seq_num << endl;
			    		 	}
			    			// congestion controll -> timeout
			    			cwnd = cogctrl(cwnd, multiACK, TRUE);
	    				
	    				}
	    			
	    			
	    			}
	
	    		}else{
		    		cout << "Failed to rcv ack" << endl;
		    		exit(1);
	    		
	    		}
    		
    		}else{ //normal ACK
    			pkt rcv_pkt;
		    	memcpy(&rcv_pkt,rcv_buffer,sizeof(pkt));
		    	int ackNum = rcv_pkt.ack_num;
		    	int dataType = rcv_pkt.RSF;
		    	//cout << "received ackNum: " << ackNum << endl;
		    	if(dataType == FIN){  //case 2 end of TCP connection received fin_ack
		    		printf("received FIN_ACK");
		    		break;
		    	
		    	}
    			//case1 recieved ack for last pkt
			else if(dataType == ACK){
				
				
				if(ackNum == finalPKTNum ){
			    		cout << "sending FIN" << endl;
			    		pkt fin_pkt;
			    		fin_pkt.seq_num = -1;
			    		fin_pkt.ack_num = -1;
			 		fin_pkt.RSF = FIN;
			 		memcpy(snd_buffer, &fin_pkt, sizeof(pkt));
			    		if((numbytes = sendto(s, snd_buffer, sizeof(pkt),0,(struct sockaddr*)&si_other,slen))== -1){   //sendto ??????
			    		 	cout << "Failed to send FIN " << endl;
			    		 	exit(1);
			    		 }

		    		}else if(ackNum > NotYetACK.front().seq_num){   //received ack, clear previously unacked packets
		    			
		    			while(NotYetACK.front().seq_num < ackNum && !NotYetACK.empty()){
		    				NotYetACK.pop_front();
		    				timestamps.pop_front();
		    			}
		    			cwnd = cogctrl(cwnd, multiACK, FALSE);
		    	
		    		}else{
		    		
		    		  if(ackNum == NotYetACK.front().seq_num){
		    		  	multiACK ++;
		    		  	cwnd = cogctrl(cwnd, multiACK, FALSE);
		    		  	
		    		  	if(multiACK == 3){
		    		  	    	multiACK = 0;
		    		  	    	memcpy(snd_buffer, &NotYetACK.front(), sizeof(pkt));
				    		if((numbytes = sendto(s, snd_buffer, sizeof(pkt),0,(struct sockaddr*)&si_other,slen))== -1){   //sendto ??????
				    		 	cout << "Failed to send FIN " << endl;
				    		 	exit(1);
				    		 }else{cout << "3 duplicate ACK, resend: " << NotYetACK.front().seq_num << endl;}
			    		  	
		    		  	}
		    		  }
		    		
		    		
		    		}
    		
    		
    		
    		
    		}
    		
    			
	    	
     	
    	}
    
    
    }	
	

    printf("Closing the socket\n");
    close(s);
    return;

}



/*
 * 
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;
    unsigned long long int numBytes;

    if (argc != 5) {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int) atoi(argv[2]);
    numBytes = atoll(argv[4]);



    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);


    return (EXIT_SUCCESS);
}

