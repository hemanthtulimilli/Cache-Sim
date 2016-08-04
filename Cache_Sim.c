/*
File Name:			Cache_Sim.c
Author:				Venkata Hemanth Tulimilli
Class:				ECE 586 Computer Architecture

Description:		Develop L1 Data Cache simulator for 32-bit computer with Byte addressable.
					Given cache properties are:
					4-way set associative cache; 32 Byte lines;
					1024 sets; uses LRU for replacement policy;
					uses write-allocate or write-back policy for interaction with main memory.
					This implementation also has 3 debug command modes:
					-v	--> displays version of the code and exits execution.
					-t	--> this will echo the input trace from the time -t is encountered until the end.
					-d	--> dumps all relevant info after simulation run from the time -d is encountered until the end.
					
Functional flow:	Program flow starts with the main(). For each and every input memory access cache is checked
					using the function check_cache() which returns line number and whether it is a hit or a miss.
					And then depending on whether it is a cache hit or a cache miss, we perform required operations
					like writing back to memory if there is a cache miss and dirty bit is set and etc. the final step
					for a single memory access is to set LRU after each and every memory access to the cache.
					
Version revisions:
Version	Date		Description
v0.1	07/22/2016	Implement reading from the input trace and directly accessing memory from main memory.
v1.0	07/24/2016	Implement Direct mapped cache for all 1024 sets.
v1.1	07/28/2016	Extend the implementation from Direct Mapped cache to 4-way set associative cache with LRU.
v1.2	07/30/2016	Implement write-back policy for the 4-way set associative cache using dirty bit.
v1.3	08/01/2016	Implement Debug commands and verifying whether it works or not.

Inputs Uses for the project:
1.	trace1.txt
2.	trace2.txt
3.	trace3.txt
4.	Ultimate_Trace.txt

*/

// Input trace file name
#define INPUT_TRACE			"Ultimate_Trace.txt"

// File Debug definitions
#define TRACE_OUT			TRUE

// Version definition
#define VERSION				1.3

// Cache Definitions
#define NO_OF_SETS			1024
#define NO_OF_LINES			4
#define SIZE_OF_LINE		32

// Regular Definitions for readability
#define uint_64				unsigned long int
#define uint_32				unsigned int
#define uint_8				unsigned char
#define TRUE				1
#define FALSE				0

// Main Memory Constants
#define OFFSET_MSK			0x0000001F
#define INDEX_MSK			0x00007FE0
#define TAG_MSK				0xFFFF8000
#define OFFSET_VAL			0
#define INDEX_VAL			5
#define TAG_VAL				15

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Function Declarations
uint_8 check_cache(uint_32 tag, uint_32 index, bool *hit_flag, bool *empty_flag);
void set_lru (uint_8 line_num, uint_32 index, bool *hit_flag, bool *empty_flag);

// Structure Definition
struct cache_addr
{
	uint_32		tag: 17;
	uint_8		lru: 2;
	uint_8		valid: 1;
	uint_8		dirty: 1;
	uint_8		data[SIZE_OF_LINE];
};

// Global variable declarations
struct cache_addr l1cache[NO_OF_SETS][NO_OF_LINES];

