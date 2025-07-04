# gamma_table_generator.py

import math

GAMMA = 2.2
MAX_INPUT = 127
MAX_OUTPUT = 4095

def gamma_correct_table(gamma=GAMMA, in_max=MAX_INPUT, out_max=MAX_OUTPUT):
    values = []
    for i in range(in_max + 1):
        norm = i / in_max
        corrected = int((norm ** gamma) * out_max + 0.5)
        values.append(corrected)
    values[-1] = out_max  # Force last value to hit full range
    return values

def format_c_array(name, values, line_len=8):
    lines = []
    lines.append(f"const uint16_t {name}[{len(values)}] = {{")
    for i in range(0, len(values), line_len):
        chunk = ", ".join(f"{v:4d}" for v in values[i:i + line_len])
        lines.append(f"    {chunk},")
    lines[-1] = lines[-1].rstrip(',')  # remove trailing comma from last line
    lines.append("};")
    return "\n".join(lines)

if __name__ == "__main__":
    table = gamma_correct_table()
    c_code = format_c_array("midiToBrightness", table)
    print(c_code)
