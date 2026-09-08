#!/bin/bash
# SBUS Joy 开机自启动安装脚本
# 用法: sudo bash install_autostart.sh

set -e

if [ "$EUID" != "0" ]; then
    echo "Error: 请使用 sudo 运行此脚本"
    echo "用法: sudo bash $0"
    exit 1
fi

SERVICE_NAME="sbus_joy"
SERVICE_FILE="${SERVICE_NAME}.service"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_FILE="${SCRIPT_DIR}/${SERVICE_FILE}"
TARGET_FILE="/etc/systemd/system/${SERVICE_FILE}"

echo "========================================"
echo "SBUS Joy 开机自启动安装"
echo "========================================"

# 检查 service 文件是否存在
if [ ! -f "$SOURCE_FILE" ]; then
    echo "Error: 找不到 service 文件: $SOURCE_FILE"
    exit 1
fi

# 复制 service 文件
echo "复制 service 文件到 /etc/systemd/system/"
cp "$SOURCE_FILE" "$TARGET_FILE"

# 重新加载 systemd
echo "重新加载 systemd 配置..."
systemctl daemon-reload

# 启用服务
echo "启用开机自启动..."
systemctl enable "$SERVICE_NAME"

# 启动服务
echo "启动服务..."
systemctl start "$SERVICE_NAME"

# 显示状态
echo ""
echo "========================================"
echo "安装完成！"
echo "========================================"
echo ""
echo "服务状态:"
systemctl status "$SERVICE_NAME" --no-pager

echo ""
echo "常用命令:"
echo "  查看状态: systemctl status $SERVICE_NAME"
echo "  查看日志: journalctl -u $SERVICE_NAME -f"
echo "  停止服务: systemctl stop $SERVICE_NAME"
echo "  启动服务: systemctl start $SERVICE_NAME"
echo "  禁用自启: systemctl disable $SERVICE_NAME"
echo "  重新启用: systemctl reenable $SERVICE_NAME"
echo ""
