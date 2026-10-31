# Compile the README's statement snippets directly, with one function per block.
# Headers are collected here so an include never appears inside a function.
BEGIN {
  print "#include \"roo_time.h\""
  print "#include \"roo_time/format.h\""
  print "#include \"roo_time/timezone.h\""
}
/^```cpp$/ {
  print "void ReadmeExample" ++count "() {"
  inside = 1
  next
}
/^```$/ && inside {
  print "}"
  inside = 0
  next
}
inside && !/^#include/ { print }
END {
  if (inside || count == 0) exit 1
}
