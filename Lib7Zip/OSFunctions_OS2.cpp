#ifdef _OS2
#include "unistd.h"
#include "dirent.h"
#include <iostream>
#define INCL_DOSMODULEMGR
#define INCL_DOSERRORS
#include <os2.h>

HMODULE *hmod2;
void *GetProcAddress (HMODULE hmod, const char *symbol) {
    void *addr = NULL;
    PFN      ModuleAddr     = 0;
    APIRET rc = 0;
    HMODULE hmod3 = *hmod2;
    //HMODULE hmod3 = hmod;
    std::cerr << "DosQueryProcAddr " << symbol << " from module hmod3: " << hmod2 << std::endl;
    char *symbol3 = strdup(symbol);
    //std::cerr << "DosQueryProcAddr (2) " << symbol2 << " from module hmod3: " << hmod2 << std::endl;

    /* Load DLL and get hmod with DosLoadModule*/
    /* Get address for process in DLL */
    rc = DosQueryProcAddr(hmod3, 0, (PSZ )symbol3, &ModuleAddr);
    /*
    return codes:
    0 NO_ERROR
    6 ERROR_INVALID_HANDLE
    123 ERROR_INVALID_NAME
    182 ERROR_INVALID_ORDINAL
    65079 ERROR_ENTRY_IS_CALLGATE (this error code is not valid in OS/2 Warp PowerPC Edition)
    */
    if(rc) {
        // error;
#if defined(_OS2_LIBDEBUG)
        std::cerr << "DosQueryProcAddr of " << symbol << " from module at " << hmod2 << " failed. " <<  " (ret code: " << rc << ")" << std::endl;
#endif
    } else {
        addr = (void *)ModuleAddr;
#if defined(_OS2_LIBDEBUG)
        std::cerr << "DosQueryProcAddr " << symbol << " from module at " << hmod2 << " succeeded (addr: " << &ModuleAddr << " ). " << std::endl;
#endif

    }
    return addr;
}

#endif //_OS2