/* 
------------------------------------------------------------------------------------------------------------------
Function name:	main()
------------------------------------------------------------------------------------------------------------------
Arguments passed:	void
Return argunemts:	void

Description:	Controls the flow from extracting arguments from input trace file to implementing simulator logic
				and once the execution is completed, it gives out the statistics of how many clock cycles did it
				take to complete the whole execution, hit count, miss count, read count, write count, etc.
*/
void main (void)
{
	// Local variable declarations
	char arg[16], addr[16];
	bool hit_flag = FALSE;
	bool empty_flag = FALSE;
	bool version_flag = FALSE, debug_flag = FALSE, trace_flag = FALSE;
	uint_8 line_num = 0;

	// Temporary variable declarations
	uint_32 mem_addr;
	uint_32 temp_tag, temp_index, temp_offset;
	
	// File pointers
	FILE *fp;
	FILE *fp_out;
	
	// Counts for different categories to calculate efficiency
	uint_64 clock_cycles = 0;
	uint_64 read_hits = 0, write_hits = 0, read_misses = 0, write_misses = 0, reads = 0, writes = 0;
	uint_64 accesses = 0, hits = 0, misses = 0, write_backs = 0;
	
	// Opening input trace file in read mode and output trace file in writing mode
	fp = fopen(INPUT_TRACE, "r");
	fp_out = fopen("Output.txt", "w");
	
	// Initialize Cache to 0s
	for (uint_32 temp_li1 = 0; temp_li1 < NO_OF_SETS; temp_li1 ++)
	{
		for (uint_32 temp_li2 = 0; temp_li2 < NO_OF_LINES; temp_li2 ++)
		{
			l1cache[temp_li1][temp_li2].tag = 0;
			l1cache[temp_li1][temp_li2].lru = 0;
			l1cache[temp_li1][temp_li2].valid = FALSE;
			for(uint_8 temp_li3 = 0; temp_li3 < SIZE_OF_LINE; temp_li3 ++)
				l1cache[temp_li1][temp_li2].data[temp_li3] = 0;
		}
	}
		
	// Main Logic to read the trace and perform required modifications
	while (fscanf(fp, "%s", arg) != EOF)					// reads arguments like 'r', 'w', "-v", "-t" and "-d"
	{
		// Handling "-v", "-t" and "-d" arguments and making sure arg contains either 'r' or 'w' only.
		while (arg[0] == '-') 
		{
			switch (arg[1])
			{
				case 'v':
				case 'V':	version_flag = TRUE;
							break;
				case 't':
				case 'T':	trace_flag = TRUE;
							break;
				case 'd':
				case 'D':	debug_flag = TRUE;
							break;
			}
			
			// If input trace file ends with "-v", "-t" and "-d", it will end the execution
			if (fscanf(fp, "%s", arg) == EOF)
			{
				break;
			}
		}
		
		fscanf(fp, "%s", addr);						// getting address of 'r' or 'w' argument
		// If version_flag is set, print version and exit execution.
		if (version_flag)
		{
			printf("Version %.1f\n", VERSION);
			break;
		}
		// Echo input command if trace flag is set.
		if (trace_flag)
		{
			printf("%s %s\n", arg, addr);
			if (TRACE_OUT)
				fprintf(fp_out, "%s %s\n", arg, addr);
		}
		accesses ++;
		
		// Seperating tag, index and offset fields from the address field.
		mem_addr 	= (uint_32) strtol (addr, NULL, 0);
		temp_tag 	= (mem_addr & TAG_MSK) >> TAG_VAL;
		temp_index 	= (mem_addr & INDEX_MSK) >> INDEX_VAL;
		temp_offset = mem_addr & OFFSET_MSK;
		line_num 	= check_cache(temp_tag, temp_index, &hit_flag, &empty_flag);
		clock_cycles++;					// increment a clock cycle for every cache visit.
		if (hit_flag)	// cache hit
		{
			hits ++;
			if ((arg[0] == 'r') || (arg[0] == 'R'))
			{
				if(debug_flag)				// if debug_flag is set, dump relevent information
				{
					printf("cache read hit to line %d\n", line_num);
					if (TRACE_OUT)			// to write the same dumped information in an output trace file
						fprintf(fp_out, "cache read hit to line %d\n", line_num);
				}
				read_hits ++;
				reads ++;
			}
			else if ((arg[0] == 'w') || (arg[0] == 'W'))
			{
				if(debug_flag)				// if debug_flag is set, dump relevent information
				{
					printf("cache write hit to line %d\n", line_num);
					if (TRACE_OUT)
						fprintf(fp_out, "cache write hit to line %d\n", line_num);
				}
				write_hits ++;
				writes ++;
				l1cache[temp_index][line_num].dirty = TRUE;				// if cache hit and is a write, then set dirty bit
			}
		}
		else		// cache miss
		{
			misses ++;
			if ((arg[0] == 'r') || (arg[0] == 'R'))
			{
				if(debug_flag)				// if debug_flag is set, dump relevent information
				{
					printf("cache read miss to line %d\n", line_num);
					if (TRACE_OUT)
						fprintf(fp_out, "cache read miss to line %d\n", line_num);
				}
				read_misses ++;
				reads ++;
			}
			else if ((arg[0] == 'w') || (arg[0] == 'W'))
			{
				if(debug_flag)				// if debug_flag is set, dump relevent information
				{
					printf("cache write miss to line %d\n", line_num);
					if (TRACE_OUT)
						fprintf(fp_out, "cache write miss to line %d\n", line_num);
				}
				write_misses++;
				writes ++;
			}
			
			// If dirty bit is set, write back cache line to main memory and reset dirty bit to false
			if (l1cache[temp_index][line_num].dirty == TRUE)
			{
				write_backs ++;
				if(debug_flag)				// if debug_flag is set, dump relevent information
				{
					printf("Dirty bit is set. So writing back line %d of set: %d to main memory\n", line_num, temp_index);
					if (TRACE_OUT)
						fprintf(fp_out, "Dirty bit is set. So writing back line %d of set: %d to main memory\n", line_num, temp_index);
				}
				clock_cycles += 50;			// memory access time to replace the cache line to memory
				l1cache[temp_index][line_num].dirty = FALSE;			// reset dirty bit
			}
			
			clock_cycles += 50;				// cache miss penalty(memory access to get cache line)
			l1cache[temp_index][line_num].tag = temp_tag;			// write tag field when new cache line is replaced
		}
		
		// now set the LRU field for the recently accessed line of a set
		set_lru(line_num, temp_index, &hit_flag, &empty_flag);
		
		// print the accessed set information 
		if(debug_flag)				// if debug_flag is set, dump relevent information
		{
			printf("\t\t\tSet: %d\n", temp_index);
			if (TRACE_OUT)
				fprintf(fp_out, "\t\t\tSet: %d\n", temp_index);
			for (uint_8 temp = 0; temp < NO_OF_LINES; temp ++)
			{
				printf("Line: %d\tTag: 0x%05X\tLRU: %d\tValid: %d\tDirty: %d\n", temp, l1cache[temp_index][temp].tag, l1cache[temp_index][temp].lru, l1cache[temp_index][temp].valid, l1cache[temp_index][temp].dirty);
				if (TRACE_OUT)
					fprintf(fp_out, "Line: %d\tTag: 0x%5X\tLRU: %d\tValid: %d\tDirty: %d\n", temp, l1cache[temp_index][temp].tag, l1cache[temp_index][temp].lru, l1cache[temp_index][temp].valid, l1cache[temp_index][temp].dirty);
			}
		}
	}
	
	// print statistics.
	printf("Total number of clockcycles = %ld\naccesses = %ld; hits = %ld; misses = %ld; reads = %ld; writes = %ld;\nread hits = %ld; write hits = %ld; read misses = %ld; write misses = %ld; write backs = %ld\n", clock_cycles, accesses, hits, misses, reads, writes, read_hits, write_hits, read_misses, write_misses, write_backs);
	printf("Miss ratio = %.4f, Average cycles per instruction = %.4f\n", ((float) misses / accesses), ((float) clock_cycles / accesses)); 
	if (TRACE_OUT)
	{
		fprintf(fp_out, "Total number of clockcycles = %ld\naccesses = %ld; hits = %ld; misses = %ld; reads = %ld; writes = %ld;\nread hits = %ld; write hits = %ld; read misses = %ld; write misses = %ld; write backs = %ld\n", clock_cycles, accesses, hits, misses, reads, writes, read_hits, write_hits, read_misses, write_misses, write_backs);
		fprintf(fp_out, "Miss ratio = %.4f, Average cycles per instruction = %.4f\n", ((float) misses / accesses), ((float) clock_cycles / accesses)); 
	}
	
	// closing all the opened files
	fclose(fp);
	fclose(fp_out);
}

