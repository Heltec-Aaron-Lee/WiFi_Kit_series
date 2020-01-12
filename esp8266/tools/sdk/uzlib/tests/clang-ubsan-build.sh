# Build with Clang UBSan to catch any runtime issues.
# https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html

make clean
make CC="clang -fsanitize=undefined -fno-sanitize-recover=undefined"
