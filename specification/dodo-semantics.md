# Dodo Semantis Specification

This specification explains the standard dodo semantics.

## Inline Short Forms

The following inline short forms shall be equivalent to their named counterparts.

+ `*` to `i` to `italic`
+ `=` to `b` to `bold`
+ `_` to `u` to `underline`
+ `-` to `s` to `strikethrough`
+ `/` to `c` to `code`

## Inline Commands

The following inline commands shall have a fixed meaning.

+ `italic` is italic text
+ `bold` is bold text
+ `underline` is underlined text
+ `strikethrough` is crossed out text
+ `code` is code text
+ `link` is a link where its argument is the destination

## Block Commands

The following block commands shall have a fixed meaning.

**Lists**

+ `list` is an unordered list
+ `ul` is an unordered list
+ `ol` is an ordered list
+ `text` as child of `list` or `ul` or `ol` is a list child
+ `item` as child of a list is a list child as well

**Items**

+ `item` as child of a command that expects items is one item of that command

**Blocks**

+ `block` (also `::`) as a child of a command that expects text shall be a highlighted text block (as `>` is .md)
+ commands may expect `block` as a grouping of children instead, in which case it shall behave like `item`

**Code**

+ Text inside `code` shall be interpreted as code and displayed in an indent-preserving and codelike manner
