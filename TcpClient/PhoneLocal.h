#ifndef PhoneLocalH
#define PhoneLocalH

int SetVariable(const char* name, const char* value);
int ClearVariable(const char* name);

int QueuePush(const char* name, const char* value);
int QueueClear(const char* name);
int QueueGetSize(const char* name);

#endif
