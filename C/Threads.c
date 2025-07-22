/* Threads.c -- multithreading library
2017-06-26 : Igor Pavlov : Public domain */

#include "Precomp.h"

#ifndef UNDER_CE
#include <process.h>
#endif

#include "Threads.h"

static WRes GetError()
{
  DWORD res = GetLastError();
  return res ? (WRes)res : 1;
}

static WRes HandleToWRes(HANDLE h) { return (h != NULL) ? 0 : GetError(); }
static WRes BOOLToWRes(BOOL v) { return v ? 0 : GetError(); }

WRes HandlePtr_Close(HANDLE *p)
{
  if (*p != NULL)
  {
    if (!CloseHandle(*p))
      return GetError();
    *p = NULL;
  }
  return 0;
}

WRes Handle_WaitObject(HANDLE h)
{
  DWORD dw = WaitForSingleObject(h, INFINITE);
  /*
    (dw) result:
    WAIT_OBJECT_0  // 0
    WAIT_ABANDONED // 0x00000080 : is not compatible with Win32 Error space
    WAIT_TIMEOUT   // 0x00000102 : is     compatible with Win32 Error space
    WAIT_FAILED    // 0xFFFFFFFF
  */
  if (dw == WAIT_FAILED)
  {
    dw = GetLastError();
    if (dw == 0)
      return WAIT_FAILED;
  }
  return (WRes)dw;
}

#define Thread_Wait(p) Handle_WaitObject(*(p))

WRes Thread_Wait_Close(CThread *p)
{
  WRes res = Thread_Wait(p);
  WRes res2 = Thread_Close(p);
  return (res != 0 ? res : res2);
}

typedef struct MY_PROCESSOR_NUMBER {
    WORD  Group;
    BYTE  Number;
    BYTE  Reserved;
} MY_PROCESSOR_NUMBER, *MY_PPROCESSOR_NUMBER;

typedef struct MY_GROUP_AFFINITY {
#if defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION < 100000)
    // KAFFINITY is not defined in old mingw
    ULONG_PTR
#else
    KAFFINITY
#endif
      Mask;
    WORD   Group;
    WORD   Reserved[3];
} MY_GROUP_AFFINITY, *MY_PGROUP_AFFINITY;

typedef BOOL (WINAPI *Func_SetThreadGroupAffinity)(
    HANDLE hThread,
    CONST MY_GROUP_AFFINITY *GroupAffinity,
    MY_PGROUP_AFFINITY PreviousGroupAffinity);

typedef BOOL (WINAPI *Func_GetThreadGroupAffinity)(
    HANDLE hThread,
    MY_PGROUP_AFFINITY GroupAffinity);

typedef BOOL (WINAPI *Func_GetProcessGroupAffinity)(
    HANDLE hProcess,
    PUSHORT GroupCount,
    PUSHORT GroupArray);


WRes Thread_Create(CThread *p, THREAD_FUNC_TYPE func, LPVOID param)
{
  /* Windows Me/98/95: threadId parameter may not be NULL in _beginthreadex/CreateThread functions */
  
  #ifdef UNDER_CE
  
  DWORD threadId;
  *p = CreateThread(0, 0, func, param, 0, &threadId);

  #else

  unsigned threadId;
  *p = (HANDLE)_beginthreadex(NULL, 0, func, param, 0, &threadId);
   
  #endif

  /* maybe we must use errno here, but probably GetLastError() is also OK. */
  return HandleToWRes(*p);
}


WRes Thread_Create_With_Affinity(CThread *p, THREAD_FUNC_TYPE func, LPVOID param, CAffinityMask affinity)
{
  #ifdef USE_THREADS_CreateThread

  UNUSED_VAR(affinity)
  return Thread_Create(p, func, param);
  
  #else
  
  /* Windows Me/98/95: threadId parameter may not be NULL in _beginthreadex/CreateThread functions */
  HANDLE h;
  WRes wres;
  unsigned threadId;
  h = (HANDLE)(_beginthreadex(NULL, 0, func, param, CREATE_SUSPENDED, &threadId));
  *p = h;
  wres = HandleToWRes(h);
  if (h)
  {
    {
      // DWORD_PTR prevMask =
      SetThreadAffinityMask(h, (DWORD_PTR)affinity);
      /*
      if (prevMask == 0)
      {
        // affinity change is non-critical error, so we can ignore it
        // wres = GetError();
      }
      */
    }
    {
      const DWORD prevSuspendCount = ResumeThread(h);
      /* ResumeThread() returns:
         0 : was_not_suspended
         1 : was_resumed
        -1 : error
      */
      if (prevSuspendCount == (DWORD)-1)
        wres = GetError();
    }
  }

  /* maybe we must use errno here, but probably GetLastError() is also OK. */
  return wres;

  #endif
}


