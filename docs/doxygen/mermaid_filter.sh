#!/usr/bin/env sh
# Doxygen input filter for Markdown: a ```mermaid fence becomes a <pre class="mermaid"> block the
# footer's mermaid.js renders; the same file stays plain Markdown for GitHub and editors.
awk '
/^```mermaid[[:space:]]*$/ { print "\\htmlonly"; print "<pre class=\"mermaid\">"; inside = 1; next }
inside && /^```[[:space:]]*$/ { print "</pre>"; print "\\endhtmlonly"; inside = 0; next }
inside { gsub(/&/, "\\&amp;"); gsub(/</, "\\&lt;"); gsub(/>/, "\\&gt;"); print; next }
{ print }
' "$1"
