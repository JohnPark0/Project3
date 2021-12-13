#include "fs.h"

void mount(void) {
	pFileSystem = fopen("disk.img", "r+");
	pFileDump = fopen("File_dump.txt", "w");

	if (pFileSystem == NULL) {
		printf("There are no disk.img,\n");
		exit(0);
	}

	fread(&part.super, sizeof(superBlock), 1, pFileSystem);										//copy sizeof(superBlock) bytes to buffer<part.super>
	fprintf(pFileDump, "---------------------SuperBlcok Contents---------------------\n");
	fprintf(pFileDump, "  Partition Type   - %d\n", part.super.partitionType);
	fprintf(pFileDump, "  Block Size       - %d bytes\n", part.super.blockSize);
	fprintf(pFileDump, "  Inode Size       - %d bytes\n", part.super.inodeSize);
	fprintf(pFileDump, "  First Inode      - %d\n", part.super.firstInode);
	fprintf(pFileDump, "  num Inodes       - %d\n", part.super.numInodes);
	fprintf(pFileDump, "  num Inode Blocks - %d\n", part.super.numInodeBlocks);
	fprintf(pFileDump, "  num Free Inodes  - %d\n", part.super.numFreeInodes);
	fprintf(pFileDump, "  num Blocks       - %d\n", part.super.numBlocks);
	fprintf(pFileDump, "  num Free Blocks  - %d\n", part.super.numFreeBlocks);
	fprintf(pFileDump, "  First Data Block - %d\n", part.super.firstDataBlock);
	fprintf(pFileDump, "  Volume Name      - %s\n", part.super.volumeName);
	fprintf(pFileDump, "-------------------------------------------------------------\n");

	for (int i = 0; i < 224; i++) {
		fread(&part.inodeTable[i], sizeof(inode), 1, pFileSystem);

		fprintf(pFileDump, "---------------------Inode[%3d] Contents---------------------\n", i);
		fprintf(pFileDump, "  Mode           - %d\n", part.inodeTable[i].mode);					//File type + Mode ex) 0x1:0777 = Reg(0x1:0000) + AC_ALL(0x777)
		fprintf(pFileDump, "  Locked         - %d\n", part.inodeTable[i].locked);
		fprintf(pFileDump, "  Date           - %d\n", part.inodeTable[i].date);
		fprintf(pFileDump, "  File Size      - %d bytes\n", part.inodeTable[i].size);
		fprintf(pFileDump, "  Indirect Blcok - %d\n", part.inodeTable[i].indirectBlock);
		fprintf(pFileDump, "  Blocks - ");
		for (int t = 0; t < 6; t++) {
			fprintf(pFileDump, "%d ", part.inodeTable[i].blocks[t]);
		}
		fprintf(pFileDump, "\n-------------------------------------------------------------\n");
	}
	fclose(pFileDump);
}

void printRootDir(void) {
	int blockNum = part.inodeTable[part.super.firstInode].size / BLOCK_SIZE;							//BLOCK_SIZE = 0x400
	int blockLocation;

	if ((part.inodeTable[part.super.firstInode].size % BLOCK_SIZE) != 0) {
		blockNum++;
	}

	pFileDump = fopen("File_dump.txt", "a+");

	fprintf(pFileDump, "-------------------------------------------------Root Directory------------------------------------------------\n");
	for (int i = 0, m = 1; i < blockNum; i++) {
		blockLocation = (BLOCK_SIZE * part.super.firstDataBlock) + (part.inodeTable[part.super.firstInode].blocks[i] * BLOCK_SIZE);
		fseek(pFileSystem, blockLocation, SEEK_SET);
		for (int t = 0; t < BLOCK_SIZE; t = t + 32, m++) {
			fread(&dirEntry.inode, sizeof(int), 1, pFileSystem);
			fread(&dirEntry.dirLength, sizeof(int), 1, pFileSystem);
			fread(&dirEntry.nameLen, sizeof(int), 1, pFileSystem);
			fread(&dirEntry.fileType, sizeof(int), 1, pFileSystem);
			fread(&dirEntry.name, sizeof(char), 16, pFileSystem);
			if (dirEntry.inode == 0) {								//End -> stop printing
				break;
			}
			fprintf(pFileDump, "%15s ", dirEntry.name);
			if (m % 7 == 0) {																		//Every 7 print -> new line
				fprintf(pFileDump, "\n");
			}
		}
	}
	fprintf(pFileDump, "\n---------------------------------------------------------------------------------------------------------------\n");

	fclose(pFileDump);
}

