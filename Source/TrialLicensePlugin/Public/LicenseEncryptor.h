// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FTrialLicenseInfo.h"

/**
 * 许可证加密器
 */
class TRIALLICENSEPLUGIN_API FLicenseEncryptor
{
public:
	// 生成许可证密钥字符串
	static FString GenerateLicenseKey(const FTrialLicenseInfo& LicenseInfo, const FString& PassPhrase);

	// 解析许可证密钥字符串
	static FTrialLicenseInfo ParseLicenseKey(const FString& LicenseKey, const FString& PassPhrase);

	// SHA256哈希
	static FString SHA256Hash(const FString& Input);

	// AES加密
	static FString AESEncrypt(const FString& Input, const FString& Key);

	// AES解密
	static FString AESDecrypt(const FString& Input, const FString& Key);

	// 生成签名
	static FString GenerateSignature(const FString& PassPhrase, const FString& MachineID, const FDateTime& Time, const FString& ProjectId, bool bPerpetual);
};