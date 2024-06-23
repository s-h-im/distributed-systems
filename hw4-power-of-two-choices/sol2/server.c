#include "util.h"

int SERVER_PORT; // 서버 포트번호
char* kv[DATASET_SIZE]; // 정적 key value stores
void init_kvs(){
  	for(int i =0;i<DATASET_SIZE;i++){
		kv[i] = malloc(VALUE_SIZE);
		strcpy(kv[i], "DDDCCCCBBBBAAAA");
		//printf("%s\n",kv[i]);
	}

}

static volatile int quit = 0; // Trigger conditions for SIGINT
void signal_handler(int signum) {
	if(signum == SIGINT){  // Functions for Ctrl+C (SIGINT)
		quit = 1;
	}
}

int main(int argc, char *argv[]) {
	// 프로그램 시작시 입력받은 매개변수를 parsing한다.
	if ( argc < 2 ){
	 printf("Input : %s port number\n", argv[0]);
	 return 1;
	}

	signal(SIGINT, signal_handler); // SIGINT에 대한 핸들러 등록

	SERVER_PORT = atoi(argv[1]); // 입력받은 argument를 포트번호 변수에 넣어준다.

	// 서버의 정보를 담을 소켓 구조체 생성 및 초기화
	struct sockaddr_in srv_addr;
	memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(SERVER_PORT);
	srv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0 i.e., 자기 자신의 IP

	// 소켓을 생성한다.
	int sock;
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Could not create listen socket\n");
		exit(1);
	}

	// 생성한 소켓에 소켓구조체를 bind시킨다.
	if ((bind(sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr))) < 0) {
		printf("Could not bind socket\n");
		exit(1);
	}

	init_kvs(); // key-value store 초기화

	int n = 0;
	struct KVS RecvMsg={0,}; // 수신용으로 쓸 메시지 구조체 생성 및 초기화

	struct sockaddr_in src_addr; // 패킷을 수신하였을 때, 해당 패킷을 보낸 송신자(Source)의 정보를 저장하기 위한 소켓 구조체
	socklen_t src_addr_len = sizeof(src_addr);
	size_t pkt_size = 0;
	while (!quit) {
		n = recvfrom(sock, &RecvMsg, sizeof(RecvMsg), MSG_DONTWAIT, (struct sockaddr *)&src_addr, &src_addr_len); // 클라이언트로부터 메시지를 수신
		if (n > 0) { // 수신에 성공하면
			if (RecvMsg.type == READ_REQ){ // 메시지 타입이 읽기 요청이면
				printf("Type: %s Key: %s\n", get_type(RecvMsg), RecvMsg.key); // 수신한 내용 출력
				RecvMsg.type = READ_REP; // 메시지 타입을 읽기 응답으로 변경
				uint64_t index = hash64(RecvMsg.key) % DATASET_SIZE; // kv store에 접근할 인덱스 구하기
				strcpy(RecvMsg.value, kv[index]); // 해당 인덱스의 value값을 메시지의 value값으로 복사
				pkt_size = n + VALUE_SIZE; // value값 변경에 따른 패킷 크기 수정
			}
			sendto(sock, &RecvMsg, pkt_size, 0, (struct sockaddr *)&src_addr, sizeof(src_addr)); // 클라이언트로 답장 송신
			memset(&RecvMsg, 0, sizeof(RecvMsg)); // 버퍼 초기화
		}
	}

	printf("\nCtrl+C pressed. Exit the program after closing the socket\n");
	close(sock);

	return 0;
}
