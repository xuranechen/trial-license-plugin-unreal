"""加密服务兼容性测试。

这些测试锁定后端（Python ``EncryptionService``）与 UE 插件
（C++ ``FLicenseEncryptor``）之间的加密 / 签名格式契约：

    Base64( JSON{ S: base64(salt16), C: base64(AES-256-ECB(零填充明文)), M: sha256(passphrase+salt+ct) } )

任何一侧改动以下任一环节都会让测试失败，从而在发布前暴露
「后端签发成功、客户端验证失败」这类线上事故：

* 密钥派生（标准 CRC-32，低位在前的 4 字节循环填充）
* AES 模式（ECB，零填充，无 IV）
* JSON 字段名与 UE ``FDateTime::ToString`` 时间格式
* 签名拼接顺序：PassPhrase + MachineID + Time + ProjectId + ("1"/"0")
"""

import base64
import json
import zlib
from datetime import datetime, timedelta

import pytest

from app.services.encryption import EncryptionService


def _make_license(
    *,
    duration_hours: float = 72,
    expiry_date: datetime | None = None,
    passphrase: str = "MyGame",
    project_id: str = "MyGame",
    machine_id: str = "MACHINE-ABC",
    is_perpetual: bool = False,
    show_watermark: bool = True,
) -> str:
    return EncryptionService.generate_license_key(
        duration_hours=duration_hours,
        expiry_date=expiry_date or (datetime.now() + timedelta(days=30)),
        passphrase=passphrase,
        project_id=project_id,
        machine_id=machine_id,
        is_perpetual=is_perpetual,
        show_watermark=show_watermark,
    )


# ---------------------------------------------------------------------------
# 密钥派生 / 基础原语
# ---------------------------------------------------------------------------


def test_crc32_matches_standard_polynomial():
    """UE 的 FCrc::MemCrc32 即标准 CRC-32（IEEE 802.3），与 zlib.crc32 一致。"""
    assert zlib.crc32(b"123456789") & 0xFFFFFFFF == 0xCBF43926


class TestKeyDerivation:
    def test_known_vectors(self):
        # key[i] = (crc32 >> ((i % 4) * 8)) & 0xFF，即 CRC 的 4 个字节
        # 以「低位在前」的顺序循环填充成 32 字节。
        assert EncryptionService.derive_aes_key("MyGame") == bytes.fromhex(
            "017b9b54" * 8
        )
        assert EncryptionService.derive_aes_key("DEMO:Project") == bytes.fromhex(
            "32c6afec" * 8
        )

    def test_key_is_256_bit(self):
        assert len(EncryptionService.derive_aes_key("anything")) == 32

    def test_non_ascii_passphrase_uses_utf8_bytes(self):
        """非 ASCII 口令必须按 UTF-8 字节派生密钥。

        对应 C++ ``ComputeKeyCrc32``：若误用 ``FString::Len()``（TCHAR 个数）
        而不是 UTF-8 字节长度，中文项目名会派生出不同的密钥。
        """
        passphrase = "中文项目"
        assert passphrase.encode("utf-8").hex() == "e4b8ade69687e9a1b9e79bae"
        assert zlib.crc32(passphrase.encode("utf-8")) & 0xFFFFFFFF == 0x77DA9753
        assert EncryptionService.derive_aes_key(passphrase) == bytes.fromhex(
            "5397da77" * 8
        )

    def test_non_ascii_passphrase_roundtrip(self):
        token = EncryptionService.aes_encrypt("你好，世界", "中文项目")
        assert EncryptionService.aes_decrypt(token, "中文项目") == "你好，世界"


# ---------------------------------------------------------------------------
# 线上传输格式 Base64(JSON{S,C,M})
# ---------------------------------------------------------------------------


