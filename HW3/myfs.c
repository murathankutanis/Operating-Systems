/////////////////////// SOURCE FILE README/////////////////////////

//This file contains the source code (`myfs.c`) for a simple user mode file 
// system (FS) that employs a file allocation table (FAT). 

// Command-line Options
// * `./myfs disk –format`: Overwrites the disk header with an empty FAT and an empty file list.
// * `./myfs disk -write source_file destination_file`: Copies a file to the disk with the specified name.
// * `./myfs disk -read source_file destination_file`: Copies a file from the disk to the computer.
// * `./myfs disk -delete file`: Deletes a file in the disk.
// * `./myfs disk –list`: Prints all visible files and their respective sizes in the disk.
// * `./myfs disk –sorta`: Sorts files by size in ascending order.
// * `./myfs disk –rename source_file new_name`: Renames a file in the disk..
// * `./myfs disk –duplicate source_file`: Duplicates a file in the disk..
// * `./myfs disk -search source_file`: Searches for a file in the disk and gives a result of "YES" or "NO".
// * `./myfs disk –hide source_file`: Hides a file in the disk.
// * `./myfs disk –unhide source_file`: Unhides a file in the disk.
// * `./myfs disk –printfilelist`: Prints the File List to the "filelist.txt" file.
// * `./myfs disk –printfat`: Prints the File Allocation Table (FAT) to the "fat.txt" file.
// * `./myfs disk –defragment`: Merges fragmented files into one contiguous space or block.

// In the code, there are multiple usage of IA code generators 
// accompanying with different open source repositories. The used
// repositories are given in the reference at the end.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DataSize 512
#define FatSize 4096
#define ListSize 128
#define NameSize 248

// Define Structures
typedef struct {
    char data[DataSize];
} DataBlock;

typedef struct {
    DataBlock datablock[DataSize];
} Data;

typedef struct {
    unsigned int dummy;
} FatEntry;

typedef struct {
    FatEntry listentries[FatSize];
} FAT;

typedef struct {
    char filename[NameSize];
    uint32_t first_block;
    uint32_t size;
    int hidden; 
} FileEntry;

typedef struct {
    FileEntry filelist[ListSize];
} FileList;

// Global Variables Declaration
char *disk_location;
FILE *disk, *disk2;

// Locate a free FAT entry 
int FindFreeFatEntry(FAT *fat, int start_index) {
    for (int i = start_index; i < FatSize; i++) {
        if (fat->listentries[i].dummy == 0x00000000) {
            return i;
        }
    }
    return -1;
}

// Locate an available entry 
int FindFreeFileListEntry(FileList *filelist) {
    for (int i = 0; i < ListSize; i++) {
        if (filelist->filelist[i].filename[0] == '\0') {
            return i;
        }
    }
    return -1;
}

// Return the index of the file in the filelist.
int FindFileInList(FileList *filelist, char *file_name) {
    for (int i = 0; i < ListSize; i++) {
        char *f_name = filelist->filelist[i].filename;
        if (strcmp(f_name, file_name) == 0) {
            return i;
        }
    }
    return -1;
}

// Convert little endian to big endian and vice versa
uint32_t EndianConversion(uint32_t dummy) {
    return ((dummy & 0xFF) << 24) | ((dummy & 0xFF00) << 8) |
           ((dummy & 0xFF0000) >> 8) | ((dummy & 0xFF000000) >> 24);
}


// Function to format disk
void FormatDisk() {
    // Open Disk
    FILE *disk = fopen(disk_location, "wb+");
    if (disk == NULL) {
        printf("Error: Could not open disk\n");
        return;
    }

    // Initialize and create FAT
    FAT fat;
    memset(&fat, 0, sizeof(FAT));
    fat.listentries[0].dummy = 0xFFFFFFFF;

    // Initialize and create File List
    FileList filelist;
    memset(&filelist, 0, sizeof(FileList));

    // Write FAT and File List to disk
    fseek(disk, 0, SEEK_SET);
    fwrite(&fat, sizeof(FAT), 1, disk);
    fwrite(&filelist, sizeof(FileList), 1, disk);

    // Initialize and create Data Blocks
    Data data;
    memset(&data, 0, sizeof(Data));

    // Write Data Blocks to disk
    fseek(disk, sizeof(FAT) + sizeof(FileList), SEEK_SET);
    fwrite(&data, sizeof(Data), 1, disk);

    printf("Disk formatted\n");
    fclose(disk);
}


