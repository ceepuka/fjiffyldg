// A test utility for validating library configuration correctness.
#include "fjiffyldg.h"
#include <stdio.h>

int main()
{
	printf("file size: %lld\n\n", GetFileSizeByteCount("test.txt"));
	
	fjiffyldg_ptr fm = fjiffyldg_create();
	if(! LoadAndScanFile(fm, "test.txt")){
		// File loaded successfully
		printf("file lines: %lld\n", GetFileLineCount(fm));
		printf("Positions of lines 1 (index 0) and 2 (index 1): %lld (offset) and %lld (offset)\n",
			GetFileLinePos(fm, 0), GetFileLinePos(fm, 1));
		long long len = GetFileLineLength(fm, 1);
		printf("the length of line 2: %lld (bytes)\n", len);
		const char* hello = ReadFileData(fm, GetFileLinePos(fm, 1), (unsigned int*) &len);
		if(hello) printf("%.5s\n", hello);	// Hello
		long long size;
		const char* all = GetFileMappedHuge(fm, "test.txt", &size);
		printf("total characters in the file: %d\n", GetUtf8TextCharCount(&all, size));
		if (! ToCloneFile("test.txt", "UTF-8 text.txt")){
			printf("Copied successfully!\n");
			ToConcatenateFile("UTF-8 text.txt", "test.txt");
			printf("file size: %lld\n\n", GetFileSizeByteCount("UTF-8 text.txt"));
		}
	}

	if (!LoadAndScanFile(fm, "UTF-8 text.txt")){
		int len = 0;
		const char* defaultData = ReadFileData(fm, 0, (unsigned int*) &len);
		if (defaultData) printf("%s\n", defaultData);
	}
	
	fjiffyldg_clear(fm);
	getchar();
	return 0;
}
