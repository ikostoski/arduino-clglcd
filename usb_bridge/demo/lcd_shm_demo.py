#
# Python demo for CLGLCD.
#
# Talks to clglcd_usb_host 'driver' via shared memory IPC
# 
# Requires Pillow, and can make use of WMI and CV2 libraries.
#
# Copyright (c) 2019 Ivan Kostoski
#

import time
import clglcd

try:
  from PIL import Image, ImageFont, ImageDraw
except ImportError:
  raise "PIL (or Pillow) library is required for demo"

lcd = clglcd.IPC()

try:

  # Standby
  # To show PNG image on the LCD, all you need are the below two lines
  standby = Image.open('standby320.png').convert('L')
  lcd.show(standby.tobytes())

  print("Touch to start")
  if not lcd.wait_for_touch(300):
    raise SystemExit(-1)
  
  # Intro
  text = (
    "This is STN monochrome 320x240\n'Controllerless' Graphics LCD module\n"
    "driven by Arduino Leonardo\n(ATmega32u4 MCU) USB 'bridge'.\n\n"
    "Let's see what we can do with it..."
  )
  font = ImageFont.truetype('arial.ttf', 18)
  intro = Image.new('L', (320, 240), 0) 
  draw = ImageDraw.Draw(intro)
  (tsx, tsy) = draw.textsize(text, font)
  draw.text((160-tsx/2, 120-tsy/2), text, fill=255, font=font, align='center')
  lcd.show(intro.tobytes())
  lcd.wait_for_touch(30)

  # Touchpanel test
  font = ImageFont.truetype('arial.ttf', 13)
  touch_base = Image.new('L', (320, 240), 0) 
  draw = ImageDraw.Draw(touch_base)
  draw.text((0, 0), "Touchpanel test. Touch the 'X' to stop.", font=font, fill=255)
  draw.rectangle((300, 0, 319, 19), outline=255)
  draw.text((306, 3), "X", font=font, fill=255)
  while True:
    touch = touch_base.copy()
    x, y, p = lcd.touch_point()
    if p > 0:  
      if (x > 300 and y < 20): break
      draw = ImageDraw.Draw(touch)
      text = "X:"+str(x)+" Y:"+str(y)+" P:"+str(p)
      (tsx, tsy) = draw.textsize(text, font)
      draw.text((160-tsx/2, 239-tsy), text, font=font, fill=255)
      # Raw touchpanel readings
      #rx, ry, rz1, rz2, rcnt = lcd.raw_touch()
      #text = "rX:"+str(rx)+" rY:"+str(ry)+" rZ1:"+str(rz1)+" rZ2:"+str(rz2)
      #(tsx, tsy) = draw.textsize(text, font)
      #draw.text((160-tsx/2, 225-tsy), text, font=font, fill=255)
      draw.ellipse((x-p/2, y-p/2, x+p/2, y+p/2), outline=127, width=2)    
      draw.ellipse((x-p/4, y-p/4, x+p/4, y+p/4), outline=192, width=2)    
      draw.ellipse((x-1, y-1, x+1, y+1), fill=255, width=1)    
    lcd.show(touch.tobytes())
    time.sleep(0.1)

  
  # Display OpenHardwareMonitor sensors
  cpu_load = None
  try:
    import wmi, subprocess
    kill_load = time.time() + 7

    try:
      cpu_load = subprocess.Popen(["cpuburn", "-u=1", "-n=7"], shell=False, stdout=subprocess.PIPE) 
    except FileNotFoundError:
      pass

    ohmwmi = wmi.WMI(namespace="root\\OpenHardwareMonitor")
    ohm_sensors = {
      "cpu_load": "/intelcpu/0/load/0", 
      "cpu_temp": "/lpc/it8628e/temperature/2",
      "gpu_load": "/nvidiagpu/0/load/0", 
      "gpu_temp": "/nvidiagpu/0/temperature/0"
    }
    wql = ("SELECT Identifier, Value FROM Sensor WHERE " +
          "(Identifier='" + "' OR Identifier='".join(ohm_sensors.values())+"')")
    id_map = {v: k for k, v in ohm_sensors.items()}
    values = {}

    title_font = ImageFont.truetype('calibri.ttf', 18)
    label_font = ImageFont.truetype('arial.ttf', 20)
    value_font = ImageFont.truetype('impact.ttf', 60)
    hw_base = Image.new('L', (320, 240), 0) 
    draw = ImageDraw.Draw(hw_base)
    (tsx, tsy) = draw.textsize("OpenHardwareMonitor sensors", title_font)
    draw.text((160-tsx/2, 2), "OpenHardwareMonitor sensors", font=title_font, fill=255)
    (tsx, tsy) = draw.textsize("CPU", label_font)
    draw.text((80-tsx/2, 30), "CPU", font=label_font, fill=255)
    draw.text((240-tsx/2, 30), "GPU", font=label_font, fill=255)
    draw.rectangle((18, 123, 142, 142), outline=255)
    draw.rectangle((178, 123, 302, 142), outline=255)

    t_x = range(20, 300)
    cpu_y = [220] * 280
    gpu_y = [220] * 280

    t = time.time()
    time_target = t + 20 
    while t < time_target:
      for sensor in ohmwmi.query(wql):
        values[id_map[sensor.Identifier]] = int(sensor.Value)

      cpu_y.pop(0)
      cpu_y.append(int(220 - 0.7*values["cpu_load"]))
      gpu_y.pop(0)
      gpu_y.append(int(220 - 0.7*values["gpu_load"]))

      hw_img = hw_base.copy()
      draw = ImageDraw.Draw(hw_img)
      (tsx, tsy) = draw.textsize((str(values['cpu_temp']) + "°C"), value_font)
      draw.text((80-tsx/2, 50), (str(values['cpu_temp']) + "°C"), font=value_font, fill=255)
      (tsx, tsy) = draw.textsize((str(values['gpu_temp']) + "°C"), value_font)
      draw.text((240-tsx/2, 50), (str(values['gpu_temp']) + "°C"), font=value_font, fill=255)
      draw.rectangle((20, 125, 20 + 1.2*values["cpu_load"], 140), fill=192)
      draw.rectangle((180, 125, 180 + 1.2*values["gpu_load"], 140), fill=192)
      draw.line(list(zip(t_x, gpu_y)), fill=192, width=3)
      draw.line(list(zip(t_x, cpu_y)), fill=255, width=2)

      lcd.show(hw_img.tobytes())
      t = time.time()
      if cpu_load and ((t > kill_load) or lcd.is_touched(release_timeout=-1)):
        cpu_load.terminate()
        cpu_load = None
      time.sleep(0.5)
      if lcd.is_touched(): break
    
  except ImportError:
    pass
  finally:
    if cpu_load:
      cpu_load.terminate()
      cpu_load.wait()

  # Grayscale bars
  grayscale = Image.new('L', (320, 240), 0) 
  font = ImageFont.truetype('arial.ttf', 13)
  draw = ImageDraw.Draw(grayscale)
  for c in range(0, 16):
    draw.text((0, c*15+1), str(c), font=font, fill=(c<<4))
    draw.rectangle((20, c*15, 300, c*15+15), fill=(c<<4))
  text = "4-bit Temporal Dithering (FRC)"
  font = ImageFont.truetype('arial.ttf', 16)
  (tsx, tsy) = draw.textsize(text, font)
  draw.text((160-tsx/2, 1), text, font=font, fill=255)
  lcd.show(grayscale.tobytes())
  lcd.wait_for_touch(10)

  # Lena
  lena = Image.open('lena320x240.png').convert('L')
  lcd.show(lena.tobytes())
  lcd.wait_for_touch(10)

  # Big buck bunny @25fps, needs CV2
  try:
    import cv2
    vidcap = cv2.VideoCapture('SampleVideo_640x360_10mb.mp4')
    t = time.time()
    time_target = t + 30
    success, image = vidcap.read()
    while (success and (t < time_target)):
      thumb = cv2.resize(image, (320, 240), cv2.INTER_AREA)
      grayscale = cv2.cvtColor(thumb, cv2.COLOR_RGB2GRAY)
      lcd.show(grayscale.tobytes())
      success, image = vidcap.read()
      delta = time.time() - t
      if delta < 0.04: time.sleep(0.04 - delta)
      if lcd.is_touched(): break
      t = time.time()
  except ImportError:
    pass

  # Explanation
  text = (
    "The content is rendered with Python\n"
   "on 'Big' PC and streamed to the Arduino \n"
  "via USB at ~5.7Mbps for 70Hz refresh rate.\n" 
  "By using 'temporal dithering' (FRC) of the\n"
    "LCD pixels we get 16 grayscale levels\n"
   "The Arduino is providing the control and \n"
     "timing of the LCD module and acting\n"
          "as a USB to LCD brigde\n\n"
               
    "The protoboard provides LCD drive voltage\n"
    "(-24V) and control for the LED backlight."
  )
  font = ImageFont.truetype('arial.ttf', 15)
  intro = Image.new('L', (320, 240), 0) 
  draw = ImageDraw.Draw(intro)
  (tsx, tsy) = draw.textsize(text, font)
  draw.text((160-tsx/2, 120-tsy/2), text, fill=255, font=font, align='center')
  lcd.show(intro.tobytes())
  lcd.wait_for_touch(20)

  # TY
  ty = Image.new('L', (320, 240), 0)
  font = ImageFont.truetype('arial.ttf', 50)
  draw = ImageDraw.Draw(ty)
  text = "Thank You!"
  (tsx, tsy) = draw.textsize(text, font)
  draw.text((160-tsx/2, 120-tsy/2), text, font=font, fill=255)
  lcd.show(ty.tobytes())
  lcd.wait_for_touch(5)

# Shutdown the display
finally:
  lcd.close()

