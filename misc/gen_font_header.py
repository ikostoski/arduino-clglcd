#
# Python script to generate clglcd_font.h
# Needs PIL/Pillow Python library
#
from PIL import Image, ImageDraw, ImageFont

### Font to convert ###

FONT_FILE     = "\\path\\to\\your_font.ttf"
FONT_ENCODING = "cp437" 
FONT_SIZE     = 16
FONT_PT_SIZE  = 12
FONT_Y_OFFSET = 0

#######################

if (240 % FONT_SIZE) != 0:
  raise Exception("Currenty only font sizes that fit as whole number in 240 lines are support")
screen_lines = 240 / FONT_SIZE

img = Image.new('1', (2048, FONT_SIZE), 0) 
font = ImageFont.truetype(FONT_FILE, FONT_PT_SIZE)
draw = ImageDraw.Draw(img)
for c in range(0, 256):
  str = bytes([c]).decode(FONT_ENCODING)
  draw.text((c*8, FONT_Y_OFFSET), str, font=font, fill=1)

# Draw/Fix/Replace/ your custom charactes here

# Save PNG image so you can inspect generated font
img.save('clglcd_font.png')

fp = open("clglcd_font.h", "w")
fp.write("//\n")
fp.write("// Font for 'Controllerless GLCD'\n")
fp.write("// Generated from '"+FONT_FILE+"'\n")
fp.write("//\n\n")
fp.write("#define CLGLCD_FONT_LINES %d\n" % FONT_SIZE)
fp.write("#define CLGLCD_Y_LINES    %d\n\n" % screen_lines)
fp.write("// Layout is 8 bits of horizontal pixels of all 256 chracters\n")
fp.write("// (256 bytes), mutiplied by number of vertial lines for the\n")
fp.write("// characters in the font.\n\n") 
fp.write("const unsigned char fixed_font[CLGLCD_FONT_LINES * 256] __attribute__((progmem,aligned(256))) = {\n")

ib = img.tobytes()
for y in range(0, 16*FONT_SIZE):
  l = list(ib[y*16:(y+1)*16])
  fp.write("  0x"+", 0x".join(["{:02x}".format(x) for x in l])+",\n")
fp.write(");\n")
