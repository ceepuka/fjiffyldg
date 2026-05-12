#include "uppFilemodel.h"

// 通常文件单次最多加载 1MB
constexpr int FILEBLOCK = MB;
// 大文件单次映射最多 1GB
constexpr int MMAP_FILECHUNK = GB;
// 文件行结构扫描 10MB 作为一个块
constexpr int SCAN_FILE_CHUNK = (10 * MB);

FilemodelInfo::FilemodelInfo()
{
	StdLogSetup(LOG_FILE | LOG_APPEND | LOG_ELAPSED);
}

FilemodelInfo::~FilemodelInfo()
{
	BackstageRequestStop(true);
	lnscan.ShutdownThreads();
}

// 配置线程安全的共享映射区
static bool SetSharedFileMap(RWMutex &mt, FileMapping &map, int64 offset, size_t mapl)
{
	RWMutex::WriteLock lock(mt);
	return !!map.Map(offset, mapl);
}

template <typename T>
void FilemodelBase::UpdateLineStats(FileMapping &map, const T* q, byte &last, byte c)
{
	const byte * const b = map.Begin();
	const byte * const e = map.End();
	const int64 i = map.GetOffset();
	if(last == '\r'){
		last = 0;
		if(! IsNewlineLF(*q, c)) linestats.AddLine(i);
	}
	
	while((byte*)q < e - sizeof(T)){
		if(IsReadNewlineChar(q, c)){
			linestats.AddLine(i+((byte*)q-b)+sizeof(T));
		}
		q++;
	}
	
	if((byte*)q == e - sizeof(T)){
		if(IsNewlineLF(*q, c)) linestats.AddLine(i + map.GetCount());
		else if(IsNewlineCR(*q, c)) last = '\r';
	}
}

void FilemodelBase::ScanLineStats(FileMapping &map, int64 filesize)
{
	byte last = 0;	// 表示已扫描过的块最后一个字节
	int64 checkl = 0;
	do{
		byte* buffer = map.Begin();
		switch(utfmode){
			case 1: UpdateLineStats(map, (word* )buffer, last); break;		// utf16le
			case 2: UpdateLineStats(map, (word* )buffer, last, 1); break;	// utf16be
			case 3: UpdateLineStats(map, (dword* )buffer, last); break;		// utf32le
			case 4: UpdateLineStats(map, (dword* )buffer, last, 3); break;	// utf32be
			default:
				UpdateLineStats(map, buffer, last);
		}
		checkl = map.GetOffset()+map.GetCount();
		if( ! linescanRunning.load(std::memory_order_relaxed)) return;
	}while( (checkl < filesize) &&
		SetSharedFileMap(linestats.maplock, map, checkl, map.GetCount()) );
	if(last == '\r') linestats.AddLine(checkl);
}

void FilemodelBase::BackstageFileLinesInitTask(const char *path, int64 offset, bool utfverifiable)
{
	FileMapping &map = linestats.GetFileMapOpen(path);
	if(!map.IsOpen()||!map.Map(offset, SCAN_FILE_CHUNK)){
		LOG("Warning: File'"<< path <<"'cannot be scanned line by line.");
		return;
	}
	const int64 filesize = map.GetFileSize();
	// 默认以 BOM 标签确定 utfmode
	if(!utfverifiable && filesize >= 4){
		const byte *p = map.Begin();
		// utf-32le
		if(*(dword*)p == 0xFEFF) utfmode = 3;
		// utf-32be
		else if(*(dword*)p == 0xFFFE0000) utfmode = 4;
		// utf-16le
		else if(*(word*)p == 0xFEFF) utfmode = 1;
		// utf-16be
		else if(*(word*)p == 0xFFFE) utfmode = 2;
	}
	ScanLineStats(map, filesize);
	
	if(linescanRunning.load(std::memory_order_acquire)){
		if(map.GetOffset()+(int64)map.GetCount()<filesize){
			LOG("Warning: File'"<< path <<"'did not full complete scanned line.");
		}
		linestats.SetLinesInTotal();
		linescanRunning.store(false, std::memory_order_release);	// 线程自动结束
	}
}

