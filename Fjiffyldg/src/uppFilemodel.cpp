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
	Cout() << "Fjiffyldg debug version. \n";
	StdLogSetup(LOG_FILE | LOG_APPEND | LOG_ELAPSED);
#endif
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
	Buffer<byte> data(MB);
	if(data.IsEmpty()) return;
	byte* &&buffer = data.Get();
	do{
		int64 checkl = scan.GetPos();
		int size = scan.Get(buffer, MB);
		if( ! linescanRunning.load(std::memory_order_relaxed)|| !size) return;
		t(buffer, checkl, size, last);
	}while(!scan.IsEof());
	if(last == '\r') linestats.AddLine(scan.GetPos());
}

void FilemodelBase::BackstageFileLinesInitTask(const char *path, int64 offset, bool utfverifiable)
{
	FileIn scanFile;
	const int64 filesize = GetFileLength(path);
	if(offset<0 || offset>=filesize || !scanFile.Open(path)){
		linescanRunning.store(false, std::memory_order_release);
		return;
	}
	
	FileMapping &map = linestats.GetFileMapOpen(path);
	if(!map.IsOpen()||!map.Map(0, FILEBLOCK)){
		map.Close();
		// 极端情况后台不扫描过大文件
		if(filesize > INT_MAX){
			linescanRunning.store(false, std::memory_order_release);
			return;
		}
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
		case 1:ScanLineStats([this](byte* buffer, int64 pos, int len, byte &last) {UpdateLineStats((word* )buffer, pos, len, last);}, scanFile);break;
		case 2:ScanLineStats([this](byte* buffer, int64 pos, int len, byte &last) {UpdateLineStats((word* )buffer, pos, len, last, 1);}, scanFile);break;
		case 3:ScanLineStats([this](byte* buffer, int64 pos, int len, byte &last) {UpdateLineStats((dword* )buffer, pos, len, last);}, scanFile);break;
		case 4:ScanLineStats([this](byte* buffer, int64 pos, int len, byte &last) {UpdateLineStats((dword* )buffer, pos, len, last, 3);}, scanFile);break;
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

	int64 size = GetFileLength(path);
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
		fin.Open(path);
		if(!fin.IsOpen()){
			LOG("Error: File'"<< path <<"'is inaccessible!");
			errorcode = 1;
			return false;
		}
		content = fin.Get(FILEBLOCK);
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
		if(int64 rem = fin.GetPos()%FILEBLOCK) fin.SeekCur(-rem);
		int64 l = pos - fin.GetPos();
		fin.SeekCur(calDistance(l, FILEBLOCK));
		content = fin.Get(FILEBLOCK);
	}
	else{
		int64 l = pos-fmap.GetOffset();
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