// Write file to disk
void WriteToDisk(char *src_path, char *dest_file_name) {
    FILE *disk = fopen(disk_location, "rb+");
    
    if (disk == NULL) {
        printf("Error: Could not open disk\n");
        return;
    }
    
    // Get data from the disk
    FAT *t_fat = malloc(sizeof(FAT));
    fseek(disk, 0, SEEK_SET);
    fread(t_fat, sizeof(FAT), 1, disk);

    FileList *t_filelist = malloc(sizeof(FileList));
    fseek(disk, sizeof(FAT), SEEK_SET);
    fread(t_filelist, sizeof(FileList), 1, disk);

    FILE *src_file = fopen(src_path, "rb");
    if (src_file == NULL) {
        printf("Error: Could not open source file\n");
        fclose(disk);
        free(t_fat);
        free(t_filelist);
        return;
    }

    // Find free file list entry
    int filelist_index = FindFreeFileListEntry(t_filelist);
    if (filelist_index == -1) {
        printf("Error: File list is full\n");
        fclose(src_file);
        fclose(disk);
        free(t_fat);
        free(t_filelist);
        return;
    }

    // Find free FAT entry
    int fat_index = FindFreeFatEntry(t_fat, 1);
    if (fat_index == -1) {
        printf("Error: Disk is full\n");
        printf("Error: File list is full\n");
        fclose(src_file);
        fclose(disk);
        free(t_fat);
        free(t_filelist);
        return;
    }

    // Count free FAT entries
    int free_fat_entries = 0;
    for (int i = 0; i < FatSize; i++) {
        if (t_fat->listentries[i].dummy == 0x00000000) {
            free_fat_entries++;
        }
    }
    
    //int free_fat_entries = CountFreeFATEntries(t_fat);
    
    int f_size = 0;
    int n_clusters = 0;
    while (!feof(src_file)) {
        DataBlock block;
        int b_read = fread(block.data, 1, DataSize, src_file);
        if (b_read > 0) {
            n_clusters++;
            f_size += b_read;
        }
    }

    src_file = fopen(src_path, "rb");

    if (free_fat_entries < n_clusters) {
        fclose(src_file);
        fclose(disk);
        free(t_fat);
        free(t_filelist);
        printf("Error: Not enough free space on disk\n");
        return;
    }

    int num_clusters = 0;

    // Write file to disk
    FileEntry file_entry;
    file_entry.first_block = fat_index;
    file_entry.size = 0;
    file_entry.hidden = 0;
    strncpy(file_entry.filename, dest_file_name, NameSize);
    file_entry.filename[NameSize - 1] = '\0';  // Ensure null-termination

    int data_index = fat_index;
    while (!feof(src_file)) {
        DataBlock data_block;
        int bytes_read = fread(data_block.data, 1, DataSize, src_file);
        if (bytes_read == 0) {
            break;
        }
        fseek(disk,
              sizeof(FAT) + sizeof(FileList) + (data_index * sizeof(DataBlock)),
              SEEK_SET);
        fwrite(&data_block, sizeof(DataBlock), 1, disk);

        // Update FAT
        uint32_t fat_dummy;
        if (bytes_read < DataSize) {
            fat_dummy = 0xFFFFFFFF;
        } else {
            fat_dummy = FindFreeFatEntry(t_fat, data_index + 1);
        }

        if (t_fat->listentries[data_index].dummy == 0x00000000) {
            t_fat->listentries[data_index].dummy = EndianConversion(fat_dummy);
        }

        if (bytes_read > 0) {
            num_clusters++;
        }

        // Update fileentry
        file_entry.size += bytes_read;

        // Move to next data block
        data_index = t_fat->listentries[data_index].dummy;
        data_index = EndianConversion(data_index);
    }

    // Update filelist
    t_filelist->filelist[filelist_index] = file_entry;
    t_filelist->filelist[filelist_index].hidden = 0;

    // Write FAT and filelist to disk
    fseek(disk, 0, SEEK_SET);
    fwrite(t_fat, sizeof(FAT), 1, disk);
    fwrite(t_filelist, sizeof(FileList), 1, disk);

    printf("File: %s written to disk\n", dest_file_name);
    fclose(src_file);
    fclose(disk);
    free(t_fat);
    free(t_filelist);
}

