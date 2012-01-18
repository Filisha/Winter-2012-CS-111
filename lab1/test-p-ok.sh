#! /bin/sh

# UCLA CS 111 Lab 1 - Test that valid syntax is processed correctly.

tmp=$0-$$.tmp
mkdir "$tmp" || exit

(
cd "$tmp" || exit

cat >test.sh <<'EOF'
true

g++ -c foo.c

: : :

cat < /etc/passwd | tr a-z A-Z | sort -u || echo sort failed!

a b<c > d

cat < /etc/passwd | tr a-z A-Z | sort -u > out || echo sort failed!

a&&b||
 c &&
  d | e && f|

g<h

# This is a weird example: nobody would ever want to run this.
a<b>c|d<e>f|g<h>i

# Check ignoring multiple comments
# Sanity Check
a && b || c > files    #comment tester

# Use of parenthesis
a || ( b && (c || d) || e ) > files         

# Use of semicolons
(cat a; cat b)

# Use of pipes
a || b && (c || d) && e; f && g | h || i

# Long Strings / Memory Allocation
abcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcde &&
abcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcde

EOF

cat >test.exp <<'EOF'
# 1
  true
# 2
  g++ -c foo.c
# 3
  : : :
# 4
      cat</etc/passwd \
    |
      tr a-z A-Z \
    |
      sort -u \
  ||
    echo sort failed!
# 5
  a b<c>d
# 6
      cat</etc/passwd \
    |
      tr a-z A-Z \
    |
      sort -u>out \
  ||
    echo sort failed!
# 7
        a \
      &&
        b \
    ||
      c \
  &&
      d \
    |
      e \
  &&
      f \
    |
      g<h
# 8
    a<b>c \
  |
    d<e>f \
  |
    g<h>i
# 9
      a \
    &&
      b \
  ||
    c>files
# 10
    a \
  ||
    (
         b \
       &&
         (
            c \
          ||
            d
         ) \
     ||
       e
    )>files
# 11
  (
     cat a \
   ;
     cat b
  )
# 12
      a \
    ||
      b \
  &&
    (
       c \
     ||
       d
    ) \
  &&
    e
# 13
      f \
    &&
        g \
      |
        h \
  ||
    i
# 14
    abcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcde \
  &&
    abcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcdeabcde
EOF

../timetrash -p test.sh >test.out 2>test.err || exit

diff -u test.exp test.out || exit
test ! -s test.err || {
  cat test.err
  exit 1
}

) || exit

rm -fr "$tmp"
