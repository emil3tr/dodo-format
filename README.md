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
+ seperates *syntax* from *semantics*. The dodo syntax specification specifies how dodo is parsed into a tree of commands.
The dodo semantics specification gives meaning to some standard commands. This means that
  + dodo can be used for styling, but also for configuration files.
  
# Development

I am currently working on finishing the dodo specifications and writing a sample
parser implementation. Later, there should be a program to convert dodo to pdf and html available and a wysiwyg online editor.
