import argparse
import os
import pprint
import time
import logging
import sys
import pickle

# ChipWhisperer
import chipwhisperer as cw
import numpy

logger = logging.getLogger(__name__)

SCOPETYPE = 'OPENADC'
PLATFORM = 'CW308_STM32F0'
CRYPTO_TARGET = 'NONE'
SUCCESS_OUTPUT = "Yes!"
fw_dir = "cw_glitching"
fw_path = "build/cw_glitching.hex"


def build_firmware(directory, function="LOOP"):
    """
    Change the directory and run the appropriate make command
    :param directory:
    :param function:
    :return:
    """
    from subprocess import call
    os.chdir(directory)
    call(["make", "clean"])
    rtn = call(["make", "FUNC_SEL=%s" % function])

    return rtn == 0


def reset_target(scope):
    """
    Reset our target board
    :param scope:
    :return:
    """
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


def run_firmware(arm=True, multi_glitch=False):
    """
    Run our firmware and return the UART output

    :type arm: Should we arm our trigger value?
    :return:
    """
    # Flush UART
    target.flush()
    if arm:
        logger.debug("Arming ChipWhisperer...")
        scope.arm()
    reset_target(scope)

    rtn = target.read(timeout=.01)

    # Should we rearm?
    if multi_glitch and arm:

        ret = scope.capture()
        if ret:
            logger.warn("Scope capture timed out")

        # print(rtn, len(rtn))
        if "Yes" not in rtn:
            logger.debug("First glitch failed.")
            return rtn
        logger.debug("Caught first trigger, arming ChipWhisperer again...")
        scope.arm()

    # Capture scope
    if arm:
        ret = scope.capture()
        if ret:
            logger.error("Scope capture timed out")

    # Read UART
    rtn += target.read(timeout=.01)
    return rtn


def long_glitch(ext_offsets, widths, offsets, repeats):
    """
    Attempt to lengthen the glitch to skip multiple branches
    :param ext_offsets:
    :param widths:
    :param offsets:
    :param repeats:
    :return:
    """
    for repeat in repeats:
        for ext_offset in ext_offsets:
            for width in widths:
                for offset in offsets:

                    logger.debug("%d %f %f" % (ext_offset, width, offset))

                    scope.glitch.repeat = repeat
                    scope.glitch.offset = offset
                    scope.glitch.ext_offset = ext_offset
                    scope.glitch.width = width

                    successes = 0
                    partials = 0
                    success_result = ""

                    for i in range(sample_size):

                        # Run firmware
                        response = run_firmware()

                        # check for glitch success (depends on targets active firmware)
                        if SUCCESS_OUTPUT in repr(response):
                            logger.info("Got a success!")
                            success_result = response
                            successes += 1

                    # Save our successful glitches
                    params = [scope.glitch.width, scope.glitch.offset,
                              successes / sample_size,
                              repr(success_result), ext_offset]

                    # Did we have any successes?
                    if successes > 0 or partials > 0:
                        print("* Ext. Offset: %d Width: %f Offset: %f Repeat: "
                              "%d | Successes: %f Partial: %d) | %s" % (
                                  ext_offset,
                                  width,
                                  offset,
                                  repeat,
                                  successes / sample_size,
                                  partials,
                                  repr(success_result)))
                        successful_glitches.append(params)


