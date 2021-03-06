// ntTraverseDir.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>

#include "windows.h"
#include "WinNT.h"

typedef __success(return >= 0) long NTSTATUS;
#define OBJ_CASE_INSENSITIVE    0x00000040L

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

typedef struct _FILE_NAMES_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_NAMES_INFORMATION, *PFILE_NAMES_INFORMATION;

NTSTATUS (WINAPI * pRtlInitUnicodeString)(PUNICODE_STRING, PCWSTR);
NTSTATUS (WINAPI * pNtCreateFile)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG);
NTSTATUS (WINAPI * pNtOpenFile)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG);
NTSTATUS (WINAPI * pNtQueryDirectoryFile)(HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS, BOOLEAN, PUNICODE_STRING, BOOLEAN);


void loadNtDLL(VOID)
{
    HMODULE hModule = LoadLibraryA("Ntdll.dll");

    pRtlInitUnicodeString = (NTSTATUS(WINAPI *)(PUNICODE_STRING, PCWSTR)) GetProcAddress(hModule, "RtlInitUnicodeString");
    pNtOpenFile = (NTSTATUS(WINAPI *)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG)) GetProcAddress(hModule, "NtOpenFile");
    pNtQueryDirectoryFile = (NTSTATUS(WINAPI *) (HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS, BOOLEAN, PUNICODE_STRING, BOOLEAN)) GetProcAddress(hModule, "NtQueryDirectoryFile");
}

std::vector<std::wstring> listFiles(WCHAR* dir) {
    std::vector<std::wstring> files;

    HANDLE hFile;
    ULONG access = FILE_GENERIC_READ | FILE_LIST_DIRECTORY;
    IO_STATUS_BLOCK ioStatus;
    OBJECT_ATTRIBUTES attrs;
    UNICODE_STRING name;
    NTSTATUS status;

    // what I need is:
    // FILE_NAMES_INFORMATION fileNamesInfo;
    // what the docs specify is that FILE_NAMES_INFORMATION.FileName is a WCHAR [1]
    //    - "Specifies the first character of the file name string. This is followed in memory by the remainder of the string.
    // So my fileNamesInfo needs to be some amount larger to hold an nt file name as a result. 256k is enough to hold 1k files of MAX_PATH.
    std::array<char, 262144> *fileNamesInfo = new std::array<char, 262144>();

    status = pRtlInitUnicodeString(&name, dir);

    if (status < 0)
        std::cout << "Unable to create unicode string from wchar*\n";

    InitializeObjectAttributes(&attrs, &name, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = pNtOpenFile(&hFile, access, &attrs, &ioStatus, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL);

    if (status < 0) {
        std::cout << "Unable to open file, error = 0x" << std::hex << status << '\n';
        return files;
    }

    status = pNtQueryDirectoryFile(hFile, NULL, NULL, NULL, &ioStatus, static_cast<void*>(fileNamesInfo), fileNamesInfo->size(), FileNamesInformation, false, NULL, false);
    FILE_NAMES_INFORMATION *fiEntry = (FILE_NAMES_INFORMATION *)fileNamesInfo->data();

    if (status < 0) {
        std::cout << "Failed to query directory\n";
        return files;
    }

    FILE_NAMES_INFORMATION *last = nullptr;

    while (fiEntry != last) {

        //Only terminate the loop after processing the record that contains NextEntryOffset == 0
        last = fiEntry;

        // fiEntry->FileName is not null terminated and the length given is in bytes.
        files.push_back(std::wstring(fiEntry->FileName, fiEntry->FileNameLength / sizeof(WCHAR) ));

        // Gross pointer math. NT is the wild west. The next file info record is located by NextEntryOffset, but
        // this offset is relative to the current record pointer, not the start of the full query response.
        fiEntry = (FILE_NAMES_INFORMATION *)((char*)(fiEntry) + fiEntry->NextEntryOffset);
    }

    CloseHandle(hFile);
    return files;
}


void createFile(std::wstring root, std::wstring filename)
{

    std::wstring newfile = root + L"\\temp\\" + filename;
    DWORD access = GENERIC_READ | GENERIC_WRITE;

    HANDLE hFile = CreateFile(newfile.c_str(), access, NULL, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        std::wcerr << "Failed to create file :" << newfile << '\n';
    }

    CloseHandle(hFile);
    return;
}

int main(int argc, char *argv[])
{
    char *path = (char *) "\\??\\C:\\Temp";

    size_t newsize = 0;

    if (argc == 2) {
        path = argv[1];
    }

    newsize = strlen(path) + 1;
    wchar_t * wcstring = new wchar_t[newsize];
    size_t converted = 0;

    mbstowcs_s(&converted, wcstring, newsize, path, _TRUNCATE);

    WCHAR *wszDirectory = wcstring;
    std::wstring root { wszDirectory };

    loadNtDLL();
    std::vector<std::wstring> files = listFiles(wszDirectory);

    std::for_each(files.begin(), files.end(), [](std::wstring str) { std::wcout << str << '\n'; });

    return 0;
}

