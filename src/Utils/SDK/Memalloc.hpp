#pragma once

typedef size_t (*MemAllocFailHandler_t)(size_t);

struct _CrtMemState;

struct GenericMemoryStat_t {
	const char *name;
	int value;
};

class IVirtualMemorySection {
public:
	// Information about memory section
	virtual void *GetBaseAddress() = 0;
	virtual size_t GetPageSize() = 0;
	virtual size_t GetTotalSize() = 0;

	// Functions to manage physical memory mapped to virtual memory
	virtual bool CommitPages(void *pvBase, size_t numBytes) = 0;
	virtual void DecommitPages(void *pvBase, size_t numBytes) = 0;

	// Release the physical memory and associated virtual address space
	virtual void Release() = 0;
};

class IMemAlloc {
public:
	// Release versions
	virtual void *Alloc(size_t nSize) = 0;

public:
	virtual void *Realloc(void *pMem, size_t nSize) = 0;

	virtual void Free(void *pMem) = 0;
	virtual void *Expand_NoLongerSupported(void *pMem, size_t nSize) = 0;

	// Debug versions
	virtual void *Alloc(size_t nSize, const char *pFileName, int nLine) = 0;

public:
	virtual void *Realloc(void *pMem, size_t nSize, const char *pFileName, int nLine) = 0;
	virtual void Free(void *pMem, const char *pFileName, int nLine) = 0;
	virtual void *Expand_NoLongerSupported(void *pMem, size_t nSize, const char *pFileName, int nLine) = 0;

	inline void *IndirectAlloc(size_t nSize) { return Alloc(nSize); }
	inline void *IndirectAlloc(size_t nSize, const char *pFileName, int nLine) { return Alloc(nSize, pFileName, nLine); }

	// Returns the size of a particular allocation (NOTE: may be larger than the size requested!)
	virtual size_t GetSize(void *pMem) = 0;

	// Force file + line information for an allocation
	virtual void PushAllocDbgInfo(const char *pFileName, int nLine) = 0;
	virtual void PopAllocDbgInfo() = 0;

	// FIXME: Remove when we have our own allocator
	// these methods of the Crt debug code is used in our codebase currently
	virtual int CrtSetBreakAlloc(int lNewBreakAlloc) = 0;
	virtual int CrtSetReportMode(int nReportType, int nReportMode) = 0;
	virtual int CrtIsValidHeapPointer(const void *pMem) = 0;
	virtual int CrtIsValidPointer(const void *pMem, unsigned int size, int access) = 0;
	virtual int CrtCheckMemory(void) = 0;
	virtual int CrtSetDbgFlag(int nNewFlag) = 0;
	virtual void CrtMemCheckpoint(_CrtMemState *pState) = 0;

	// FIXME: Make a better stats interface
	virtual void DumpStats() = 0;

	enum DumpStatsFormat_t {
		FORMAT_TEXT,
		FORMAT_HTML
	};

	virtual void DumpStatsFileBase(char const *pchFileBase, DumpStatsFormat_t nFormat = FORMAT_TEXT) = 0;
	virtual size_t ComputeMemoryUsedBy(char const *pchSubStr) = 0;

	// FIXME: Remove when we have our own allocator
	virtual void *CrtSetReportFile(int nRptType, void *hFile) = 0;
	virtual void *CrtSetReportHook(void *pfnNewHook) = 0;
	virtual int CrtDbgReport(int nRptType, const char *szFile, int nLine, const char *szModule, const char *pMsg) = 0;

	virtual int heapchk() = 0;

	virtual bool IsDebugHeap() = 0;

	virtual void GetActualDbgInfo(const char *&pFileName, int &nLine) = 0;
	virtual void RegisterAllocation(const char *pFileName, int nLine, size_t nLogicalSize, size_t nActualSize, unsigned nTime) = 0;
	virtual void RegisterDeallocation(const char *pFileName, int nLine, size_t nLogicalSize, size_t nActualSize, unsigned nTime) = 0;

	virtual int GetVersion() = 0;

	virtual void CompactHeap() = 0;

	// Function called when malloc fails or memory limits hit to attempt to free up memory (can come in any thread)
	virtual MemAllocFailHandler_t SetAllocFailHandler(MemAllocFailHandler_t pfnMemAllocFailHandler) = 0;

	virtual void DumpBlockStats(void *) = 0;

	virtual void SetStatsExtraInfo(const char *pMapName, const char *pComment) = 0;

	// Returns 0 if no failure, otherwise the size_t of the last requested chunk
	virtual size_t MemoryAllocFailed() = 0;

	virtual void CompactIncremental() = 0;

	virtual void OutOfMemory(size_t nBytesAttempted = 0) = 0;

	// Region-based allocations
	virtual void *RegionAlloc(int region, size_t nSize) = 0;
	virtual void *RegionAlloc(int region, size_t nSize, const char *pFileName, int nLine) = 0;

	// Replacement for ::GlobalMemoryStatus which accounts for unused memory in our system
	virtual void GlobalMemoryStatus(size_t *pUsedMemory, size_t *pFreeMemory) = 0;

	// Obtain virtual memory manager interface
	virtual IVirtualMemorySection *AllocateVirtualMemorySection(size_t numMaxBytes) = 0;

	// Request 'generic' memory stats (returns a list of N named values; caller should assume this list will change over time)
	virtual int GetGenericMemoryStats(GenericMemoryStat_t **ppMemoryStats) = 0;

	virtual ~IMemAlloc(){};

	// handles storing allocation info for coroutines
	virtual unsigned int GetDebugInfoSize() = 0;
	virtual void SaveDebugInfo(void *pvDebugInfo) = 0;
	virtual void RestoreDebugInfo(const void *pvDebugInfo) = 0;
	virtual void InitDebugInfo(void *pvDebugInfo, const char *pchRootFileName, int nLine) = 0;
};

inline IMemAlloc *g_pMemAlloc;

inline void *MemAlloc_Alloc(size_t nSize) {
	return g_pMemAlloc->IndirectAlloc(nSize);
}

inline void *MemAlloc_Alloc(size_t nSize, const char *pFileName, int nLine) {
	return g_pMemAlloc->IndirectAlloc(nSize, pFileName, nLine);
}