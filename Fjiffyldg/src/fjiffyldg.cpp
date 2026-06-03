/***
 * Self-contained: U++ source code included internally (BSD License)
 * U++ official website: https://www.ultimatepp.org/
 * 
 * Copyright (c) 2025-2026 Du Jie (@ceepuka). All rights reserved.
**/

// 库接口函数的定义实现
#include "fjiffyldg.h"
#include "uppFilemodel.h"

#ifdef UPP_VERSION
  #ifdef BUILDING_FJIFFYLDG
extern "C" {
FJIFFYLDG_API	fjiffyldg_ptr fjiffyldg_create(void);
FJIFFYLDG_API	void fjiffyldg_clear(fjiffyldg_ptr fm);
}
  #endif

struct fjiffyldg_t : public FilemodelInfo{};

#endif

// 过长行的临界值
static constexpr int CRITICAL_LONGLINE_LEN = (4 * KB);
// 分块单次文本加载默认缓冲区大小
static constexpr int READ_SIZE_FIXED = (128 * KB);

FJIFFYLDG_API fjiffyldg_ptr fjiffyldg_create(void)
{
	return static_cast<fjiffyldg_ptr> (new FilemodelInfo());
}

FJIFFYLDG_API void fjiffyldg_clear(fjiffyldg_ptr fm)
{
	delete static_cast<FilemodelInfo*>(fm);
}

FJIFFYLDG_API int LoadAndScanFile(fjiffyldg_ptr fm, const char *name)
{
	static_cast<FilemodelInfo*>(fm)->uppGlobalLoadFileProcess(name, true);
	return static_cast<FilemodelInfo*>(fm)->GetErrorcode();
}

FJIFFYLDG_API int LoadFileOnly(fjiffyldg_ptr fm, const char *name)
{
	static_cast<FilemodelInfo*>(fm)->uppGlobalLoadFileProcess(name, false);
	return static_cast<FilemodelInfo*>(fm)->GetErrorcode();
}

FJIFFYLDG_API int GetFileIsLoaded(fjiffyldg_ptr fm)
{
	int status = static_cast<FilemodelInfo*>(fm)->GetErrorcode();
	if(!static_cast<FilemodelInfo*>(fm)->IsLoaded() && !status) return -1;
	return status;
}

FJIFFYLDG_API void RestartScanFile(fjiffyldg_ptr fm, const char *name, long long offset, int utf)
{
	static_cast<FilemodelInfo*>(fm)->SetUtfMode(utf);
	static_cast<FilemodelInfo*>(fm)->BackstageFileLinesReScan(name, offset, utf != -1);
}

FJIFFYLDG_API void WaitFileScanTaskFinished(fjiffyldg_ptr fm)
{
	static_cast<FilemodelInfo*>(fm)->LineScanThreadWaitFinished();
}

FJIFFYLDG_API long long GetFileSizeByteCount(const char *name)
{
	return GetFileLength(name);
}

FJIFFYLDG_API long long GetFileLineCount(fjiffyldg_ptr fm)
{
	return static_cast<FilemodelInfo*>(fm)->GetLineCount();
}

FJIFFYLDG_API long long GetFileLinePos(fjiffyldg_ptr fm, long long index)
{
	return static_cast<FilemodelInfo*>(fm)->GetLinePos(index);
}

FJIFFYLDG_API long long GetFileLineLength(fjiffyldg_ptr fm, long long index)
{
	return static_cast<FilemodelInfo*>(fm)->GetLineLength(index);
}

FJIFFYLDG_API long long GetFileLineIndex(fjiffyldg_ptr fm, long long pos)
{
	return static_cast<FilemodelInfo*>(fm)->GetLineByPos(pos);
}

FJIFFYLDG_API const char* ReadFileData(fjiffyldg_ptr fm, long long pos, unsigned int *len)
{
	uint32 length = *len;
	if(!length) length = READ_SIZE_FIXED;
	const char* data = static_cast<FilemodelInfo*>(fm)->ReadData(pos, length);
	*len = length;		// 返回实际读取字节数
	return data;
}

