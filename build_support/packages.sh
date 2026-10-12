#!/usr/bin/env bash
# ============================================================================
# packages.sh —— HDB 依赖一键安装（需要 root，供学生sudo执行）
# HDB 的依赖刻意保持极简：编译器 / CMake / Make / GTest（仅测试需要）。
#
# 用法：
#   sudo ./build_support/packages.sh          # 自动识别发行版
#   ./build_support/packages.sh --print       # 只打印命令不执行
# ============================================================================
set -euo pipefail

PKGS_PRINT="${1:-}"

say() { echo "[HDB-deps] $*"; }

install_dnf() {
    say "检测到 Fedora/RHEL 系，使用 dnf"
    local pkgs=(gcc-c++ cmake make gtest-devel)
    echo "dnf install -y ${pkgs[*]}"
    [[ "$PKGS_PRINT" == "--print" ]] || dnf install -y "${pkgs[@]}"
}

install_apt() {
    say "检测到 Debian/Ubuntu 系，使用 apt"
    local pkgs=(g++ cmake make libgtest-dev)
    echo "apt-get update && apt-get install -y ${pkgs[*]}"
    [[ "$PKGS_PRINT" == "--print" ]] || { apt-get update && apt-get install -y "${pkgs[@]}"; }
}

install_zypper() {
    say "检测到 openSUSE 系，使用 zypper"
    local pkgs=(gcc-c++ cmake make gtest)
    echo "zypper install -y ${pkgs[*]}"
    [[ "$PKGS_PRINT" == "--print" ]] || zypper install -y "${pkgs[@]}"
}

install_pacman() {
    say "检测到 Arch 系，使用 pacman"
    local pkgs=(gcc cmake make gtest)
    echo "pacman -Sy --needed ${pkgs[*]}"
    [[ "$PKGS_PRINT" == "--print" ]] || pacman -Sy --needed "${pkgs[@]}"
}

case "$(grep -E '^ID=' /etc/os-release 2>/dev/null | cut -d= -f2 | tr -d '\"' || echo unknown)" in
    fedora | rhel | centos | rocky | almalinux) install_dnf ;;
    debian | ubuntu | linuxmint | deepin) install_apt ;;
    opensuse*) install_zypper ;;
    arch | manjaro) install_pacman ;;
    *)
        say "无法识别发行版。请手动安装：g++(>=9) cmake(>=3.16) make gtest"
        say "Ubuntu: sudo apt install g++ cmake make libgtest-dev"
        say "Fedora: sudo dnf install gcc-c++ cmake make gtest-devel"
        exit 1
        ;;
esac

say "完成。验证：g++ --version && cmake --version"
say "随后回到仓库根目录执行 ./build.sh 开始构建"
