// ntTraverseDir.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


#include <cstdlib>
#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>

#include "windows.h"

//#include "ntdef.h"
#include "WinNT.h"


//#define LONG NTSTATUS;
typedef __success(return >= 0) long NTSTATUS;

typedef enum _FILE_INFORMATION_CLASS {
	FileDirectoryInformation = 1,
	FileFullDirectoryInformation,
	FileBothDirectoryInformation,
	FileBasicInformation,
	FileStandardInformation,
	FileInternalInformation,
	FileEaInformation,
	FileAccessInformation,
	FileNameInformation,
	FileRenameInformation,
	FileLinkInformation,
	FileNamesInformation,
	FileDispositionInformation,
	FilePositionInformation,
	FileFullEaInformation,
	FileModeInformation,
	FileAlignmentInformation,
	FileAllInformation,
	FileAllocationInformation,
	FileEndOfFileInformation,
	FileAlternateNameInformation,
	FileStreamInformation,
	FilePipeInformation,
	FilePipeLocalInformation,
	FilePipeRemoteInformation,
	FileMailslotQueryInformation,
	FileMailslotSetInformation,
	FileCompressionInformation,
	FileObjectIdInformation,
	FileCompletionInformation,
	FileMoveClusterInformation,
	FileQuotaInformation,
	FileReparsePointInformation,
	FileNetworkOpenInformation,
	FileAttributeTagInformation,
	FileTrackingInformation,
	FileIdBothDirectoryInformation,
	FileIdFullDirectoryInformation,
	FileValidDataLengthInformation,
	FileShortNameInformation,
	FileIoCompletionNotificationInformation,
	FileIoStatusBlockRangeInformation,
	FileIoPriorityHintInformation,
	FileSfioReserveInformation,
	FileSfioVolumeInformation,
	FileHardLinkInformation,
	FileProcessIdsUsingFileInformation,
	FileNormalizedNameInformation,
	FileNetworkPhysicalNameInformation,
	FileIdGlobalTxDirectoryInformation,
	FileIsRemoteDeviceInformation,
	FileUnusedInformation,
	FileNumaNodeInformation,
	FileStandardLinkInformation,
	FileRemoteProtocolInformation,
	FileRenameInformationBypassAccessCheck,
	FileLinkInformationBypassAccessCheck,
	FileVolumeNameInformation,
	FileIdInformation,
	FileIdExtdDirectoryInformation,
	FileReplaceCompletionInformation,
	FileHardLinkFullIdInformation,
	FileIdExtdBothDirectoryInformation,
	FileDispositionInformationEx,
	FileRenameInformationEx,
	FileRenameInformationExBypassAccessCheck,
	FileMaximumInformation
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

typedef struct _LSA_UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} LSA_UNICODE_STRING, *PLSA_UNICODE_STRING, UNICODE_STRING, *PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
	ULONG           Length;
	HANDLE          RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG           Attributes;
	PVOID           SecurityDescriptor;
	PVOID           SecurityQualityOfService;
}  OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

//++
//
// VOID
// InitializeObjectAttributes(
//     _Out_ POBJECT_ATTRIBUTES p,
//     _In_ PUNICODE_STRING n,
//     _In_ ULONG a,
//     _In_ HANDLE r,
//     _In_ PSECURITY_DESCRIPTOR s
//     )
//
//--

#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES );          \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    (p)->SecurityDescriptor = s;                        \
    (p)->SecurityQualityOfService = NULL;               \
    }

typedef struct _IO_STATUS_BLOCK {
	union {
		NTSTATUS Status;
		PVOID    Pointer;
	};
	ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef
VOID(NTAPI *PIO_APC_ROUTINE) (
	IN PVOID ApcContext,
	IN PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG Reserved
	);

NTSTATUS (WINAPI * pNtOpenFile)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG);
NTSTATUS (WINAPI * pNtQueryDirectoryFile)(HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS, BOOLEAN, PUNICODE_STRING, BOOLEAN);

void loadNtDLL(VOID)
{
	HMODULE hModule = LoadLibraryA("Ntdll.dll");

	pNtOpenFile = (NTSTATUS(WINAPI *)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG)) GetProcAddress(hModule, "NtOpenFile");
	pNtQueryDirectoryFile = (NTSTATUS(WINAPI *) (HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS, BOOLEAN, PUNICODE_STRING, BOOLEAN)) GetProcAddress(hModule, "NtQueryDirectoryFile");

}

std::vector<std::string> listFiles(WCHAR* dir) {
	std::vector<std::string> files;

	HANDLE hFile;
	//ULONG access = GENERIC_READ | FILE_GENERIC_READ | FILE_LIST_DIRECTORY | FILE_TRAVERSE;
	ULONG access = FILE_LIST_DIRECTORY;
	IO_STATUS_BLOCK ioStatus;
	OBJECT_ATTRIBUTES attrs;
	HANDLE root;
	UNICODE_STRING name;
	NTSTATUS status;

	name.Buffer = dir;
	name.Length = wcslen(dir);
	name.MaximumLength = name.Length;
	
	InitializeObjectAttributes(&attrs, &name, NULL, NULL, NULL);

	status = pNtOpenFile(&hFile, access, &attrs, &ioStatus, FILE_SHARE_READ | FILE_SHARE_WRITE , NULL);

	if (status < 0)
		std::cout << "Unable to open file, error = 0x" << std::hex << status << '\n';

	return files;
}


int main(int argc, char *argv[])
{
	char *path = (char *) "C:/Temp";
	size_t newsize = 0;

	if (argc == 2) {
		path = argv[1];
	}

	newsize = strlen(path) + 1;
	wchar_t * wcstring = new wchar_t[newsize];
	size_t converted = 0;

	mbstowcs_s(&converted, wcstring, newsize, path, _TRUNCATE);

	WCHAR *wszDirectory = wcstring;
    
	std::cout << wszDirectory << '\n';
		
	loadNtDLL();
	std::vector<std::string> files = listFiles(wszDirectory);

	return 0;
}

