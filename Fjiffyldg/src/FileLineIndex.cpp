#include "uppFilemodel.h"

void LineIndex::SetMappingPartitionsByLine(int64 pos)
{
	const int64 d = pos - chunk.Top().pos;
	if(d < CHUNK_SIZE){
		chunk.Top().index++;	// 在分区内更新行号
	}
	else{
		// 添加新分区
		chunk.Add(LindexPos(chunk.Top().index + 1, pos));
	}
}

void LineIndex::AddLine(int64 pos)
{
	RWMutex::WriteLock lock(veclock);
	if(direct.GetCount()+exdirect.GetCount() < DIRECT_LINES_MAX){
		pos <= (int64)UINT_MAX ? direct.Add(pos) : exdirect.Add(pos);
		return;
	}
	
	if(chunk.IsEmpty()){
		// 添加第一个分区
		chunk.Add(LindexPos(CHUNK_BEGIN, pos));
		return;
	}
	
	if(chunk.GetCount() < CHUNK_COUNT_MAX) SetMappingPartitionsByLine( pos);
	else{
		lines++;
		if(!overstep) overstep = pos;
	}
}

int64 LineIndex::GetLindexPos(int64 i, int utfmode)
{
	if(i < 0) return -1;
	if(i == lastline) return lastpos;
	if(i == 0) return 0;
	RWMutex::ReadLock lock(veclock);	// vector 加锁
	int i32 = i-1;
	const int n = direct.GetCount();
	if(i-1 < n) return direct[i32];
	else if(i-1 < n + exdirect.GetCount()) return exdirect[i32 - n];
	
	// 二分法查找返回行 i 所在的分区的索引
	i32 = FindLowerBound(chunk, LindexPos(i));
	// 获取起始位置和行，所需内存映射长度
	int64 pos, l, ml;
	if(i32 < chunk.GetCount()){
		pos = chunk[i32].pos;
		l = (i32 ? (chunk[i32 - 1].index + 1) : CHUNK_BEGIN);
		ml = CHUNK_SIZE;
	}
	else if(overstep){
		pos = overstep;
		l = chunk.Top().index + 1;
		ml = fileSize - pos;
	}
	else return -1;

	if(i==l) return pos;	// l 行的位置即分区位置
	// 连续递增访问优化判断
	if(lastline > l && i > lastline){
		l = lastline;
		pos = lastpos;
	}
	
	const byte* buffer = NULL;
	if(map.IsOpen()){
		if(pos>=map.GetOffset() && (pos + ml) <= map.GetOffset()+map.GetCount()){
			buffer = ~map + pos - map.GetOffset();
		}
		else{
			int64 mapl = (overstep ? ml : FILEBLOCK);
			buffer = map.Map(pos, mapl);
		}
	}
	
	Buffer<byte> data;
	if(!buffer && !overstep){
		data.Alloc(CHUNK_SIZE);
		if(data.IsEmpty()) return -1;
		FileIn in(filename);
		in.Seek(pos);
		ml = in.Get(data, CHUNK_SIZE);
		if((int)ml <= 0) return -1;
		buffer = data.Get();
	}
	if(!buffer) return -1;	// 典型例如文件被其他应用程序修改，导致某些数据无法访问
	
	int64 offset = pos;
	switch(utfmode){
		// 考虑宽字符处理 utf-16 和 utf-32
		case 1:		// utf16le
		case 2: pos += FindLinePosOffset((word *)buffer, l, i, ml, utfmode - 1); break;	// utf16be
		case 3:		// utf32le
		case 4: pos += FindLinePosOffset((dword *)buffer, l, i, ml, (utfmode ^ 7) & 3); break;	// utf32be
		default:
			pos += FindLinePosOffset(buffer, l, i, ml);
	}
	if(pos <= offset) return -1;
	// 更新作为上次访问
	lastline = i;
	lastpos = pos;
	return pos;
}

int64 LineIndex::GetLineLen(int64 i, int utfmode)
{
	if(!lnscanned || i >= lines || i < 0) return -1;
	int64 pos = GetLindexPos(i, utfmode);
	if(pos < 0) return -1;
	if(i+1 == lines) return fileSize - pos;
	
	int64 len = GetLindexPos(i+1, utfmode) - pos;
	if(len <= 0) return -1;
	const byte* buffer = NULL;
	if(map.IsOpen()) buffer = map.Map(pos, len);
	
	Buffer<byte> data;
	if(!buffer && len <= (dword)(INT_MAX ^ UINT_MAX) ){
		data.Alloc(len);
		if(data.IsEmpty()) return -1;
		FileIn in(filename);
		in.Seek(pos);
		int size = in.Get(data, len);
		if(size <= 0 || size < len) return -1;
		buffer = data.Get();
	}
	if(!buffer) return -1;	// 典型例如文件被其他应用程序修改，导致某些数据无法访问
	
	switch(utfmode){
		case 1:
		case 2: len -= GetNewlineByteCount((word *)(buffer + len), len, utfmode - 1); break;
		case 3:
		case 4: len -= GetNewlineByteCount((dword *)(buffer + len), len, (utfmode ^ 7) & 3); break;
		default:
			len -= GetNewlineByteCount(buffer + len, len);
	}
	return len;
}

int64 LineIndex::GetLineByPos(int64 pos, int utfmode)
{
	if( !lnscanned || pos < 0 || pos > fileSize ) return -1;
	int64 i;
	i = (pos <= (int64)UINT_MAX ? FindLowerBound(direct, pos) :
	FindLowerBound(exdirect, pos) + direct.GetCount()) + 1;
	if(i < CHUNK_BEGIN) return (pos == GetLindexPos(i) ? i : i-1);
	
	int64 index;
	i = FindLowerBound(chunk, LindexPos(0, pos), LindexPos::LessByPos);
	if(i < CHUNK_COUNT_MAX){
		int below = 0;
		if(pos < chunk[(int)i].pos){
			i--;
			below++;
		}
		if(i == -1) return CHUNK_BEGIN - 1;
		
		index = (i == 0 ? CHUNK_BEGIN : chunk[(int)i-1].index + 1);
		if(!below) return index;
	}
	else{
		index = ( !overstep ? chunk[CHUNK_COUNT_MAX - 2].index + 1 : chunk.Top().index + 1);
	}
	while(pos > (uint64)GetLindexPos(index+1, utfmode) )index++;
	
	int64 p = GetLindexPos(index+1, utfmode);
	if(p < 0) return -1;
	return (pos == p? index+1 : index);
}

void LineIndex::SetLinesInTotal()
{
	if(lnscanned) return;
	lines += (chunk.GetCount()? chunk.Top().index : direct.GetCount()+exdirect.GetCount());
	lnscanned = true;
}
// 总是结束后台线程以后清理保证线程安全
void LineIndex::ClearLineIndexStats()
{
	lnscanned = false;
	
	direct.Clear();
	exdirect.Clear();
	chunk.Clear();
	overstep = 0;
	lines = 1;
	lastline = 0;
	lastpos = 0;
	filename.Clear();
}
