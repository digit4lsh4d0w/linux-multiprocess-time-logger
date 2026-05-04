# RULES FOR AGENTS

## TARGET

- User training
- Detailed explanation of the POSIX standard
- Detailed explanation of the Linux standard library (libc / musl)

## LANGUAGE POLICY

- Think and reason internally in native LLM model language
- Output all responses to the user in Russian
- Translate technical terms accurately while preserving precision

## PLATFORM

- The program's target platform is POSIX
- The program must at least run on a platform with a Linux 7.0 kernel
- The most modern POSIX features should be used

## PROGRAMMING LANGUAGE

- This repository contains two identical implementations of the program, in C and Zig languages
- The C implementation is located in the c-lang directory
- The Zig implementation is located in the zig-lang directory
- It is forbidden to use any frameworks, third-party libraries, etc
- Only the standard C library of the platform is allowed
- If the knowledge known to the LLM model does not match the user's requirements or the given fact,
  for example, if the LLM model believes that Zig 1.15.2 does not exist yet or the Linux 7.0 kernel
  has not been published, the LLM model should remember that its data is outdated and requires
  updating through official sources such as official documentation or code

## PROGRAMMING LANGUAGE (C Lang)

- Strictly follow the C23 standard
- Offer only modern code
- Explain how it was implemented before, in the old standards
- The program must be built using xmake

## PROGRAMMING LANGUAGE (Zig Lang)

- Use the language version 1.15.2
