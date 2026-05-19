#!/usr/bin/env bash

set -euo pipefail

REPO="https://github.com/sandrocaster16/my-top.git"
CLONE_DIR="my-top-frontend"

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
            sudo apt-get install -y git nodejs npm
            ;;
        dnf)
            sudo dnf install -y git nodejs npm
            ;;
        pacman)
            sudo pacman -Sy --noconfirm git nodejs npm
            ;;
        zypper)
            sudo zypper install -y git nodejs npm
            ;;
    esac

    info "Устанавливаем TypeScript..."
    sudo npm install -g typescript
}

clone_repo(){
    if [ -d "$CLONE_DIR" ]; then
        warning "Директория $CLONE_DIR уже существует, пропускаем клонирование."
        return
    fi

    info "Клонируем репозиторий (только frontend/)..."
    git clone --filter=blob:none --sparse "$REPO" "$CLONE_DIR"
    cd "$CLONE_DIR"
    git sparse-checkout set frontend
    cd ..
}

build(){
    info "Компилируем TypeScript..."
    cd "$CLONE_DIR/frontend"
    tsc
    cd ../..
}

main(){
    echo "=== my-top frontend installer ==="

    local pm
    pm=$(detect_pkg_manager)

    install_deps "$pm"
    clone_repo
    build

    local index
    index="$(pwd)/$CLONE_DIR/frontend/index.html"

    echo ""
    info "Готово!"
    info "Открой в браузере: $index"
}

main
