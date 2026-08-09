# SM_TD overview

> Documentation version: 0.6.5-SNAPSHOT

SM_TD is a QMK community module for reliable home-row modifiers, layer taps, and multi-tap actions. It decides whether an input is a tap or a hold by looking at both press and release timing.

## Why it exists

Fast typing commonly overlaps adjacent keys. Stock tap-hold behavior can interpret the first key as a hold as soon as a following key is pressed, even when the user intended two letters. SM_TD delays that decision long enough to use the release rhythm as additional evidence.

Key capabilities include:

- Human-friendly tap-tap versus hold-tap decisions for `MT()` and `LT()` behavior
- Multi-tap sequences and tap-then-hold actions
- Per-key and global timing configuration
- Optional dynamic release windows based on the user's typing rhythm
- Integration with Caps Word, layers, Leader, VIA/Vial, and the QMK processing pipeline
- Optional chordal hold rules for same-hand and opposite-hand rolls

## Timing model

The engine tracks a macro key and the key that follows it. Sequences A and B below have similar overlap and resolve as hold-tap. Sequence C keeps the following key pressed much longer and resolves as tap-tap. Dynamic release configuration can further adapt the decision window to the press rhythm.


```
         |     Sequence A.     |     Sequence B.     |      Sequence C.    |
0ms  - - | - ┌—————┐ - - - - - | - ┌—————┐ - - - - - | - ┌—————┐ - - - - - |
         |   │macro│           |   │macro│           |   │macro│           |
         |   │ key │           |   │ key │           |   │ key │           |
         |   │     │           |   │     │           |   │     │           |
10ms - - | - │     │ ┌—————┐ - | - │     │ ┌—————┐ - | - │     │ ┌—————┐ - |
         |   │     │ │foll.│   |   │     │ │foll.│   |   │     │ │foll.│   |
         |   │     │ │ key │   |   │     │ │ key │   |   │     │ │ key │   |
         |   │     │ │     │   |   │     │ │     │   |   │     │ │     │   |
         |   │     │ │     │   |   │     │ │     │   |   │     │ │     │   |
         |   │     │ │     │   |   │     │ │     │   |   │     │ │     │   |
         |   │     │ │     │   |   │     │ │     │   |   │     │ │     │   |
49ms - - | - │     │ └—————┘ - |   │     │ │     │   |   │     │ │     │   |
50ms - - | - └—————┘ - - - - - | - └—————┘ │     │ - | - └—————┘ │     │ - |
51ms - - | - - - - - - - - - - | - - - - - └—————┘ - |           │     │   |
         |                     |                     |           │     │   |
         |                     |                     |           │     │   |
         |                     |                     |           │     │   |
         |                     |                     |           │     │   |
200ms  - | - - - - - - - - - - | - - - - - - - - - - | - - - - - └—————┘ - |  
```


