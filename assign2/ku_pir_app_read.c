#include <stdio.h>

#include "ku_pir.h"
int ku_pir_open();
int ku_pir_close(int fd);
void ku_pir_read(int fd, struct ku_pir_data *data);
void ku_pir_flush(int fd);
int ku_pir_insertData(long unsigned int ts, char rf_flag);


int main(void){
	int fd, ret;
	int c = 0;
	int tmp;
	struct ku_pir_data kpd;
	long unsigned int ts = 0;
	char rf_flag = 0;

	fd = ku_pir_open();
	if(fd > 0){
		printf("[ku_pir_app]open success %d\n", fd);
	}
	else{
		printf("[ku_pir_app]open failed %d\n", fd);
		return -1;
	}

	while(c != 9){
		printf("[ku_pir_app](read:1|exit:9|flush:4|insertD:5)\n");
		scanf("%d", &c);
		if(c == 9){ 
			break;
		}
		else if(c == 4){
			ku_pir_flush(fd);
		}
		else if(c == 1){ 
			ku_pir_read(fd, &kpd);
			printf("[ku_pir_app]ts : %ld\n", kpd.timestamp);
			printf("[ku_pir_app]rf : %c\n", kpd.rf_flag);
			kpd.timestamp = 0;
			kpd.rf_flag = 'x'; 
		}	
		else if(c == 5){
			//scanf("%d",&tmp);
			printf("ts, rf_flag :   ");
			scanf("%ld %c", &ts, &rf_flag);
			ku_pir_insertData(ts, rf_flag);
			printf("insertData: %ld %c\n", ts, rf_flag);
		}else{
		
		}
	}


	ret = ku_pir_close(fd);

	if(ret == 0){
		printf("[ku_pir_app]close success\n");
	}
	else{
		printf("[ku_pir_app]close failed\n");
	}

	return 0;

}
