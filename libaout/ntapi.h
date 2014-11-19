/*
 * This is essentially cobbled together from the parts of the ReactOS NT NDK that I actually needed for this,
 * which is substantially less header code than what is actually present in the entire NDK.
 * (The version of ntapi.h that I have that contains the entire NDK is 173K, this is ~22K
 *  and it's much better commented than that one, too).
 * Much thanks to many people including Mike McCormack and the entire ReactOS team for figuring
 * a lot of this stuff out - it saved me a lot of time.
 */

#ifndef __NTAPI_H__
#define __NTAPI_H__

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winnt.h>
#include <winioctl.h>
#include <ntstatus.h>
#define NtCurrentProcess() ((HANDLE)-1)
#define NtCurrentThread() ((HANDLE)-2)
#define PAGE_SIZE 0x1000
#define FILE_SHARE_ALL FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE

//
// Definitions for Object Creation
//
#define OBJ_INHERIT                             0x00000002L
#define OBJ_PERMANENT                           0x00000010L
#define OBJ_EXCLUSIVE                           0x00000020L
#define OBJ_CASE_INSENSITIVE                    0x00000040L
#define OBJ_OPENIF                              0x00000080L
#define OBJ_OPENLINK                            0x00000100L
#define OBJ_KERNEL_HANDLE                       0x00000200L
#define OBJ_FORCE_ACCESS_CHECK                  0x00000400L
#define OBJ_VALID_ATTRIBUTES                    0x000007F2L

//
// Basic NT Types
//
typedef long NTSTATUS, *PNTSTATUS;

typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    WCHAR *Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _STRING
{
    USHORT Length;
    USHORT MaximumLength;
    CHAR *Buffer;
} STRING;
typedef STRING ANSI_STRING;

//
// Constant String Macro
//
#define RTL_CONSTANT_STRING(__SOURCE_STRING__)                  \
{                                                               \
    sizeof(__SOURCE_STRING__) - sizeof((__SOURCE_STRING__)[0]), \
    sizeof(__SOURCE_STRING__),                                  \
    (__SOURCE_STRING__)                                         \
}