// Read file from disk
void ReadFromDisk(char *src_file_name, char *dest_path) {

    // Open the disk for reading
    FILE *disk = fopen(disk_location, "rb");
    if (disk == NULL) {
        printf("Error: Could not open disk\n");
    return;
    }
    
    // Read from Disk FAT
    FAT *t_fat = malloc(sizeof(FAT));
    fseek(disk, 0, SEEK_SET);
    fread(t_fat, sizeof(FAT), 1, disk);

    // Read from Disk File List
    FileList *t_filelist = malloc(sizeof(FileList));
    fseek(disk, sizeof(FAT), SEEK_SET);
    fread(t_filelist, sizeof(FileList), 1, disk);

    // Find the file in the File List
    int file_index = FindFileInList(t_filelist, src_file_name);
    if (file_index == -1) {
        printf("Error: File not found\n");
        fclose(disk);
        free(t_fat);
        free(t_filelist);
        return;
    }

    // Open the destination file for writing
    FILE *dest_file = fopen(dest_path, "wb");
    if (dest_file == NULL) {
        printf("Error: Could not open destination file\n");
        fclose(disk);
        free(t_fat);
        free(t_filelist);
        return;
    }

    // Read file from disk
    int data_index = t_filelist->filelist[file_index].first_block;
    int remaining_size = t_filelist->filelist[file_index].size;

    // Read data block from disk and check if it is the last one
    while (data_index != 0xFFFFFFFF && remaining_size > 0) {
        // Read data block from disk
        DataBlock data_block;
        fseek(disk,sizeof(FAT) + sizeof(FileList) + (data_index * sizeof(DataBlock)),SEEK_SET);
        fread(&data_block, sizeof(DataBlock), 1, disk);

        // Write data block to destination file
        int bytes_to_write =
            (remaining_size < DataSize) ? remaining_size : DataSize; fwrite(data_block.data, 1, bytes_to_write, dest_file);

        // Move to next data block
        data_index = t_fat->listentries[data_index].dummy;
        data_index = EndianConversion(data_index);
        remaining_size -= bytes_to_write;
    }

    // Close the disk and destination file
    fclose(dest_file);
    fclose(disk);

    printf("File: %s read from disk\n", src_file_name);

    // Free memory
    free(t_fat);
    free(t_filelist);
}

// Deletes a file from the disk
void DeleteFromDisk(char *src_file_name) {
    FILE *disk = fopen(disk_location, "rb+");
    
    if (disk == NULL) {
        printf("Error: Could not open disk\n");
    return;
    }
    
    // Read from Disk FAT
    FAT *t_fat = malloc(sizeof(FAT));
    fseek(disk, 0, SEEK_SET);
    fread(t_fat, sizeof(FAT), 1, disk);

    // Read from Disk File List
    FileList *t_filelist = malloc(sizeof(FileList));
    fseek(disk, sizeof(FAT), SEEK_SET);
    fread(t_filelist, sizeof(FileList), 1, disk);

    // Find the file in the File List
    int file_index = FindFileInList(t_filelist, src_file_name);
    if (file_index == -1) {
        printf("Error: File not found\n");
        fclose(disk);
        free(t_fat);
        free(t_filelist);
        return;
    }

    // Free the FAT entries occupied by the file
    int block_index = t_filelist->filelist[file_index].first_block;
    while (block_index != 0xFFFFFFFF) {
        int next_block_index = t_fat->listentries[block_index].dummy;
        next_block_index = EndianConversion(next_block_index);
        t_fat->listentries[block_index].dummy = 0x00000000;
        block_index = next_block_index;
    }

    // Remove the file from the file list
    t_filelist->filelist[file_index].filename[0] = '\0';
    t_filelist->filelist[file_index].first_block = 0;
    t_filelist->filelist[file_index].size = 0;

    printf("File deleted: %s\n", src_file_name);

    // Write to Disk FAT
    fseek(disk, 0, SEEK_SET);
    fwrite(t_fat, sizeof(FAT), 1, disk);

    // Write to Disk File List
    fseek(disk, sizeof(FAT), SEEK_SET);
    fwrite(t_filelist, sizeof(FileList), 1, disk);


    fclose(disk);
    free(t_fat);
    free(t_filelist);
}

