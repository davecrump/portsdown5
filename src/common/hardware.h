#ifndef __HARDWARE_H__
#define __HARDWARE_H__

int FindMouseEvent();
int CheckMouse();
int CheckPicoConnect();
void GetIPAddr(char IPAddress[18]);
void GetIPAddr2(char IPAddress[18]);
void Get_wlan0_IPAddr(char IPAddress[18]);
int CheckTuner();
int PinToBroadcom(int Pin);
int BroadcomToPin(int Broadcom);

#endif /* __HARDWARE__ */