int fileOpen(char* fileName, int mode) {
	int inodeNum = hashFun(fileName);
	int inodeLocation;
	inode inodeBuffer;
	int permission;

	if (inodeNum == -1) {
		printf("There are no file : %s\n", fileName);
		return 1;
	}
	inodeLocation = (sizeof(superBlock)) + (inodeNum * 32);
	fseek(pFileSystem, inodeLocation, SEEK_SET);
	fread(&inodeBuffer, sizeof(inode), 1, pFileSystem);
	permission = inodeBuffer.mode & 0xFFF;

	if (mode == 0) {			//mode 0 -> read mode
		if (permission == 0x777 || permission == 0x1) {
			printf("File [%s] Open as Read Mode\n",fileName);
			return 0;
		}
		else {
			printf("Permission Deny\n");
			return 1;
		}
	}
	else if (mode == 1) {		//mode 1 -> write mode
		if (permission == 0x777 || permission == 0x2) {
			fseek(pFileSystem, inodeLocation, SEEK_SET);
			inodeBuffer.locked = 1;
			fwrite(&inodeBuffer, 1, sizeof(inode), pFileSystem);		//Update inode Lock condition
			printf("File [%s] Open as Write Mode\n",fileName);
			return 0;
		}
		else {
			printf("Permission Deny\n");
			return 1;
		}
	}
	else {
		printf("Wrong Mode : File Open Fail\n");
		return 1;
	}
}

int fileOpen2(char* fileName, int mode) {
	int blockLocation;
	int blockNum = part.inodeTable[part.super.firstInode].size / BLOCK_SIZE;

	if ((part.inodeTable[part.super.firstInode].size % BLOCK_SIZE) != 0) {
		blockNum++;
	}

	if (mode == 0 || mode == 1) {										//mode 0 => Read, mode 1 => write
		for (int i = 0; i < blockNum; i++) {
			blockLocation = (BLOCK_SIZE * part.super.firstDataBlock) + (part.inodeTable[part.super.firstInode].blocks[i] * BLOCK_SIZE);
			fseek(pFileSystem, blockLocation, SEEK_SET);
			for (int t = 0; t < BLOCK_SIZE; t = t + 32) {
				fread(&dirEntry.inode, sizeof(int), 1, pFileSystem);
				fread(&dirEntry.dirLength, sizeof(int), 1, pFileSystem);
				fread(&dirEntry.nameLen, sizeof(int), 1, pFileSystem);
				fread(&dirEntry.fileType, sizeof(int), 1, pFileSystem);
				fread(&dirEntry.name, sizeof(char), 16, pFileSystem);
				if (strcmp(fileName, dirEntry.name) == 0) {					//FILE exist
					printf("File open \n");
					return 0;
				}
			}
		}
	}
	else {
		printf("Wrong Mode\n");
		memset(&dirEntry, 0, sizeof(dentry));
		return 1;
	}
	printf("There are no File\n");
	memset(&dirEntry, 0, sizeof(dentry));
	return 2;
}

void randFileSelect(char* fileName) {
	int fileNum = part.inodeTable[part.super.firstInode].size / 32;										//Directroy Entry = 32 bytes
	int randFile;
	int fileLocation;
	int blockNum;

	//======Random Seed reset========
	randFile = rand();
	srand(time(NULL) + randFile);
	//===============================
	randFile = rand() % fileNum;
	
	blockNum = randFile / 32;

	fileLocation = (BLOCK_SIZE * part.super.firstDataBlock) + (part.inodeTable[part.super.firstInode].blocks[blockNum] * BLOCK_SIZE) + ((randFile % 32) * 32) + 16;
	fseek(pFileSystem, fileLocation, SEEK_SET);
	fread(&dirEntry.name, sizeof(char), 16, pFileSystem);
	strcpy(fileName, dirEntry.name);
}

void fileWrite(char* writeBuffer) {
	int blockNum = part.inodeTable[dirEntry.inode].size / BLOCK_SIZE;
	int fileLocation;
	char writeData[BLOCK_SIZE];
	strcpy(writeData, writeBuffer);

	if ((part.inodeTable[part.super.firstInode].size % BLOCK_SIZE) == 0) {
		blockNum--;
	}

	fileLocation = (BLOCK_SIZE * part.super.firstDataBlock) + (BLOCK_SIZE * part.inodeTable[dirEntry.inode].blocks[blockNum]);
	fseek(pFileSystem, fileLocation, SEEK_SET);
	fwrite(writeData, sizeof(char), BLOCK_SIZE, pFileSystem);
}

void fileClose(char* fileName) {
	memset(&dirEntry, 0, sizeof(dentry));
}

int hashFun(char* fileName) {
	int hash;
	char buffer[20];
	char* fNameBuffer;
	int fNumBuffer;
	
	strcpy(buffer, fileName);

	fNameBuffer = strtok(buffer, " file_");
	if (fNameBuffer == NULL) {
		return -1;					//No hash -> No File
	}
	fNumBuffer = atoi(fNameBuffer);
	return fNumBuffer + 2;			//return inode number
}