// List all files on the disk
void List() {
    disk = fopen(disk_location, "rb");

    // Read from Disk File List
    FileList *t_filelist = malloc(sizeof(FileList));
    fseek(disk, sizeof(FAT), SEEK_SET);
    fread(t_filelist, sizeof(FileList), 1, disk);

    // Check if there are any files on the disk
    if (t_filelist->filelist[0].filename[0] == '\0' &&
        t_filelist->filelist[1].filename[1] == '\0' &&
        t_filelist->filelist[2].filename[2] == '\0' &&
        t_filelist->filelist[4].filename[4] == '\0' &&
        t_filelist->filelist[5].filename[5] == '\0') {
        printf("No files on disk\n");
        return;
    }
    
    printf("Filename\tSize\n");
    for (int i = 0; i < ListSize; i++) {
        char *filename = t_filelist->filelist[i].filename;
        int size = t_filelist->filelist[i].size;
        int hidden = t_filelist->filelist[i].hidden;
        
        if (filename[0] == '\0' || hidden) {
            continue; // Skip hidden files or empty entries
        }
        
        printf("%-15s\t%d\n", filename, size);
    }
    // Close the disk
    fclose(disk);

    // Free the allocated memory
    free(t_filelist);
}

// Arrange the file list in ascending order based on file size
void SortFilesBySize() {
    FILE *disk = fopen(disk_location, "rb+");
    
    if (disk == NULL) {
        printf("Error: Could not open disk\n");
    return;
    }	
    
    // Read from Disk
    FileList *t_filelist = malloc(sizeof(FileList));
    fseek(disk, sizeof(FAT), SEEK_SET);
    fread(t_filelist, sizeof(FileList), 1, disk);

    // Close the disk
    fclose(disk);

    // Whether there are file on the disk
    if (t_filelist->filelist[0].filename[0] == '\0') {
        printf("No files on disk\n");
        return;
    }

    // Sort files in the disk using bubble sort (Ascending Order)
    for (int i = 0; i < ListSize - 1; i++) {
        for (int j = i + 1; j < ListSize; j++) {
            int i_size = t_filelist->filelist[i].size;
            int j_size = t_filelist->filelist[j].size;

            if (i_size > j_size) {
                FileEntry temp = t_filelist->filelist[i];
                t_filelist->filelist[i] = t_filelist->filelist[j];
                t_filelist->filelist[j] = temp;
            }
        }
    }
    
    // Display sorted file list
    printf("Filename\tSize\n");
    for (int i = 0; i < ListSize; i++) {
        char *filename = t_filelist->filelist[i].filename;
        int size = t_filelist->filelist[i].size;

    // Check whether file entry is valid 
        if (filename[0] != '.' && filename[0] != '\0') {
            printf("%-15s\t%d\n", filename, size);
        }
    }
    // Free allocated Memory
    free(t_filelist);
}

// Function to rename a file on the disk
void RenameFile(char *source_file, char *new_name) {
    FILE *disk = fopen(disk_location, "rb+");
    if (disk == NULL) {
        printf("Error: Could not open disk\n");
        return;
    }

    FileList *t_filelist = malloc(sizeof(FileList));
    
    //Get the files
    fseek(disk, sizeof(FAT), SEEK_SET);
    fread(t_filelist, sizeof(FileList), 1, disk);

    // Index of the source_file
    int index = -1;
    for (int i = 0; i < ListSize; i++) {
        if (strcmp(t_filelist->filelist[i].filename, source_file) == 0) {
            index = i;
            break;
        }
    }

    // Error case
    if (index == -1) {
        printf("Error: File not found\n");
        fclose(disk);
        free(t_filelist);
        return;
    }
    
    // Check if the new name is the same as the old name
    if (strcmp(source_file, new_name) == 0) {
        printf("Error: New name is the same as the old name\n");
        fclose(disk);
        free(t_filelist);
        return;
    }
    
    // Check if the new name is the same as the old name
    if (strcmp(source_file, new_name) == 0) {
        printf("Error: New name is the same as the old name\n");
        fclose(disk);
        free(t_filelist);
        return;
    }
    
    // Rename the source_file
    strncpy(t_filelist->filelist[index].filename, new_name, NameSize);
    t_filelist->filelist[index].filename[NameSize - 1] = '\0';

    // Update the disk
    fseek(disk, sizeof(FAT), SEEK_SET);
    fwrite(t_filelist, sizeof(FileList), 1, disk);
    printf("The name of the file %s is changed to %s \n", source_file, new_name);
    fclose(disk);
    free(t_filelist);
}

