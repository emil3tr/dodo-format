# dodo-format

dodo (doubledot format) is a text description format. It is designed for styling and organizing text. dodo is inspired by markdown
and html and aims to offer a simpler syntax than markown+html+custom_shortcodes, especially for writing web content.

Like **markdown**, dodo
+ is easy to learn.
+ has minimal (visual) overhead.
+ is human-readable as plaintext, even for someone who does not know dodo.

Like **html**, dodo
+ structures a document as a tree of commands.
+ allows advanced styling through arguments.

**Unique**ly, dodo
+ only uses *one* special character, the colon. However, a colon like this: does not start a command. This means that
  + minimal escapes are needed.
  + dodo is compatible with other formats (like markdown or html) and technologies (like katex).
  + if you paste something, you most likely don't break anything.
  + it is rather easy to parse. 
+ separates *syntax* from *semantics*. The dodo syntax specification specifies how dodo is parsed into a tree of commands.
The dodo semantics specification gives meaning to some standard commands. This means that
  + dodo can be used for styling, but also for configuration files.
  
# Development

The parser currently works for the tested inputs. However, it still needs

+ more testing for correctness and edge cases.
+ a better buffer interface.
+ a lot of rewriting to improve readability and code quality.
+ once the buffer interface is clear, major rewriting of the main parsing functions to improve speed
  and strategies to reduce copying.
+ specialized algorithms to make the common case fast.

The format also still misses

+ a finished syntax guide.
+ a finished semantics guide.
+ examples.