def optimize_glitch(ext_offsets, widths, offsets, depth=1, repeat=1,
                    multi_glitch=False, stop_at_optimal=False,
                    max_depth=6):
    global partial_successes
    if depth > max_depth:
        return False

    increment = 1 / (10.0 * depth);

    # print(ext_offsets)
    # print(widths)
    # print(offsets)
    for ext_offset in ext_offsets:
        for width in widths:
            for offset in offsets:
                logger.debug("%d %f %f" % (ext_offset, width, offset))

                scope.glitch.repeat = repeat
                scope.glitch.offset = offset
                scope.glitch.ext_offset = ext_offset
                scope.glitch.width = width

                successes = 0
                partials = 0
                success_result = ""
                partial_success = False

                for i in range(sample_size):

                    # Run firmware
                    response = run_firmware(multi_glitch=multi_glitch)

                    # Did we get a partial success?
                    if "Yes1" in repr(response) and multi_glitch:
                        partials += 1
                        partial_successes += 1
                        if success_result == "":
                            success_result = repr(response)

                    # Check for glitch success
                    if SUCCESS_OUTPUT in repr(response):
                        logger.info("Got a success!")
                        success_result = response
                        successes += 1

                # Save our successful glitches
                params = [ext_offset, scope.glitch.width, scope.glitch.offset,
                          successes, partial_successes, sample_size,
                          repr(success_result)]

                # Are we searching for the most optimal solution?
                if successes == sample_size and stop_at_optimal:
                    return params

                # Did we have any successes?
                if successes > 0 or partials > 0:
                    print("* Ext. Offset: %d Width: %f Offset: %f Repeat: "
                          "%d | Successes: %f Partial: %d) | %s" % (
                              ext_offset,
                              width,
                              offset,
                              repeat,
                              successes / sample_size,
                              partials,
                              repr(success_result)))
                    successful_glitches.append(params)

                    # Only Optimize the parameters if we are looking for the
                    # optimal
                    if stop_at_optimal:
                        print("** Optimizing...")
                        rtn = optimize_glitch(range(ext_offset, ext_offset +
                                                    repeat),
                                              numpy.arange(width - increment,
                                                           width + increment,
                                                           increment),
                                              numpy.arange(offset - increment,
                                                           offset + increment,
                                                           increment),
                                              depth=depth + 1,
                                              multi_glitch=multi_glitch,
                                              stop_at_optimal=stop_at_optimal,
                                              max_depth=max_depth)

                        if rtn is not False:
                            return rtn
    return False


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("experiment", type=str,
                        help="'single', 'multi', or 'long' glitch experiment")
    parser.add_argument("-d", "--debug", action="store_true",
                        help="Enable debug output")
    parser.add_argument("-o", "--output", type=str,
                        default="glitch_successes.pickle",
                        help="Output filename")
    args = parser.parse_args()

    # Debugging?
    if args.debug:
        logging.basicConfig(level=logging.DEBUG)
    else:
        logging.basicConfig(level=logging.ERROR)

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

    # Setup scope
    scope.default_setup()
    scope.glitch.clk_src = "clkgen"
    scope.glitch.output = "clock_xor"
    scope.glitch.trigger_src = "ext_single"

    scope.io.hs2 = "glitch"

    successful_glitches = []
    partial_successes = 0

    # Find optimal glitch parameters
    if args.experiment == "optimal":

        # Build and flash our firmware
        if not build_firmware(fw_dir):
            logger.error("Could not build firmware")
        cw.program_target(scope, prog, fw_path)

        # Run the firmware once
        response = run_firmware(arm=False)
        print("Benign result: ", response)

        sample_size = 10
        optimial_params = optimize_glitch(range(0, 10),
                                          numpy.arange(-49, 49, 1),
                                          numpy.arange(-49, 49, 1),
                                          repeat=10,
                                          stop_at_optimal=True)

        print("Found optimal params:", optimial_params)

        pprint.pprint(successful_glitches)

    if args.experiment == "store":

        # Build and flash our firmware
        if not build_firmware(fw_dir, function="NOZERO"):
            logger.error("Could not build firmware")
        cw.program_target(scope, prog, fw_path)

        # Run the firmware once
        response = run_firmware(arm=False)
        print("Benign result: ", response)

        sample_size = 1
        optimial_params = optimize_glitch(range(0, 10),
                                          numpy.arange(-49, 49, 1),
                                          numpy.arange(-49, 49, 1),
                                          repeat=1,
                                          stop_at_optimal=False,
                                          max_depth=2)

        print("Found optimal params:", optimial_params)

        pprint.pprint(successful_glitches)
    # Single glitch
    elif args.experiment == "single":

        # Build and flash our firmware
        if not build_firmware(fw_dir):
            logger.error("Could not build firmware")
        cw.program_target(scope, prog, fw_path)

        # Run the firmware once
        response = run_firmware(arm=False)
        print("Benign result: ", response)

        sample_size = 1
        optimial_params = optimize_glitch(range(0, 10),  # range(37, 37 + 8,
                                          #  1),
                                          numpy.arange(-49, 49, 1),
                                          numpy.arange(-49, 49, 1),
                                          repeat=1)

        print("Found optimal params:", optimial_params)

        pprint.pprint(successful_glitches)


    # Multi-glitch
    elif args.experiment == "multi":

        # Build and flash our firmware
        if not build_firmware(fw_dir, function="DOUBLE"):
            logger.error("Could not build firmware")
        cw.program_target(scope, prog, fw_path)

        # Run the firmware once
        benign = run_firmware(arm=True, multi_glitch=True)
        print("Benign result: ", benign)

        sample_size = 1
        optimial_params = optimize_glitch(range(0, 10),
                                          numpy.arange(-49, 49, 1),
                                          numpy.arange(-49, 49, 1),
                                          repeat=1,
                                          multi_glitch=True)

        print("Found optimal params:", optimial_params)

        pprint.pprint(successful_glitches)
        print("Partials: ", partial_successes)

    elif args.experiment == "long":

        # Build and flash our firmware
        if not build_firmware(fw_dir, function="LONG"):
            logger.error("Could not build firmware")
        cw.program_target(scope, prog, fw_path)

        # Run the firmware once
        benign = run_firmware(arm=False, multi_glitch=False)
        print("Benign result: ", benign)

        sample_size = 1
        long_glitch([0],  # range(37, 37 + 8, 1),
                                      numpy.arange(-10, 10, 1),
                                      numpy.arange(-49, 30, 1),
                                      range(100, 10, -10))

        pprint.pprint(successful_glitches)
        print("Partials: ", partial_successes)

    scope.dis()
    target.dis()

    print("* Saving output to %s..." % args.output)
    pickle.dump(successful_glitches, open(args.output, "wb"))
