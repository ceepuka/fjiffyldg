#ifndef _uppFilemodel_uppFilemodel_h
#define _uppFilemodel_uppFilemodel_h

#include <Core/Core.h>

using namespace Upp;

constexpr int KB = 1024;
constexpr int MB = (1024 * KB);
constexpr int GB = (1024 * MB);
// 通用文件块
constexpr int FILEBLOCK = 10 * MB;

// #define WORD_SIZE_WIDTH sizeof(size_t)

#include "FileLineIndex.h"
#include "FileLineIndex.hpp"

class FilemodelBase{
	std::atomic<bool> linescanRunning = false;
	// 适应宽字符文本模式 0:默认,1~2:utf-16le/be,3~4:utf-32le/be
	int utfmode = 0;
	// 文件行的索引处理
	LineIndex linestats;
	template <class Target> void ScanLineStats(Target t, FileIn &scan);
	
	template <typename T> void UpdateLineStats(const T* q, int64 pos, int len, byte &last, byte c = 0);
	
protected:
	Thread lnscan;
	void BackstageFileLinesInitTask(const char *path, int64 offset, bool utfverifiable);
	void BackstageFileLinesInitTaskRun(const char *path, int64 offset = 0, bool utfverifiable = false);
	void BackstageRequestStop(bool shutdown = false);
public:
	inline int64 GetLineCount()const {return (linescanRunning) ? -1 :
										(linestats.IsScanned() ? linestats.GetLines(): 0);}
	inline int64 GetLinePos(int64 i) {return (linestats.IsScanned() || linescanRunning) ?
										linestats.GetLindexPos(i, utfmode): -1;}
	
	inline int64 GetLineLength(int64 i) {return linestats.GetLineLen(i, utfmode);}
	inline int64 GetLineByPos(int64 pos) {return linestats.GetLineByPos(pos, utfmode);}
	inline void SetUtfMode(int utf) {utfmode = (utf>=0 && utf<=4) ? utf : 0;}
	// 重新扫描文件行结构
	void BackstageFileLinesReScan(const char *path, int64 offset, bool utfverifiable);
	inline void LineScanThreadWaitFinished() {lnscan.Wait();}
};

class FilemodelInfo : public FilemodelBase{
	int errorcode = 0;
	FileIn fin;
	const byte* finBegin = NULL;
	FileMapping fmap;
	FileMapping huger;	// 最大限度地访问文件内容
	int64 fsize = -1;
	inline const char* GetFileData()const {return fmap.IsOpen() ? (const char*)fmap.Begin() : (const char*)finBegin;}
	
	const char* GetFileData(int64 offs, uint32& len) const;
	inline int64 GetDataLength()const {return fmap.IsOpen() ? fmap.GetCount() : min<uint64>(fin.GetBufferSize(), fsize - fin.GetPos());}
	
	inline int64 GetDataPos()const {return fmap.IsOpen() ? fmap.GetOffset() : fin.GetPos();}
	void ReloadData(int64 pos);

public:
	
	FilemodelInfo();
	inline bool IsLoaded()const {return fsize > -1;}
	inline int64 GetFileSize()const {return fsize;}
	bool uppGlobalLoadFileProcess(const char *path, bool enablescan);
	inline int GetErrorcode()const {return errorcode;}
	const char* ReadData(int64 pos, uint32& size);
	const char* GetHugerBuffer(const char *path, int64& size);
	inline void ClearHuger() {huger.Close();}
	~FilemodelInfo();
};

#endif
