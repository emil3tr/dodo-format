# Dodo Syntax Specification

This specification explains the syntax of the dodo format.

## Commands

Commands hold the structure of a dodo file. There are four types of commands:

+ **Block** commands may have other block / text / code commands as children.
+ **Text** commands may have text and inline commands as children.
+ **Code** commands may only have text as children.
+ **Inline** commands may have inline commands or text as children.

Two special types of commands are:

+ **InlineEmpty** commands are inline commands that cannot hold text or children.
+ The **root** commands is a block command with indent -1 that starts at the start of the file
and ends at the end of the file. It is the root of the command tree

A sample command tree might look like this:

```
root
| block
| | text
| | | inline
| | | text
| | | inline
| | | | text
| | block
| block
| | block
```

### Declaring Commands

### Indent

If a block command starts and has higher indent than the current active command, it shall be a child of that
command. Otherwise all active commands with a higher or equal indent to the new command end and the
new command is a child of the first command with lower indent than it. Note that

+ the root command always has the lowest indent of any command and
+ code / text commands will end commands with higher or equal indent, but cannot have children themselves.