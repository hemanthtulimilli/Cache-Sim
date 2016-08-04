#include <stdio.h>
#include <stdlib.h>

void main (void)
{
	char temp[16];
	char arg[8];
	FILE *fp;
	
	fp = fopen("trace.txt", "r");
	while (fscanf(fp, "%s", arg) != EOF)
	{
//		fscanf(fp, "%s", arg);
		fscanf(fp, "%s", temp);
		printf("%s\t%d\n", arg, (int) strtol (temp, NULL, 0));
	}
}
