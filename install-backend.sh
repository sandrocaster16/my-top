#!/usr/bin/env bash

set -euo pipefail

REPO="https://github.com/sandrocaster16/my-top.git"
CLONE_DIR="my-top-backend"
INSTALL_DIR="/usr/local/bin"
BINARY_NAME="my-top"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

info()    { echo -e "${GREEN}[+]${NC} $1"; }
warning() { echo -e "${YELLOW}[!]${NC} $1"; }
error()   { echo -e "${RED}[✗]${NC} $1"; exit 1; }

detect_pkg_manager(){
    if   command -v apt-get &>/dev/null; then echo "apt"
    elif command -v dnf     &>/dev/null; then echo "dnf"
    elif command -v pacman  &>/dev/null; then echo "pacman"
    elif command -v zypper  &>/dev/null; then echo "zypper"
    else error "Пакетный менеджер не найден. Поддерживаются: apt, dnf, pacman, zypper."
    fi
}

install_deps(){
    local pm=$1
    info "Устанавливаем зависимости через $pm..."

    case $pm in
        apt)
            sudo apt-get update -qq
            sudo apt-get install -y git cmake g++ curl
            ;;
        dnf)
            sudo dnf install -y git cmake gcc-c++ curl
            ;;
        pacman)
            sudo pacman -Sy --noconfirm git cmake base-devel curl
            ;;
        zypper)
            sudo zypper install -y git cmake gcc-c++ curl
            ;;
    esac
}

clone_repo(){
    if [ -d "$CLONE_DIR" ]; then
        warning "Директория $CLONE_DIR уже существует, пропускаем клонирование."
        return
    fi

    info "Клонируем репозиторий (только backend/)..."
    git clone --filter=blob:none --sparse "$REPO" "$CLONE_DIR"
    cd "$CLONE_DIR"
    git sparse-checkout set backend
    cd ..
}

fetch_headers(){
    local vendor_dir="$CLONE_DIR/backend/vendor"
    mkdir -p "$vendor_dir/nlohmann"

    if [ ! -f "$vendor_dir/httplib.h" ]; then
        info "Скачиваем httplib.h..."
        curl -fsSL "https://raw.githubusercontent.com/yhirose/cpp-httplib/v0.18.3/httplib.h" \
             -o "$vendor_dir/httplib.h"
    fi

    if [ ! -f "$vendor_dir/nlohmann/json.hpp" ]; then
        info "Скачиваем nlohmann/json.hpp..."
        curl -fsSL "https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp" \
             -o "$vendor_dir/nlohmann/json.hpp"
    fi
}

build(){
    info "Собираем бэкенд..."
    cd "$CLONE_DIR/backend"
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel "$(nproc)"
    cd ../..
}

install_binary(){
    local binary="$CLONE_DIR/backend/build/app"

    if [ ! -f "$binary" ]; then
        error "Бинарник не найден: $binary"
    fi

    info "Устанавливаем $BINARY_NAME в $INSTALL_DIR..."
    sudo install -m 755 "$binary" "$INSTALL_DIR/$BINARY_NAME"
}

main(){
    echo "=== my-top backend installer ==="

    local pm
    pm=$(detect_pkg_manager)

    install_deps "$pm"
    clone_repo
    fetch_headers
    build
    install_binary

    echo ""
    info "Готово! Запуск: $BINARY_NAME"
    info "Опции: $BINARY_NAME --help"
}

main
