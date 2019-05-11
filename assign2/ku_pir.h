#define KUPIR_MAX_MSG 5
#define KUPIR_SENSOR 17
#define DEV_NAME "ku_pir_dev"

struct ku_pir_data{
	long unsigned int timestamp;
	char rf_flag;
};

struct ku_pir_read_param{
	int fd;
	struct ku_pir_data *data;
};

struct ku_pir_insertData_param{
	struct ku_pir_data data;
};
