#!/bin/bash
# SBUS Joy 开机自启动卸载脚本
# 用法: sudo bash uninstall_autostart.sh

set -e

if [ "$EUID" != "0" ]; then
    echo "Error: 请使用 sudo 运行此脚本"
    echo "用法: sudo bash $0"
    exit 1
fi

SERVICE_NAME="sbus_joy"
TARGET_FILE="/etc/systemd/system/${SERVICE_NAME}.service"

echo "========================================"
echo "SBUS Joy 开机自启动卸载"
echo "========================================"

# 停止服务
echo "停止服务..."
systemctl stop "$SERVICE_NAME" 2>/dev/null || true

# 禁用服务
echo "禁用开机自启动..."
systemctl disable "$SERVICE_NAME" 2>/dev/null || true

# 删除 service 文件
if [ -f "$TARGET_FILE" ]; then
    echo "删除 service 文件..."
    rm "$TARGET_FILE"
fi

# 重新加载 systemd
echo "重新加载 systemd 配置..."
systemctl daemon-reload

echo ""
echo "========================================"
echo "卸载完成！"
echo "========================================"
echo ""
