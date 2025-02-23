# Reproduction of #17

## Description

See [LED functionality improvements](https://github.com/IoMT-Lab/neuralert_firmware/issues/17) with corresponding [pull request](https://github.com/IoMT-Lab/neuralert_firmware/pull/18).

## Usage

You can use the extracted LLVM IR in this directory to reproduce the issue.

```bash
# Version before the fix
clang -emit-llvm -c before_fix.ll -o before_fix.bc
clang -emit-llvm -c main.c -o main.bc
llvm-link before_fix.bc main.bc -o test.bc
klee test.bc
```

You are expected to see the following output:
```
KLEE: ERROR: (location information missing) ASSERTION FAIL: !BIT_SET(processLists, USER_PROCESS_BOOTUP) || ledColor == YELLOW
```

```bash
# Version after the fix
clang -emit-llvm -c after_fix.ll -o after_fix.bc
clang -emit-llvm -c main.c -o main.bc
llvm-link after_fix.bc main.bc -o test.bc
klee test.bc
```

If you want to manually extract the LLVM IR/Bytecode, you can checkout to the version and use the following commands:

```bash
extract-bc neuralert.elf
llvm-extract --glob=processLists --glob=pUserData --glob=ledColor --func=notify_user_LED --func=setLEDState -o before_fix.bc --recursive neuralert.elf.bc
```
