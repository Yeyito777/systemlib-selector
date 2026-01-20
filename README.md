# Selector

A file selector utility that finds text files matching glob patterns while respecting `.gitignore` rules.

## Usage

```
selector [flags] [patterns...]
```

## Flags

- `-a` - Include all files (ignore `.gitignore` rules)

## Patterns

- `*` - Match all files (default if no pattern provided)
- `*.c` - Match all C files
- `src` or `src/` - Match all files in the `src` directory
- `src/*.h` - Match header files in `src`

## Features

- Respects `.gitignore` by default
- Automatically skips binary files (ELF, images, PDFs, etc.)
- Recursively searches directories
