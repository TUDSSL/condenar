#include "Screen/Screen.h"
#include "Helpers/Misc.h"
#include "Kernel.h"
#include "Kernel_Draw.h"
#include "SEGGER_RTT.h"
#include "Screen/COM50/ScreenCOM50.h"

void Screen_Initialize(){
    LOG_I("Initializing screen...");
    Screen_IMPL_Initialize();
    LOG_I("Screen initalized");   
}

//Usual screen update procedure:
// Call k_BeginScreenUpdate()
// Call any k_DRAW APIs
// Call k_EndScreenUpdate()

void k_BeginScreenUpdate(){
    //Screen_IMPL_WaitForPendingFrames(); //Waits for either a VSYNC interval or (if internally buffered) until the last frame was sent


}

void k_EndScreenUpdate(bool checkpointsAllowed, bool showOverlays){
    if(showOverlays) k_DrawOverlays();

    #ifdef SEND_SCREEN_OVER_RTT
    if(k_GetSettingBool("/Other/Debugging/Send screen over RTT", false)){
        while (SEGGER_RTT_GetBytesInBuffer(0)!=0) {}; //Wait for the screen to be send to the PC
    }        
    #endif

    Screen_IMPL_SendFrame(); //Forces the buffered frame to be sent (probably in an async way), or does nothing if not using a buffered approach    
    k_DoEndOfFrameStuff(checkpointsAllowed);
} 

void k_WaitForPendingScreenFrames(){
    Screen_IMPL_WaitForPendingFrames();
}