- You don't have to use the full allocation of heap memory, you can alloc in
  chunks and avoid having to realloc memory every single character.
- Seperation of concerns is brutally important, I understand this and implement
  it when I properly plan out my code but I realized very quickly not having a
  difference from the actual text buffer and what was being drawn is awful.
- Planning isn't the only time for thinking, think when coding too.
- Actually understand what you're writing, do not do anything without absolute
  assurity, or else you end up with basically all the insane decisions made
  here.
- C++ sucks, C is superior.
