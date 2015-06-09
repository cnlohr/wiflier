#ifndef _AVRTOOL_H
#define _AVRTOOL_H

#define AVRTOOL_ADDRESS 0x20

//Returns nonzero if failed.
int SetupAVRTool();

//Returns voltage of Vcc 1..500.  Returns 0 on fault.
int RunAVRTool( unsigned char * motors );

#endif

