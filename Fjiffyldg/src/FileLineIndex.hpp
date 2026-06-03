// 字符处理相关的函数模板定义

// i 表示取（宽）字符中的某个字节
// 大端时取最后字节，其他为第一个
// 故 i 只能为 0, 1, 3
template <typename T>
force_inline bool IsNewlineLF(T c, byte i)
{
	dword LF = 0;
	((byte*)&LF)[i] = '\n';
	return c == LF;
}

template <typename T>
force_inline bool IsNewlineCR(T c, byte i)
{
	dword CR = 0;
	((byte*)&CR)[i] = '\r';
	return c == CR;
}

template <typename T>
force_inline bool IsReadNewlineChar(const T* &q, byte i)
{
	if(IsNewlineCR(*q, i)){
		// CRLF 时需跳过一个
		if(IsNewlineLF(q[1], i)) q++;
		return true;
	}
	return IsNewlineLF(*q, i);
}

template <typename T>
int FindLinePosOffset(const T* buffer, int64 begin, int64 end, byte c=0)
{ // begin, end 分别表示起始行索引和目标行索引，c 表示字节偏移量
	const T* q = buffer;
	do{
		if(IsReadNewlineChar(q, c)) begin++;
		q++;
	}while(begin < end);
	return (byte *)q - (byte *)buffer;
}

template <typename T>
force_inline int GetNewlineByteCount(const T* tail, int64 len, byte c=0)
{ // tail 表示从行尾端检查，len 为行的存储长度
	len -= sizeof(T);
	if(len && IsNewlineLF(*(tail-1), c)  && IsNewlineCR(*(tail-2), c)) return sizeof(T) * 2;
	return sizeof(T);
}
