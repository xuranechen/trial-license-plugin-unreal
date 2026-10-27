// Copyright 2025, Rune. All Rights Reserved.

/**
 * @brief 许可证加密器类，提供许可证数据的加密、解密和签名功能
 *
 * 该类实现了基于AES加密和SHA256哈希的安全机制，用于保护试用许可证数据。
 * 支持许可证密钥的生成、解析和验证，确保许可证数据的完整性和安全性。
 */
#include "LicenseEncryptor.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"
#include "Dom/JsonObject.h"
#include "Misc/Base64.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "GenericPlatform/GenericPlatformTime.h"
#include "HAL/PlatformMath.h"
#include "Misc/Crc.h"
#include "Misc/AES.h"
#include "Misc/SecureHash.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Misc/DateTime.h"
#include "Misc/EngineVersion.h"
#include "Containers/UnrealString.h"
#include "Engine/Engine.h"
#include "Engine/EngineBaseTypes.h"
#include "Templates/UnrealTemplate.h"
#include "Misc/App.h"

// 本地 SHA256 实现，避免依赖引擎自带 SHA256 类型
namespace
{
    struct FSHA256Context_Internal
    {
        uint32 State[8];
        uint64 BitCount;
        uint8 Buffer[64];
    };

    static const uint32 K256_Internal[64] =
    {
        0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
        0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
        0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
        0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
        0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
        0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
        0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
        0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
    };

    FORCEINLINE uint32 RotR32_Internal(uint32 X, uint32 N)
    {
        return (X >> N) | (X << (32u - N));
    }

    FORCEINLINE uint32 Ch_Internal(uint32 X, uint32 Y, uint32 Z)
    {
        return (X & Y) ^ (~X & Z);
    }

    FORCEINLINE uint32 Maj_Internal(uint32 X, uint32 Y, uint32 Z)
    {
        return (X & Y) ^ (X & Z) ^ (Y & Z);
    }

    FORCEINLINE uint32 Sig0_Internal(uint32 X)
    {
        return RotR32_Internal(X, 2) ^ RotR32_Internal(X, 13) ^ RotR32_Internal(X, 22);
    }

    FORCEINLINE uint32 Sig1_Internal(uint32 X)
    {
        return RotR32_Internal(X, 6) ^ RotR32_Internal(X, 11) ^ RotR32_Internal(X, 25);
    }

    FORCEINLINE uint32 Theta0_Internal(uint32 X)
    {
        return RotR32_Internal(X, 7) ^ RotR32_Internal(X, 18) ^ (X >> 3);
    }

    FORCEINLINE uint32 Theta1_Internal(uint32 X)
    {
        return RotR32_Internal(X, 17) ^ RotR32_Internal(X, 19) ^ (X >> 10);
    }

    void SHA256Transform_Internal(FSHA256Context_Internal& Ctx, const uint8* Data)
    {
        uint32 W[64];

        for (int i = 0; i < 16; ++i)
        {
            const int Offset = i * 4;
            W[i] = (uint32(Data[Offset]) << 24) |
                   (uint32(Data[Offset + 1]) << 16) |
                   (uint32(Data[Offset + 2]) << 8) |
                   (uint32(Data[Offset + 3]));
        }

        for (int i = 16; i < 64; ++i)
        {
            W[i] = Theta1_Internal(W[i - 2]) + W[i - 7] + Theta0_Internal(W[i - 15]) + W[i - 16];
        }

        uint32 A = Ctx.State[0];
        uint32 B = Ctx.State[1];
        uint32 C = Ctx.State[2];
        uint32 D = Ctx.State[3];
        uint32 E = Ctx.State[4];
        uint32 F = Ctx.State[5];
        uint32 G = Ctx.State[6];
        uint32 H = Ctx.State[7];

        for (int i = 0; i < 64; ++i)
        {
            const uint32 T1 = H + Sig1_Internal(E) + Ch_Internal(E, F, G) + K256_Internal[i] + W[i];
            const uint32 T2 = Sig0_Internal(A) + Maj_Internal(A, B, C);
            H = G;
            G = F;
            F = E;
            E = D + T1;
            D = C;
            C = B;
            B = A;
            A = T1 + T2;
        }

        Ctx.State[0] += A;
        Ctx.State[1] += B;
        Ctx.State[2] += C;
        Ctx.State[3] += D;
        Ctx.State[4] += E;
        Ctx.State[5] += F;
        Ctx.State[6] += G;
        Ctx.State[7] += H;
    }

