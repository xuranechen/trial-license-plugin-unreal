// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FTrialLicenseInfo.h"

/**
 * @brief 许可证加密器类，提供许可证数据的加密、解密和签名功能
 *
 * 该类实现了基于AES加密和SHA256哈希的安全机制，用于保护试用许可证数据。
 * 支持许可证密钥的生成、解析和验证，确保许可证数据的完整性和安全性。
 * 所有方法都是静态的，无需创建实例即可使用。
 */
class TRIALLICENSEPLUGIN_API FLicenseEncryptor
{
public:
	/**
	 * @brief 生成加密的许可证密钥字符串
	 *
	 * 将许可证信息结构体序列化为JSON格式，然后使用AES加密算法加密，
	 * 生成最终的许可证密钥字符串。
	 *
	 * @param LicenseInfo 包含许可证详细信息的结构体
	 * @param PassPhrase 用于加密的密码短语
	 * @return 加密后的许可证密钥字符串
	 */
	static FString GenerateLicenseKey(const FTrialLicenseInfo& LicenseInfo, const FString& PassPhrase);

	/**
	 * @brief 解析加密的许可证密钥字符串
	 *
	 * 解密许可证密钥字符串，解析其中的JSON数据，并提取许可证信息填充到结构体中。
	 * 如果解密失败或JSON解析失败，返回空的许可证信息结构体。
	 *
	 * @param LicenseKey 要解析的加密许可证密钥字符串
	 * @param PassPhrase 用于解密的密码短语
	 * @return 解析出的许可证信息结构体，包含所有许可证详细信息
	 */
	static FTrialLicenseInfo ParseLicenseKey(const FString& LicenseKey, const FString& PassPhrase);

	/**
	 * @brief 计算输入字符串的SHA256哈希值
	 *
	 * 将输入字符串转换为UTF-8字节数组，然后使用内部SHA256实现计算哈希值，
	 * 并将结果格式化为十六进制字符串返回。
	 *
	 * @param Input 要计算哈希的输入字符串
	 * @return 64字符的十六进制SHA256哈希字符串，如果输入为空则返回空字符串
	 */
	static FString SHA256Hash(const FString& Input);

	/**
	 * @brief 使用AES加密算法加密输入字符串
	 *
	 * 该函数使用AES-256加密输入数据，并添加盐值和消息认证码(MAC)以增强安全性。
	 * 加密过程包括：填充明文到AES块大小、生成密钥、AES加密、生成盐值和MAC、
	 * 最后将所有数据打包为JSON并进行Base64编码。
	 *
	 * @param Input 要加密的输入字符串
	 * @param Key 用于派生AES密钥的密钥字符串
	 * @return 加密后的Base64编码字符串，如果输入或密钥为空则返回空字符串
	 */
	static FString AESEncrypt(const FString& Input, const FString& Key);

	/**
	 * @brief 使用AES解密算法解密输入字符串
	 *
	 * 该函数解密由AESEncrypt加密的字符串，支持新格式(JSON包装)和旧格式(直接Base64)。
	 * 解密过程包括：Base64解码、JSON解析、MAC验证、AES解密和填充移除。
	 *
	 * @param Input 要解密的Base64编码字符串
	 * @param Key 用于派生AES密钥的密钥字符串
	 * @return 解密后的原始字符串，如果解密失败或MAC验证失败则返回空字符串
	 */
	static FString AESDecrypt(const FString& Input, const FString& Key);

	/**
	 * @brief 生成许可证签名
	 *
	 * 使用SHA256哈希算法对许可证的关键信息生成数字签名，用于验证许可证的完整性和真实性。
	 * 签名数据包括密码短语、机器ID、时间戳、项目ID和永久许可证标志。
	 *
	 * @param PassPhrase 密码短语，用于生成签名的密钥
	 * @param MachineID 机器唯一标识符
	 * @param Time 许可证生成或验证的时间戳
	 * @param ProjectId 项目标识符
	 * @param bPerpetual 是否为永久许可证
	 * @return 生成的SHA256哈希签名字符串
	 */
	static FString GenerateSignature(const FString& PassPhrase, const FString& MachineID, const FDateTime& Time, const FString& ProjectId, bool bPerpetual);
};
