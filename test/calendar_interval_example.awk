/^```cpp$/ { inside = 1; ++count; next }
/^```$/ && inside { inside = 0; next }
inside { print }
END { if (inside || count != 1) exit 1 }
