int AESEncode (const char* srcString, int srcLen, char** dstString, int* dstLen);
int AESDecode (const char* srcString, int srcLen, char** dstString, int* dstLen);
void ChangeUserKey(char *pwd);