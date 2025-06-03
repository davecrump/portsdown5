#ifndef __SIGGEN_H__
#define __SIGGEN_H__

int CalcOPLevel(uint64_t DisplayFreq, int level, bool ModOn);
int CheckExpressConnect();
int CheckExpressRunning();
int StartExpressServer();
void ExpressOn(uint64_t DisplayFreq, int level);
void ExpressOnWithMod(uint64_t DisplayFreq, int level);
void ExpressOff();
void ExpressModOff();
void ExpressModOn();
void StopExpressServer();
void ChangeExpressFreq(uint64_t DisplayFreq);
void ChangeExpressLevel(int level);

#endif /* __SIGGEN__ */

