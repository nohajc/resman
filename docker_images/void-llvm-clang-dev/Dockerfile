FROM voidlinux/voidlinux-musl

RUN xbps-install -Syu && xbps-install -Syu && xbps-install -Sy zlib-devel ncurses-devel make cmake llvm clang

RUN useradd -m dev
USER dev