// Function to duplicate a file on the disk

void DuplicateFile(char *source_file) {
    FILE *disk = fopen(disk_location, "rb+");
    if (disk == NULL) {
        printf("Error: Could not open disk\n");
        return;
    }

    FileList *t_filelist = malloc(sizeof(FileList));
    if (t_filelist == NULL) {
        printf("Error: Memory allocation failed\n");
        fclose(disk);
        return;
    }
    
    // Read the file 
    fseek(disk, sizeof(FAT), SEEK_SET);
    fread(t_filelist, sizeof(FileList), 1, disk);

    // Find the index of the source_file
    int index = -1;
    for (int i = 0; i < ListSize; i++) {
        if (strcmp(t_filelist->filelist[i].filename, source_file) == 0) {
            index = i;
            break;
        }
    }

    // Whether the file is available
    if (index == -1) {
        printf("Error: File not found\n");
        fclose(disk);
        free(t_filelist);
        return;
    }
    
    // Empty Slot
    int copy_index = -1;
    for (int i = 0; i < ListSize; i++) {
        if (t_filelist->filelist[i].filename[0] == '\0') {
            copy_index = i;
            break;
        }
    }

    // Whether the empty slot available
    if (copy_index == -1) {
        printf("Error: No space to duplicate file\n");
        fclose(disk);
        free(t_filelist);
        return;
    }
    
    // Create the duplicate file entry
    snprintf(t_filelist->filelist[copy_index].filename, NameSize, "%s_copy", source_file);
    t_filelist->filelist[copy_index].first_block = t_filelist->filelist[index].first_block;
    t_filelist->filelist[copy_index].size = t_filelist->filelist[index].size;
    t_filelist->filelist[copy_index].hidden = 0;

    // Update the Disk
    fseek(disk, sizeof(FAT), SEEK_SET);
    fwrite(t_filelist, sizeof(FileList), 1, disk);

    fclose(disk);
    free(t_filelist);
}

// Function to search for a file on the disk
void Search(char *source_file) {
    FILE *disk = fopen(disk_location, "rb");
    if (disk == NULL) {
        printf("Error: Could not open disk\n");
        return;
    }

    FileList *t_filelist = malloc(sizeof(FileList));
    
    // Read the file list from the disk
    fseek(disk, sizeof(FAT), SEEK_SET);
    fread(t_filelist, sizeof(FileList), 1, disk);

    // Search for the file in the file list
    int found = 0;
    for (int i = 0; i < ListSize; i++) {
        if (strcmp(t_filelist->filelist[i].filename, source_file) == 0 && t_filelist->filelist[i].hidden == 0) {
            found = 1;
            break;
        }
    }

    if (found) {
        printf("YES\n");
    } else {
        printf("NO\n");
    }

    fclose(disk);
    free(t_filelist);
}

void Hide(char *source_file) {
    // Open the disk
    FILE *disk = fopen(disk_location, "rb+");
    if (disk == NULL) {
        printf("Error: Could not open disk\n");
        return;
    }

    // Read the FileList from the disk
    FileList *t_filelist = malloc(sizeof(FileList));
    fseek(disk, sizeof(FAT), SEEK_SET);
    fread(t_filelist, sizeof(FileList), 1, disk);

    // Find the file in the FileList
    int index = -1;
    for (int i = 0; i < ListSize; i++) {
        if (strcmp(t_filelist->filelist[i].filename, source_file) == 0) {
            index = i;
            break;
        }
    }

    // Check if the file is found
    if (index == -1) {
        printf("Error: File not found\n");
        fclose(disk);
        free(t_filelist);
        return;
    }

    // Update the FileList to hide the file
    t_filelist->filelist[index].hidden = 1;

    // Write the updated FileList back to the disk
    fseek(disk, sizeof(FAT), SEEK_SET);
    fwrite(t_filelist, sizeof(FileList), 1, disk);

    printf("File: %s is hided successfully \n", source_file);
    // Close the disk and free memory
    fclose(disk);
    free(t_filelist);
}