FJIFFYLDG_API const char* ReadFileDataLLineCut(fjiffyldg_ptr fm, long long *index, long long *bpos, long long *epos, unsigned int *len)
{
	auto fmode = static_cast<FilemodelInfo*>(fm);
	int64 begin;
	if((begin = fmode->GetLinePos(*index)) < 0){
		*len = 0;
		return NULL;
	}
	*bpos = begin;
	
	uint32 length = *len;
	if(!length) length = READ_SIZE_FIXED;
	else if(length > UINT_MAX - 1 - CRITICAL_LONGLINE_LEN){
		length = UINT_MAX - 1 - CRITICAL_LONGLINE_LEN;	// 限制取值大小防止溢出
	}
	int64 curpos = begin;
	int64 nextpos = fmode->GetLinePos(*index + 1);
	while(nextpos > 0 && nextpos - begin <= length
	&& nextpos - curpos <= CRITICAL_LONGLINE_LEN ){
		(*index)++;
		curpos = nextpos;
		nextpos = fmode->GetLinePos(*index + 1);
	}
	
	if(nextpos < 0){
		if((fmode->GetFileSize() - curpos) <= CRITICAL_LONGLINE_LEN){
			nextpos = fmode->GetFileSize();	// 文件末尾处理
		}
	}

	if( nextpos < 0 || nextpos - curpos > CRITICAL_LONGLINE_LEN ){
		nextpos = curpos + CRITICAL_LONGLINE_LEN;
	}
	length = nextpos - begin;
	*epos = nextpos;	// 存储目标读取位置
	const char* data = fmode->ReadData(begin, length);
	*len = length;			// 返回实际读取字节数
	return data;
}

FJIFFYLDG_API const char* ReadFileDataEndOfLine(fjiffyldg_ptr fm, long long index, long long pos, unsigned int *len)
{
	auto fmode = static_cast<FilemodelInfo*>(fm);
	int64 begin;
	int64 end = fmode->GetFileSize();
	if((index < 0) || (pos > end) || (begin = fmode->GetLinePos(index)) < 0 || (pos < begin)){
		*len = 0;
		return NULL;
	}
	
	// index 未被确认最后一行则需要更改 end
	if(index+1 != GetFileLineCount(fm)){
		end = fmode->GetLinePos(index+1);
		if(pos > end){
			*len = 0;
			return NULL;
		}
	}
	uint32 length = *len;
	if(!length) length = CRITICAL_LONGLINE_LEN;

	if(length > end - pos) length = end - pos;
	const char* data = fmode->ReadData(pos, length);
	*len = length;		// 返回实际读取字节数
	return data;
}

FJIFFYLDG_API const char* GetFileMappedHuge(fjiffyldg_ptr fm, const char *fileName, long long *bufferSize)
{
	return static_cast<FilemodelInfo*>(fm)->GetHugerBuffer(fileName, *bufferSize);
}

FJIFFYLDG_API void ClearHugeBuffer(fjiffyldg_ptr fm)
{
	static_cast<FilemodelInfo*>(fm)->ClearHuger();
}

FJIFFYLDG_API unsigned int CheckTextASCII(const char * __restrict text, unsigned int len)
{
	const char *const end = text + len;
	while(text < end){
		if((*text & 0x80) != 0) break;
		text++;
	}
	return end - text;
}

template <class Target>
force_inline void Utf8SliceCharHandle(Target t, uint8 &p, uint8 w)
{
	while( (p < w) && ((0xC0 & t(p)) == 0x80) ){
		p++;
	}
}
// utf-8 首字节严格匹配，返回字符宽度
force_inline uint8 CheckUtf8SliceCharWidth(char c)
{
	uint8 w;				// utf-8 字符宽度
	if(!(0x80 & c)) w = 1;
	else if((0xE0 & c) == 0xC0) w = 2;
	else if((0xF0 & c) == 0xE0) w = 3;
	else if((0xF8 & c) == 0xF0) w = 4;
	else
		w = 0;			// 0 表示非 utf-8
	return w;
}
static bool CheckIsUtf8(const char *s, const char *const lim, uint8 &w)
{
	w = CheckUtf8SliceCharWidth(*s);
	if(!w || w > lim - s) return false;
	uint8 slice = 1;
	Utf8SliceCharHandle([s](uint8 p) {return s[p]; }, slice, w);
	/*while( (slice < w) && ((0xC0 & s[slice]) == 0x80) ){
		slice++;
	}*/
	return slice == w;
}

FJIFFYLDG_API unsigned int CheckWholeTextUtf8(const char *text, unsigned int len)
{
	const char *lim = text + len;
	for(uint8 w = 0; (text < lim) && CheckIsUtf8(text, lim, w);){
		text += w;
	}
	return lim - text;
}

FJIFFYLDG_API unsigned int GetUtf8TextCharCount(const char * *text, unsigned int len)
{
	unsigned int count = 0;
	const char *lim = *text + len;
	for(uint8 w = 0; (*text < lim) && CheckIsUtf8(*text, lim, w);){
		count++;
		*text += w;
	}
	return count;
}