WRes Thread_Create_With_Group(CThread *p, THREAD_FUNC_TYPE func, LPVOID param, unsigned group, CAffinityMask affinityMask)
{
#ifdef USE_THREADS_CreateThread

  UNUSED_VAR(group)
  UNUSED_VAR(affinityMask)
  return Thread_Create(p, func, param);
  
#else
  
  /* Windows Me/98/95: threadId parameter may not be NULL in _beginthreadex/CreateThread functions */
  HANDLE h;
  WRes wres;
  unsigned threadId;
  h = (HANDLE)(_beginthreadex(NULL, 0, func, param, CREATE_SUSPENDED, &threadId));
  *p = h;
  wres = HandleToWRes(h);
  if (h)
  {
    // PrintProcess_Info();
    {
      const
         Func_SetThreadGroupAffinity fn =
        (Func_SetThreadGroupAffinity) Z7_CAST_FUNC_C GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")),
             "SetThreadGroupAffinity");
      if (fn)
      {
        // WRes wres2;
        MY_GROUP_AFFINITY groupAffinity, prev_groupAffinity;
        memset(&groupAffinity, 0, sizeof(groupAffinity));
        // groupAffinity.Mask must use only bits that supported by current group
        // (groupAffinity.Mask = 0) means all allowed bits
        groupAffinity.Mask = affinityMask;
        groupAffinity.Group = (WORD)group;
        // wres2 =
        fn(h, &groupAffinity, &prev_groupAffinity);
        /*
        if (groupAffinity.Group == prev_groupAffinity.Group)
          wres2 = wres2;
        else
          wres2 = wres2;
        if (wres2 == 0)
        {
          wres2 = GetError();
          PRF(printf("\n==SetThreadGroupAffinity error: %u\n", wres2);)
        }
        else
        {
          PRF(printf("\n==Thread_Create_With_Group::SetThreadGroupAffinity()"
            " threadId = %6u"
            " group=%u mask=%x\n",
            threadId,
            prev_groupAffinity.Group,
            (UInt32)prev_groupAffinity.Mask);)
        }
        */
      }
    }
    {
      const DWORD prevSuspendCount = ResumeThread(h);
      /* ResumeThread() returns:
         0 : was_not_suspended
         1 : was_resumed
        -1 : error
      */
      if (prevSuspendCount == (DWORD)-1)
        wres = GetError();
    }
  }

  /* maybe we must use errno here, but probably GetLastError() is also OK. */
  return wres;

  #endif
}


static WRes Event_Create(CEvent *p, BOOL manualReset, int signaled)
{
  *p = CreateEvent(NULL, manualReset, (signaled ? TRUE : FALSE), NULL);
  return HandleToWRes(*p);
}

WRes Event_Set(CEvent *p) { return BOOLToWRes(SetEvent(*p)); }
WRes Event_Reset(CEvent *p) { return BOOLToWRes(ResetEvent(*p)); }

WRes ManualResetEvent_Create(CManualResetEvent *p, int signaled) { return Event_Create(p, TRUE, signaled); }
WRes AutoResetEvent_Create(CAutoResetEvent *p, int signaled) { return Event_Create(p, FALSE, signaled); }
WRes ManualResetEvent_CreateNotSignaled(CManualResetEvent *p) { return ManualResetEvent_Create(p, 0); }
WRes AutoResetEvent_CreateNotSignaled(CAutoResetEvent *p) { return AutoResetEvent_Create(p, 0); }


WRes Semaphore_Create(CSemaphore *p, UInt32 initCount, UInt32 maxCount)
{
  *p = CreateSemaphore(NULL, (LONG)initCount, (LONG)maxCount, NULL);
  return HandleToWRes(*p);
}

static WRes Semaphore_Release(CSemaphore *p, LONG releaseCount, LONG *previousCount)
  { return BOOLToWRes(ReleaseSemaphore(*p, releaseCount, previousCount)); }
WRes Semaphore_ReleaseN(CSemaphore *p, UInt32 num)
  { return Semaphore_Release(p, (LONG)num, NULL); }
WRes Semaphore_Release1(CSemaphore *p) { return Semaphore_ReleaseN(p, 1); }

WRes CriticalSection_Init(CCriticalSection *p)
{
  /* InitializeCriticalSection can raise only STATUS_NO_MEMORY exception */
  #ifdef _MSC_VER
  __try
  #endif
  {
    InitializeCriticalSection(p);
    /* InitializeCriticalSectionAndSpinCount(p, 0); */
  }
  #ifdef _MSC_VER
  __except (EXCEPTION_EXECUTE_HANDLER) { return 1; }
  #endif
  return 0;
}

void ThreadNextGroup_Init(CThreadNextGroup *p, UInt32 numGroups, UInt32 startGroup)
{
  // printf("\n====== ThreadNextGroup_Init numGroups = %x: startGroup=%x\n", numGroups, startGroup);
  if (numGroups == 0)
      numGroups = 1;
  p->NumGroups = numGroups;
  p->NextGroup = startGroup % numGroups;
}


UInt32 ThreadNextGroup_GetNext(CThreadNextGroup *p)
{
  const UInt32 next = p->NextGroup;
  p->NextGroup = (next + 1) % p->NumGroups;
  return next;
}

#undef PRF
#undef Print
