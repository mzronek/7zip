// Windows/System.cpp

#include "StdAfx.h"

#include "../Common/MyWindows.h"

#include "../Common/Defs.h"

#include "System.h"

namespace NWindows {
namespace NSystem {

typedef DWORD (WINAPI *Func_GetActiveProcessorCount)(WORD GroupNumber);
typedef WORD (WINAPI *Func_GetActiveProcessorGroupCount)(VOID);

bool CCpuGroups::Load()
{
  NumThreadsTotal = 0;
  GroupSizes.Clear();
  const HMODULE hmodule = ::GetModuleHandleA("kernel32.dll");
  // Is_Win11_Groups = GetProcAddress(hmodule, "SetThreadSelectedCpuSetMasks") != NULL;
  const
      Func_GetActiveProcessorGroupCount
        fn_GetActiveProcessorGroupCount = Z7_GET_PROC_ADDRESS(
      Func_GetActiveProcessorGroupCount, hmodule,
          "GetActiveProcessorGroupCount");
  const
      Func_GetActiveProcessorCount
        fn_GetActiveProcessorCount = Z7_GET_PROC_ADDRESS(
      Func_GetActiveProcessorCount, hmodule,
          "GetActiveProcessorCount");
  if (!fn_GetActiveProcessorGroupCount ||
      !fn_GetActiveProcessorCount)
    return false;

  const unsigned numGroups = fn_GetActiveProcessorGroupCount();
  if (numGroups == 0)
    return false;
  UInt32 sum = 0;
  for (unsigned i = 0; i < numGroups; i++)
  {
    const UInt32 num = fn_GetActiveProcessorCount((WORD)i);
    /*
    if (num == 0)
    {
      // it means error
      // but is it possible that some group is empty by some reason?
      // GroupSizes.Clear();
      // return false;
    }
    */
    sum += num;
    GroupSizes.Add(num);
  }
  NumThreadsTotal = sum;
  // NumThreadsTotal = fn_GetActiveProcessorCount(MY_ALL_PROCESSOR_GROUPS);
  return true;
}

UInt32 CountAffinity(DWORD_PTR mask)
{
  UInt32 num = 0;
  for (unsigned i = 0; i < sizeof(mask) * 8; i++)
  {
    num += (UInt32)(mask & 1);
    mask >>= 1;
  }
  return num;
}

#ifdef _WIN32

BOOL CProcessAffinity::Get()
{
  IsGroupMode = false;
  Groups.Load();
  // SetThreadAffinityMask(GetCurrentThread(), 1);
  // SetProcessAffinityMask(GetCurrentProcess(), 1);
  BOOL res = GetProcessAffinityMask(GetCurrentProcess(),
      &processAffinityMask, &systemAffinityMask);
  /* DOCs: On a system with more than 64 processors, if the threads
     of the calling process  are in a single processor group, the
     function sets the variables pointed to by lpProcessAffinityMask
     and lpSystemAffinityMask to the process affinity mask and the
     processor mask of active logical processors for that group.
     If the calling process contains threads in multiple groups,
     the function returns zero for both affinity masks

     note: tested in Win10: GetProcessAffinityMask() doesn't return 0
           in (processAffinityMask) and (systemAffinityMask) masks.
     We need to test it in Win11: how to get mask==0 from GetProcessAffinityMask()?
  */
  if (!res)
  {
    processAffinityMask = 0;
    systemAffinityMask = 0;
  }
  if (Groups.GroupSizes.Size() > 1 && Groups.NumThreadsTotal)
    if (// !res ||
        processAffinityMask == 0 || // to support case described in DOCs and for (!res) case
        processAffinityMask == systemAffinityMask) // for default nonchanged affinity
    {
      // we set IsGroupMode only if processAffinity is default (not changed).
      res = TRUE;
      IsGroupMode = true;
    }
  return res;
}


UInt32 CProcessAffinity::Load_and_GetNumberOfThreads()
{
  if (Get())
  {
    const UInt32 numProcessors = GetNumProcessThreads();
    if (numProcessors)
      return numProcessors;
  }
  SYSTEM_INFO systemInfo;
  GetSystemInfo(&systemInfo);
  // the number of logical processors in the current group
  return systemInfo.dwNumberOfProcessors;
}

UInt32 GetNumberOfProcessors()
{
  // We need to know how many threads we can use.
  // By default the process is assigned to one group.
  CProcessAffinity pa;
  return pa.Load_and_GetNumberOfThreads();
}


#else

UInt32 GetNumberOfProcessors()
{
  return 1;
}

#endif


#ifdef _WIN32

#ifndef UNDER_CE

#if !defined(_WIN64) && defined(__GNUC__)

typedef struct _MY_MEMORYSTATUSEX {
  DWORD dwLength;
  DWORD dwMemoryLoad;
  DWORDLONG ullTotalPhys;
  DWORDLONG ullAvailPhys;
  DWORDLONG ullTotalPageFile;
  DWORDLONG ullAvailPageFile;
  DWORDLONG ullTotalVirtual;
  DWORDLONG ullAvailVirtual;
  DWORDLONG ullAvailExtendedVirtual;
} MY_MEMORYSTATUSEX, *MY_LPMEMORYSTATUSEX;

#else

#define MY_MEMORYSTATUSEX MEMORYSTATUSEX
#define MY_LPMEMORYSTATUSEX LPMEMORYSTATUSEX

#endif

typedef BOOL (WINAPI *GlobalMemoryStatusExP)(MY_LPMEMORYSTATUSEX lpBuffer);

#endif

#endif


bool GetRamSize(UInt64 &size)
{
  size = (UInt64)(sizeof(size_t)) << 29;

  #ifdef _WIN32
  
  #ifndef UNDER_CE
    MY_MEMORYSTATUSEX stat;
    stat.dwLength = sizeof(stat);
  #endif
  
  #ifdef _WIN64
    
    if (!::GlobalMemoryStatusEx(&stat))
      return false;
    size = MyMin(stat.ullTotalVirtual, stat.ullTotalPhys);
    return true;

  #else
    
    #ifndef UNDER_CE
      GlobalMemoryStatusExP globalMemoryStatusEx = (GlobalMemoryStatusExP)
          ::GetProcAddress(::GetModuleHandle(TEXT("kernel32.dll")), "GlobalMemoryStatusEx");
      if (globalMemoryStatusEx && globalMemoryStatusEx(&stat))
      {
        size = MyMin(stat.ullTotalVirtual, stat.ullTotalPhys);
        return true;
      }
    #endif
  
    {
      MEMORYSTATUS stat2;
      stat2.dwLength = sizeof(stat2);
      ::GlobalMemoryStatus(&stat2);
      size = MyMin(stat2.dwTotalVirtual, stat2.dwTotalPhys);
      return true;
    }
  
  #endif

  #else

  return false;

  #endif
}

}}
