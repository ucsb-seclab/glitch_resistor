import time

# ChipWhisperer
import chipwhisperer as cw

SCOPETYPE = 'OPENADC'
PLATFORM = 'CW308_STM32F0'
CRYPTO_TARGET = 'NONE'
sample_size = 5

# # Build the firmware
# %%bash -s "$PLATFORM" "$CRYPTO_TARGET"
# cd ../hardware/victims/firmware/glitch-simple
#
# # Flash it to the board
# make PLATFORM=$1 CRYPTO_TARGET=$2 FUNC_SEL=GLITCH1


# Connect our scope
scope = cw.scope()

# Connect to target
target = cw.target(scope)

# Initialize our programmer
if "STM" in PLATFORM or PLATFORM == "CWLITEARM" or PLATFORM == "CWNANO":
    prog = cw.programmers.STM32FProgrammer
elif PLATFORM == "CW303" or PLATFORM == "CWLITEXMEGA":
    prog = cw.programmers.XMEGAProgrammer
else:
    prog = None

# Total hack?
time.sleep(0.05)

scope.default_setup()


def reset_target(scope):
    if PLATFORM == "CW303" or PLATFORM == "CWLITEXMEGA":
        scope.io.pdic = 'low'
        time.sleep(0.05)
        scope.io.pdic = 'high_z'  # XMEGA doesn't like pdic driven high
        time.sleep(0.05)
    else:
        scope.io.nrst = 'low'
        time.sleep(0.05)
        scope.io.nrst = 'high'
        time.sleep(0.05)


fw_path = "cw_glitching/build/cw_glitching.hex"
# fw_path = "../chipwhisperer/hardware/victims/firmware/glitch-simple" \
#           "/glitchsimple-{" \
#           "}.hex".format(PLATFORM)
cw.program_target(scope, prog, fw_path)

# Flush UART
target.flush()
scope.arm()
reset_target(scope)

# Capture scope
ret = scope.capture()
if ret:
    print("Scope capture timed out")

# Read UART
response = target.read(timeout=10)
print(response)

# Setup glitching parameters
from collections import namedtuple

scope.glitch.clk_src = "clkgen"
scope.glitch.output = "clock_xor"
scope.glitch.trigger_src = "ext_single"

scope.io.hs2 = "glitch"

Range = namedtuple('Range', ['min', 'max', 'step'])
if PLATFORM == "CWLITEXMEGA" or PLATFORM == "CW303":
    offset_range = Range(-10, 10, 1)
    scope.glitch.repeat = 105
elif PLATFORM == "CWLITEARM" or PLATFORM == "CW308_STM32F0":
    offset_range = Range(-49, -30, 1)
    scope.glitch.ext_offset = 37
    scope.glitch.repeat = 10

print(scope.glitch)

width_range = Range(-10, 10, 1)

scope.glitch.width = width_range.min
attack1_data = []


# Glitch it!!
while scope.glitch.width < width_range.max:
    scope.glitch.offset = offset_range.min
    while scope.glitch.offset < offset_range.max:
        successes = 0
        success_result = ""
        for i in range(sample_size):
            scope.arm()
            reset_target(scope)
            ret = scope.capture()
            if ret:
                print('Timeout happened during acquisition')

            response = target.read(timeout=10)

            print(repr(response))
            # check for glitch success (depends on targets active firmware)
            success = '1234' in repr(response)
            if success:
                print("Got a success!!!")
                success_result = response
                successes += 1

        attack1_data.append(
            [scope.glitch.width, scope.glitch.offset, successes / sample_size,
             repr(success_result)])
        # run aux stuff that should happen after trace here
        scope.glitch.offset += offset_range.step
    scope.glitch.width += width_range.step
print("Done glitching")


# Print Successes
for row in attack1_data:
    if row[2] > 0:
        print(row)
    #print(row)

scope.dis()
target.dis()
