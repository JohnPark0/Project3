#include "fs.h"

int CPID[MAX_PROCESS];// child process pid.
int KEY[MAX_PROCESS];// key value for message queue.
int CONST_TICK_COUNT;
int TICK_COUNT;
int RUN_TIME;

nodeList* waitQueue;
nodeList* readyQueue;
Node* cpuRunNode;
Node* ioRunNode;

int main(int argc, char* argv[]) {
	int originCpuBurstTime[3000];
	int originIoBurstTime[3000];
	int ppid = getpid();			// get parent process id.

	mount();
	printRootDir();

	struct itimerval newItimer;
	struct itimerval oldItimer;

	newItimer.it_interval.tv_sec = 0;
	newItimer.it_interval.tv_usec = TIME_TICK;
	newItimer.it_value.tv_sec = 1;
	newItimer.it_value.tv_usec = 0;

	struct sigaction tick;
	struct sigaction cpu;
	struct sigaction io;

	memset(&tick, 0, sizeof(tick));
	memset(&cpu, 0, sizeof(cpu));
	memset(&io, 0, sizeof(io));

	tick.sa_handler = &signalTimeTick;
	cpu.sa_handler = &signalRRcpuSchedOut;
	io.sa_handler = &signalIoSchedIn;

	sigaction(SIGALRM, &tick, NULL);
	sigaction(SIGUSR1, &cpu, NULL);
	sigaction(SIGUSR2, &io, NULL);

	waitQueue = malloc(sizeof(nodeList));
	readyQueue = malloc(sizeof(nodeList));
	cpuRunNode = malloc(sizeof(Node));
	ioRunNode = malloc(sizeof(Node));

	if (waitQueue == NULL || readyQueue == NULL) {
		perror("list malloc error");
		exit(EXIT_FAILURE);
	}
	if (cpuRunNode == NULL || ioRunNode == NULL) {
		perror("node malloc error");
		exit(EXIT_FAILURE);
	}

	// initialize ready, sub-ready, wait queues.
	initNodeList(waitQueue);
	initNodeList(readyQueue);

	CONST_TICK_COUNT = 0;
	TICK_COUNT = 0;
	RUN_TIME = 0;

	// create message queue key.
	for (int innerLoopIndex = 0; innerLoopIndex < MAX_PROCESS; innerLoopIndex++) {
		KEY[innerLoopIndex] = 0x6123 * (innerLoopIndex + 1);
		msgctl(msgget(KEY[innerLoopIndex], IPC_CREAT | 0666), IPC_RMID, NULL);
		KEY[innerLoopIndex] = 0x3216 * (innerLoopIndex + 1);
		msgctl(msgget(KEY[innerLoopIndex], IPC_CREAT | 0666), IPC_RMID, NULL);
	}

	// handle main function arguments.
	if (argc == 1 || argc == 2) {
		printf("COMMAND <TEXT FILE> <RUN TIME(sec)>\n");
		printf("./RR.o time_set.txt 10\n");
		exit(EXIT_SUCCESS);
	}
	else {
		// open time_set.txt file.
		rpburst = fopen((char*)argv[1], "r");
		if (rpburst == NULL) {
			perror("file open error");
			exit(EXIT_FAILURE);
		}

		int preCpuTime;
		int preIoTime;

		// read time_set.txt file.
		for (int innerLoopIndex = 0; innerLoopIndex < 3000; innerLoopIndex++) {
			if (fscanf(rpburst, "%d , %d", &preCpuTime, &preIoTime) == EOF) {
				printf("fscanf error");
				exit(EXIT_FAILURE);
			}
			originCpuBurstTime[innerLoopIndex] = preCpuTime;
			originIoBurstTime[innerLoopIndex] = preIoTime;
		}
		// set program run time.
		RUN_TIME = atoi(argv[2]);
		RUN_TIME = RUN_TIME * 1000000 / TIME_TICK;
	}
	printf("\x1b[33m");
	printf("TIME TICK   PROC NUMBER   REMAINED CPU TIME\n");
	printf("\x1b[0m");

	//////////////////////////////////////////////////////////////////////////////////////////////////

	for (int outerLoopIndex = 0; outerLoopIndex < MAX_PROCESS; outerLoopIndex++) {
		// create child process.
		int ret = fork();

		// parent process part.
		if (ret > 0) {
			CPID[outerLoopIndex] = ret;
			pushBackNode(readyQueue, outerLoopIndex, originCpuBurstTime[outerLoopIndex], originIoBurstTime[outerLoopIndex]);
		}

		// child process part.
		else {
			int BurstCycle = 1;
			int procNum = outerLoopIndex;
			int cpuBurstTime = originCpuBurstTime[procNum];
			int ioBurstTime = originIoBurstTime[procNum];
			int VAadr[10];

			// child process waits until a tick happens.
			kill(getpid(), SIGSTOP);

			// cpu burst part.
			while (true) {
				cpuBurstTime--;// decrease cpu burst time by 1.
				printf("            %02d            %02d\n", procNum, cpuBurstTime);
				printf("式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式\n");
				for (int i = 0; i < 10; i++) {
					srand(time(NULL) + (CONST_TICK_COUNT << (i * 2)) + i);
					VAadr[i] = rand() % 0x40000;
				}

				// cpu task is over.
				if (cpuBurstTime == 0) {
					cpuBurstTime = originCpuBurstTime[procNum + (BurstCycle * 10)];	// set the next cpu burst time.

					// send the data of child process to parent process.
					cMsgSndIocpu(KEY[procNum], cpuBurstTime, ioBurstTime);
					ioBurstTime = originIoBurstTime[procNum + (BurstCycle * 10)];	// set the next io burst time.

					BurstCycle++;
					if (BurstCycle > 298)
					{
						BurstCycle = 1;
					}
					kill(ppid, SIGUSR2);
				}
				// cpu task is not over.
				else {
					kill(ppid, SIGUSR1);
				}
				// child process waits until the next tick happens.
				kill(getpid(), SIGSTOP);
			}
		}
	}

	// get the first node from ready queue.
	popFrontNode(readyQueue, cpuRunNode);
	setitimer(ITIMER_REAL, &newItimer, &oldItimer);

	// parent process excutes until the run time is over.
	while (RUN_TIME != 0);
	// remove message queues and terminate child processes.
	for (int innerLoopIndex = 0; innerLoopIndex < MAX_PROCESS; innerLoopIndex++) {
		msgctl(msgget(KEY[innerLoopIndex], IPC_CREAT | 0666), IPC_RMID, NULL);
		KEY[innerLoopIndex] = 6123 * (innerLoopIndex + 1);
		msgctl(msgget(KEY[innerLoopIndex], IPC_CREAT | 0666), IPC_RMID, NULL);
		kill(CPID[innerLoopIndex], SIGKILL);
	}

	// free dynamic memory allocation.
	deleteNode(readyQueue);
	deleteNode(waitQueue);
	free(readyQueue);
	free(waitQueue);
	free(cpuRunNode);
	free(ioRunNode);

	return 0;
}