void Unhide(char *source_file) {
    // Open the disk
    FILE *disk = fopen(disk_location, "rb+");
    if (disk == NULL) {
        printf("Error: Could not open disk\n");
        return;
    }

    // Read the FileList from the disk
    FileList *t_filelist = malloc(sizeof(FileList));
    fseek(disk, sizeof(FAT), SEEK_SET);
    fread(t_filelist, sizeof(FileList), 1, disk);

    // Find the file in the FileList
    int index = -1;
    for (int i = 0; i < ListSize; i++) {
        if (strcmp(t_filelist->filelist[i].filename, source_file) == 0 &&
            t_filelist->filelist[i].hidden == 1) {
            index = i;
            break;
        }
    }

    // Check if the file is found and hidden
    if (index == -1) {
        printf("Error: File not found or not hidden\n");
        fclose(disk);
        free(t_filelist);
        return;
    }

    // Update the FileList to unhide the file
    t_filelist->filelist[index].hidden = 0;

    // Write the updated FileList back to the disk
    fseek(disk, sizeof(FAT), SEEK_SET);
    fwrite(t_filelist, sizeof(FileList), 1, disk);
    printf("File: %s is unhided successfully \n", source_file);
    // Close the disk and free memory
    fclose(disk);
    free(t_filelist);
}

// Creates the filelist.txt
void PrintFileList() {
    FILE *disk = fopen(disk_location, "rb");
    
    if (disk == NULL) {
        printf("Error: Could not open disk\n");
        return;
    }
    
    FileList *t_filelist = malloc(sizeof(FileList));
    if (t_filelist == NULL) {
        printf("Error: Memory allocation failed\n");
        fclose(disk);
        return;
    }
    
    // Read File
    fseek(disk, sizeof(FAT), SEEK_SET);
    fread(t_filelist, sizeof(FileList), 1, disk);

    // Write to file
    FILE *filelist_file = fopen("filelist.txt", "w");
    
    if (filelist_file == NULL) {
        printf("Error: Could not create filelist.txt\n");
        fclose(disk);
        free(t_filelist);
        return;
    }
    
    fprintf(filelist_file,
            "Item\tFilename\t\tFirst Block\t\tFile Size(bytes)\n");
    for (int i = 0; i < ListSize; i++) {
        char *filename = t_filelist->filelist[i].filename;
        uint32_t first_block = t_filelist->filelist[i].first_block;
        uint32_t size = t_filelist->filelist[i].size;
        int hidden = t_filelist->filelist[i].hidden;

	// Create a list
        if (filename[0] == '\0' || hidden) {
            continue; // Skip hidden files or empty entries
        }

        fprintf(filelist_file, "%d\t\t%-15s\t%4d\t\t\t%d\n", i, filename, first_block, size);
    }
    
    printf("File: filelist.txt is created successfully \n");
    // Close the disk and file
    fclose(filelist_file);
    fclose(disk);

    // Free the memory
    free(t_filelist);
}

// Creates the fat.txt 
void PrintFAT() {
    // Open the disk
    disk = fopen(disk_location, "rb+");
    
    //Open Disk error
    if (disk == NULL) {
        printf("Error: Could not open disk\n");
        return;
    }
    
    // Read Disk FAT
    FAT *t_fat = malloc(sizeof(FAT));
   
   // Memory Allocation Error
    if (t_fat == NULL) {
        printf("Error: Memory allocation failed\n");
        fclose(disk);
        return;
    }
    
    fseek(disk, 0, SEEK_SET);
    fread(t_fat, sizeof(FAT), 1, disk);

    // Print the FAT
    FILE *fat_file = fopen("fat.txt", "w");
    
    // Create file error
    if (fat_file == NULL) {
        printf("Error: Could not create fat.txt\n");
        fclose(disk);
        free(t_fat);
        return;
    }
    fprintf(fat_file,
            "Entry\tdummy\t\tEntry\tdummy\t\tEntry\tdummy\t\tEntry\tdummy\n");
    for (int i = 0; i < FatSize; i += 4) {
        fprintf(fat_file, "%04x\t%08x\t%04x\t%08x\t%04x\t%08x\t%04x\t%08x\n", i,
                t_fat->listentries[i].dummy, i + 1,
                t_fat->listentries[i + 1].dummy, i + 2,
                t_fat->listentries[i + 2].dummy, i + 3,
                t_fat->listentries[i + 3].dummy);
    }

    printf("File: fat.txt is created successfully \n");
    
    // Close the disk and file
    fclose(fat_file);
    fclose(disk);

    // Free the memory
    free(t_fat);
}