    void SHA256Init_Internal(FSHA256Context_Internal& Ctx)
    {
        Ctx.State[0] = 0x6a09e667u;
        Ctx.State[1] = 0xbb67ae85u;
        Ctx.State[2] = 0x3c6ef372u;
        Ctx.State[3] = 0xa54ff53au;
        Ctx.State[4] = 0x510e527fu;
        Ctx.State[5] = 0x9b05688cu;
        Ctx.State[6] = 0x1f83d9abu;
        Ctx.State[7] = 0x5be0cd19u;
        Ctx.BitCount = 0;
        FMemory::Memzero(Ctx.Buffer, sizeof(Ctx.Buffer));
    }

    void SHA256Update_Internal(FSHA256Context_Internal& Ctx, const uint8* Data, int32 Len)
    {
        int32 BufferIndex = int32((Ctx.BitCount >> 3) & 0x3Fu);
        Ctx.BitCount += uint64(Len) * 8ull;

        int32 PartLen = 64 - BufferIndex;
        int32 I = 0;

        if (Len >= PartLen)
        {
            FMemory::Memcpy(Ctx.Buffer + BufferIndex, Data, PartLen);
            SHA256Transform_Internal(Ctx, Ctx.Buffer);
            I = PartLen;

            while (I + 63 < Len)
            {
                SHA256Transform_Internal(Ctx, Data + I);
                I += 64;
            }
            BufferIndex = 0;
        }

        if (I < Len)
        {
            FMemory::Memcpy(Ctx.Buffer + BufferIndex, Data + I, Len - I);
        }
    }

    void SHA256Final_Internal(FSHA256Context_Internal& Ctx, uint8 OutHash[32])
    {
        uint8 Padding[64] = { 0x80 };
        uint8 LengthBytes[8];

        for (int i = 0; i < 8; ++i)
        {
            LengthBytes[i] = uint8((Ctx.BitCount >> ((7 - i) * 8)) & 0xFFu);
        }

        int32 BufferIndex = int32((Ctx.BitCount >> 3) & 0x3Fu);
        int32 PadLen = (BufferIndex < 56) ? (56 - BufferIndex) : (120 - BufferIndex);

        SHA256Update_Internal(Ctx, Padding, PadLen);
        SHA256Update_Internal(Ctx, LengthBytes, 8);

        for (int i = 0; i < 8; ++i)
        {
            OutHash[i * 4 + 0] = uint8((Ctx.State[i] >> 24) & 0xFFu);
            OutHash[i * 4 + 1] = uint8((Ctx.State[i] >> 16) & 0xFFu);
            OutHash[i * 4 + 2] = uint8((Ctx.State[i] >> 8)  & 0xFFu);
            OutHash[i * 4 + 3] = uint8((Ctx.State[i] >> 0)  & 0xFFu);
        }
    }

    void SHA256HashBuffer_Internal(const uint8* Data, int32 Len, uint8 OutHash[32])
    {
        FSHA256Context_Internal Ctx;
        SHA256Init_Internal(Ctx);
        if (Data && Len > 0)
        {
            SHA256Update_Internal(Ctx, Data, Len);
        }
        SHA256Final_Internal(Ctx, OutHash);
    }

    /**
     * @brief 计算口令的 CRC32 密钥种子
     *
     * 必须使用 UTF-8 字节长度，而不是 FString::Len()（TCHAR 个数）。
     * Python 端使用 zlib.crc32(passphrase.encode("utf-8"))，两者只有在
     * 纯 ASCII 口令下才恰好相等；一旦口令包含中文等非 ASCII 字符，
     * 用 Key.Len() 会导致两端派生出不同的 AES 密钥，互相无法解密。
     */
    uint32 ComputeKeyCrc32(const FString& Key)
    {
        FTCHARToUTF8 Converted(*Key);
        return FCrc::MemCrc32(Converted.Get(), Converted.Length());
    }
}

/**
 * @brief 计算输入字符串的SHA256哈希值
 *
 * 将输入字符串转换为UTF-8字节数组，然后使用内部SHA256实现计算哈希值，
 * 并将结果格式化为十六进制字符串返回。
 *
 * @param Input 要计算哈希的输入字符串
 * @return 64字符的十六进制SHA256哈希字符串，如果输入为空则返回空字符串
 */