typedef struct _OBJECT_ATTRIBUTES
{
    ULONG Length;
    HANDLE RootDirectory;
    UNICODE_STRING *ObjectName;
    ULONG Attributes;
    VOID *SecurityDescriptor;
    VOID *SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef struct _IO_STATUS_BLOCK {
    union {
        NTSTATUS Status;
        VOID *Pointer;
    };
    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef enum _CREATE_DISPOSITION {
    FILE_SUPERSEDE                  =0x00000000,
    FILE_OPEN                       =0x00000001,
    FILE_CREATE                     =0x00000002,
    FILE_OPEN_IF                    =0x00000003,
    FILE_OVERWRITE                  =0x00000004,
    FILE_OVERWRITE_IF               =0x00000005,
    FILE_MAXIMUM_DISPOSITION        =0x00000005
} CREATE_DISPOSITION;

// Event Types
typedef enum _EVENT_TYPE
{
    NotificationEvent,
    SynchronizationEvent
} EVENT_TYPE;

typedef enum _MEMORY_INFORMATION_CLASS {
    MemoryBasicInformation
} MEMORY_INFORMATION_CLASS, *PMEMORY_INFORMATION_CLASS;

typedef enum _SECTION_INHERIT {
    ViewShare=1,
    ViewUnmap=2
} SECTION_INHERIT, *PSECTION_INHERIT;

typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation = 0,
    SystemCpuInformation = 1,
    SystemPerformanceInformation = 2,
    SystemTimeOfDayInformation = 3,
    SystemProcessInformation = 5,
    SystemProcessorPerformanceInformation = 8,
    SystemModuleInformation = 11,
    SystemHandleInformation = 16,
    SystemPageFileInformation = 18,
    SystemCacheInformation = 21,
    SystemInterruptInformation = 23,
    SystemDpcBehaviourInformation = 24,
    SystemFullMemoryInformation = 25,
    SystemNotImplemented6 = 25,
    SystemLoadImage = 26,
    SystemUnloadImage = 27,
    SystemTimeAdjustmentInformation = 28,
    SystemTimeAdjustment = 28,
    SystemSummaryMemoryInformation = 29,
    SystemNotImplemented7 = 29,
    SystemNextEventIdInformation = 30,
    SystemNotImplemented8 = 30,
    SystemEventIdsInformation = 31,
    SystemCrashDumpInformation = 32,
    SystemExceptionInformation = 33,
    SystemCrashDumpStateInformation = 34,
    SystemKernelDebuggerInformation = 35,
    SystemContextSwitchInformation = 36,
    SystemRegistryQuotaInformation = 37,
    SystemCurrentTimeZoneInformation = 44,
    SystemTimeZoneInformation = 44,
    SystemLookasideInformation = 45,
    SystemSetTimeSlipEvent = 46,
    SystemCreateSession = 47,
    SystemDeleteSession = 48,
    SystemInvalidInfoClass4 = 49,
    SystemRangeStartInformation = 50,
    SystemVerifierInformation = 51,
    SystemAddVerifier = 52,
    SystemSessionProcessesInformation   = 53,
    SystemInformationClassMax
} SYSTEM_INFORMATION_CLASS, *PSYSTEM_INFORMATION_CLASS;

typedef enum _PROCESSINFOCLASS {
    ProcessBasicInformation = 0,
    ProcessQuotaLimits = 1,
    ProcessIoCounters = 2,
    ProcessVmCounters = 3,
    ProcessTimes = 4,
    ProcessBasePriority = 5,
    ProcessRaisePriority = 6,
    ProcessDebugPort = 7,
    ProcessExceptionPort = 8,
    ProcessAccessToken = 9,
    ProcessLdtInformation = 10,
    ProcessLdtSize = 11,
    ProcessDefaultHardErrorMode = 12,
    ProcessIoPortHandlers = 13,
    ProcessPooledUsageAndLimits = 14,
    ProcessWorkingSetWatch = 15,
    ProcessUserModeIOPL = 16,
    ProcessEnableAlignmentFaultFixup = 17,
    ProcessPriorityClass = 18,
    ProcessWx86Information = 19,
    ProcessHandleCount = 20,
    ProcessAffinityMask = 21,
    ProcessPriorityBoost = 22,
    ProcessDeviceMap = 23,
    ProcessSessionInformation = 24,
    ProcessForegroundInformation = 25,
    ProcessWow64Information = 26,
    ProcessImageFileName = 27,
    ProcessLUIDDeviceMapsEnabled = 28,
    ProcessBreakOnTermination = 29,
    ProcessDebugObjectHandle = 30,
    ProcessDebugFlags = 31,
    ProcessHandleTracing = 32,
    MaxProcessInfoClass
} PROCESSINFOCLASS, PROCESS_INFORMATION_CLASS;

typedef enum _THREADINFOCLASS {
    ThreadBasicInformation,
    ThreadTimes,
    ThreadPriority,
    ThreadBasePriority,
    ThreadAffinityMask,
    ThreadImpersonationToken,
    ThreadDescriptorTableEntry,
    ThreadEnableAlignmentFaultFixup,
    ThreadEventPair_Reusable,
    ThreadQuerySetWin32StartAddress,
    ThreadZeroTlsCell,
    ThreadPerformanceCount,
    ThreadAmILastThread,
    ThreadIdealProcessor,
    ThreadPriorityBoost,
    ThreadSetTlsArrayAddress,
    ThreadIsIoPending,
    MaxThreadInfoClass
} THREADINFOCLASS;

typedef enum _SECTION_INFORMATION_CLASS {
    SectionBasicInformation,
    SectionImageInformation,
} SECTION_INFORMATION_CLASS;

typedef struct _SYSTEM_BASIC_INFORMATION {
    DWORD dwUnknown1;
    ULONG TimerResolution;
    ULONG PageSize;
    ULONG MmNumberOfPhysicalPages;
    ULONG MmLowestPhysicalPage;
    ULONG MmHighestPhysicalPage;
    ULONG AllocationGranularity;
    PVOID pLowestUserAddress;
    PVOID pMmHighestUserAddress;
    ULONG KeActiveProcessors;
    ULONG KeNumberProcessors;
} SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;

typedef struct _SYSTEM_CPU_INFORMATION {
    WORD Architecture;
    WORD Level;
    WORD Revision;
    WORD Reserved;
    DWORD FeatureSet;
} SYSTEM_CPU_INFORMATION, *PSYSTEM_CPU_INFORMATION;

typedef struct _CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _PEB_LDR_DATA {
    BYTE Reserved1[8];
    PVOID Reserved2[3];
    LIST_ENTRY InMemoryOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _FILE_STANDARD_INFORMATION {
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG NumberOfLinks;
    BOOLEAN DeletePending;
    BOOLEAN Directory;
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

typedef struct RTL_DRIVE_LETTER_CURDIR {
    USHORT Flags;
    USHORT Length;
    ULONG TimeStamp;
    UNICODE_STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

typedef struct _CURDIR {
    UNICODE_STRING DosPath;
    PVOID Handle;
} CURDIR, *PCURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS {
    ULONG AllocationSize;
    ULONG Size;
    ULONG Flags;
    ULONG DebugFlags;
    HANDLE ConsoleHandle;
    ULONG ConsoleFlags;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
    CURDIR CurrentDirectory;
    UNICODE_STRING DllPath;
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
    PWSTR Environment;
    ULONG dwX;
    ULONG dwY;
    ULONG dwXSize;
    ULONG dwYSize;
    ULONG dwXCountChars;
    ULONG dwYCountChars;
    ULONG dwFillAttribute;
    ULONG dwFlags;
    ULONG wShowWindow;
    UNICODE_STRING WindowTitle;
    UNICODE_STRING Desktop;
    UNICODE_STRING ShellInfo;
    UNICODE_STRING RuntimeInfo;
    RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20];
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef struct _PEB {
    BYTE Reserved1[2];
    BYTE BeingDebugged;
    BYTE Reserved2[1];
    PVOID Reserved3[2];
    PPEB_LDR_DATA Ldr;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    VOID *SubSystemData;
    VOID *ProcessHeap;
    BYTE Reserved4[88];
    PVOID Reserved5[53];
    BYTE Reserved6[128];
    PVOID Reserved7[1];
    ULONG SessionId;
} PEB, *PPEB;

typedef struct _THREAD_BASIC_INFORMATION {
    NTSTATUS  ExitStatus;
    PVOID     TebBaseAddress;
    CLIENT_ID ClientId;
    ULONG     AffinityMask;
    LONG      Priority;
    LONG      BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

typedef struct _PROCESS_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    struct _PEB *PebBaseAddress;
    ULONG_PTR AffinityMask;
    ULONG BasePriority;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION;

typedef struct _PROCESS_PRIORITY_CLASS
{
    BOOLEAN Foreground;
    UCHAR PriorityClass;
} PROCESS_PRIORITY_CLASS;
#define PROCESS_PRIORITY_CLASS_UNKNOWN 0
#define PROCESS_PRIORITY_CLASS_IDLE 1
#define PROCESS_PRIORITY_CLASS_NORMAL 2
#define PROCESS_PRIORITY_CLASS_HIGH 3
#define PROCESS_PRIORITY_CLASS_REALTIME 4
#define PROCESS_PRIORITY_CLASS_BELOW_NORMAL 5
#define PROCESS_PRIORITY_CLASS_ABOVE_NORMAL 6

typedef struct _SECTION_IMAGE_INFORMATION
{
    PVOID EntryPoint;
    ULONG StackZeroBits;
    UCHAR _PADDING0_[0x4];
    ULONG64 StackReserved;
    ULONG64 StackCommit;
    ULONG ImageSubsystem;
    union {
          struct
          {
               WORD SubSystemMinorVersion;
               WORD SubSystemMajorVersion;
          };
          ULONG SubSystemVersion;
    };
    ULONG Unknown1;
    USHORT ImageCharacteristics;
    USHORT DllChracteristics;
    USHORT Machine;
    UCHAR ImageContainsCode;
    union
    {
        struct
        {
            UCHAR ComPlusNativeReady:1;
            UCHAR ComPlusILOnly:1;
            UCHAR ImageDynamicallyRelocated:1;
            UCHAR ImageMappedFlat:1;
            UCHAR Reserved:4;
        };
        UCHAR ImageFlags;
    };
    ULONG LoaderFlags;
    ULONG ImageFileSize;
    ULONG CheckSum;
} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION;

typedef LONG KPRIORITY;

typedef struct _VM_COUNTERS {
    SIZE_T        PeakVirtualSize;
    SIZE_T        VirtualSize;
    ULONG         PageFaultCount;
    SIZE_T        PeakWorkingSetSize;
    SIZE_T        WorkingSetSize;
    SIZE_T        QuotaPeakPagedPoolUsage;
    SIZE_T        QuotaPagedPoolUsage;
    SIZE_T        QuotaPeakNonPagedPoolUsage;
    SIZE_T        QuotaNonPagedPoolUsage;
    SIZE_T        PagefileUsage;
    SIZE_T        PeakPagefileUsage;
} VM_COUNTERS;

typedef struct _SYSTEM_THREAD_INFORMATION {
    LARGE_INTEGER   KernelTime;
    LARGE_INTEGER   UserTime;
    LARGE_INTEGER   CreateTime;
    ULONG           WaitTime;
    PVOID           StartAddress;
    CLIENT_ID       ClientId;
    KPRIORITY       Priority;
    KPRIORITY       BasePriority;
    ULONG           ContextSwitchCount;
    LONG            State;
    LONG            WaitReason;
} SYSTEM_THREAD_INFORMATION, * PSYSTEM_THREAD_INFORMATION;

typedef struct _SYSTEM_PROCESS_INFORMATION {
    ULONG           NextEntryDelta;
    ULONG           ThreadCount;
    ULONG           Reserved1[6];
    LARGE_INTEGER   CreateTime;
    LARGE_INTEGER   UserTime;
    LARGE_INTEGER   KernelTime;
    UNICODE_STRING  ProcessName;
    KPRIORITY       BasePriority;
    ULONG           ProcessId;
    ULONG           InheritedFromProcessId;
    ULONG           HandleCount;
    ULONG           Reserved2[2];
    VM_COUNTERS     VmCounters;
#if _WIN32_WINNT >= 0x500
    IO_COUNTERS     IoCounters;
#endif
    SYSTEM_THREAD_INFORMATION Threads[1];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

typedef struct _INITIAL_TEB {
    VOID *StackBase;
    VOID *StackLimit;
    VOID *StackCommit;
    VOID *StackCommitMax;
    VOID *StackReserved;
} INITIAL_TEB, *PINITIAL_TEB;

typedef struct _LPC_MESSAGE {
    USHORT DataSize;
    USHORT MessageSize;
    USHORT MessageType;
    USHORT VirtualRangesOffset;
    CLIENT_ID ClientId;
    ULONG MessageId;
    ULONG SectionSize;
    UCHAR Data[ANYSIZE_ARRAY];
} LPC_MESSAGE, *PLPC_MESSAGE;

typedef struct __TEB {
  NT_TIB                  Tib;
  PVOID                   EnvironmentPointer;
  CLIENT_ID               Cid;
  PVOID                   ActiveRpcInfo;
  PVOID                   ThreadLocalStoragePointer;
  PPEB                    ProcessEnvironmentBlock;
  BYTE Reserved1[1848];
  PVOID Reserved2[412];
  PVOID TlsSlots[64];
  BYTE Reserved3[8];
  PVOID Reserved4[26];
  PVOID ReservedForOle;
  PVOID Reserved5[4];
  PVOID TlsExpansionSlots;
} _TEB;
#define NtCurrentPeb() (((_TEB*)NtCurrentTeb())->ProcessEnvironmentBlock)

static inline void RtlGetCommandLine(UNICODE_STRING *wcmdln)
{
    UNICODE_STRING *cmdline = &(NtCurrentPeb()->ProcessParameters->CommandLine);
    wcmdln->Buffer = cmdline->Buffer;
    wcmdln->Buffer[cmdline->Length] = L'\0';
    wcmdln->Length = cmdline->Length;
    wcmdln->MaximumLength = cmdline->MaximumLength;
}

static inline HANDLE RtlGetConsoleHandle(void)
{
    return (NtCurrentPeb()->ProcessParameters->hStdError);
}

static inline void RtlGetCwdByName(UNICODE_STRING *cwd)
{
    UNICODE_STRING *_cwd = &(NtCurrentPeb()->ProcessParameters->CurrentDirectory.DosPath);
    cwd->Buffer = _cwd->Buffer;
    cwd->Buffer[_cwd->Length] = L'\0';
    cwd->Length = _cwd->Length;
    cwd->MaximumLength = _cwd->MaximumLength;
}

static inline HANDLE RtlGetCwdByHandle(void)
{
    return (NtCurrentPeb()->ProcessParameters->CurrentDirectory.Handle);
}

typedef void (NTAPI *PIO_APC_ROUTINE)(PVOID,PIO_STATUS_BLOCK,ULONG);

NTSTATUS NTAPI NtAdjustPrivilegesToken(HANDLE TokenHandle, BOOLEAN DisableAllPrivileges,
                                       TOKEN_PRIVILEGES *NewState, ULONG BufferLength,
                                       TOKEN_PRIVILEGES *PreviousState, ULONG *ReturnLength);
NTSTATUS NTAPI NtAllocateVirtualMemory(HANDLE ProcessHandle, VOID **BaseAddress, ULONG ZeroBits, SIZE_T *Size, ULONG Flags, ULONG Prot);
NTSTATUS NTAPI NtClose(HANDLE Handle);
NTSTATUS NTAPI NtCreateEvent(HANDLE *EventHandle, ACCESS_MASK Access, OBJECT_ATTRIBUTES *oa, EVENT_TYPE EventType, BOOLEAN InitialState);
NTSTATUS NTAPI NtCreateFile(HANDLE *FileHandle, ACCESS_MASK Access, OBJECT_ATTRIBUTES *oa, IO_STATUS_BLOCK *iosb,
                            LARGE_INTEGER *AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition,
                            ULONG CreateOptions, VOID *EaBuffer, ULONG EaLength);
NTSTATUS NTAPI NtCreatePort(PHANDLE,POBJECT_ATTRIBUTES,ULONG,ULONG,PULONG);
NTSTATUS NTAPI NtCreateProcess(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,HANDLE,BOOLEAN,HANDLE,HANDLE,HANDLE);
NTSTATUS NTAPI NtCreateSection(HANDLE *SectionHandle, ACCESS_MASK Access, OBJECT_ATTRIBUTES *oa, LARGE_INTEGER *MaximumSize, ULONG PageAttributes,
                               ULONG SectionAttributes, HANDLE FileHandle);
NTSTATUS NTAPI NtCreateThread(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,HANDLE,PCLIENT_ID,PCONTEXT,PINITIAL_TEB,BOOLEAN);
NTSTATUS NTAPI NtDelayExecution(BOOLEAN Alertable, LARGE_INTEGER *Interval);
NTSTATUS NTAPI NtDeviceIoControlFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext,
                                     IO_STATUS_BLOCK *IoStatusBlock, ULONG IoControlCode, VOID *InputBuffer,
                                     ULONG InputBufferLength, VOID *OutputBuffer, ULONG OutputBufferLength);
NTSTATUS NTAPI NtDisplayString(UNICODE_STRING *OutputString);
NTSTATUS NTAPI NtFreeVirtualMemory(HANDLE ProcessHandle, VOID **BaseAddress, ULONG *RegionSize, ULONG FreeType);
NTSTATUS NTAPI NtGetContextThread(HANDLE ThreadHandle, CONTEXT *ThreadContext);
NTSTATUS NTAPI NtMapViewOfSection(HANDLE SectionHandle, HANDLE ProcessHandle, VOID **BaseAddress, ULONG ZeroBits, SIZE_T CommitSize, LARGE_INTEGER *SectionOffset,
                                  SIZE_T *ViewSize, SECTION_INHERIT InheritDisposition, ULONG AllocationType, ULONG Prot);
NTSTATUS NTAPI NtOpenFile(HANDLE *FileHandle, ACCESS_MASK Access, OBJECT_ATTRIBUTES *oa, IO_STATUS_BLOCK *iosb, ULONG ShareAccess, ULONG OpenOptions);
NTSTATUS NTAPI NtOpenProcessToken(HANDLE ProcessHandle, ACCESS_MASK DesiredAccess, HANDLE *TokenHandle);
NTSTATUS NTAPI NtOpenProcessTokenEx(HANDLE ProcessHandle, ACCESS_MASK DesiredAccess,
                                    ULONG HandleAttributes, HANDLE *TokenHandle);
NTSTATUS NTAPI NtProtectVirtualMemory(HANDLE,PVOID*,SIZE_T*,ULONG,ULONG*);
NTSTATUS NTAPI NtQueryInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length,
                                      ULONG FileInformationClass);
NTSTATUS NTAPI NtQueryInformationProcess(HANDLE ProcessHandle, ULONG ProcessInformationClass,
                                         VOID *ProcessInfoBuffer, ULONG ProcessInfoBufferSize,
                                         ULONG *BufSizeRet);
NTSTATUS NTAPI NtQueryInformationThread(HANDLE ThreadHandle, THREADINFOCLASS ThreadInformationClass,
                                        VOID *ThreadInfoBuffer, ULONG ThreadInfoBufferSize,
                                        ULONG *BufSizeRet);
NTSTATUS NTAPI NtQueryPerformanceCounter(LARGE_INTEGER *PerformanceCounter, LARGE_INTEGER *PerformanceFrequency);
NTSTATUS NTAPI NtQuerySection(HANDLE,SECTION_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS NTAPI NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS NTAPI NtQueryVirtualMemory(HANDLE,PVOID,MEMORY_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS NTAPI NtReadVirtualMemory(HANDLE,LPCVOID,LPVOID,SIZE_T,SIZE_T*);
NTSTATUS NTAPI NtReplyWaitReceivePort(HANDLE,PULONG,PLPC_MESSAGE,PLPC_MESSAGE);
NTSTATUS NTAPI NtResetEvent(HANDLE,PULONG);
NTSTATUS NTAPI NtResumeThread(HANDLE,PULONG);
NTSTATUS NTAPI NtSetEvent(HANDLE,PULONG);
NTSTATUS NTAPI NtTerminateProcess(HANDLE ProcessHandle, NTSTATUS ret);
NTSTATUS NTAPI NtTerminateThread(HANDLE ThreadHandle, NTSTATUS ret);
NTSTATUS NTAPI NtSetInformationProcess(HANDLE ProcessHandle, ULONG ProcessInformationClass,
                                       VOID *ProcessInformation, ULONG ProcessInformationLength);
NTSTATUS NTAPI NtWaitForSingleObject(HANDLE EventHandle, BOOLEAN Alertable, LARGE_INTEGER *Timeout);
NTSTATUS NTAPI NtWriteFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext,
                           IO_STATUS_BLOCK *iosb, VOID *Buffer, ULONG Length, LARGE_INTEGER *ByteOffset, ULONG *Key);
NTSTATUS NTAPI NtWriteVirtualMemory(HANDLE,void*,const void*,SIZE_T,SIZE_T*);
VOID NTAPI RtlInitUnicodeString(UNICODE_STRING *DestinationString, PCWSTR SourceString);
VOID NTAPI RtlFreeUnicodeString(UNICODE_STRING *UnicodeString);
BOOLEAN NTAPI RtlDosPathNameToNtPathName_U(PCWSTR DosPathName, UNICODE_STRING *NtPathName,
                                           PCWSTR *NtFileNamePart, CURDIR *DirectoryInfo);

/* SystemTime [out]: A pointer to a LARGE_INTEGER structure 
                     that receives the system time. 
   This value means number of 100-nanosecond units since 1600, 1 January. 
   Time is incremented 10.000.000 times per second.,i.e 1s=10.000.000
*/
NTSTATUS NTAPI NtQuerySystemTime(LARGE_INTEGER *SystemTime);

/* NtSetSystemTime() is similar to NtQuerySystemTime()
   STATUS_SUCCESS is returned if the service is successfully executed.
   STATUS_PRIVILEGE_NOT_HELD is returned if the caller does not have the privilege
     to set the system time.
   STATUS_ACCESS_VIOLATION is returned if the input parameter for the system time 
     cannot be read or the output parameter for the system time cannot be written.
   STATUS_INVALID_PARAMETER is returned if the input system time is negative.
 
   SeSystemtimePrivilege is required to set the system time!!!
*/
NTSTATUS NTAPI NtSetSystemTime(LARGE_INTEGER *SystemTime,
                               LARGE_INTEGER *PreviousTime);

typedef struct _RTL_PROCESS_MODULE_INFORMATION
{
    ULONG Section;
    PVOID MappedBase;
    PVOID ImageBase;
    ULONG ImageSize;
    ULONG Flags;
    USHORT LoadOrderIndex;
    USHORT InitOrderIndex;
    USHORT LoadCount;
    USHORT OffsetToFileName;
    CHAR FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES
{
    ULONG NumberOfModules;
    RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES;

typedef struct _RTL_PROCESS_MODULE_INFORMATION_EX
{
    ULONG NextOffset;
    RTL_PROCESS_MODULE_INFORMATION BaseInfo;
    ULONG ImageCheckSum;
    ULONG TimeDateStamp;
    PVOID DefaultBase;
} RTL_PROCESS_MODULE_INFORMATION_EX, *PRTL_PROCESS_MODULE_INFORMATION_EX;

VOID NTAPI RtlInitAnsiString(ANSI_STRING *DestinationString, const char *SourceString);
VOID NTAPI RtlFreeAnsiString(ANSI_STRING *AnsiString);
NTSTATUS NTAPI LdrGetProcedureAddress(PVOID BaseAddress, ANSI_STRING *Name, ULONG Ordinal, PVOID *ProcedureAddress);
NTSTATUS NTAPI LdrLoadDll(PWSTR SearchPath, PULONG LoadFlags, PUNICODE_STRING Name, PVOID *BaseAddress);
NTSTATUS NTAPI LdrQueryProcessModuleInformation(PRTL_PROCESS_MODULES ModuleInformation, ULONG Size, PULONG ReturnedSize);
NTSTATUS NTAPI LdrUnloadDll(PVOID BaseAddress);

//
// Heap Functions
//
// Custom Heap Commit Routine for RtlCreateHeap
typedef NTSTATUS
(NTAPI * PRTL_HEAP_COMMIT_ROUTINE)(VOID *Base, VOID **CommitAddress, SIZE_T *CommitSize);

// Parameters for RtlCreateHeap
typedef struct _RTL_HEAP_PARAMETERS
{
    ULONG Length;
    SIZE_T SegmentReserve;
    SIZE_T SegmentCommit;
    SIZE_T DeCommitFreeBlockThreshold;
    SIZE_T DeCommitTotalFreeThreshold;
    SIZE_T MaximumAllocationSize;
    SIZE_T VirtualMemoryThreshold;
    SIZE_T InitialCommit;
    SIZE_T InitialReserve;
    PRTL_HEAP_COMMIT_ROUTINE CommitRoutine;
    SIZE_T Reserved[2];
} RTL_HEAP_PARAMETERS, *PRTL_HEAP_PARAMETERS;
#define RtlGetProcessHeap() (NtCurrentPeb()->ProcessHeap)

PVOID NTAPI
RtlAllocateHeap(VOID *HeapHandle, ULONG Flags, SIZE_T Size);

PVOID NTAPI
RtlCreateHeap(ULONG Flags, VOID *BaseAddress, SIZE_T SizeToReserve,
              SIZE_T SizeToCommit, VOID *Lock,
              RTL_HEAP_PARAMETERS *Parameters);

HANDLE NTAPI
RtlDestroyHeap(HANDLE Heap);

ULONG NTAPI
RtlExtendHeap(HANDLE Heap, ULONG Flags, VOID *P, SIZE_T Size);

BOOLEAN NTAPI
RtlFreeHeap(HANDLE HeapHandle, ULONG Flags, VOID *P);

PVOID NTAPI
RtlReAllocateHeap(HANDLE Heap, ULONG Flags, VOID *Ptr, SIZE_T Size);

#endif
