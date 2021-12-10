#include "fs.h"

void signalTimeTick(int signo) {								//SIGALRM
	if (RUN_TIME == 0) {
		return;
	}
	CONST_TICK_COUNT++;
	printf("%05d       PROC NUMBER   REMAINED CPU TIME\n", CONST_TICK_COUNT);

	// io burst part.
	Node* nodePtr = waitQueue->head;
	int waitQueueSize = 0;

	// get the size of wait queue.
	while (nodePtr != NULL) {
		nodePtr = nodePtr->next;
		waitQueueSize++;
	}

	for (int i = 0; i < waitQueueSize; i++) {
		popFrontNode(waitQueue, ioRunNode);
		ioRunNode->ioTime--;			// decrease io time by 1.

		// io task is over, then push node to ready queue.
		if (ioRunNode->ioTime == 0) {
			pushBackNode(readyQueue, ioRunNode->procNum, ioRunNode->cpuTime, ioRunNode->ioTime);
		}
		// io task is not over, then push node to wait queue again.
		else {
			pushBackNode(waitQueue, ioRunNode->procNum, ioRunNode->cpuTime, ioRunNode->ioTime);
		}
	}
	// cpu burst part.
	if (cpuRunNode->procNum != -1) {
		kill(CPID[cpuRunNode->procNum], SIGCONT);
	}

	RUN_TIME--;// run time decreased by 1.
	return;
}

void signalRRcpuSchedOut(int signo) {							//SIGUSR1
	TICK_COUNT++;

	// scheduler changes cpu preemptive process at every time quantum.
	if (TICK_COUNT >= TIME_QUANTUM) {
		pushBackNode(readyQueue, cpuRunNode->procNum, cpuRunNode->cpuTime, cpuRunNode->ioTime);

		// pop the next process from the ready queue.
		popFrontNode(readyQueue, cpuRunNode);
		TICK_COUNT = 0;
	}
	return;
}

void signalIoSchedIn(int signo) {								//SIGUSR2
	pMsgRcvIocpu(cpuRunNode->procNum, cpuRunNode);

	// process that has no io task go to the end of the ready queue.
	if (cpuRunNode->ioTime == 0) {
		pushBackNode(readyQueue, cpuRunNode->procNum, cpuRunNode->cpuTime, cpuRunNode->ioTime);
	}
	// process that has io task go to the end of the wait queue.
	else {
		pushBackNode(waitQueue, cpuRunNode->procNum, cpuRunNode->cpuTime, cpuRunNode->ioTime);
	}

	// pop the next process from the ready queue.
	popFrontNode(readyQueue, cpuRunNode);
	TICK_COUNT = 0;
	return;
}

void initNodeList(nodeList* list) {
	list->head = NULL;
	list->tail = NULL;
	list->listSize = 0;
	return;
}

void pushBackNode(nodeList* list, int procNum, int cpuTime, int ioTime) {
	Node* newNode = (Node*)malloc(sizeof(Node));
	if (newNode == NULL) {
		perror("push node malloc error");
		exit(EXIT_FAILURE);
	}

	newNode->next = NULL;
	newNode->procNum = procNum;
	newNode->cpuTime = cpuTime;
	newNode->ioTime = ioTime;

	// the first node case.
	if (list->head == NULL) {
		list->head = newNode;
		list->tail = newNode;
	}
	// another node cases.
	else {
		list->tail->next = newNode;
		list->tail = newNode;
	}
	return;
}

void popFrontNode(nodeList* list, Node* runNode) {
	Node* oldNode = list->head;

	// empty list case.
	if (isEmptyList(list) == true) {
		runNode->cpuTime = -1;
		runNode->ioTime = -1;
		runNode->procNum = -1;
		return;
	}

	// pop the last node from a list case.
	if (list->head->next == NULL) {
		list->head = NULL;
		list->tail = NULL;
	}
	else {
		list->head = list->head->next;
	}

	*runNode = *oldNode;
	free(oldNode);
	return;
}

bool isEmptyList(nodeList* list) {
	if (list->head == NULL)
		return true;
	else
		return false;
}

void deleteNode(nodeList* list) {
	while (isEmptyList(list) == false) {
		Node* delNode;
		delNode = list->head;
		list->head = list->head->next;
		free(delNode);
	}
}

void cMsgSndIocpu(int key, int cpuBurstTime, int ioBurstTime) {
	int qid = msgget(key, IPC_CREAT | 0666);				// create message queue ID.

	struct msgBufIocpu msg;
	memset(&msg, 0, sizeof(msg));

	msg.mType = 1;
	msg.mData.pid = getpid();
	msg.mData.cpuTime = cpuBurstTime;						// child process cpu burst time.
	msg.mData.ioTime = ioBurstTime;							// child process io burst time.

	// child process sends its data to parent process.
	if (msgsnd(qid, (void*)&msg, sizeof(dataIocpu), 0) == -1) {
		perror("Child Msgsnd Error");
		exit(EXIT_FAILURE);
	}
	return;
}

void pMsgRcvIocpu(int procNum, Node* nodePtr) {
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
	nodePtr->pid = msg.mData.pid;
	nodePtr->cpuTime = msg.mData.cpuTime;
	nodePtr->ioTime = msg.mData.ioTime;
	return;
}