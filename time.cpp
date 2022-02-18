#include <process.h>
#include<windows.h>
#include<cstring>

float GetMyCPUTime()
{
    HANDLE hProcess;    // specifies the process of interest
    FILETIME lpCreationTime;    // when the process was created
    FILETIME lpExitTime;    // when the process exited
    FILETIME lpKernelTime;    // time the process has spent in kernel mode
    FILETIME lpUserTime;
    LARGE_INTEGER t1, t2;


    hProcess = GetCurrentProcess();
    GetProcessTimes(hProcess, &lpCreationTime, &lpExitTime, &lpKernelTime, &lpUserTime);
    memcpy(&t1, &lpUserTime, sizeof(FILETIME));
    memcpy(&t2, &lpKernelTime, sizeof(FILETIME));
    return ((float)(t1.QuadPart+t2.QuadPart)/10000000);
}