/* 
------------------------------------------------------------------------------------------------------------------
Function name:	check_cache()
------------------------------------------------------------------------------------------------------------------
Arguments passed:	tag, index, *hit_flag, *empty_flag
Return argunemts:	line_num

Description:	Checks if there is required memory address in any cache lines for given index, it sets hit_flag
				to TRUE and returns	line number back to main(). If there are empty cache lines, it sets empty_flag
				to TRUE and returns line number back to main(). But if there is a cache miss, it leaves hit_flag to 
				FALSE and returns the least recently used line number.
*/
uint_8 check_cache(uint_32 tag, uint_32 index, bool *hit_flag, bool *empty_flag)
{
	uint_8 temp_li;
	uint_8 line_num = 0;
	*hit_flag = FALSE;
	*empty_flag = FALSE;
	for (temp_li = 0; temp_li < NO_OF_LINES; temp_li ++)
	{
		if (l1cache[index][temp_li].valid == TRUE)
		{
			if (l1cache[index][temp_li].tag == tag)
			{
				*hit_flag = TRUE;
				line_num = temp_li;
				break;
			}
			else
			{
				if (l1cache[index][temp_li].lru == 3)
				{
					line_num = temp_li;
				}
			}
		}
		else
		{
			*empty_flag = TRUE;
			line_num = temp_li;
			l1cache[index][line_num].valid = TRUE;
			break;
		}
	}
	return line_num;
}
			
/* 
------------------------------------------------------------------------------------------------------------------
Function name:	set_lru()
------------------------------------------------------------------------------------------------------------------
Arguments passed:	line_num, index, *hit_flag, *empty_flag
Return argunemts:	void

Description:	If there is a cache hit, it modifies LRU fields of the lines in a set based on recent usage.
				If there is a cache miss and if empty_flag is set, LRUs of filled cache lines are set and 
				if empty_flag is not set, it will increament all the LRU bit fields in a set by 1 and resets 
				LRU bit field of the cache line num to 0.
*/
void set_lru (uint_8 line_num, uint_32 index, bool *hit_flag, bool *empty_flag)
{
	uint_8 temp_li;
	if(*hit_flag)
	{
		for(temp_li = 0; temp_li < NO_OF_LINES; temp_li ++)
		{
			if(l1cache[index][temp_li].lru < l1cache[index][line_num].lru)
			{
				l1cache[index][temp_li].lru ++;
			}
		}
		l1cache[index][line_num].lru = 0;
	}
	else
	{
		if(*empty_flag)
		{
			for(temp_li = 0; temp_li < line_num; temp_li ++)
			{
				l1cache[index][temp_li].lru ++;
			}
			l1cache[index][line_num].lru = 0;
		}
		else
		{
			for(temp_li = 0; temp_li < NO_OF_LINES; temp_li ++)
			{
				l1cache[index][temp_li].lru ++;
			}
		}
	}
}

