//
// Created by liuliwu on 2020-05-14.
//

#ifndef TLBS_PERSISTENCE_H
#define TLBS_PERSISTENCE_H

#include <sys/types.h>

int persistenceSaveBackground(char *persistenceDataDir);
int persistenceSave(char *persistenceDataDir);
void backgroundSaveDoneHandler(int exitcode, int bysignal);
void backgroundSaveDoneHandlerDisk(int exitcode, int bysignal);
void backgroundSaveDoneHandlerSocket(int exitcode, int bysignal);
void killPersistenceChild();
void persistenceRemoveTempFile(pid_t childpid);
int persistenceSaveDb(char *persistenceDataDir, int dbnum);

#endif //TLBS_PERSISTENCE_H
