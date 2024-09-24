# Fractal (PS5)

This app explores the Mandelbrot set and was developed with SDL2/C for jailbroken PS5 (1.xx-4xx).

## Prerequisites
- elfldr.elf
- shsrv.elf
- ftpsrv.elf

## How to Use
1. Send the elfldr.elf payload to port 9020.
2. Send the shsrv.elf payload to port 9021.
3. Send the ftpsrv.elf payload to port 9021.
4. Copy the fractal.elf or fractal-fs.elf payload to `/data` directory on your PS5.
5. Execute `telnet [PS5-IP] 2323`.
6. cd `/data`.
6. Run the command `hbldr fractal.elf`.

## Controls

X - Zoom in

O - Zoom out

D-pad - Moves the cursor


## Credits
J. Tornblom - SDK and required payloads.
