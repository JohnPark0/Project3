#include "fs.h"

void mount(void) {
	pFileSystem = fopen("disk.img", "r+");
	FileDump = fopen("File_dump.txt", "w");

	if (pFileSystem == NULL) {
		printf("There are no disk.img,\n");
		exit(0);
	}

	fread(&part.super, sizeof(superBlock), 1, pFileSystem);										//copy sizeof(superBlock) bytes to buffer<part.super>
	fprintf(FileDump, "---------------------SuperBlcok Contents---------------------\n");
	fprintf(FileDump, "  Partition Type   - %d\n", part.super.partitionType);
	fprintf(FileDump, "  Block Size       - %d bytes\n", part.super.blockSize);
	fprintf(FileDump, "  Inode Size       - %d bytes\n", part.super.inodeSize);
	fprintf(FileDump, "  First Inode      - %d\n", part.super.firstInode);
	fprintf(FileDump, "  num Inodes       - %d\n", part.super.numInodes);
	fprintf(FileDump, "  num Inode Blocks - %d\n", part.super.numInodeBlocks);
	fprintf(FileDump, "  num Free Inodes  - %d\n", part.super.numFreeInodes);
	fprintf(FileDump, "  num Blocks       - %d\n", part.super.numBlocks);
	fprintf(FileDump, "  num Free Blocks  - %d\n", part.super.numFreeBlocks);
	fprintf(FileDump, "  First Data Block - %d\n", part.super.firstDataBlock);
	fprintf(FileDump, "  Volume Name      - %s\n", part.super.volumeName);
	fprintf(FileDump, "-------------------------------------------------------------\n");

	for (int i = 0; i < 224; i++) {
		fread(&part.inodeTable[i], sizeof(inode), 1, pFileSystem);

		fprintf(FileDump, "---------------------Inode[%3d] Contents---------------------\n", i);
		fprintf(FileDump, "  Mode           - %d\n", part.inodeTable[i].mode);					//File type + Mode ex) 0x1:0777 = Reg(0x1:0000) + AC_ALL(0x777)
		fprintf(FileDump, "  Locked         - %d\n", part.inodeTable[i].locked);
		fprintf(FileDump, "  Date           - %d\n", part.inodeTable[i].date);
		fprintf(FileDump, "  File Size      - %d bytes\n", part.inodeTable[i].size);
		fprintf(FileDump, "  Indirect Blcok - %d\n", part.inodeTable[i].indirectBlock);
		fprintf(FileDump, "  Blocks - ");
		for (int t = 0; t < 6; t++) {
			fprintf(FileDump, "%d ", part.inodeTable[i].blocks[t]);
		}
		fprintf(FileDump, "\n-------------------------------------------------------------\n");
	}
	fclose(FileDump);
}

void printRootDir(void) {
	int blockNum = part.inodeTable[part.super.firstInode].size / BLOCK_SIZE;							//BLOCK_SIZE = 0x400
	int blockLocation;

	FileDump = fopen("File_dump.txt", "a+");

	fprintf(FileDump, "------------------------Root Directory-----------------------\n");
	for (int i = 0, m = 1; i < blockNum + 1; i++) {
		//blockLocation = First Data Block -> 
		blockLocation = (BLOCK_SIZE * part.super.firstDataBlock) + (part.inodeTable[part.super.firstInode].blocks[i] * BLOCK_SIZE);
		fseek(pFileSystem, blockLocation, SEEK_SET);
		for (int t = 0; t < BLOCK_SIZE; t = t + 32, m++) {
			fread(&dirEntry.inode, sizeof(int), 1, pFileSystem);
			fread(&dirEntry.dirLength, sizeof(int), 1, pFileSystem);
			fread(&dirEntry.nameLen, sizeof(int), 1, pFileSystem);
			fread(&dirEntry.fileType, sizeof(int), 1, pFileSystem);
			fread(&dirEntry.name, sizeof(char), 16, pFileSystem);
			fprintf(FileDump, "%15s ", dirEntry.name);
			if (m % 7 == 0) {																		//Every 7 print -> new line
				fprintf(FileDump, "\n");
			}
		}
	}
	fprintf(FileDump, "\n-------------------------------------------------------------\n");

	fclose(FileDump);
}

int fileOpen(char* name, int mode) {
	int blockLocation;
	int blockNum = part.inodeTable[part.super.firstInode].size / BLOCK_SIZE;

	if (mode == 1 || mode == 2) {										//mode 1 => Read, mode 2 => write
		for (int i = 0; i < blockNum + 1; i++) {
			blockLocation = (BLOCK_SIZE * part.super.firstDataBlock) + (part.inodeTable[part.super.firstInode].blocks[i] * BLOCK_SIZE);
			fseek(pFileSystem, blockLocation, SEEK_SET);
			for (int t = 0; t < BLOCK_SIZE; t = t + 32) {
				fread(&dirEntry.inode, sizeof(int), 1, pFileSystem);
				fread(&dirEntry.dirLength, sizeof(int), 1, pFileSystem);
				fread(&dirEntry.nameLen, sizeof(int), 1, pFileSystem);
				fread(&dirEntry.fileType, sizeof(int), 1, pFileSystem);
				fread(&dirEntry.name, sizeof(char), 16, pFileSystem);
				if (strcmp(name, dirEntry.name) == 0) {					//FILE exist
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

void fileWrite(char* writeBuffer, int dataSize) {
	int blockNum = part.inodeTable[dirEntry.inode].size / BLOCK_SIZE;
	int fileLocation;
	char writeData[BLOCK_SIZE] = "writeData";

	fileLocation = (BLOCK_SIZE * part.super.firstDataBlock) + (BLOCK_SIZE * part.inodeTable[dirEntry.inode].blocks[blockNum]);
	fseek(pFileSystem, fileLocation, SEEK_SET);
	fwrite(writeData, sizeof(char), BLOCK_SIZE, pFileSystem);
}


void fileClose(void) {
	memset(&dirEntry, 0, sizeof(dentry));
}