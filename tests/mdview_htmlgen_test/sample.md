---
title: mdview sample
author: Open Salamander
---

# mdview HTML Renderer

A rendered Markdown sample demonstrating the **new WebView2 pipeline**:
real tables, margins, *typography*, `inline code`, ~~strikethrough~~, and
faithful embedded HTML. Press <kbd>Esc</kbd> to close, <kbd>Ctrl</kbd>+<kbd>F</kbd>
to search.

## Feature comparison

| Feature            | v1 (RTF/RichEdit) |      v2 (WebView2) |
| :----------------- | :---------------: | -----------------: |
| Tables             |    ASCII text     |      **real grid** |
| Margins            |       none        | reading measure ✓ |
| Embedded HTML      |   literal text    |         rendered ✓ |
| Inline images      |   placeholder □   |            inline ✓ |

## Code with highlighting

```c
// fenced code block, tier-1 language
int add(int a, int b) {
    return a + b; // returns the sum
}
```

## Lists

- Unordered item
  - Nested item
- Another item

1. First
2. Second

- [x] Implemented md4c parser
- [x] WebView2 lockdown
- [ ] Ship it

> A blockquote renders with a left accent bar and quote color.
> Second line of the same quote.

Superscript E = mc<sup>2</sup> and subscript H<sub>2</sub>O via embedded HTML.

A [safe external link](https://example.com) and an [internal anchor](#lists).

---

Inline image (data URI, always safe/offline):
![red dot](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg==)