class TestWireFormat:
    # 固定 salt(0..15) + 固定明文生成的测试向量，用于锁死解密路径。
    GOLDEN_TOKEN = (
        "eyJTIjoiQUFFQ0F3UUZCZ2NJQ1FvTERBME9Edz09IiwiQyI6IjBvdzF5aHpQWXMrYmRLOHgyM0FO"
        "YStWL096MHJzM1lnQmxCV092NnJDcmgyeGh5dTNqa1lTSWUyQkl2QkRxZmt2VEVRMmFEbnVlNVdr"
        "d0FUL0ZkYXZ3PT0iLCJNIjoiY2JiNjJmMDA4ZGYyODIzMzA3MTQ0YzU0N2M3ZmE3NDU4MGM2NWQ5"
        "ZmMyNTlkZDUxN2VjNDdiMTQyNzlkZGQ0MCJ9"
    )
    GOLDEN_PLAINTEXT = (
        '{"GenerateTime":"2026.10.27-09.00.00","MachineID":"MACHINE-ABC"}'
    )

    def test_decrypt_known_vector(self):
        assert (
            EncryptionService.aes_decrypt(self.GOLDEN_TOKEN, "MyGame")
            == self.GOLDEN_PLAINTEXT
        )

    def test_encrypt_wrapper_shape_and_mac(self):
        token = EncryptionService.aes_encrypt("hello", "MyGame")
        wrapper = json.loads(base64.b64decode(token).decode("utf-8"))

        assert set(wrapper) == {"S", "C", "M"}
        assert len(base64.b64decode(wrapper["S"])) == 16  # 16 字节 salt
        assert len(wrapper["M"]) == 64  # SHA-256 十六进制
        # MAC = sha256(passphrase + salt_b64 + cipher_b64)
        assert wrapper["M"] == EncryptionService.sha256_hash(
            "MyGame" + wrapper["S"] + wrapper["C"]
        )

    @pytest.mark.parametrize(
        "text",
        ["", "abc", "中文水印 测试", '{"a":1}', "line\nbreak", "x" * 500],
    )
    def test_roundtrip(self, text):
        token = EncryptionService.aes_encrypt(text, "MyGame")
        assert EncryptionService.aes_decrypt(token, "MyGame") == text

    def test_decrypt_tolerates_whitespace(self):
        # UE 侧可能输出被格式化的 Base64，解密需先剥离空白。
        token = EncryptionService.aes_encrypt("hello", "MyGame")
        wrapped = "\n ".join([token[:20], token[20:]])
        assert EncryptionService.aes_decrypt(wrapped, "MyGame") == "hello"

    def test_wrong_passphrase_raises(self):
        token = EncryptionService.aes_encrypt("hello", "MyGame")
        with pytest.raises(ValueError):
            EncryptionService.aes_decrypt(token, "OtherGame")


# ---------------------------------------------------------------------------
# 许可证结构与签名
# ---------------------------------------------------------------------------


class TestLicenseKey:
    def test_roundtrip_fields_and_ue_time_format(self):
        key = _make_license()
        info = EncryptionService.parse_license_key(key, "MyGame")

        assert info["DurationHours"] == 72
        assert info["MachineID"] == "MACHINE-ABC"
        assert info["ProjectId"] == "MyGame"
        assert info["Perpetual"] is False
        assert info["ShowWatermark"] is True
        # UE FDateTime::ToString 默认格式，秒精度
        datetime.strptime(info["GenerateTime"], "%Y.%m.%d-%H.%M.%S")
        datetime.strptime(info["ExpireTime"], "%Y.%m.%d-%H.%M.%S")

    def test_signature_valid_and_tamper_detected(self):
        info = EncryptionService.parse_license_key(_make_license(), "MyGame")
        assert EncryptionService.validate_license_signature(info, "MyGame")

        for field, value in (
            ("MachineID", "OTHER-MACHINE"),
            ("ProjectId", "OtherProject"),
            ("Perpetual", True),
        ):
            forged = dict(info)
            forged[field] = value
            assert not EncryptionService.validate_license_signature(
                forged, "MyGame"
            ), f"签名未覆盖字段 {field}"

    def test_legacy_bperpetual_field_is_accepted(self):
        generate_time = datetime(2026, 10, 27, 9, 0, 0)
        legacy = {
            "DurationHours": 0,
            "GenerateTime": EncryptionService._to_ue_datetime_string(generate_time),
            "ExpireTime": "2099.12.31-00.00.00",
            "MachineID": "MACHINE-ABC",
            "ProjectId": "MyGame",
            "bPerpetual": True,  # 历史字段名
            "Signature": EncryptionService.generate_signature(
                "MyGame", "MACHINE-ABC", generate_time, "MyGame", True
            ),
        }
        token = EncryptionService.aes_encrypt(json.dumps(legacy), "MyGame")
        info = EncryptionService.parse_license_key(token, "MyGame")

        assert info["Perpetual"] is True  # 已归一化为新字段
        assert EncryptionService.validate_license_signature(info, "MyGame")

    def test_missing_required_field_rejected(self):
        broken = {
            "DurationHours": 72,
            "GenerateTime": "2026.10.27-09.00.00",
            # 缺 ExpireTime / MachineID / ProjectId / Signature
        }
        token = EncryptionService.aes_encrypt(json.dumps(broken), "MyGame")
        with pytest.raises(ValueError):
            EncryptionService.parse_license_key(token, "MyGame")

    def test_datetime_parser_supports_ue_and_iso(self):
        assert EncryptionService._parse_license_datetime(
            "2026.10.27-09.00.00"
        ) == datetime(2026, 10, 27, 9, 0, 0)
        assert EncryptionService._parse_license_datetime(
            "2026-10-27T09:00:00Z"
        ) == datetime(2026, 10, 27, 9, 0, 0)