FJIFFYLDG_API unsigned int CheckExtractTextUtf8(const char *text, unsigned int len)
{
	// 最短有效截断长度为10字节
	if(len < 10) return CheckWholeTextUtf8(text, len);
	// 文本首尾截断字符的处理
	uint8 slice = 0;
	Utf8SliceCharHandle([text](uint8 p) {return text[p]; }, slice, 3);
	
	text += slice;
	if( ! CheckUtf8SliceCharWidth(*text)) return len - slice;
	len -= slice;
	slice = 1;
	Utf8SliceCharHandle([text, len](uint8 p) {return *(text+len-p); }, slice, 4);
	// slice 刚好表示最后一个字符剩余宽度
	if(CheckUtf8SliceCharWidth(*(text+len-slice)) < slice) return CheckWholeTextUtf8(text, len);
	unsigned int r = CheckWholeTextUtf8(text, len - slice);
	return (r ? r + slice : 0);
}

#if defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__)
	static constexpr int mapChunk = GB;
	static constexpr int bufferSize = 4 * MB;
#else
	static constexpr int mapChunk = 128 * MB;
	static constexpr int bufferSize = MB;
#endif

static bool FileDataIsEqual(const char *first, const char *second, int64 pos=0)
{
	FileIn file1(first);
	FileIn file2(second);
	if(!file1.IsOpen() || !file2.IsOpen()) return false;
	const int64 fileSize = file1.GetSize();
	const int64 last = fileSize - 16*KB;
	const int64 step = fileSize / 8;
	file1.SetBufferSize(16*KB);
	file2.SetBufferSize(16*KB);
	Buffer<byte> data1(16*KB);
	Buffer<byte> data2(16*KB);
	while(pos<last){
		file1.Seek(pos);
		file2.Seek(pos);
		if(file1.Get(data1, 16*KB) != 16*KB || file2.Get(data2, 16*KB) != 16*KB) return false;
		if(memcmp(data1, data2, 16*KB)) return false;
		pos += step;
	}
	file1.Seek(last);
	file2.Seek(last);
	if(file1.Get(data1, 16*KB) != 16*KB || file2.Get(data2, 16*KB) != 16*KB) return false;
	return !memcmp(data1, data2, 16*KB);
}

static bool FileDataIsEqualFastCompare(const char *first, const char *second)
{
	FileMapping map1, map2;
	if(map1.Open(first) && map2.Open(second)){
		const int64 fileSize = map1.GetFileSize();
		const int64 last = fileSize - 16*KB;
		const int64 step = fileSize / 8;
		const byte* data1;
		const byte* data2;
		int64 offset = 0;
		for(int i=0; (i<8) && (data1 = map1.Map(offset, 16*KB))&&(data2 = map2.Map(offset, 16*KB)); i++){
			if(memcmp(data1, data2, 16*KB)) return false;
			offset += step;
		}
		if(offset == step * 8){
			if((data1 = map1.Map(last, 16*KB)) && (data2 = map2.Map(last, 16*KB))) return !memcmp(data1, data2, 16*KB);
			offset = last;
		}
		return FileDataIsEqual(first, second, offset);
	}
	return FileDataIsEqual(first, second);
}

FJIFFYLDG_API int ToCloneFile(const char *oldFileName, const char *newFileName)
{
	const int64 fileSize = GetFileLength(oldFileName);
	if(fileSize < 0 || String(oldFileName).IsEqual(newFileName)) return -1;
	
	if(fileSize > MB && fileSize == GetFileLength(newFileName)
	&& FileGetTime(oldFileName).Compare(FileGetTime(newFileName)) == 0
	&& FileDataIsEqualFastCompare(oldFileName, newFileName)) return 0;
	
	if(!FileCopy(oldFileName, newFileName)) return -1;
	return 0;
}

static constexpr int alignSize = (UINT_MAX<<12) & INT_MAX;

FJIFFYLDG_API int ToSaveFile(const char *fileName, const char *buffer, long long len)
{
	if(len < 0) return -1;
	
	FileOut out(fileName);
	if(!out.IsOpen() || out.IsError()) return -1;
	out.SetSize(len);
	out.SetBufferSize(bufferSize);
	
#if defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__)
	out.Put64(buffer, len);
#else
	int64 rem = len;
	while(rem > alignSize){
		out.Put(buffer, alignSize);
		if(out.IsError()) return -1;
		buffer += alignSize;
		rem -= alignSize;
	}
	out.Put(buffer, rem);
#endif
	out.Close();
	if(!out.IsOK()) return -1;
	
	ASSERT(len == GetFileLength(fileName));	// 未正确保存
	return 0;
}

