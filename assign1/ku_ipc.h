

#define KUIPC_MAXMSG 100
#define KUIPC_MAXVOL 300
#define KU_IPC_CREAT 0x00000001
#define KU_IPC_EXCL 0x00000010
#define KU_IPC_NOWAIT 0x00000100
#define KU_MSG_NOERROR 0x00001000

typedef struct{
        int key;
        int msgflg;
}ku_msgget_param;

typedef struct{
        int msqid;
}ku_msgclose_param;

typedef struct{
        int msqid;
        void* msgp;
        int msgsz;
        int msgflg;
}ku_msgsnd_param;

typedef struct{
        int msqid;
        void* msgp;
        int msgsz;
        long msgtyp;
        int msgflg;
}ku_msgrcv_param;