// // Function to defragment the disk
void Defragment() {
    // Open the disk for reading and writing
    FILE *disk = fopen(disk_location, "rb+");
    if (disk == NULL) {
        printf("Error: Could not open disk\n");
        return;
    }

    // Create dummy file to write to
    char temp_file_name[] = "/tmp/disk.XXXXXX";
    int temp_file_descriptor = mkstemp(temp_file_name);
    if (temp_file_descriptor == -1) {
        printf("Error: could not create dummy file.\n");
        fclose(disk);
        return;
    }

    // Open dummy file
    FILE *disk2 = fdopen(temp_file_descriptor, "wb+");
    if (disk2 == NULL) {
        printf("Error: Could not open dummy file\n");
        fclose(disk);
        remove(temp_file_name);
        return;
    }

    // Read from Disk
    FAT *t_fat = malloc(sizeof(FAT));
    if (t_fat == NULL) {
        printf("Error: Memory allocation failed\n");
        fclose(disk);
        fclose(disk2);
        remove(temp_file_name);
        return;
    }
    fseek(disk, 0, SEEK_SET);
    fread(t_fat, sizeof(FAT), 1, disk);

    FileList *t_filelist = malloc(sizeof(FileList));
    if (t_filelist == NULL) {
        printf("Error: Memory allocation failed\n");
        fclose(disk);
        fclose(disk2);
        remove(temp_file_name);
        free(t_fat);
        return;
    }
    fseek(disk, sizeof(FAT), SEEK_SET);
    fread(t_filelist, sizeof(FileList), 1, disk);

    // Create new FAT and File List
    FAT *new_fat = malloc(sizeof(FAT));
    
    if (new_fat == NULL) {
        printf("Error: Memory allocation failed\n");
        fclose(disk);
        fclose(disk2);
        remove(temp_file_name);
        free(t_fat);
        free(t_filelist);
        return;
    }
    memcpy(new_fat, t_fat, sizeof(FAT));

    FileList *new_filelist = malloc(sizeof(FileList));
    memcpy(new_filelist, t_filelist, sizeof(FileList));

    printf("Defragmenting...\n");

    // Find number of files
    int num_files = 0;
    for (int i = 0; i < ListSize; i++) {
        if (t_filelist->filelist[i].filename[0] != '\0') {
            num_files++;
        }
    }

    // Find first blocks of each file
    uint32_t first_blocks[num_files];
    for (int i = 0; i < num_files; i++) {
        // If file exists, add first block to array
        if (t_filelist->filelist[i].filename[0] != '\0') {
            first_blocks[i] = t_filelist->filelist[i].first_block;
        } else {
            first_blocks[i] = 0xFFFFFFFF;
        }
    }

    printf("Number of files: %d\n", num_files);
    // Move data blocks and update FAT and File List
    int new_index = 1;
    for (int i = 0; i < num_files; i++) {
        printf("%d ", first_blocks[i]);

        int once = 0;
        int dif = 0;
        int current_block = first_blocks[i];
        while (current_block != 0xFFFFFFFF) {
            // Read data block from the original disk
            int next_block = t_fat->listentries[current_block].dummy;
            DataBlock data_block;
            // Write data block to the temporary disk
            fseek(disk,
                  sizeof(FAT) + sizeof(FileList) +
                      (current_block * sizeof(DataBlock)),
                  SEEK_SET);
            fread(&data_block, sizeof(DataBlock), 1, disk);
            fseek(disk2,
                  sizeof(FAT) + sizeof(FileList) +
                      (new_index * sizeof(DataBlock)),
                  SEEK_SET);
            fwrite(&data_block, sizeof(DataBlock), 1, disk2);

            next_block = EndianConversion(next_block);

            // If last block, set next block to 0xFFFFFFFF
            if (next_block == 0xFFFFFFFF) {
                new_fat->listentries[new_index].dummy =
                    EndianConversion(next_block);
            } else {
                new_fat->listentries[new_index].dummy =
                    EndianConversion(new_index + 1);
            }

            // // Write data block to the temporary disk
            if (once == 0) {
                new_filelist->filelist[i].first_block = new_index;
                once = 1;
            }

            current_block = next_block;
            new_index++;
        }
    }
    printf("\n");

    // Write the new FAT and File List to the temporary disk
    fseek(disk2, 0, SEEK_SET);
    fwrite(new_fat, sizeof(FAT), 1, disk2);
    fwrite(new_filelist, sizeof(FileList), 1, disk2);
    free(t_fat);
    free(t_filelist);
    free(new_fat);
    free(new_filelist);
    
    // Close files
    fclose(disk);
    fclose(disk2);
    
    // Rename the temporary file to the original disk location
    if (rename(temp_file_name, disk_location) == -1) {
        printf("Error: Could not rename temporary file\n");
        remove(temp_file_name);
        return;
    }
    printf("Defragmentation complete\n");
}

