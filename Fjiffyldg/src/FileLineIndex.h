#ifndef _uppFilemodel_FileLineIndex_h_
#define _uppFilemodel_FileLineIndex_h_

// 文件行索引管理模块，提供文件按行访问的基础结构
// 此结构策略：分级访问（直接索引与分块索引）
// 限制文件行数过多或体积过大时内存膨胀

class LineIndex {
	// 定义直接索引 100 万行
	static inline constexpr int DIRECT_LINES_MAX = 1000000;
	// 超过 100 万行从 chunk 开始行索引
	static inline constexpr int CHUNK_BEGIN = DIRECT_LINES_MAX + 1;
	// 每个分区 chunk 映射到文件相应位置 128KB 块
	static inline constexpr int CHUNK_SIZE = 128 * KB;
	// chunk 数量限制为 8M 个，8M * 128KB = 1TB
	static inline constexpr int CHUNK_COUNT_MAX = 8 * MB;
	// 通常的文件大小远不会超过 4GB 故首先用 32 位大小
	Vector<uint32> direct;
	Vector<int64> exdirect;	// 超大文件拓展
	
	FileMapping map;
	// 映射离散分区索引结构
	struct LindexPos {
		int64 index;	// 分区内最大行索引
		int64 pos;		// 分区开始行偏移位置
		LindexPos(int64 i, int64 p = 0): index(i), pos(p){}
		// 重载运算符 < 以便二分法查找 index
		inline bool operator<(const LindexPos& other)const {return index < other.index;}
		static bool LessByPos(const LindexPos& A, const LindexPos& B) {return A.pos < B.pos;}
	};
	
	Vector<LindexPos> chunk;
	
	int64 overstep = 0;	// 超出内存索引管理限制的位置（默认 0 ）
	int64 lines = 1;	// 扫描的行数（文件至少 1 行）
	void SetMappingPartitionsByLine(int64 pos);
	bool lnscanned = false;
	// 一定条件存储上次访问可连续访问优化
	int64 lastline = 0;
	int64 lastpos = 0;
	
public:
	RWMutex maplock, veclock;
	inline FileMapping& GetFileMapOpen(const char *file) {map.Open(file); return map;}
	void AddLine(int64 pos);
	// 根据行索引返回行的偏移位置
	int64 GetLindexPos(int64 i, int utfmode = 0);
	int64 GetLineLen(int64 i, int utfmode);
	void SetLinesInTotal();
	inline bool IsScanned()const {return lnscanned;}
	inline int64 GetLines()const {return lines;}
	void ClearLineIndexStats();
	// 获取 pos 所在的行（索引）
	int64 GetLineByPos(int64 pos, int utfmode);
};

#endif
