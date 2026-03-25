"""
邮件通知服务
"""

import smtplib
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
from typing import List, Optional
from datetime import datetime
from app.config import settings
from app.services.config_service import get_runtime_system_config
import logging

logger = logging.getLogger(__name__)


class EmailService:
    """邮件服务类"""

    def __init__(self):
        self.smtp_server = getattr(settings, "SMTP_SERVER", "smtp.gmail.com")
        self.smtp_port = getattr(settings, "SMTP_PORT", 587)
        self.smtp_username = getattr(settings, "SMTP_USERNAME", "")
        self.smtp_password = getattr(settings, "SMTP_PASSWORD", "")
        self.from_email = getattr(settings, "FROM_EMAIL", self.smtp_username)
        self.from_name = getattr(settings, "FROM_NAME", "License Service")

    def _load_runtime_config(self):
        runtime = get_runtime_system_config()
        return {
            "enabled": bool(runtime.get("smtp_enabled")),
            "smtp_server": runtime.get("smtp_server") or self.smtp_server,
            "smtp_port": runtime.get("smtp_port") or self.smtp_port,
            "smtp_username": runtime.get("smtp_username") or self.smtp_username,
            "smtp_password": runtime.get("smtp_password") or self.smtp_password,
            "smtp_use_tls": bool(runtime.get("smtp_use_tls", True)),
            "from_email": runtime.get("from_email")
            or runtime.get("smtp_username")
            or self.from_email,
            "from_name": runtime.get("from_name") or self.from_name,
        }

    def send_email(
        self,
        to_email: str,
        subject: str,
        html_content: str,
        text_content: Optional[str] = None,
    ) -> bool:
        """
        发送邮件

        Args:
            to_email: 收件人邮箱
            subject: 邮件主题
            html_content: HTML内容
            text_content: 纯文本内容（可选）

        Returns:
            是否发送成功
        """
        try:
            runtime = self._load_runtime_config()
            if not runtime["enabled"]:
                logger.warning("SMTP 未启用，跳过邮件发送")
                return False

            # 创建邮件
            msg = MIMEMultipart("alternative")
            msg["Subject"] = subject
            msg["From"] = f"{runtime['from_name']} <{runtime['from_email']}>"
            msg["To"] = to_email

            # 添加文本内容
            if text_content:
                part1 = MIMEText(text_content, "plain", "utf-8")
                msg.attach(part1)

            # 添加HTML内容
            part2 = MIMEText(html_content, "html", "utf-8")
            msg.attach(part2)

            # 发送邮件（添加超时设置）
            with smtplib.SMTP(
                runtime["smtp_server"], runtime["smtp_port"], timeout=10
            ) as server:
                if runtime["smtp_use_tls"]:
                    server.starttls()
                if runtime["smtp_username"]:
                    server.login(runtime["smtp_username"], runtime["smtp_password"])
                server.send_message(msg)

            logger.info(f"邮件发送成功: {to_email}")
            return True

        except Exception as e:
            logger.error(f"邮件发送失败: {str(e)}")
            return False

    def send_license_activation_email(
        self,
        to_email: str,
        user_name: str,
        license_key: str,
        app_name: str,
        expire_date: Optional[datetime] = None,
    ) -> bool:
        """发送许可证激活邮件"""

        expire_info = ""
        if expire_date:
            expire_info = f"<p><strong>到期时间：</strong>{expire_date.strftime('%Y-%m-%d %H:%M:%S')}</p>"
        else:
            expire_info = "<p><strong>类型：</strong>永久许可证</p>"

        html_content = f"""
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset="utf-8">
            <style>
                body {{ font-family: Arial, sans-serif; line-height: 1.6; color: #333; }}
                .container {{ max-width: 600px; margin: 0 auto; padding: 20px; }}
                .header {{ background-color: #4CAF50; color: white; padding: 20px; text-align: center; }}
                .content {{ padding: 20px; background-color: #f9f9f9; }}
                .license-box {{ background-color: white; border: 2px solid #4CAF50; padding: 15px; margin: 20px 0; }}
                .license-key {{ font-size: 18px; font-weight: bold; color: #4CAF50; word-break: break-all; }}
                .footer {{ text-align: center; padding: 20px; color: #666; font-size: 12px; }}
            </style>
        </head>
        <body>
            <div class="container">
                <div class="header">
                    <h1>许可证激活成功</h1>
                </div>
                <div class="content">
                    <p>尊敬的 {user_name}，</p>
                    <p>您的许可证已成功激活！</p>
                    
                    <div class="license-box">
                        <p><strong>应用名称：</strong>{app_name}</p>
                        {expire_info}
                        <p><strong>许可证密钥：</strong></p>
                        <p class="license-key">{license_key}</p>
                    </div>
                    
                    <p>请妥善保管您的许可证密钥。如有任何问题，请联系我们的客户支持团队。</p>
                </div>
                <div class="footer">
                    <p>此邮件由系统自动发送，请勿回复。</p>
                    <p>&copy; 2024 License Service. All rights reserved.</p>
                </div>
            </div>
        </body>
        </html>
        """

        text_content = f"""
        许可证激活成功
        
        尊敬的 {user_name}，
        
        您的许可证已成功激活！
        
        应用名称：{app_name}
        许可证密钥：{license_key}
        
        请妥善保管您的许可证密钥。
        """

        return self.send_email(
            to_email=to_email,
            subject=f"【{app_name}】许可证激活成功",
            html_content=html_content,
            text_content=text_content,
        )

    def send_subscription_expiry_reminder(
        self,
        to_email: str,
        user_name: str,
        app_name: str,
        expire_date: datetime,
        days_left: int,
    ) -> bool:
        """发送订阅到期提醒"""

        html_content = f"""
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset="utf-8">
            <style>
                body {{ font-family: Arial, sans-serif; line-height: 1.6; color: #333; }}
                .container {{ max-width: 600px; margin: 0 auto; padding: 20px; }}
                .header {{ background-color: #FF9800; color: white; padding: 20px; text-align: center; }}
                .content {{ padding: 20px; background-color: #f9f9f9; }}
                .warning-box {{ background-color: #FFF3CD; border: 2px solid #FF9800; padding: 15px; margin: 20px 0; }}
                .footer {{ text-align: center; padding: 20px; color: #666; font-size: 12px; }}
                .button {{ display: inline-block; padding: 10px 20px; background-color: #FF9800; color: white; text-decoration: none; border-radius: 5px; margin-top: 10px; }}
            </style>
        </head>
        <body>
            <div class="container">
                <div class="header">
                    <h1>订阅即将到期提醒</h1>
                </div>
                <div class="content">
                    <p>尊敬的 {user_name}，</p>
                    
                    <div class="warning-box">
                        <p><strong>您的订阅即将在 {days_left} 天后到期！</strong></p>
                        <p><strong>应用名称：</strong>{app_name}</p>
                        <p><strong>到期时间：</strong>{expire_date.strftime("%Y-%m-%d %H:%M:%S")}</p>
                    </div>
                    
                    <p>为了避免服务中断，请及时续费。</p>
                    <p><a href="#" class="button">立即续费</a></p>
                </div>
                <div class="footer">
                    <p>此邮件由系统自动发送，请勿回复。</p>
                </div>
            </div>
        </body>
        </html>
        """

        return self.send_email(
            to_email=to_email,
            subject=f"【提醒】您的{app_name}订阅即将到期",
            html_content=html_content,
        )

    def send_license_expiry_reminder(
        self,
        to_email: str,
        user_name: str,
        app_name: str,
        expire_date: datetime,
        days_left: int,
    ) -> bool:
        """发送许可证即将到期提醒"""

        html_content = f"""
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset="utf-8">
            <style>
                body {{ font-family: Arial, sans-serif; line-height: 1.6; color: #333; }}
                .container {{ max-width: 600px; margin: 0 auto; padding: 20px; }}
                .header {{ background-color: #FF9800; color: white; padding: 20px; text-align: center; }}
                .content {{ padding: 20px; background-color: #f9f9f9; }}
                .warning-box {{ background-color: #FFF3CD; border: 2px solid #FF9800; padding: 15px; margin: 20px 0; }}
            </style>
        </head>
        <body>
            <div class="container">
                <div class="header"><h1>许可证即将到期提醒</h1></div>
                <div class="content">
                    <p>尊敬的 {user_name}，</p>
                    <div class="warning-box">
                        <p><strong>您的许可证将在 {days_left} 天内到期。</strong></p>
                        <p><strong>应用名称：</strong>{app_name}</p>
                        <p><strong>到期时间：</strong>{expire_date.strftime("%Y-%m-%d %H:%M:%S")}</p>
                    </div>
                    <p>如需继续使用，请及时完成续费或重新购买。</p>
                </div>
            </div>
        </body>
        </html>
        """

        return self.send_email(
            to_email=to_email,
            subject=f"【提醒】您的 {app_name} 许可证即将到期",
            html_content=html_content,
        )

    def send_license_expired_notice(
        self,
        to_email: str,
        user_name: str,
        app_name: str,
        expire_date: datetime,
    ) -> bool:
        """发送许可证已到期通知"""

        html_content = f"""
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset="utf-8">
            <style>
                body {{ font-family: Arial, sans-serif; line-height: 1.6; color: #333; }}
                .container {{ max-width: 600px; margin: 0 auto; padding: 20px; }}
                .header {{ background-color: #E45868; color: white; padding: 20px; text-align: center; }}
                .content {{ padding: 20px; background-color: #f9f9f9; }}
                .notice-box {{ background-color: #FFE3E7; border: 2px solid #E45868; padding: 15px; margin: 20px 0; }}
            </style>
        </head>
        <body>
            <div class="container">
                <div class="header"><h1>许可证已到期</h1></div>
                <div class="content">
                    <p>尊敬的 {user_name}，</p>
                    <div class="notice-box">
                        <p><strong>您的许可证已到期。</strong></p>
                        <p><strong>应用名称：</strong>{app_name}</p>
                        <p><strong>到期时间：</strong>{expire_date.strftime("%Y-%m-%d %H:%M:%S")}</p>
                    </div>
                    <p>如需继续使用，请重新购买或续费许可证。</p>
                </div>
            </div>
        </body>
        </html>
        """

        return self.send_email(
            to_email=to_email,
            subject=f"【通知】您的 {app_name} 许可证已到期",
            html_content=html_content,
        )

    def send_payment_success_email(
        self,
        to_email: str,
        user_name: str,
        order_no: str,
        amount: float,
        product_name: str,
    ) -> bool:
        """发送支付成功邮件"""

        html_content = f"""
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset="utf-8">
            <style>
                body {{ font-family: Arial, sans-serif; line-height: 1.6; color: #333; }}
                .container {{ max-width: 600px; margin: 0 auto; padding: 20px; }}
                .header {{ background-color: #4CAF50; color: white; padding: 20px; text-align: center; }}
                .content {{ padding: 20px; background-color: #f9f9f9; }}
                .order-box {{ background-color: white; border: 2px solid #4CAF50; padding: 15px; margin: 20px 0; }}
                .footer {{ text-align: center; padding: 20px; color: #666; font-size: 12px; }}
            </style>
        </head>
        <body>
            <div class="container">
                <div class="header">
                    <h1>支付成功</h1>
                </div>
                <div class="content">
                    <p>尊敬的 {user_name}，</p>
                    <p>您的订单已支付成功！</p>
                    
                    <div class="order-box">
                        <p><strong>订单号：</strong>{order_no}</p>
                        <p><strong>产品名称：</strong>{product_name}</p>
                        <p><strong>支付金额：</strong>¥{amount:.2f}</p>
                    </div>
                    
                    <p>感谢您的购买！</p>
                </div>
                <div class="footer">
                    <p>此邮件由系统自动发送，请勿回复。</p>
                </div>
            </div>
        </body>
        </html>
        """

        return self.send_email(
            to_email=to_email, subject="支付成功通知", html_content=html_content
        )


# 单例实例
email_service = EmailService()
