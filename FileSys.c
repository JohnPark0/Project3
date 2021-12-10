#include "fs.h"

void mount(void) {
	pFileSystem = fopen("disk.img", "r+");
	File_dump = fopen("File_dump.txt", "w");

	if (pFileSystem == NULL) {
		printf("There are no disk.img,\n");
		exit(0);
	}

	fread(&part.s, sizeof(super_block), 1, pFileSystem);		//copy sizeof(super_block) bytes to buffer<part.s>
	fprintf(File_dump, "---------------------SuperBlcok Contents---------------------\n");
	fprintf(File_dump, "  Partition Type   - %d\n", part.s.partition_type);
	fprintf(File_dump, "  Block Size       - %d bytes\n", part.s.block_size);
	fprintf(File_dump, "  Inode Size       - %d bytes\n", part.s.inode_size);
	fprintf(File_dump, "  First Inode      - %d\n", part.s.first_inode);
	fprintf(File_dump, "  num Inodes       - %d\n", part.s.num_inodes);
	fprintf(File_dump, "  num Inode Blocks - %d\n", part.s.num_inode_blocks);
	fprintf(File_dump, "  num Free Inodes  - %d\n", part.s.num_free_inodes);
	fprintf(File_dump, "  num Blocks       - %d\n", part.s.num_blocks);
	fprintf(File_dump, "  num Free Blocks  - %d\n", part.s.num_free_blocks);
	fprintf(File_dump, "  First Data Block - %d\n", part.s.first_data_block);
	fprintf(File_dump, "  Volume Name      - %s\n", part.s.volume_name);
	fprintf(File_dump, "-------------------------------------------------------------\n");

	for (int i = 0; i < 224; i++) {
		fread(&part.inode_table[i], sizeof(inode), 1, pFileSystem);
		
		fprintf(File_dump, "---------------------Inode[%3d] Contents---------------------\n", i);
		fprintf(File_dump, "  Mode           - %d\n", part.inode_table[i].mode);					//File type + Mode ex) 0x1:0777 = Reg(0x1:0000) + AC_ALL(0x777)
		fprintf(File_dump, "  Locked         - %d\n", part.inode_table[i].locked);
		fprintf(File_dump, "  Date           - %d\n", part.inode_table[i].date);
		fprintf(File_dump, "  File Size      - %d bytes\n", part.inode_table[i].size);
		fprintf(File_dump, "  Indirect Blcok - %d\n", part.inode_table[i].indirect_block);
		fprintf(File_dump, "  Blocks - ");
		for (int t = 0; t < 6; t++) {
			fprintf(File_dump, "%d ", part.inode_table[i].blocks[t]);
		}
		fprintf(File_dump, "\n-------------------------------------------------------------\n");
	}
	fclose(File_dump);
}

void printRootDir(void) {
	int blockNum = part.inode_table[part.s.first_inode].size / BLOCK_SIZE;							//BLOCK_SIZE = 0x400
	int blockLocation;

	File_dump = fopen("File_dump.txt", "a+");

	fprintf(File_dump, "------------------------Root Directory-----------------------\n");
	for (int i = 0, m = 1; i < blockNum + 1; i++) {
		//blockLocation = First Data Block -> 
		blockLocation = (BLOCK_SIZE * part.s.first_data_block) + (part.inode_table[part.s.first_inode].blocks[i] * BLOCK_SIZE);
		fseek(pFileSystem, blockLocation, SEEK_SET);
		for (int t = 0; t < BLOCK_SIZE; t = t + 32, m++) {
			fread(&dirEntry.inode, sizeof(int), 1, pFileSystem);
			fread(&dirEntry.dir_length, sizeof(int), 1, pFileSystem);
			fread(&dirEntry.name_len, sizeof(int), 1, pFileSystem);
			fread(&dirEntry.file_type, sizeof(int), 1, pFileSystem);
			fread(&dirEntry.name, sizeof(char), 16, pFileSystem);
			fprintf(File_dump, "%15s ", dirEntry.name);
			if (m % 7 == 0) {																		//Every 7 print -> new line
				fprintf(File_dump, "\n");
			}
		}
	}
	fprintf(File_dump, "\n-------------------------------------------------------------\n");

	fclose(File_dump);
}