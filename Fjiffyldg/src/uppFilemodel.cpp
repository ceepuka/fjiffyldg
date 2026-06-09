#include "uppFilemodel.h"

#if defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__)
// 大文件单次映射最多 1GB
static constexpr int MMAP_FILECHUNK = GB;
#else
static constexpr int MMAP_FILECHUNK = 128 * MB;
#endif

FilemodelInfo::FilemodelInfo()
{
#ifdef _DEBUG
	LOG("Fjiffyldg debug version.");
	StdLogSetup(LOG_FILE | LOG_APPEND | LOG_ELAPSED);
#endif
	LOG("File model Create!");
}

FilemodelInfo::~FilemodelInfo()
{
	BackstageRequestStop(true);
	lnscan.ShutdownThreads();
}

template <typename T>
void FilemodelBase::UpdateLineStats(const T* q, int64 pos, int len, byte &last, byte c)
{
	const byte * const b = (byte*)q;
	const byte * const e = (byte*)q + len;
	if(last == '\r'){
		last = 0;
		if(! IsNewlineLF(*q, c)) linestats.AddLine(pos);
	}
	
	while((byte*)q < e - sizeof(T)){
		if(IsReadNewlineChar(q, c)){
			linestats.AddLine(pos+((byte*)q-b)+sizeof(T));
		}
		q++;
	}
	
	if((byte*)q == e - sizeof(T)){
		if(IsNewlineLF(*q, c)) linestats.AddLine(pos + len);
		else if(IsNewlineCR(*q, c)) last = '\r';
	}
}

template <class Target>
void FilemodelBase::ScanLineStats(Target t, FileIn &scan)
{
	byte last = 0;	// 表示已扫描过的块最后一个字节
	
	int size;
	int64 offset = scan.GetPos();
	for(const byte* buffer = scan.GetSzPtr(size); size > 0 ;){
		t((byte*)buffer, offset, size, last);
		if( ! linescanRunning.load(std::memory_order_relaxed)) return;
		
		offset = scan.GetPos();
		buffer = scan.GetSzPtr(size);
	}
	
	if(last == '\r') linestats.AddLine(scan.GetPos());
}

void FilemodelBase::BackstageFileLinesInitTask(const char *path, int64 offset, bool utfverifiable)
{
	FileIn scanFile;
	const int64 filesize = GetFileLength(path);
	
#if defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__)
	scanFile.SetBufferSize((dword)minmax<int64>(filesize, 4096, MB));
#else
	scanFile.SetBufferSize((dword)minmax<int64>(filesize, 4096, 64 * KB));
#endif
	
	if(offset<0 || offset>=filesize || !scanFile.Open(path)){
		linescanRunning.store(false, std::memory_order_release);
		return;
	}
	
	FileMapping &map = linestats.GetFileMapOpen(path);
	if(!map.IsOpen()||!map.Map(0, FILEBLOCK)){
		map.Close();
	}
	
	// 默认以 BOM 标签确定 utfmode
	if(!utfverifiable && filesize >= 4){
		const byte *p = map.Begin();
		byte data[4];
		if(!p){
			int getsize = scanFile.Get(data, 4);
			if(getsize < 4){
				linescanRunning.store(false, std::memory_order_release);
				return;
			}
			p = data;
		}
		// utf-32le
		if(*(dword*)p == 0xFEFF) utfmode = 3;
		// utf-32be
		else if(*(dword*)p == 0xFFFE0000) utfmode = 4;
		// utf-16le
		else if(*(word*)p == 0xFEFF) utfmode = 1;
		// utf-16be
		else if(*(word*)p == 0xFFFE) utfmode = 2;
	}
	scanFile.Seek(offset);
	switch(utfmode){
		case 1:
		case 2:ScanLineStats([this](byte* buffer, int64 pos, int len, byte &last) {UpdateLineStats((word* )buffer, pos, len, last, utfmode - 1);}, scanFile);break;
		case 3:
		case 4:ScanLineStats([this](byte* buffer, int64 pos, int len, byte &last) {UpdateLineStats((dword* )buffer, pos, len, last, (utfmode ^ 7) & 3);}, scanFile);break;
		default:
			ScanLineStats([this](byte* buffer, int64 pos, int len, byte &last) {UpdateLineStats(buffer, pos, len, last);}, scanFile);
	}
	
	if(linescanRunning.load(std::memory_order_acquire) && filesize == scanFile.GetPos()){
		linestats.SetLinesInTotal();
	}
	linescanRunning.store(false, std::memory_order_release);	// 线程自动结束
}

void FilemodelBase::BackstageFileLinesInitTaskRun(const char *path, int64 offset, bool utfverifiable)
{
	if( lnscan.Run([this, path, offset, utfverifiable]{
	BackstageFileLinesInitTask(path, offset, utfverifiable);}) ){
		linescanRunning.store(true, std::memory_order_release);
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

	const int64 size = GetFileLength(path);
	if(size<0){
		LOG("Error: File'"<< path <<"'does not exist!");
		errorcode = -1;
		return false;
	}
	// 启用后台扫描分析文件行结构的任务
	if(scan){
		BackstageRequestStop();
		BackstageFileLinesInitTaskRun(path);
	}
	// 大文件优化的加载方式
	if(!fmap.Open(path) || !(size<MMAP_FILECHUNK ? fmap.Map() : fmap.Map(0,MMAP_FILECHUNK))){
		fmap.Close();
		fin.SetBufferSize((dword)minmax<int64>(size, 4096, FILEBLOCK));
		fin.Open(path);
		if(fin.GetSize()!= size || fin.Peek() < 0){
			LOG("Error: File'"<< path <<"'is inaccessible!");
			errorcode = 1;
			return false;
		}
		finBegin = fin.PeekPtr();
	}
	else{
		if(fmap.GetFileSize()!= size){
			errorcode = 1;
			return false;
		}
	}
	fsize = size;
	return true;
}

static force_inline int64 calDistance(int64 raw, uint32 base)
{
	int64 d = (raw/base)*base;
	if(d!=raw && raw<0) d -= base;
	return d;
}
void FilemodelInfo::ReloadData(int64 pos)
{
	if(fin.IsOpen()){
		int64 l = pos - fin.GetPos();
		fin.SeekCur(calDistance(l, fin.GetBufferSize()) );
		finBegin = (fin.Peek() >= 0 ? fin.PeekPtr() : NULL);
	}
	else{
		int64 l = pos - fmap.GetOffset();
		fmap.Map(fmap.GetOffset()+calDistance(l,MMAP_FILECHUNK), MMAP_FILECHUNK);
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
	char* buffer = ((size_t)huger.GetFileSize() == huger.GetFileSize()) ?
		(char*)huger.Map() : (char*)huger.Map(0, (size_t)-1);
	size = huger.GetCount();
	return buffer;
}
