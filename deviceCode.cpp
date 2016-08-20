﻿
#ifdef WIN32
    #include<stdio.h>
    #include<tchar.h>
    #include<Windows.h>
    #include<Iphlpapi.h>
    #pragma comment(lib, "IPHLPAPI.lib")
#endif

#include<iostream>
#include<boost/filesystem.hpp>
#include<boost/filesystem/fstream.hpp>

#include<core/os.hpp>
#include<core/md5sum.hpp>
#include<core/deviceCode.hpp>

namespace core
{

class DeviceCodeImpl: public DeviceCode
{
public:
    DeviceCodeImpl()
    {
        codeGen();
    }

#ifdef WIN32
private:
#define  FILE_DEVICE_SCSI              0x0000001b
#define  IOCTL_SCSI_MINIPORT_IDENTIFY  ( ( FILE_DEVICE_SCSI << 16 ) + 0x0501 )
#define  IOCTL_SCSI_MINIPORT 0x0004D008 
#define  IDENTIFY_BUFFER_SIZE  512
#define  SENDIDLENGTH  ( sizeof( SENDCMDOUTPARAMS ) + IDENTIFY_BUFFER_SIZE )
#define  IDE_ATAPI_IDENTIFY  0xA1  //  Returns ID sector for ATAPI.
#define  IDE_ATA_IDENTIFY    0xEC  //  Returns ID sector for ATA.
#define  DFP_RECEIVE_DRIVE_DATA   0x0007c088

    typedef struct _IDSECTOR
    {
        USHORT  wGenConfig;
        USHORT  wNumCyls;
        USHORT  wReserved;
        USHORT  wNumHeads;
        USHORT  wBytesPerTrack;
        USHORT  wBytesPerSector;
        USHORT  wSectorsPerTrack;
        USHORT  wVendorUnique[3];
        CHAR    sSerialNumber[20];
        USHORT  wBufferType;
        USHORT  wBufferSize;
        USHORT  wECCSize;
        CHAR    sFirmwareRev[8];
        CHAR    sModelNumber[40];
        USHORT  wMoreVendorUnique;
        USHORT  wDoubleWordIO;
        USHORT  wCapabilities;
        USHORT  wReserved1;
        USHORT  wPIOTiming;

        USHORT  wDMATiming;
        USHORT  wBS;
        USHORT  wNumCurrentCyls;
        USHORT  wNumCurrentHeads;
        USHORT  wNumCurrentSectorsPerTrack;
        ULONG   ulCurrentSectorCapacity;
        USHORT  wMultSectorStuff;
        ULONG   ulTotalAddressableSectors;
        USHORT  wSingleWordDMA;
        USHORT  wMultiWordDMA;
        BYTE    bReserved[128];
    } IDSECTOR, *PIDSECTOR;

    typedef struct _SRB_IO_CONTROL
    {
        ULONG HeaderLength;
        UCHAR Signature[8];
        ULONG Timeout;
        ULONG ControlCode;
        ULONG ReturnCode;
        ULONG Length;
    } SRB_IO_CONTROL, *PSRB_IO_CONTROL;

    typedef struct _GETVERSIONOUTPARAMS
    {
        BYTE bVersion;      // Binary driver version.
        BYTE bRevision;     // Binary driver revision.
        BYTE bReserved;     // Not used.
        BYTE bIDEDeviceMap; // Bit map of IDE devices.
        DWORD fCapabilities; // Bit mask of driver capabilities.
        DWORD dwReserved[4]; // For future use.
    } GETVERSIONOUTPARAMS, *PGETVERSIONOUTPARAMS, *LPGETVERSIONOUTPARAMS;

    //////////////////////////////////////////////////////////////////////
    //结构定义
    typedef struct _UNICODE_STRING
    {
        USHORT  Length;//长度
        USHORT  MaximumLength;//最大长度
        PWSTR  Buffer;//缓存指针
    } UNICODE_STRING, *PUNICODE_STRING;