// Check if disk can be opened in a function
int OpenDisk(char *loc) {
    // Check if disk exists
    FILE *disk = fopen(loc, "rb+");
    if (disk == NULL) {
        printf("Error: Could not open disk\n");
        return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s folder_name -function [args]\n", argv[0]);
        return 1;
    }

    // Get the disk location
    disk_location = argv[1];

    // Open the disk
    int disk_opened = OpenDisk(disk_location);
    char *command = argv[2];

    if (disk_opened == 0) {
        printf("Error: Could not open disk\n");
        return 1;
    }

    if (strcmp(command, "-format") == 0) {
        FormatDisk();
    } else if (strcmp(command, "-write") == 0) {
        if (argc != 5) {
            printf("Error: Invalid number of arguments for -write\n");
            return 1;
        }
        WriteToDisk(argv[3], argv[4]);
    } else if (strcmp(command, "-read") == 0) {
        if (argc != 5) {
            printf("Error: Invalid number of arguments for -read\n");
            return 1;
        }
        ReadFromDisk(argv[3], argv[4]);
    } else if (strcmp(command, "-delete") == 0) {
        if (argc != 4) {
            printf("Error: Invalid number of arguments for -delete\n");
            return 1;
        }
        DeleteFromDisk(argv[3]);
    } else if (strcmp(command, "-list") == 0) {
        if (argc != 3) {
            printf("Error: Invalid number of arguments for -list\n");
            return 1;
        }
        List();
    } else if (strcmp(command, "-sorta") == 0) {
        if (argc != 3) {
            printf("Error: Invalid number of arguments for -sorta\n");
            return 1;
        }
        SortFilesBySize();
    } else if (strcmp(command, "-rename") == 0) {
        if (argc != 5) {
            printf("Error: Invalid number of arguments for -rename\n");
            return 1;
        }
        RenameFile(argv[3], argv[4]);
    } else if (strcmp(command, "-duplicate") == 0) {
        if (argc != 4) {
            printf("Error: Invalid number of arguments for -duplicate\n");
            return 1;
        }
        DuplicateFile(argv[3]);
    } else if (strcmp(command, "-search") == 0) {
        if (argc != 4) {
            printf("Error: Invalid number of arguments for -search\n");
            return 1;
        }
        Search(argv[3]);
    } else if (strcmp(command, "-hide") == 0) {
        if (argc != 4) {
            printf("Error: Invalid number of arguments for -hide\n");
            return 1;
        }
        Hide(argv[3]);
    } else if (strcmp(command, "-unhide") == 0) {
        if (argc != 4) {
            printf("Error: Invalid number of arguments for -unhide\n");
            return 1;
        }
        Unhide(argv[3]);
    } else if (strcmp(command, "-printfilelist") == 0) {
        if (argc != 3) {
            printf("Error: Invalid number of arguments for -printfilelist\n");
            return 1;
        }
        PrintFileList();    
    } else if (strcmp(command, "-printfat") == 0) {
        if (argc != 3) {
            printf("Error: Invalid number of arguments for -printfat\n");
            return 1;
        }
        PrintFAT();
    } else if (strcmp(command, "-defragment") == 0) {
        if (argc != 3) {
            printf("Error: Invalid number of arguments for -defragment\n");
            return 1;
        }
        Defragment();
    } else {
        printf("Error: Invalid command\n");
        return 1;
    }

    return 0;
}

// Referneces:
// *AlperKutay. (n.d.). EE-442-OPERATING-SYSTEMS. GitHub. https://github.com/AlperKutay/EE-442-OPERATING-SYSTEMS
// *UseProxy305. (n.d.). EE442-Operating-Systems. GitHub. https://github.com/UseProxy305/EE442-Operating-Systems
// *Karakay, D. (n.d.). EE442. GitHub. https://github.com/dkarakay/EE442
// *Y2P. (n.d.). EE442_hw3. GitHub. https://github.com/Y2P/EE442_hw3

