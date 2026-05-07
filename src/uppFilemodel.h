#ifndef _uppFilemodel_uppFilemodel_h
#define _uppFilemodel_uppFilemodel_h

#include <Core/Core.h>

using namespace Upp;

constexpr int KB = 1024;
constexpr int MB = (1024 * KB);
constexpr int GB = (1024 * MB);

#include "FileLineIndex.h"
#include "FileLineIndex.hpp"

class FilemodelBase{
	std::atomic<bool> linescanRunning = false;
	// 适应宽字符文本模式 0:默认,1~2:utf-16le/be,3~4:utf-32le/be
	int utfmode = 0;
	// 文件行的索引处理
	LineIndex linestats;
	void ScanLineStats(FileMapping &map, int64 filesize);
	
	template <typename T> void UpdateLineStats(FileMapping &map, const T* q, byte &last, byte c = 0);
	
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
	String content;
	FileIn fin;
	FileMapping fmap;
	FileMapping huger;	// 最大限度地访问文件内容
	int64 fsize = -1;
	inline const char* GetFileData()const {return fsize > USUALLY_IO_SIZE_MAX ?
										(const char*)fmap.Begin() : content.Begin();}
	
	const char* GetFileData(int64 offs, uint32& len) const;
	inline int64 GetDataLength()const {return fsize > USUALLY_IO_SIZE_MAX ?
										fmap.GetCount() : content.GetLength();}
	
	inline int64 GetDataPos()const {return fsize > USUALLY_IO_SIZE_MAX ?
										fmap.GetOffset() : fin.GetPos()-content.GetLength();}
	void ReloadData(int64 pos);

public:
	// 10MB 大小作为文件流式与映射访问的临界值
	static inline constexpr int USUALLY_IO_SIZE_MAX = (10 * MB);
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
