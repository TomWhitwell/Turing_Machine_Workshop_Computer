import rtmidi
import time

def send_config_sysex(midi_out):
    # Fake config data matching the HTML parser structure
    config = [
        0x12, 0x34, 0x56, 0x78,  # magic
        0x2C, 0x01,              # bpm = 300
        0x04,                    # divide
        0x01,                    # cvRange
    ]

    for i in range(2):  # two presets
        config += [
            0x10 + i,  # scale
            0x20 + i,  # notes
            0x30 + i,  # range
            0x40 + i,  # length
            0x50 + i,  # looplen
            0x60 + i,  # pulseMode1
            0x70 + i,  # pulseMode2
        ]

    sysex = [0xF0, 0x7D, 0x01, 0x02] + config + [0xF7]
    print("📤 Responding with SysEx config...")
    midi_out.send_message(sysex)

# Set up MIDI ports
midi_in = rtmidi.MidiIn()
midi_out = rtmidi.MidiOut()
port_name = "Computer"

midi_in.open_virtual_port(port_name)
midi_out.open_virtual_port(port_name)

print(f"✅ Virtual MIDI port '{port_name}' is open.")
print("📡 Listening for incoming MIDI...")

try:
    while True:
        msg = midi_in.get_message()
        if msg:
            message, timestamp = msg
            print(f"📥 Received: {message}")
            # Check for hello request
            if message == [0xF0, 0x7D, 0x01, 0x01, 0xF7]:
                send_config_sysex(midi_out)
        time.sleep(0.01)

except KeyboardInterrupt:
    print("\n🛑 Exiting cleanly.")

finally:
    del midi_in
    del midi_out
