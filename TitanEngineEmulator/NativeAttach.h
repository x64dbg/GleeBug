#pragma once

static DWORD BaseSetLastNTError(IN NTSTATUS Status)
{
    DWORD dwErrCode;
    dwErrCode = RtlNtStatusToDosError(Status);
    SetLastError(dwErrCode);
    return dwErrCode;
}

static HANDLE WINAPI ProcessIdToHandle(IN DWORD dwProcessId)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    CLIENT_ID ClientId;

    /* If we don't have a PID, look it up */
    //if (dwProcessId == MAXDWORD) dwProcessId = (DWORD_PTR)CsrGetProcessId();

    /* Open a handle to the process */
    ClientId.UniqueThread = NULL;
    ClientId.UniqueProcess = UlongToHandle(dwProcessId);
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
    Status = NtOpenProcess(&Handle,
        PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE | PROCESS_VM_READ |
        PROCESS_SUSPEND_RESUME | PROCESS_QUERY_INFORMATION,
        &ObjectAttributes,
        &ClientId);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        BaseSetLastNTError(Status);
        return 0;
    }

    /* Return the handle */
    return Handle;
}

#define THREAD_CREATE_FLAGS_CREATE_SUSPENDED 0x00000001
#define THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH 0x00000002
#define THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER 0x00000004
#define THREAD_CREATE_FLAGS_HAS_SECURITY_DESCRIPTOR 0x00000010
#define THREAD_CREATE_FLAGS_ACCESS_CHECK_IN_TARGET 0x00000020
#define THREAD_CREATE_FLAGS_INITIAL_THREAD 0x00000080

static NTSTATUS CreateThreadSkipAttach(IN HANDLE ProcessHandle, IN PUSER_THREAD_START_ROUTINE StartRoutine, IN PVOID Argument)
{
    NTSTATUS Status;
    HANDLE hThread;

    typedef NTSTATUS(NTAPI *t_NtCreateThreadEx)(
        PHANDLE /* ThreadHandle */,
        ACCESS_MASK /* DesiredAccess */,
        POBJECT_ATTRIBUTES /* ObjectAttributes */,
        HANDLE /* ProcessHandle */,
        PUSER_THREAD_START_ROUTINE /* StartRoutine */,
        PVOID /* Argument */,
        ULONG /* CreateFlags */,
        ULONG_PTR /* ZeroBits */,
        SIZE_T /* StackSize */,
        SIZE_T /* MaximumStackSize */,
        PPS_ATTRIBUTE_LIST /* AttributeList */
        );

    auto p_NtCreateThreadEx = (t_NtCreateThreadEx)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtCreateThreadEx");
    if (p_NtCreateThreadEx)
    {
        // Based on: https://chromium-review.googlesource.com/c/crashpad/crashpad/+/339263/16/client/crashpad_client_win.cc#697
        Status = p_NtCreateThreadEx(&hThread,
            STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL,
            nullptr,
            ProcessHandle,
            StartRoutine,
            Argument,
            THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH,
            0,
            0x4000 /* PAGE_SIZE * 4 */,
            0x4000,
            nullptr);
    }
    else
    {
        CLIENT_ID ClientId;
        Status = RtlCreateUserThread(ProcessHandle,
            NULL,
            FALSE,
            0,
            0x4000,
            0x4000 /* PAGE_SIZE * 4 */,
            StartRoutine,
            Argument,
            &hThread,
            &ClientId);
    }

    if (NT_SUCCESS(Status))
    {
        NtClose(hThread);
    }

    return Status;
}

static NTSTATUS NTAPI DbgUiIssueRemoteBreakin_(IN HANDLE Process)
{
    PUSER_THREAD_START_ROUTINE RemoteBreakFunction = (PUSER_THREAD_START_ROUTINE)DbgUiRemoteBreakin;
    LPVOID RemoteMemory = VirtualAllocEx(Process, 0, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READ);
    if (RemoteMemory)
    {
        SIZE_T written = 0;
        unsigned char payload[] = { 0xCC, 0xC3 };
        if (WriteProcessMemory(Process, RemoteMemory, payload, sizeof(payload), &written))
        {
            RemoteBreakFunction = (PUSER_THREAD_START_ROUTINE)RemoteMemory;
        }
        else
        {
            VirtualFreeEx(Process, RemoteMemory, 0, MEM_RELEASE);
        }
    }

    /* Create the thread that will perform the breakin (on Vista+ it will skip DllMain and TLS callbacks) */
    return CreateThreadSkipAttach(Process, RemoteBreakFunction, NULL);
}

static NTSTATUS NTAPI DbgUiDebugActiveProcess_(IN HANDLE Process)
{
    /* Tell the kernel to start debugging */
    NTSTATUS Status = NtDebugActiveProcess(Process, NtCurrentTeb()->DbgSsReserved[1]);
    if (NT_SUCCESS(Status))
    {
        /* Now break-in the process */
        Status = DbgUiIssueRemoteBreakin_(Process);
        if (!NT_SUCCESS(Status))
        {
            /* We couldn't break-in, cancel debugging */
            DbgUiStopDebugging(Process);
        }
    }

    /* Return status */
    return Status;
}

static NTSTATUS NTAPI DbgUiConnectToDbg_()
{
    if (NtCurrentTeb()->DbgSsReserved[1] != NULL)
        return STATUS_SUCCESS;

    OBJECT_ATTRIBUTES ObjectAttributes;
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
    return NtCreateDebugObject(&NtCurrentTeb()->DbgSsReserved[1], DEBUG_ALL_ACCESS, &ObjectAttributes, 0);
}

// Source: https://github.com/mirror/reactos/blob/c6d2b35ffc91e09f50dfb214ea58237509329d6b/reactos/dll/win32/kernel32/client/debugger.c#L480
BOOL WINAPI DebugActiveProcess_(IN DWORD dwProcessId)
{
    /* Connect to the debugger */
    NTSTATUS Status = DbgUiConnectToDbg_();
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Get the process handle */
    HANDLE Handle = ProcessIdToHandle(dwProcessId);
    if (!Handle)
    {
        return FALSE;
    }

    /* Now debug the process */
    Status = DbgUiDebugActiveProcess_(Handle);

    /* Close the handle since we're done */
    NtClose(Handle);

    /* Check if debugging worked */
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Success */
    return TRUE;
}