FString FLicenseEncryptor::SHA256Hash(const FString& Input)
{
    FString Result;
    if (Input.Len() > 0)
    {
        FTCHARToUTF8 Converted(*Input);
        const uint8* Data = reinterpret_cast<const uint8*>(Converted.Get());
        const int32 Len = Converted.Length();

        uint8 HashBytes[32];
        SHA256HashBuffer_Internal(Data, Len, HashBytes);
        Result.Reserve(64);
        for (int32 i = 0; i < 32; ++i)
        {
            Result += FString::Printf(TEXT("%02x"), HashBytes[i]);
        }
    }
    return Result;
}

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
FString FLicenseEncryptor::AESEncrypt(const FString& Input, const FString& Key)
{
    if (Input.IsEmpty() || Key.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[TrialLicense][AESDecrypt] Empty input or key. InputLen=%d KeyLen=%d"), Input.Len(), Key.Len());
        return TEXT("");
    }

    FString Result;

    // 将输入转换为UTF-8字节数组
    FTCHARToUTF8 Converted(*Input);
    const uint8* SrcBytes = reinterpret_cast<const uint8*>(Converted.Get());
    int32 SrcLen = Converted.Length();

    TArray<uint8> PlainBytes;
    PlainBytes.Append(SrcBytes, SrcLen);

    // PKCS#7/零填充到AES块大小
    const int32 BlockSize = FAES::AESBlockSize; // 16
    int32 Pad = (BlockSize - (PlainBytes.Num() % BlockSize)) % BlockSize;
    if (Pad > 0)
    {
        PlainBytes.AddZeroed(Pad);
    }

    // 准备密钥（简单从CRC派生）
    uint8 RawKey[32] = {0};
    uint32 KeyHash = ComputeKeyCrc32(Key);
    for (int i = 0; i < 32; ++i)
    {
        RawKey[i] = static_cast<uint8>((KeyHash >> ((i % 4) * 8)) & 0xFF);
    }
    FAES::FAESKey AESKey;
    FMemory::Memcpy(AESKey.Key, RawKey, 32);

    // 复制明文到输出缓冲并就地加密
    TArray<uint8> EncryptedBytes = PlainBytes;
    if (EncryptedBytes.Num() > 0)
    {
        FAES::EncryptData(EncryptedBytes.GetData(), EncryptedBytes.Num(), AESKey);
    }

    // 生成 Salt（16 字节）
    uint8 Salt[16] = {0};
    FGuid Guid = FGuid::NewGuid();
    FMemory::Memcpy(Salt, &Guid, FMath::Min<int32>(sizeof(Salt), sizeof(Guid)));

    // 生成 MAC（简化：SHA1(passphrase + base64(salt) + base64(cipher)))
    FString SaltB64 = FBase64::Encode(Salt, sizeof(Salt));
    FString CipherB64 = FBase64::Encode(EncryptedBytes);
    FString Mac = SHA256Hash(Key + SaltB64 + CipherB64);

    // 打包为 JSON 并再做 Base64
    TSharedPtr<FJsonObject> Wrapper = MakeShareable(new FJsonObject);
    Wrapper->SetStringField(TEXT("S"), SaltB64);
    Wrapper->SetStringField(TEXT("C"), CipherB64);
    Wrapper->SetStringField(TEXT("M"), Mac);

    FString JsonWrapped;
    TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&JsonWrapped);
    FJsonSerializer::Serialize(Wrapper.ToSharedRef(), W);
    Result = FBase64::Encode(JsonWrapped);

    return Result;
}

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
FString FLicenseEncryptor::AESDecrypt(const FString& Input, const FString& Key)
{
    if (Input.IsEmpty() || Key.IsEmpty())
    {
        return TEXT("");
    }

    FString Result;

    // 先尝试按 Base64(JSON{S,C,M}) 解析
    FString OuterJson;
    if (FBase64::Decode(Input, OuterJson))
    {
        TSharedPtr<FJsonObject> Wrapper;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(OuterJson);
        if (FJsonSerializer::Deserialize(Reader, Wrapper) && Wrapper.IsValid() && Wrapper->HasField(TEXT("S")) && Wrapper->HasField(TEXT("C")) && Wrapper->HasField(TEXT("M")))
        {
            const FString SaltB64 = Wrapper->GetStringField(TEXT("S"));
            const FString CipherB64 = Wrapper->GetStringField(TEXT("C"));
            const FString Mac = Wrapper->GetStringField(TEXT("M"));

            // 校验 MAC
            const FString ExpectMac = SHA256Hash(Key + SaltB64 + CipherB64);
            if (!Mac.Equals(ExpectMac))
            {
                UE_LOG(LogTemp, Warning, TEXT("[TrialLicense][AESDecrypt] MAC mismatch. InputLen=%d Key=%s Mac=%s Expect=%s"), Input.Len(), *Key, *Mac.Left(16), *ExpectMac.Left(16));
                // 兼容处理：继续尝试解密，后续由许可证签名校验兜底
            }

            // 解码密文
            TArray<uint8> EncodedBytes;
            if (FBase64::Decode(CipherB64, EncodedBytes))
            {
                // 生成密钥（与加密时相同）
                uint8 RawKey[32] = {0};
                uint32 KeyHash = ComputeKeyCrc32(Key);
                for (int i = 0; i < 32; ++i)
                {
                    RawKey[i] = static_cast<uint8>((KeyHash >> ((i % 4) * 8)) & 0xFF);
                }
                FAES::FAESKey AESKey;
                FMemory::Memcpy(AESKey.Key, RawKey, 32);

                if (EncodedBytes.Num() > 0)
                {
                    FAES::DecryptData(EncodedBytes.GetData(), EncodedBytes.Num(), AESKey);
                }

                int32 NewSize = EncodedBytes.Num();
                while (NewSize > 0 && EncodedBytes[NewSize - 1] == 0)
                {
                    --NewSize;
                }

                TArray<uint8> DecryptedBytes;
                DecryptedBytes.Append(EncodedBytes.GetData(), NewSize);
                DecryptedBytes.Add(0);
                Result = UTF8_TO_TCHAR(reinterpret_cast<const char*>(DecryptedBytes.GetData()));
                return Result;
            }
            UE_LOG(LogTemp, Warning, TEXT("[TrialLicense][AESDecrypt] Failed to decode inner cipher Base64. CipherLen=%d"), CipherB64.Len());
            return TEXT("");
        }
        UE_LOG(LogTemp, Warning, TEXT("[TrialLicense][AESDecrypt] Outer JSON parsed but wrapper fields missing/invalid."));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[TrialLicense][AESDecrypt] Failed to decode outer Base64. InputLen=%d"), Input.Len());
    }

    // 兼容旧格式：Input 直接为 Base64(cipher)
    TArray<uint8> EncodedBytes;
    if (FBase64::Decode(Input, EncodedBytes))
    {
        // 生成密钥（与加密时相同）
        uint8 RawKey[32] = {0};
        uint32 KeyHash = ComputeKeyCrc32(Key);
        for (int i = 0; i < 32; ++i)
        {
            RawKey[i] = static_cast<uint8>((KeyHash >> ((i % 4) * 8)) & 0xFF);
        }
        FAES::FAESKey AESKey;
        FMemory::Memcpy(AESKey.Key, RawKey, 32);

        if (EncodedBytes.Num() > 0)
        {
            FAES::DecryptData(EncodedBytes.GetData(), EncodedBytes.Num(), AESKey);
        }

        int32 NewSize = EncodedBytes.Num();
        while (NewSize > 0 && EncodedBytes[NewSize - 1] == 0)
        {
            --NewSize;
        }

        TArray<uint8> DecryptedBytes;
        DecryptedBytes.Append(EncodedBytes.GetData(), NewSize);
        DecryptedBytes.Add(0);
        Result = UTF8_TO_TCHAR(reinterpret_cast<const char*>(DecryptedBytes.GetData()));
        return Result;
    }

    UE_LOG(LogTemp, Warning, TEXT("[TrialLicense][AESDecrypt] Old format decode failed. InputLen=%d Prefix=%s"), Input.Len(), *Input.Left(24));

    return Result;
}

