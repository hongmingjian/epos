#include "Cache.h"

void Cache_InitEntry (tCacheEntry* pCacheEntry, void* pAttachedData)
{
  pCacheEntry->IsValid = 0;
  pCacheEntry->IsDirty = 0;
  pCacheEntry->AccessPattern = 0;
  pCacheEntry->pAttachedData = pAttachedData;
}

int Cache_GetIsValid (tCacheEntry* pCacheEntry)
{
  return pCacheEntry->IsValid;
}

void Cache_ResetIsValid (tCacheEntry* pCacheEntry)
{
  pCacheEntry->IsValid = 0;
  // also drop access history:
  pCacheEntry->AccessPattern = 0;
}

int Cache_GetIsDirty (tCacheEntry* pCacheEntry)
{
  return pCacheEntry->IsDirty;
}

void Cache_SetIsDirty (tCacheEntry* pCacheEntry)
{
  pCacheEntry->IsDirty = 1;
}

void Cache_ResetIsDirty (tCacheEntry* pCacheEntry)
{
  pCacheEntry->IsDirty = 0;
}

tCacheEntry*
Cache_FindEntryForUse (tCacheEntry* pCacheEntries, uint EntriesCount,
                       void* pReuseInfo,
                       tCacheEntryRefillFxn pRefill,
                       tCacheEntryFlushFxn pFlush,
                       tCacheEntryIsReusableFxn pIsReusable)
{
  tCacheEntry* pCacheEntry = NULL;
  uint i;
  int _IsReusable = 0;

  // First, try to reuse existing valid entry with its attached data (if in memory):
  for (i=0; i<EntriesCount; i++)
    if
    (
      pCacheEntries[i].IsValid &&
      pIsReusable (&pCacheEntries[i], pReuseInfo)
    )
    {
      pCacheEntry = &pCacheEntries[i];
      _IsReusable = 1;
      break;
    }

  // Second, try to use a free cache entry (if any):
  if (pCacheEntry == NULL)
    for (i=0; i<EntriesCount; i++)
      if (!pCacheEntries[i].IsValid)
      {
        pCacheEntry = &pCacheEntries[i];
        break;
      }

  // Third, find Not Recently Used (NRU) cache entry,
  // flush (if valid and dirty) and use it:
  if (pCacheEntry == NULL)
  {
    uint NRUidx = 0;
    for (i=0; i<EntriesCount; i++)
      if (pCacheEntries[i].AccessPattern < pCacheEntries[NRUidx].AccessPattern)
        NRUidx = i;
    pCacheEntry = &pCacheEntries[NRUidx];
    if ((pCacheEntry->IsValid) && (pCacheEntry->IsDirty))
      pFlush (pCacheEntry);
    pCacheEntry->IsDirty = 0;
    // also drop access history:
    pCacheEntry->AccessPattern = 0;
  }

  // Refill if needed:
  if (!(pCacheEntry->IsValid && _IsReusable))
    pCacheEntry->IsValid = pRefill (pCacheEntry, pReuseInfo);

  // Age cache entries and access the current entry:
  for (i=0; i<EntriesCount; i++)
    pCacheEntries[i].AccessPattern >>= 1;
  pCacheEntry->AccessPattern |= 0x80000000UL;

  return pCacheEntry;
}

void Cache_SetErrorCode (tCacheEntry* pCacheEntry, int ErrorCode)
{
  pCacheEntry->ErrorCode = ErrorCode;
}

int  Cache_GetErrorCode (tCacheEntry* pCacheEntry)
{
  return pCacheEntry->ErrorCode;
}

#if 0
#include <stdio.h>
#include <conio.h>

// Number of data items to be cached:
#define N 10
int aSlowDiskData[N];

// Number of the cach entries:
// try CN<N and CN>=N:
#define CN 4
tCacheEntry aCacheEntries[CN];
typedef struct tAttachedData {
  int Data;
  int OnDiskIndex;
} tAttachedData;
tAttachedData aCacheAttachedData[CN];

int Flush (tCacheEntry* pCacheEntry)
{
  tAttachedData* pAttachedData = (tAttachedData*)pCacheEntry->pAttachedData;

  // Don't flush a clean entry:
  if (!Cache_GetIsDirty (pCacheEntry))
    return 1;

  // Save entry to offcache storage:
  aSlowDiskData[pAttachedData->OnDiskIndex] = pAttachedData->Data;

  // Return value (flush success flag) will be ignored:
  return 1;
}