    typedef struct _OBJECT_ATTRIBUTES
    {
        ULONG Length;//长度 18h
        HANDLE RootDirectory;//  00000000
        PUNICODE_STRING ObjectName;//指向对象名的指针
        ULONG Attributes;//对象属性00000040h
        PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR，0
        PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE，0
    } OBJECT_ATTRIBUTES;

    typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;

    //函数指针变量类型
    typedef DWORD(__stdcall *ZWOS)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);
    typedef DWORD(__stdcall *ZWMV)(HANDLE, HANDLE, PVOID, ULONG, ULONG, PLARGE_INTEGER, PSIZE_T, DWORD, ULONG, ULONG);
    typedef DWORD(__stdcall *ZWUMV)(HANDLE, PVOID);

    BOOL WinNTHDSerialNumAsScsiRead(BYTE* dwSerial, UINT* puSerialLen, UINT uMaxSerialLen)
    {
        BOOL bInfoLoaded = FALSE;
        for (int iController = 0; iController < 2; ++iController)
        {
            HANDLE hScsiDriveIOCTL = 0;
            char szDriveName[256];

            //  Try to get a handle to PhysicalDrive IOCTL, report failure
            //  and exit if can't.
            sprintf(szDriveName, "\\\\.\\Scsi%d:", iController);

            //  Windows NT, Windows 2000, any rights should do
            hScsiDriveIOCTL = CreateFileA(szDriveName,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                    OPEN_EXISTING, 0, NULL);

            // if (hScsiDriveIOCTL == INVALID_HANDLE_VALUE)
            //    printf ("Unable to open SCSI controller %d, error code: 0x%lX\n",
            //            controller, GetLastError ());

            if (hScsiDriveIOCTL != INVALID_HANDLE_VALUE)
            {
                int iDrive = 0;
                for (iDrive = 0; iDrive < 2; ++iDrive)
                {
                    char szBuffer[sizeof(SRB_IO_CONTROL) + SENDIDLENGTH] = { 0 };
                    SRB_IO_CONTROL* p = (SRB_IO_CONTROL*)szBuffer;
                    SENDCMDINPARAMS* pin = (SENDCMDINPARAMS*)(szBuffer + sizeof(SRB_IO_CONTROL));
                    DWORD dwResult;

                    p->HeaderLength = sizeof(SRB_IO_CONTROL);
                    p->Timeout = 10000;
                    p->Length = SENDIDLENGTH;
                    p->ControlCode = IOCTL_SCSI_MINIPORT_IDENTIFY;
                    strncpy((char*)p->Signature, "SCSIDISK", 8);

                    pin->irDriveRegs.bCommandReg = IDE_ATA_IDENTIFY;
                    pin->bDriveNumber = iDrive;

                    if (DeviceIoControl(hScsiDriveIOCTL, IOCTL_SCSI_MINIPORT,
                                szBuffer,
                                sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDINPARAMS) - 1,
                                szBuffer,
                                sizeof(SRB_IO_CONTROL) + SENDIDLENGTH,
                                &dwResult, NULL))
                    {
                        SENDCMDOUTPARAMS* pOut = (SENDCMDOUTPARAMS*)(szBuffer + sizeof(SRB_IO_CONTROL));
                        IDSECTOR* pId = (IDSECTOR*)(pOut->bBuffer);
                        if (pId->sModelNumber[0])
                        {
                            if (*puSerialLen + 20U <= uMaxSerialLen)
                            {
                                // 序列号
                                CopyMemory(dwSerial + *puSerialLen, ((USHORT*)pId) + 10, 20);

                                // Cut off the trailing blanks
                                for (UINT i = 20; i != 0U && ' ' == dwSerial[*puSerialLen + i - 1]; --i)
                                {
                                    *puSerialLen += i;
                                }

                                // 型号
                                CopyMemory(dwSerial + *puSerialLen, ((USHORT*)pId) + 27, 40);

                                // Cut off the trailing blanks
                                for (UINT i = 40; i != 0U && ' ' == dwSerial[*puSerialLen + i - 1]; --i)
                                {
                                    *puSerialLen += i;
                                }
                                bInfoLoaded = TRUE;
                            }
                            else
                            {
                                ::CloseHandle(hScsiDriveIOCTL);
                                return bInfoLoaded;
                            }
                        }

                    }

                }

                ::CloseHandle(hScsiDriveIOCTL);
            }
        }

