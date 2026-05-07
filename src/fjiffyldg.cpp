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
constexpr int CRITICAL_LONGLINE_LEN = (4 * KB);
// 分块单次文本加载默认缓冲区大小
constexpr int BUFFER_SIZE = (128 * KB);

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
	if(!length) length = BUFFER_SIZE;
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
	if(!length) length = BUFFER_SIZE;
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

force_inline bool IsASCIIChar(const char *&s, const char *const lim)
{
	while(s < lim){
		if((*s & 0x80) != 0) return false;
		s++;
	}
	return true;
}

FJIFFYLDG_API unsigned int CheckTextASCII(const char *text, unsigned int len)
{
	const char *const end = text + len;
	if(len < 8){
		return IsASCIIChar(text, end)? 0 : end - text;
	}
	// 内存对齐
	if(uint8 offset = (size_t)text % 8){
		offset = 8 - offset;
		if(! IsASCIIChar(text, text + offset)) return end - text;
	}
	while (end >= text + 8) {
        if ((*(qword*)text & 0x8080808080808080) != 0){
            while(!(*text & 0x80)) text++;
            
            return end - text;
        }
        text += 8;
    }
	return IsASCIIChar(text, end)? 0 : end - text;
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
	if(!w || s + w > lim) return false;
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
	if(!len) return 0;
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

force_inline
void FastCopyDataQword(qword* __restrict d, const qword* __restrict s, uint64 count)
{
	for(uint64 i=0; i<count; i++){
		*d = *s;
		s++;
		d++;
	}
}
force_inline
void CopyDataUnaligned(byte* d, const byte* s, int8 len)
{
	int8 t = 0;
	if(len & 4){
		*(dword*)d = *(dword*)s;
		t = 4;
	}
	
	if(len & 2){
		((word*)d)[t/2] = ((word*)s)[t/2];
		t += 2;
	}
	
	if(len & 1){
		d[t] = s[t];
	}
}
void UnlimitedFastCopyData(byte* __restrict d, const byte* __restrict s, uint64 len)
{
	FastCopyDataQword((qword*)d, (qword*)s, len / 8);
	int8 rlen = len % 8;
	if(!rlen) return;
	/* if(rlen) for(int i=0; i<rlen; i++){
		d[i] = s[i];
	} */
	CopyDataUnaligned((d + len - rlen), (s + len - rlen), rlen);
}

force_inline
static bool DeepCopyFileMappingData(FileMapping& oldMap, FileMapping& newMap, int64 offset, int64 len)
{
	const byte* source;
	byte* dest;
	if(!(source = oldMap.Map(offset, len))
		|| !(dest = newMap.Map(offset, len))) return false;
	
	UnlimitedFastCopyData(dest, source, len);
	return true;
}
static bool FileCopyByFileMapping(const char *oldFileName, const char *newFileName)
{
	FileMapping oldMap(oldFileName);
	FileMapping newMap;
	int64 fileSize = oldMap.GetFileSize();
	newMap.Create(newFileName, fileSize);
	if(!oldMap.IsOpen() || !newMap.IsOpen()) return false;
	uint64 offset = 0;
	if(fileSize >= GB) while( offset + GB <= (uint64)fileSize ){
		if(!DeepCopyFileMappingData(oldMap, newMap, offset, GB)) return false;
		offset += GB;
	}
	
	if(offset < fileSize){
		return DeepCopyFileMappingData(oldMap, newMap, offset, fileSize - offset);
	}
	return true;
}

FJIFFYLDG_API int ToCloneFile(const char *oldFileName, const char *newFileName)
{
	const int64 fileSize = GetFileLength(oldFileName);
	if(fileSize < 0) return -1;
	if(fileSize > FilemodelInfo::USUALLY_IO_SIZE_MAX){
		if(!FileCopyByFileMapping(oldFileName, newFileName)) return -1;
	}
	else{
		if(!FileCopy(oldFileName, newFileName)) return -1;
	}
	if(fileSize != GetFileLength(newFileName)) return 1;	// 未正确复制
	return 0;
}

force_inline
static bool CopyDataByFileMapping(const byte *buffer, FileMapping& map, int64 offset, size_t maplen)
{
	byte* dest;
	if(!(dest = map.Map(offset, maplen))) return false;
	UnlimitedFastCopyData(dest, buffer, maplen);
	return true;
}
static bool FileSaveByFileMapping(const char *fileName, const byte *buffer, int64 len)
{
	FileMapping map;
	if(!map.Create(fileName, len)) return false;
	uint64 offset = 0;
	if(len >= GB) while( offset + GB <= (uint64)len ){
		if(!CopyDataByFileMapping(buffer, map, offset, GB)) return false;
		offset += GB;
	}
	
	if(offset < len){
		return CopyDataByFileMapping(buffer, map, offset, len - offset);
	}
	return true;
}

FJIFFYLDG_API int ToSaveFile(const char *fileName, const char *buffer, long long len)
{
	if(len < 0) return -1;
	if(len > FilemodelInfo::USUALLY_IO_SIZE_MAX){
		if(!FileSaveByFileMapping(fileName, (byte*)buffer, len)) return -1;
	}
	else{
		if(!SaveFile(fileName, String(buffer, len))) return -1;
	}
	if(len != GetFileLength(fileName)) return 1;	// 未正确保存
	return 0;
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
	append.SetBufferSize(8 * MB);
	
	const int optimalChunk = (len > GB)? ( (len > 100 * (int64)GB)? 64 * MB : 16 * MB ) : (4 * MB);
	int64 remain = len;
	if(len > optimalChunk) while(remain > optimalChunk){
		append.Put(buffer, optimalChunk);
		remain -= optimalChunk;
		buffer += optimalChunk;
	}
	append.Put(buffer, remain);
	if(fileSize + len != GetFileLength(fileName)) return 1;	// 未正确保存
	return 0;
}

force_inline
static int CatByFileMappingAppend(const char *catFileName, FileMapping& map, int64 offset, size_t maplen)
{
	byte* append;
	if(!(append = map.Map(offset, maplen))) return -1;
	return ToAppendFile(catFileName, (char*)append, maplen);
}

FJIFFYLDG_API int ToConcatenateFile(const char *catFileName, const char *appendFileName)
{
	if(!FileExists(appendFileName)) return -1;
	const int64 fileSize = GetFileLength(appendFileName);
	int64 destSize = GetFileLength(catFileName);
	if(destSize < 0) destSize = 0;
	if( (fileSize < 0) || (destSize += fileSize) < 0) return -1;
	FileMapping map;
	if(!map.Open(appendFileName)) return -1;
	
#if defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__)
	const int64 mapChunk = 4 * (int64)GB;
#else
	const int mapChunk = GB;
#endif
	
	uint64 offset = 0;
	if(fileSize > mapChunk) while( offset + mapChunk <= (uint64)fileSize ){
		if(CatByFileMappingAppend(catFileName, map, offset, (size_t)mapChunk) != 0) return -1;
		offset += mapChunk;
	}
	
	if(offset < fileSize){
		return CatByFileMappingAppend(catFileName, map, offset, (size_t)(fileSize - offset));
	}
	if(destSize != GetFileLength(catFileName)) return 1;	// 未正确连接
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
