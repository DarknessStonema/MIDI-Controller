from machine import UART, Pin
import time

# UART0: TX=GPIO0, RX=GPIO1 (adjust if you wired differently)
midi = UART(0, baudrate=31250, tx=Pin(0), rx=Pin(1))

def send_note_on(note=60, velocity=100, channel=0):
    status = 0x90 | (channel & 0x0F)
    midi.write(bytes([status, note & 0x7F, velocity & 0x7F]))

def send_note_off(note=60, velocity=0, channel=0):
    status = 0x80 | (channel & 0x0F)
    midi.write(bytes([status, note & 0x7F, velocity & 0x7F]))

print("Sending test note...")
send_note_on(60)   # Middle C
time.sleep(0.5)
send_note_off(60)
print("Done.")

# Echo loop: anything received on DIN IN is sent back out
while True:
    if midi.any():
        b = midi.read(1)
        if b:
            midi.write(b)
