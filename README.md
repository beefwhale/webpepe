# webpepe

A handwritten PNG and JPEG to WEBP image decoder that encodes to WebP using (libwebp)[https://github.com/webmproject/libwebp]

## purpose
To make my blog faster! and also to understand image compression on a deeper level. 

## checklist
- [ ] png decoder
- [ ] cleaning data to allow jpg
- [ ] jpg encoder
- [ ] png to webp conversion
- [ ] jpg decpder
- [ ] png to webp conversion

## references

### png 
- https://iter.ca/post/png/
- https://www.libpng.org/pub/png/spec/1.2/PNG-Structure.html

### jpg
- https://yasoob.me/posts/understanding-and-writing-jpeg-decoder-in-python/ 
- https://www.youtube.com/watch?v=Q2aEzeMDHMA&t=10s 


## design decisions

- `size_t`: Using size_t is annoying since my compiler throws warnings whenever i default back to int on my variables but the choice of defining the vector with `size_t` that stores all of my raw image bytes is to ensure that my vector is always large enough on the file format of my target architecture (2^64 -1) where my image data chunk has a max of 2^31 bytes. It also doesnt allow negative values and so my vector can never be read with a size mismatch.

## debugging 

```
xxd -s 8 -l 7 dawg.png (view from absolute pos 8, for len of 8)
```
