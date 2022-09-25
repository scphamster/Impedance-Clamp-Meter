//
// Created by scpha on 25/09/2022.
//

#include <asf.h>
#include "testfile.h"

#ifdef __cplusplus
extern "C" {
#endif

int
mytestfunc()
{
    wdt_disable(WDT);

    return 5;
}

#ifdef __cplusplus
}
#endif