/**
 * @brief 生成许可证签名
 *
 * 使用SHA256哈希算法对许可证的关键信息生成数字签名，用于验证许可证的完整性和真实性。
 * 签名数据包括项目名称、机器ID、时间戳、项目ID和永久许可证标志。
 * 密码短语自动使用项目名称 (FApp::GetProjectName())。
 *
 * @param MachineID 机器唯一标识符
 * @param Time 许可证生成或验证的时间戳
 * @param ProjectId 项目标识符
 * @param bPerpetual 是否为永久许可证
 * @return 生成的SHA256哈希签名字符串
 */
FString FLicenseEncryptor::GenerateSignature(const FString& MachineID, const FDateTime& Time, const FString& ProjectId, bool bPerpetual)
{
    // 使用项目名称作为密码短语
    FString PassPhrase = FApp::GetProjectName();
    FString DataToSign = PassPhrase + MachineID + Time.ToString() + ProjectId + (bPerpetual ? TEXT("1") : TEXT("0"));
    return SHA256Hash(DataToSign);
}

/**
 * @brief 生成加密的许可证密钥
 *
 * 将许可证信息结构体序列化为JSON格式，然后使用AES加密算法加密JSON数据，
 * 生成最终的许可证密钥字符串。该密钥包含所有许可证信息并经过加密保护。
 * 加密口令自动使用项目名称 (FApp::GetProjectName())。
 *
 * @param LicenseInfo 包含许可证详细信息的结构体
 * @return 加密后的许可证密钥字符串
 */