void FilemodelBase::BackstageFileLinesInitTaskRun(const char *path, int64 offset, bool utfverifiable)
{
	if( lnscan.Run([this, path, offset, utfverifiable]{
	BackstageFileLinesInitTask(path, offset, utfverifiable);}) ){
		linescanRunning.store(true, std::memory_order_release);
		// Sleep(1);
	}
}

void FilemodelBase::BackstageRequestStop(bool shutdown)
{
	linescanRunning = false;
	if(shutdown) return;	// 快速进入线程退出处理
	lnscan.Wait();
	// 后台线程结束，可以安全清理
	linestats.ClearLineIndexStats();
}

void FilemodelBase::BackstageFileLinesReScan(const char *path, int64 offset, bool utfverifiable)
{
	BackstageRequestStop();
	BackstageFileLinesInitTaskRun(path, offset, utfverifiable);
}

bool FilemodelInfo::uppGlobalLoadFileProcess(const char *path, bool scan)
{
	errorcode = 0;
	fsize = -1;
	if(!FileExists(path)){
		LOG("Error: File'"<< path <<"'does not exist!");
		errorcode = -1;
		return false;
	}
	
	int64 size = GetFileLength(path);
	if(size<0){
		LOG("Error: Unable to obtain the size of file'"<< path <<"'!");
		errorcode = 1;
		return false;
	}
	// 启用后台扫描分析文件行结构的任务
	if(scan){
		BackstageRequestStop();
		BackstageFileLinesInitTaskRun(path);
	}
	// 大文件优化的加载方式
	if(size<=USUALLY_IO_SIZE_MAX){
		fin.Open(path);
		if(!fin.IsOpen()){
			LOG("Error: File'"<< path <<"'is inaccessible!");
			errorcode = 1;
			return false;
		}
		fin.ClearError();
		content = fin.Get(FILEBLOCK);

		if(fin.IsError()){
			content.Clear();
			LOG("StreamError: File'"<< path <<"'IO error!");
			errorcode = 2;
			return false;
		}
	}
	else{
		if(!fmap.Open(path)){
			LOG("Error: File'"<< path <<"'is inaccessible!");
			errorcode = 1;
			return false;
		}
		if( !(size<MMAP_FILECHUNK ? fmap.Map() : fmap.Map(0,MMAP_FILECHUNK)) ){
			LOG("Error: Memory mapping failed for file'"<< path <<"'!");
			errorcode = 3;
			return false;
		}
	}
	fsize = size;
	return true;
}

static int64 calDistance(int64 raw, uint32 base)
{
	int64 d = (raw/base)*base;
	if(d!=raw && raw<0) d -= base;
	return d;
}
void FilemodelInfo::ReloadData(int64 pos)
{
	int64 l;
	if(fsize > USUALLY_IO_SIZE_MAX){
		l = pos-fmap.GetOffset();
		fmap.Map(fmap.GetOffset()+calDistance(l,MMAP_FILECHUNK), MMAP_FILECHUNK);
	}
	else{
		if(int64 rem = fin.GetPos()%FILEBLOCK) fin.SeekCur(-rem);
		l = pos - fin.GetPos();
		fin.SeekCur(calDistance(l, FILEBLOCK));
		content = fin.Get(FILEBLOCK);
	}
}

const char* FilemodelInfo::GetFileData(int64 offs, uint32& len) const
{
	int64 length = GetDataLength();
	if(offs > length) offs = length;
	if(offs+len > length) len = length - offs;
	return GetFileData() + offs;
}

const char* FilemodelInfo::ReadData(int64 pos, uint32& size)
{
	if(!IsLoaded()){
		size = 0;
		return NULL;
	}
	pos = minmax<int64>(pos, 0, fsize);	// 0 ~ fsize;

	int64 dpos = GetDataPos();
	if( (pos < dpos) || (pos >= dpos+GetDataLength()) ){
		ReloadData(pos);
	}
	const char* s = GetFileData(pos-GetDataPos(), size);
	return s;
}

const char* FilemodelInfo::GetHugerBuffer(const char *path, int64& size)
{
	if(!huger.Open(path)){
		size = 0;
		return NULL;
	}
	char* buffer = (char*)huger.Map();
	size = (buffer ? huger.GetCount() : 0);
	return buffer;
}
