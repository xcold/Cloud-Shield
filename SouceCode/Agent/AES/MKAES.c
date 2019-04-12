#include "AES.h"

//BYTE UserKey[AES_USER_KEY_LEN] ={
//	0xaf, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 
//	0x88, 0x93, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 
//	0x00, 0x12, 0x22, 0x3a, 0x44, 0x55, 0xf6, 0x77,
//	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0xd7
//};

BYTE UserKey[AES_USER_KEY_LEN] ={
	0x53, 0x79, 0x6d, 0x6d, 0x65, 0x74, 0x72, 0x69, 
	0x63, 0x45, 0x6e, 0x63, 0x6f, 0x64, 0x65, 0x72
};

void ChangeUserKey(char *pwd)
{
	int i;
	//将字符转化为16进制密钥
	for(i = 0; i < 16; i++)
	{
		UserKey[i] = (BYTE)pwd[i];
	}
}


int AESEncode (const char* srcString, int srcLen, char** dstString, int* dstLen)
{
// #ifdef _DEBUG
// int t = 0x12;
// int i;
// #endif
	//16 * (trunc(string_length / 16) + 1)?
	char *pOut=0;
	unsigned int len = 16 * (srcLen/16 + 1);

	//BYTE	UserKey[AES_USER_KEY_LEN]={0};
	BYTE	IV[AES_BLOCK_LEN]={0};

	DWORD	UKLen, IVLen, SrcLen, DstLen;
	RET_VAL	ret;
	AES_ALG_INFO	AlgInfo;
	int eelen = 0;
	SrcLen = srcLen;
	UKLen = 16;
	IVLen = 16;

// #ifdef _DEBUG
// 	for (i=0; i<16; i++)
// 	{
// 		UserKey[i] = t+i;
// 	}
// #else
// 	snprintf ((char*)UserKey, sizeof(UserKey)-1, "%s", g_Config.encryptKey);
// #endif

	pOut = (char*)calloc (1, len+4);
	if (pOut == NULL)	
		return -1;
	DstLen = len;

	//
	AES_SetAlgInfo(AES_ModeType, AES_PadType, IV, &AlgInfo);

	//	Encryption
	ret = AES_EncKeySchedule(UserKey, UKLen, &AlgInfo);
	if( ret!=CTR_SUCCESS )	
	{
		//writelog(LOG_DEBUG, "AES_EncKeySchedule() returns.");
	//	safe_free (pOut);
		return -1;
	}
	ret = AES_EncInit(&AlgInfo);
	if( ret!=CTR_SUCCESS )	
	{
	//	writelog(LOG_DEBUG, "AES_EncInit() returns.");
	//	safe_free (pOut);
		return -1;
	}

	ret = AES_EncUpdate(&AlgInfo, (unsigned char*)srcString, SrcLen, (unsigned char*)pOut, &DstLen);
	if( ret!=CTR_SUCCESS )	
	{
		//writelog(LOG_DEBUG, "AES_EncUpdate() returns.");
		//safe_free (pOut);
		return -1;
	}

	eelen = DstLen;

	ret = AES_EncFinal(&AlgInfo, (unsigned char*)pOut+eelen, &DstLen);
	if( ret!=CTR_SUCCESS )	
	{
		//writelog(LOG_DEBUG, "AES_EncFinal() returns.");
	//	safe_free (pOut);
		return -1;
	}

	eelen += DstLen;
	*dstLen = eelen;
	*dstString = pOut;

	return 0;

}

int AESDecode (const char* srcString, int srcLen, char** dstString, int* dstLen)
{
// #ifdef _DEBUG
// 	int t = 0x12;
// 	int i;
// #endif
	//FILE	*pIn, *pOut;
	char* pOut = 0;
	//unsigned char UserKey[AES_USER_KEY_LEN]={0};
	unsigned char IV[AES_BLOCK_LEN]={0};
	//unsigned char SrcData[1024+32], DstData[1024+32];
	unsigned int  UKLen, IVLen;
	unsigned int SrcLen, DstLen;
	RET_VAL	ret;
	AES_ALG_INFO	AlgInfo;
	int ddlen = 0;

	SrcLen = srcLen;
	
	pOut = (char*)calloc(1, SrcLen+2);
	if (pOut == NULL) return -1;

	DstLen = SrcLen;

	UKLen = 16;
	IVLen = 16;
// #ifdef _DEBUG
// 	
// 	for (i=0; i<16; i++)
// 	{
// 		UserKey[i] = t+i;
// 	}
// #else
// 	snprintf ((char*)UserKey, sizeof(UserKey)-1, "%s", g_Config.encryptKey);
// #endif

	AES_SetAlgInfo(AES_ModeType, AES_PadType, IV, &AlgInfo);

	//Decryption
	//if( ModeType==AI_ECB || ModeType==AI_CBC )
	ret = AES_DecKeySchedule(UserKey, UKLen, &AlgInfo);
	//else if( ModeType==AI_OFB || ModeType==AI_CFB )
	//	ret = AES_EncKeySchedule(UserKey, UKLen, &AlgInfo);

	if( ret!=CTR_SUCCESS )	
	{
		//writelog(LOG_DEBUG, "AES_DecKeySchedule() returns.");
	//	safe_free (pOut);
		return -1;
	}

	ret = AES_DecInit(&AlgInfo);
	if( ret!=CTR_SUCCESS )	
	{
	//	writelog(LOG_DEBUG, "AES_DecInit() returns.");
	//	safe_free (pOut);
		return -1;
	}

	ret = AES_DecUpdate(&AlgInfo, (unsigned char*)srcString, SrcLen, (unsigned char*)pOut, &DstLen);
	if( ret!=CTR_SUCCESS )	
	{
	//	writelog(LOG_DEBUG, "AES_DecUpdate() returns.");
	//	safe_free (pOut);
		return -1;
	}
	ddlen = DstLen;

	ret = AES_DecFinal(&AlgInfo, (unsigned char*)pOut+ddlen, &DstLen);
	if( ret!=CTR_SUCCESS )	
	{
	//	writelog(LOG_DEBUG, "AES_DecFinal() returns.");
	//	safe_free (pOut);
		return -1;
	}
	ddlen += DstLen;
	*dstLen = ddlen;
	*dstString = pOut;
	return 0;

}
