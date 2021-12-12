#include "fs.h"

void signalTimeTick(int signo) {								//SIGALRM
	if (RUN_TIME == 0) {
		return;
	}

	CONST_TICK_COUNT++;
	printf("%05d       PROC NUMBER   REMAINED CPU TIME\n", CONST_TICK_COUNT);

	// io burst part.
	PCB* PCBPtr = waitQueue->head;
	int waitQueueSize = 0;

	// get the size of wait queue.
	while (PCBPtr != NULL) {
		PCBPtr = PCBPtr->next;
		waitQueueSize++;
	}

	for (int i = 0; i < waitQueueSize; i++) {
		popPCB(waitQueue, ioRunPCB);
		ioRunPCB->ioTime--;			// decrease io time by 1.

		// io task is over, then push node to ready queue.
		if (ioRunPCB->ioTime == 0) {
			pushPCB(readyQueue, ioRunPCB->procNum, ioRunPCB->cpuTime, ioRunPCB->ioTime, ioRunPCB->fileCond);
		}
		// io task is not over, then push node to wait queue again.
		else {
			pushPCB(waitQueue, ioRunPCB->procNum, ioRunPCB->cpuTime, ioRunPCB->ioTime, ioRunPCB->fileCond);
		}
	}
	// cpu burst part.
	if (cpuRunPCB->procNum != -1) {
		kill(CPID[cpuRunPCB->procNum], SIGCONT);
	}

	RUN_TIME--;// run time decreased by 1.
	return;
}

void signalRRcpuSchedOut(int signo) {							//SIGUSR1
	TICK_COUNT++;

	int openMode;

	if (cpuRunPCB->fileCond == 0) {							//file not opened
		openMode = rand();
		srand(time(NULL) + openMode);
		//openMode = rand() % 2;
		openMode = 3;			//임시
		if (fileOpen(cpuRunPCB->fileName, openMode) == 0) {	//file oepn success
			cpuRunPCB->fileCond = 1;
		}
	}
	if (cpuRunPCB->fileCond == 1) {							//file already opened
		fileWrite("write buffer");			//수정할 부분
	}

	// scheduler changes cpu preemptive process at every time quantum.
	if (TICK_COUNT >= TIME_QUANTUM) {
		pushPCB(readyQueue, cpuRunPCB->procNum, cpuRunPCB->cpuTime, cpuRunPCB->ioTime, cpuRunPCB->fileCond);

		// pop the next process from the ready queue.
		popPCB(readyQueue, cpuRunPCB);
		TICK_COUNT = 0;
	}
	return;
}

void signalIoSchedIn(int signo) {								//SIGUSR2
	fileClose(cpuRunPCB->fileName);
	cpuRunPCB->fileCond = 0;

	pMsgRcvIocpu(cpuRunPCB->procNum, cpuRunPCB);

	// process that has no io task go to the end of the ready queue.
	if (cpuRunPCB->ioTime == 0) {
		pushPCB(readyQueue, cpuRunPCB->procNum, cpuRunPCB->cpuTime, cpuRunPCB->ioTime, cpuRunPCB->fileCond);
	}
	// process that has io task go to the end of the wait queue.
	else {
		pushPCB(waitQueue, cpuRunPCB->procNum, cpuRunPCB->cpuTime, cpuRunPCB->ioTime, cpuRunPCB->fileCond);
	}

	// pop the next process from the ready queue.
	popPCB(readyQueue, cpuRunPCB);
	TICK_COUNT = 0;
	return;
}

void initPCBList(PCBList* list) {
	list->head = NULL;
	list->tail = NULL;
	list->listSize = 0;
	return;
}

void pushPCB(PCBList* list, int procNum, int cpuTime, int ioTime, int fileCond) {
	PCB* newPCB = (PCB*)malloc(sizeof(PCB));
	if (newPCB == NULL) {
		perror("push PCB malloc error");
		exit(EXIT_FAILURE);
	}

	newPCB->next = NULL;
	newPCB->procNum = procNum;
	newPCB->cpuTime = cpuTime;
	newPCB->ioTime = ioTime;
	newPCB->fileCond = fileCond;
	newPCB->fileName[0] = '\0';

	// the first node case.
	if (list->head == NULL) {
		list->head = newPCB;
		list->tail = newPCB;
	}
	// another node cases.
	else {
		list->tail->next = newPCB;
		list->tail = newPCB;
	}
	return;
}

void popPCB(PCBList* list, PCB* runPCB) {
	PCB* oldPCB = list->head;

	// empty list case.
	if (isEmptyList(list) == true) {
		runPCB->cpuTime = -1;
		runPCB->ioTime = -1;
		runPCB->procNum = -1;
		return;
	}

	// pop the last PCB from a list case.
	if (list->head->next == NULL) {
		list->head = NULL;
		list->tail = NULL;
	}
	else {
		list->head = list->head->next;
	}

	*runPCB = *oldPCB;
	free(oldPCB);
	return;
}

bool isEmptyList(PCBList* list) {
	if (list->head == NULL)
		return true;
	else
		return false;
}

void deletePCB(PCBList* list) {
	while (isEmptyList(list) == false) {
		PCB* delPCB;
		delPCB = list->head;
		list->head = list->head->next;
		free(delPCB);
	}
}

void cMsgSndIocpu(int procNum, int cpuBurstTime, int ioBurstTime, char* randFile) {
	int key = 0x3216 * (procNum + 1);
	int qid = msgget(key, IPC_CREAT | 0666);				// create message queue ID.

	struct msgBufIocpu msg;
	memset(&msg, 0, sizeof(msg));

	msg.mType = 1;
	msg.mData.pid = getpid();
	msg.mData.cpuTime = cpuBurstTime;						// child process cpu burst time.
	msg.mData.ioTime = ioBurstTime;							// child process io burst time.
	strcpy(msg.mData.fileName, randFile);					// child process file request.

	// child process sends its data to parent process.
	if (msgsnd(qid, (void*)&msg, sizeof(dataIocpu), 0) == -1) {
		perror("Child Msgsnd Error");
		exit(EXIT_FAILURE);
	}
	return;
}

void pMsgRcvIocpu(int procNum, PCB* PCBPtr) {
	int key = 0x3216 * (procNum + 1);// create message queue key.
	int qid = msgget(key, IPC_CREAT | 0666);

	struct msgBufIocpu msg;
	memset(&msg, 0, sizeof(msg));

	// parent process receives child process data.
	if (msgrcv(qid, (void*)&msg, sizeof(msg), 0, 0) == -1) {
		perror("Msgrcv Error");
		exit(1);
	}

	// copy the data of child process to nodePtr.
	PCBPtr->pid = msg.mData.pid;
	PCBPtr->cpuTime = msg.mData.cpuTime;
	PCBPtr->ioTime = msg.mData.ioTime;
	strcpy(PCBPtr->fileName, msg.mData.fileName);
	return;
}