int IsReusable (tCacheEntry* pCacheEntry, void* pReuseInfo)
{
  tAttachedData* pAttachedData = (tAttachedData*)pCacheEntry->pAttachedData;

  return (pAttachedData->OnDiskIndex == *(int*)pReuseInfo);
}

int Refill (tCacheEntry* pCacheEntry, void* pReuseInfo)
{
  tAttachedData* pAttachedData = (tAttachedData*)pCacheEntry->pAttachedData;

  // Don't refill a valid reusable entry:
  if (Cache_GetIsValid (pCacheEntry) && IsReusable (pCacheEntry, pReuseInfo))
    return 1;

  // Load entry from offcache storage:
  pAttachedData->OnDiskIndex = *(int*)pReuseInfo;
  pAttachedData->Data = aSlowDiskData[pAttachedData->OnDiskIndex];

  // Return value (refill success flag) will affect Cache_GetIsValid():
  return 1;
}

int ReadData (int OnDiskIndex, int* pData)
{
  tCacheEntry* pCacheEntry;
  tAttachedData* pAttachedData;
  int res;

  // Find needed entry:
  pCacheEntry = Cache_FindEntryForUse (aCacheEntries, CN, &OnDiskIndex,
                                       &Refill, &Flush, &IsReusable);

  // Check validity of found entry:
  res = Cache_GetIsValid (pCacheEntry);

  // If valid, get and return attached data
  if (res)
  {
    pAttachedData = (tAttachedData*)pCacheEntry->pAttachedData;
    *pData = pAttachedData->Data;
  }

  return res;
}

int WriteData (int OnDiskIndex, int Data)
{
  tCacheEntry* pCacheEntry;
  tAttachedData* pAttachedData;

  // Find needed entry:
  pCacheEntry = Cache_FindEntryForUse (aCacheEntries, CN, &OnDiskIndex,
                                       &Refill, &Flush, &IsReusable);

  // Ignore validity of found entry (no Cache_GetIsValid() call)

  // Set attached data:
  pAttachedData = (tAttachedData*)pCacheEntry->pAttachedData;
  pAttachedData->Data = Data;

  // Set IsDirty flag to flush the entry when needed:
  Cache_SetIsDirty (pCacheEntry);

  return 1;
}

int main()
{
  int i;

  clrscr();

  printf ("Initializing aSlowDisk[]:\n");
  for (i=0;i<N;i++)
  {
    aSlowDiskData[i] = i;
    printf ("aSlowDisk[%d]=%d\n", i, aSlowDiskData[i]);
  }
  getch();

  for (i=0;i<CN;i++)
    Cache_InitEntry (&aCacheEntries[i], &aCacheAttachedData[i]);

  printf ("Reading aSlowDisk[]:\n");
  for (i=0;i<N;i++)
  {
    int d;
    ReadData (i, &d);
    printf ("Cached aSlowDisk[%d]=%d\n", i, d);
  }
  getch();

  printf ("Writing aSlowDisk[]:\n");
  for (i=0;i<N;i++)
  {
    int d = N-1-i;
    WriteData (i, d);
    printf ("Cached aSlowDisk[%d]:=%d\n", i, d);
  }
  printf ("Reading aSlowDisk[]:\n");
  for (i=0;i<N;i++)
    printf ("aSlowDisk[%d]=%d\n", i, aSlowDiskData[i]);
  getch();

  printf ("Reading aSlowDisk[]:\n");
  for (i=0;i<N;i++)
//  for (i=N-1;i>=0;i--)
  {
    int d;
    ReadData (i, &d);
    printf ("Cached aSlowDisk[%d]=%d\n", i, d);
  }
  for (i=0;i<N;i++)
    printf ("aSlowDisk[%d]=%d\n", i, aSlowDiskData[i]);
  getch();

  printf ("Flushing aSlowDisk[]:\n");
  for (i=0;i<CN;i++)
    Flush (&aCacheEntries[i]);

  printf ("Reading aSlowDisk[]:\n");
  for (i=0;i<N;i++)
    printf ("aSlowDisk[%d]=%d\n", i, aSlowDiskData[i]);

  return 0;
}
#endif