        return bInfoLoaded;
    }

    BOOL DoIdentify(HANDLE hPhysicalDriveIOCTL, PSENDCMDINPARAMS pSCIP, PSENDCMDOUTPARAMS pSCOP, BYTE bIDCmd, BYTE bDriveNum, PDWORD lpcbBytesReturned)
    {

        // Set up data structures for IDENTIFY command.
        pSCIP->cBufferSize = IDENTIFY_BUFFER_SIZE;
        pSCIP->irDriveRegs.bFeaturesReg = 0;
        pSCIP->irDriveRegs.bSectorCountReg = 1;
        pSCIP->irDriveRegs.bSectorNumberReg = 1;
        pSCIP->irDriveRegs.bCylLowReg = 0;
        pSCIP->irDriveRegs.bCylHighReg = 0;

        // calc the drive number.
        pSCIP->irDriveRegs.bDriveHeadReg = 0xA0 | ((bDriveNum & 1) << 4);

        // The command can either be IDE identify or ATAPI identify.
        pSCIP->irDriveRegs.bCommandReg = bIDCmd;
        pSCIP->bDriveNumber = bDriveNum;
        pSCIP->cBufferSize = IDENTIFY_BUFFER_SIZE;

        return DeviceIoControl(hPhysicalDriveIOCTL, DFP_RECEIVE_DRIVE_DATA,
                (LPVOID)pSCIP,
                sizeof(SENDCMDINPARAMS) - 1,
                (LPVOID)pSCOP,
                sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1,
                lpcbBytesReturned, NULL);
    }

    BOOL WinNTHDSerialNumAsPhysicalRead(BYTE* dwSerial, UINT* puSerialLen, UINT uMaxSerialLen)
    {
#define  DFP_GET_VERSION          0x00074080
        BOOL bInfoLoaded = FALSE;

        for (UINT uDrive = 0; uDrive < 4; ++uDrive)
        {
            HANDLE hPhysicalDriveIOCTL = 0;

            //  Try to get a handle to PhysicalDrive IOCTL, report failure
            //  and exit if can't.
            char szDriveName[256];

            sprintf(szDriveName, "\\\\.\\PhysicalDrive%d", uDrive);
            //  Windows NT, Windows 2000, must have admin rights

            hPhysicalDriveIOCTL = CreateFileA(szDriveName,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

            if (hPhysicalDriveIOCTL != INVALID_HANDLE_VALUE)
            {
                GETVERSIONOUTPARAMS VersionParams = { 0 };
                DWORD               cbBytesReturned = 0;

                // Get the version, etc of PhysicalDrive IOCTL
                if (DeviceIoControl(hPhysicalDriveIOCTL, DFP_GET_VERSION,
                            NULL,
                            0,
                            &VersionParams,
                            sizeof(GETVERSIONOUTPARAMS),
                            &cbBytesReturned, NULL))
                {
                    // If there is a IDE device at number "i" issue commands
                    // to the device
                    if (VersionParams.bIDEDeviceMap != 0)
                    {
                        BYTE             bIDCmd = 0;   // IDE or ATAPI IDENTIFY cmd
                        SENDCMDINPARAMS  scip = { 0 };

                        // Now, get the ID sector for all IDE devices in the system.
                        // If the device is ATAPI use the IDE_ATAPI_IDENTIFY command,
                        // otherwise use the IDE_ATA_IDENTIFY command
                        bIDCmd = (VersionParams.bIDEDeviceMap >> uDrive & 0x10) ? IDE_ATAPI_IDENTIFY : IDE_ATA_IDENTIFY;
                        BYTE IdOutCmd[sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1] = { 0 };

                        if (DoIdentify(hPhysicalDriveIOCTL,
                                    &scip,
                                    (PSENDCMDOUTPARAMS)&IdOutCmd,
                                    (BYTE)bIDCmd,
                                    (BYTE)uDrive,
                                    &cbBytesReturned))
                        {
                            if (*puSerialLen + 20U <= uMaxSerialLen)
                            {
                                CopyMemory(dwSerial + *puSerialLen, ((USHORT*)(((PSENDCMDOUTPARAMS)IdOutCmd)->bBuffer)) + 10, 20);  // 序列号

                                // Cut off the trailing blanks
                                for (UINT i = 20; i != 0U && ' ' == dwSerial[*puSerialLen + i - 1]; --i)
                                {
                                    *puSerialLen += i;
                                }

                                CopyMemory(dwSerial + *puSerialLen, ((USHORT*)(((PSENDCMDOUTPARAMS)IdOutCmd)->bBuffer)) + 27, 40); // 型号

                                // Cut off the trailing blanks
                                for (UINT i = 40; i != 0U && ' ' == dwSerial[*puSerialLen + i - 1]; --i)
                                {
                                    *puSerialLen += i;
                                }
                                bInfoLoaded = TRUE;
                            }
                            else
                            {

                                ::CloseHandle(hPhysicalDriveIOCTL);

                                return bInfoLoaded;

                            }
                        }
                    }
                }

                CloseHandle(hPhysicalDriveIOCTL);
            }
        }

        return bInfoLoaded;
    }

    UINT FindAwardBios(BYTE** ppBiosAddr)
    {
        BYTE* pBiosAddr = *ppBiosAddr + 0xEC71;
        BYTE szBiosData[128];
        CopyMemory(szBiosData, pBiosAddr, 127);
        szBiosData[127] = 0;

        int iLen = std::strlen((char*)szBiosData);
        if (iLen > 0 && iLen < 128)
        {
            //AWard:         07/08/2002-i845G-ITE8712-JF69VD0CC-00
            //Phoenix-Award: 03/12/2002-sis645-p4s333
            if (szBiosData[2] == '/' && szBiosData[5] == '/')
            {
                BYTE* p = szBiosData;
                while (*p)
                {
                    if (*p < ' ' || *p >= 127)
                    {
                        break;
                    }

                    ++p;
                }

                if (*p == 0)
                {
                    *ppBiosAddr = pBiosAddr;
                    return (UINT)iLen;
                }

            }
        }

        return 0;
    }

    UINT FindAmiBios(BYTE** ppBiosAddr)
    {
        BYTE* pBiosAddr = *ppBiosAddr + 0xF478;
        BYTE szBiosData[128];
        CopyMemory(szBiosData, pBiosAddr, 127);
        szBiosData[127] = 0;

        int iLen = std::strlen((char*)szBiosData);
        if (iLen > 0 && iLen < 128)
        {
            // Example: "AMI: 51-2300-000000-00101111-030199-"
            if (szBiosData[2] == '-' && szBiosData[7] == '-')
            {
                BYTE* p = szBiosData;
                while (*p)
                {
                    if (*p < ' ' || *p >= 127)
                    {
                        break;
                    }
                    ++p;
                }

                if (*p == 0)
                {
                    *ppBiosAddr = pBiosAddr;
                    return (UINT)iLen;
                }
            }
        }

        return 0;
    }

    UINT FindPhoenixBios(BYTE** ppBiosAddr)
    {
        UINT uOffset[3] = { 0x6577, 0x7196, 0x7550 };
        for (UINT i = 0; i < 3; ++i)
        {
            BYTE* pBiosAddr = *ppBiosAddr + uOffset[i];
            BYTE szBiosData[128];
            CopyMemory(szBiosData, pBiosAddr, 127);
            szBiosData[127] = 0;

            int iLen = std::strlen((char*)szBiosData);
            if (iLen > 0 && iLen < 128)
            {
                // Example: Phoenix "NITELT0.86B.0044.P11.9910111055"
                if (szBiosData[7] == '.' && szBiosData[11] == '.')
                {
                    BYTE* p = szBiosData;
                    while (*p)
                    {
                        if (*p < ' ' || *p >= 127)
                        {
                            break;
                        }
                        ++p;
                    }

                    if (*p == 0)
                    {
                        *ppBiosAddr = pBiosAddr;
                        return (UINT)iLen;
                    }
                }
            }
        }

        return 0;
    }

    void codeGenImpl()
    {
        // 网卡 MAC 地址，注意: MAC 地址是可以在注册表中修改的
        {
            UINT uErrorCode = 0;
            IP_ADAPTER_INFO iai;
            ULONG uSize = 0;
            DWORD dwResult = GetAdaptersInfo(&iai, &uSize);
            if (dwResult == ERROR_BUFFER_OVERFLOW)
            {
                IP_ADAPTER_INFO* piai = (IP_ADAPTER_INFO*)HeapAlloc(GetProcessHeap(), 0, uSize);
                if (piai != NULL)
                {
                    dwResult = GetAdaptersInfo(piai, &uSize);
                    if (ERROR_SUCCESS == dwResult)
                    {
                        IP_ADAPTER_INFO* piai2 = piai;
                        while (piai2 != NULL && (uSystemInfoLen + piai2->AddressLength)< 4096U)
                        {
                            CopyMemory(szSystemInfo + uSystemInfoLen, piai2->Address, piai2->AddressLength);
                            uSystemInfoLen += piai2->AddressLength;
                            piai2 = piai2->Next;
                        }
                    }
                    else
                    {
                        uErrorCode = 0xF0000000U + dwResult;
                    }

                    HeapFree(GetProcessHeap(), 0, piai);
                }
                else
                {
                    return;
                }

            }
            else
            {
                uErrorCode = 0xE0000000U + dwResult;
            }

            if (uErrorCode != 0U)
            {
                return;
            }

        }

        // 硬盘序列号，注意：有的硬盘没有序列号
        {

            //OSVERSIONINFO ovi = { 0 };

            //ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

            //GetVersionEx(&ovi);
            //if (ovi.dwPlatformId != VER_PLATFORM_WIN32_NT)
            //{
            // Only Windows 2000, Windows XP, Windows Server 2003...
            //return FALSE;
            //}
            //else
            {
                if (!WinNTHDSerialNumAsPhysicalRead(szSystemInfo, &uSystemInfoLen, 1024))
                {
                    WinNTHDSerialNumAsScsiRead(szSystemInfo, &uSystemInfoLen, 1024);
                }
            }
        }

        // CPU ID
        {
            BOOL bException = FALSE;
            BYTE szCpu[16] = { 0 };
            UINT uCpuID = 0U;
            __try
            {
                _asm
                {
                    mov eax, 0
                        cpuid
                        mov dword ptr szCpu[0], ebx
                        mov dword ptr szCpu[4], edx
                        mov dword ptr szCpu[8], ecx
                        mov eax, 1
                        cpuid
                        mov uCpuID, edx
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                bException = TRUE;
            }

            if (!bException)
            {
                CopyMemory(szSystemInfo + uSystemInfoLen, &uCpuID, sizeof(UINT));
                uSystemInfoLen += sizeof(UINT);
                uCpuID = strlen((char*)szCpu);
                CopyMemory(szSystemInfo + uSystemInfoLen, szCpu, uCpuID);
                uSystemInfoLen += uCpuID;
            }
        }

        // BIOS 编号，支持 AMI, AWARD, PHOENIX
        {
            SIZE_T ssize;
            LARGE_INTEGER so;
            so.LowPart = 0x000f0000;
            so.HighPart = 0x00000000;
            ssize = 0xffff;
            wchar_t strPH[30] = L"\\device\\physicalmemory";

            DWORD ba = 0;

            UNICODE_STRING struniph;
            struniph.Buffer = strPH;
            struniph.Length = 0x2c;
            struniph.MaximumLength = 0x2e;

            OBJECT_ATTRIBUTES obj_ar;
            obj_ar.Attributes = 64;
            obj_ar.Length = 24;
            obj_ar.ObjectName = &struniph;
            obj_ar.RootDirectory = 0;
            obj_ar.SecurityDescriptor = 0;
            obj_ar.SecurityQualityOfService = 0;

            HMODULE hinstLib = LoadLibraryA("ntdll.dll");
            ZWOS ZWopenS = (ZWOS)GetProcAddress(hinstLib, "ZwOpenSection");
            ZWMV ZWmapV = (ZWMV)GetProcAddress(hinstLib, "ZwMapViewOfSection");
            ZWUMV ZWunmapV = (ZWUMV)GetProcAddress(hinstLib, "ZwUnmapViewOfSection");

            //调用函数，对物理内存进行映射
            HANDLE hSection;
            if (0 == ZWopenS(&hSection, 4, &obj_ar) &&
                    0 == ZWmapV(
                        (HANDLE)hSection,   //打开Section时得到的句柄
                        (HANDLE)0xFFFFFFFF, //将要映射进程的句柄，
                        &ba,                  //映射的基址
                        0,
                        0xFFFF,               //分配的大小
                        &so,                  //物理内存的地址
                        &ssize,               //指向读取内存块大小的指针
                        1,                    //子进程的可继承性设定
                        0,                    //分配类型
                        2                     //保护类型
                        )
               )
                //执行后会在当前进程的空间开辟一段64k的空间，并把f000:0000到f000:ffff处的内容映射到这里
                //映射的基址由ba返回,如果映射不再有用,应该用ZwUnmapViewOfSection断开映射
            {
                BYTE* pBiosSerial = (BYTE*)ba;
                UINT uBiosSerialLen = FindAwardBios(&pBiosSerial);
                if (uBiosSerialLen == 0U)
                {
                    uBiosSerialLen = FindAmiBios(&pBiosSerial);
                    if (uBiosSerialLen == 0U)
                    {
                        uBiosSerialLen = FindPhoenixBios(&pBiosSerial);
                    }
                }

                if (uBiosSerialLen != 0U)
                {
                    CopyMemory(szSystemInfo + uSystemInfoLen, pBiosSerial, uBiosSerialLen);
                    uSystemInfoLen += uBiosSerialLen;
                }

                ZWunmapV((HANDLE)0xFFFFFFFF, (void*)ba);
            }

        }

        // 完毕， 系统特征码已取得。
    }
#else
    void codeGenImpl()
    {

    }
#endif

    void codeGen()
    {
        codeGenImpl();

        core::MD5Compute mdc;
        mdc.append(szSystemInfo, uSystemInfoLen);
        mdc.finish();

        code_=mdc.sumGet().toString();

        auto path=core::getProgramPath();
        path.remove_filename();
        path /= L"psh.device.code";

        boost::filesystem::ofstream stm(path);
        if(!stm)
        {
            std::cerr << "deviceCode: " << code_ << std::endl;
            return;
        }

        stm << code_ << std::endl;
    }

private:
    uint8_t szSystemInfo[4096]; // 在程序执行完毕后，此处存储取得的系统特征码
    uint32_t uSystemInfoLen = 0; // 在程序执行完毕后，此处存储取得的系统特征码的长度
};

DeviceCode& DeviceCode::instance()
{
    static DeviceCodeImpl gs;
    return gs;
}

}

