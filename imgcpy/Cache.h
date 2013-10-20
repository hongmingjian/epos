#ifndef _CACHE_H_
#define _CACHE_H_

#include "ComDefs.h"
#include "ComTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

// Cache entry structure
typedef struct tCacheEntry {
  int                   IsValid;        // internal: valid flag
  int                   IsDirty;        // internal: dirty flag
  uint32                AccessPattern;  // internal: NRU access pattern
  int                   ErrorCode;      // user-defined: error code
  void*                 pAttachedData;  // user-defined: attached data
} tCacheEntry;

// User-defined fxn that loads entry from offcache storage when needed:
typedef int (*tCacheEntryRefillFxn) (tCacheEntry* pCacheEntry, void* pReuseInfo);
// User-defined fxn that saves entry to offcache storage when needed:
typedef int (*tCacheEntryFlushFxn) (tCacheEntry* pCacheEntry);
// User-defined fxn that tells if the entry can be reused
// without refilling (if valid, of course):
typedef int (*tCacheEntryIsReusableFxn) (tCacheEntry* pCacheEntry, void* pReuseInfo);

// Cache entry init fxn:
extern void Cache_InitEntry (tCacheEntry* pCacheEntry, void* pAttachedData);

// Finds needed cache entry, whose attached data can then be read or written:
extern tCacheEntry*
Cache_FindEntryForUse (tCacheEntry* pCacheEntries, uint EntriesCount,
                       void* pReuseInfo,
                       tCacheEntryRefillFxn pRefill,
                       tCacheEntryFlushFxn pFlush,
                       tCacheEntryIsReusableFxn pIsReusable);

// Should be used in user-defined fxn tCacheEntryRefillFxn and
// "attached data read" fxn:
extern int Cache_GetIsValid (tCacheEntry* pCacheEntry);

// May be used in user-defined fxn
// to invalidate a cache entry:
extern void Cache_ResetIsValid (tCacheEntry* pCacheEntry);

// Should be used in user-defined fxn tCacheEntryFlushFxn:
extern int Cache_GetIsDirty (tCacheEntry* pCacheEntry);

// Should be used in user-defined "attached data write" fxn:
extern void Cache_SetIsDirty (tCacheEntry* pCacheEntry);

// Should be used in user-defined fxn tCacheEntryFlushFxn:
extern void Cache_ResetIsDirty (tCacheEntry* pCacheEntry);

// Should be used in user-defined fxn tCacheEntryRefillFxn and
// tCacheEntryFlushFxn:
extern void Cache_SetErrorCode (tCacheEntry* pCacheEntry, int ErrorCode);
extern int  Cache_GetErrorCode (tCacheEntry* pCacheEntry);

#ifdef __cplusplus
extern "C" {
#endif

#endif // _CACHE_H_