force_inline
static void AppendFileData(FileAppend &append, const char *buffer, long long len)
{
	while(len > alignSize){
		append.Put(buffer, alignSize);
		if(append.IsError()) return;
		buffer += alignSize;
		len -= alignSize;
	}
	append.Put(buffer, len);
}

FJIFFYLDG_API int ToAppendFile(const char *fileName, const char *buffer, long long len)
{
	if(len < 0) return -1;
	if(!FileExists(fileName)) FileStream(fileName, FileStream::CREATE);
	const int64 fileSize = GetFileLength(fileName);
	if(fileSize < 0 || fileSize + len < 0) return -1;
	
	FileAppend append;
	if(!append.Open(fileName)) return -1;
	append.SetSize(fileSize + len);
	append.SetBufferSize(bufferSize);
	AppendFileData(append, buffer, len);
	
	append.Close();
	if(!append.IsOK()) return -1;
	ASSERT(fileSize + len == GetFileLength(fileName));	// 未正确保存
	return 0;
}

force_inline
static bool CatByFileMappingAppend(FileAppend &append, FileMapping& map, int64 offset, int maplen)
{
	byte* data = map.Map(offset, maplen);
	if(!data) return false;
	append.Put(data, map.GetCount());
	return append.IsOK();
}

static constexpr int block = bufferSize * 32;
static bool CatByFileStreamAppend(FileAppend &append, const char *fileName, int64 len, int writeShared)
{
	FileStream file;
	if(!file.Open(fileName, FileStream::READ|writeShared)
	|| file.GetLeft()!=len) return false;
	
	file.SetBufferSize(bufferSize);
	Buffer<byte> read((int)min<int64>(block, len));
	for(int size = file.Get(read, (int)min<int64>(block, len)); size;){
		append.Put(read, size);
		if(append.IsError()) return false;
		len -= size;
		size = file.Get(read, (int)min<int64>(block, len));
	}
	
	file.Close();
	append.Close();
	return (append.IsOK() && file.IsOK());
}

FJIFFYLDG_API int ToConcatenateFile(const char *catFileName, const char *secondFileName)
{
	const int64 fileSize = GetFileLength(secondFileName);
	if(fileSize < 0) return -1;
	
	int64 destSize = (FileExists(catFileName) ? GetFileLength(catFileName) : 0);
	if((destSize += fileSize) < 0) return -1;
	
	FileAppend append;
	if(!append.Open(catFileName)) return -1;
	// Is itself ?
	int writeShared = 0;
	if(!String(catFileName).IsEqual(secondFileName)){
		append.SetSize(destSize);
		writeShared = FileStream::NOWRITESHARE;
	}
	append.SetBufferSize(bufferSize);
	
	FileMapping map;
	if(!map.Open(secondFileName, FileStream::READ | writeShared)
	|| !CatByFileMappingAppend(append, map, 0, mapChunk)){
		// 首次映射失败，改用流式操作
		if(append.IsError() || !CatByFileStreamAppend(append, secondFileName, fileSize, writeShared)) return -1;
	}
	else{
		int64 offset = mapChunk;
		while( offset <= fileSize - mapChunk){
			if(!CatByFileMappingAppend(append, map, offset, mapChunk)) return -1;
			offset += mapChunk;
		}
		
		if(offset < fileSize){
			if(!CatByFileMappingAppend(append, map, offset, mapChunk)) return -1;
		}
	}
	append.Close();
	if(!append.IsOK()) return -1;
	ASSERT(destSize == GetFileLength(catFileName));	// 未正确连接
	return 0;
}


namespace Fjiffyldg {
// C++ API


struct Fjiffyldg::Impl{
	Impl(): init(new FilemodelInfo()) {}
	fjiffyldg_ptr GetHandle();
private:
	One<FilemodelInfo> init;
};
fjiffyldg_ptr Fjiffyldg::Impl::GetHandle() { return static_cast<fjiffyldg_ptr>(init.Get());}

Fjiffyldg::Fjiffyldg(): pimpl(std::make_unique<Impl>() ) {}
fjiffyldg_ptr Fjiffyldg::GetFjiffyldgHandle() { return pimpl->GetHandle();}
Fjiffyldg::~Fjiffyldg() = default;


// C++ API end
}