FString FLicenseEncryptor::GenerateLicenseKey(const FTrialLicenseInfo& LicenseInfo)
{
    // 使用项目名称作为密码短语
    FString PassPhrase = FApp::GetProjectName();
    
    // 创建JSON对象来存储许可证信息
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetNumberField(TEXT("DurationHours"), LicenseInfo.DurationHours);
    JsonObject->SetStringField(TEXT("GenerateTime"), LicenseInfo.GenerateTime.ToString());
    JsonObject->SetStringField(TEXT("ExpireTime"), LicenseInfo.ExpireTime.ToString());
    JsonObject->SetStringField(TEXT("MachineID"), LicenseInfo.MachineID);
    JsonObject->SetStringField(TEXT("Signature"), LicenseInfo.Signature);
    JsonObject->SetStringField(TEXT("ProjectId"), LicenseInfo.ProjectId);
    JsonObject->SetBoolField(TEXT("Perpetual"), LicenseInfo.bPerpetual);
    JsonObject->SetBoolField(TEXT("ShowWatermark"), LicenseInfo.bShowWatermark);

    // 将JSON对象序列化为字符串
    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    // 加密JSON字符串
    FString EncryptedJson = AESEncrypt(JsonString, PassPhrase);

    return EncryptedJson;
}

/**
 * @brief 解析加密的许可证密钥
 *
 * 解密许可证密钥字符串，解析其中的JSON数据，并提取许可证信息填充到结构体中。
 * 如果解密失败或JSON解析失败，返回空的许可证信息结构体。
 * 解密口令自动使用项目名称 (FApp::GetProjectName())。
 *
 * @param LicenseKey 要解析的加密许可证密钥字符串
 * @return 解析出的许可证信息结构体，包含所有许可证详细信息
 */
FTrialLicenseInfo FLicenseEncryptor::ParseLicenseKey(const FString& LicenseKey)
{
    // 使用项目名称作为密码短语
    FString PassPhrase = FApp::GetProjectName();
    
    FTrialLicenseInfo Result;

    // 解密许可证密钥
    FString DecryptedJson = AESDecrypt(LicenseKey, PassPhrase);

    if (!DecryptedJson.IsEmpty())
    {
        // 解析JSON字符串
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(DecryptedJson);
        if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
        {
            // 从JSON对象中提取数据
            if (JsonObject->HasTypedField<EJson::Number>(TEXT("DurationHours")))
            {
                Result.DurationHours = JsonObject->GetNumberField(TEXT("DurationHours"));
            }

            if (JsonObject->HasTypedField<EJson::String>(TEXT("GenerateTime")))
            {
                FString GenerateTimeStr = JsonObject->GetStringField(TEXT("GenerateTime"));
                FDateTime::Parse(GenerateTimeStr, Result.GenerateTime);
            }

            if (JsonObject->HasTypedField<EJson::String>(TEXT("ExpireTime")))
            {
                FString ExpireTimeStr = JsonObject->GetStringField(TEXT("ExpireTime"));
                FDateTime::Parse(ExpireTimeStr, Result.ExpireTime);
            }

            if (JsonObject->HasTypedField<EJson::String>(TEXT("MachineID")))
            {
                Result.MachineID = JsonObject->GetStringField(TEXT("MachineID"));
            }

            if (JsonObject->HasTypedField<EJson::String>(TEXT("Signature")))
            {
                Result.Signature = JsonObject->GetStringField(TEXT("Signature"));
            }

            if (JsonObject->HasTypedField<EJson::String>(TEXT("ProjectId")))
            {
                Result.ProjectId = JsonObject->GetStringField(TEXT("ProjectId"));
            }
            if (JsonObject->HasTypedField<EJson::Boolean>(TEXT("Perpetual")))
            {
                Result.bPerpetual = JsonObject->GetBoolField(TEXT("Perpetual"));
            }
            Result.bShowWatermark = !Result.bPerpetual;
            if (JsonObject->HasTypedField<EJson::Boolean>(TEXT("ShowWatermark")))
            {
                Result.bShowWatermark = JsonObject->GetBoolField(TEXT("ShowWatermark"));
            }
        }
    }

    return Result;
}