class TestValidation:
    def test_valid_license(self):
        ok, message, info = EncryptionService.validate_license(
            _make_license(), "MyGame", "MyGame", "MACHINE-ABC"
        )
        assert ok, message
        assert info
        assert EncryptionService.calculate_remaining_time(info) > 0

    def test_expired_license_rejected(self):
        key = _make_license(expiry_date=datetime.now() - timedelta(hours=1))
        ok, message, _ = EncryptionService.validate_license(
            key, "MyGame", "MyGame", "MACHINE-ABC"
        )
        assert not ok
        assert "过期" in message

    def test_project_mismatch_rejected(self):
        ok, message, _ = EncryptionService.validate_license(
            _make_license(), "MyGame", "OtherProject", "MACHINE-ABC"
        )
        assert not ok
        assert "项目" in message

    def test_machine_mismatch_rejected(self):
        ok, message, _ = EncryptionService.validate_license(
            _make_license(), "MyGame", "MyGame", "OTHER-MACHINE"
        )
        assert not ok
        assert "设备" in message

    def test_wrong_passphrase_rejected(self):
        ok, _, _ = EncryptionService.validate_license(
            _make_license(), "OtherGame", "MyGame", "MACHINE-ABC"
        )
        assert not ok

    def test_perpetual_ignores_expiry(self):
        key = _make_license(
            duration_hours=0,
            expiry_date=datetime(2099, 12, 31),
            is_perpetual=True,
            show_watermark=False,
        )
        ok, message, info = EncryptionService.validate_license(
            key, "MyGame", "MyGame", "MACHINE-ABC"
        )
        assert ok, message
        assert info["Perpetual"] is True
        assert EncryptionService.calculate_remaining_time(info) == -1.0


# ---------------------------------------------------------------------------
# 试用请求令牌
# ---------------------------------------------------------------------------


class TestEncryptedToken:
    @staticmethod
    def _token(machine_id: str = "MACHINE-ABC", passphrase: str = "MyGame") -> str:
        payload = json.dumps(
            {
                "ProjectId": passphrase,
                "MachineID": machine_id,
                "Time": "2026.10.27-09.00.00",
            }
        )
        return EncryptionService.aes_encrypt(payload, passphrase)

    def test_parse_machine_id(self):
        assert (
            EncryptionService.parse_encrypted_token(self._token(), "MyGame")
            == "MACHINE-ABC"
        )

    def test_rejects_garbage(self):
        with pytest.raises(ValueError):
            EncryptionService.parse_encrypted_token("not-a-token", "MyGame")

    def test_rejects_missing_machine_id(self):
        token = EncryptionService.aes_encrypt(
            json.dumps({"ProjectId": "MyGame"}), "MyGame"
        )
        with pytest.raises(ValueError):
            EncryptionService.parse_encrypted_token(token, "MyGame")
