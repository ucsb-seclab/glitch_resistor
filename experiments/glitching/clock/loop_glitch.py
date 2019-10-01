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
fw_path = os.path.join(fw_dir, "build/cw_glitching.hex")


def build_firmware(directory, function="LOOP", use_glitch_resistor=False):
    """
    Change the directory and run the appropriate make command
    :param use_glitch_resistor: Build using make or ./build.sh
    :param directory: The directory of the firmware
    :param function: Do we want to pass a build parameter?
    :return:
    """
    from subprocess import call
    cwd = os.getcwd()
    os.chdir(directory)
    call(["make", "clean"])
    if use_glitch_resistor:
        rtn = call(["./build.sh"])
    else:
        rtn = call(["make", "FUNC_SEL=%s" % function])
    os.chdir(cwd)

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
    logger.info("Running firmware...")
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
    global failed_glitches, detected_glitches, successful_glitches_count
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
                    detected = False
                    for i in range(sample_size):

                        # Run firmware
                        response = run_firmware()

                        # Was it detected?
                        if "Glitch" in repr(response):
                            print ("GR Detected!!", repr(response))
                            detected = True
                            detected_glitches += 1
                        # check for glitch success (depends on targets active firmware)
                        elif SUCCESS_OUTPUT in repr(response):
                            logger.info("Got a success!")
                            success_result = response
                            successes += 1
                            successful_glitches_count += 1

                        else:
                            failed_glitches += 1

                    # Save our successful glitches
                    # Save our successful glitches
                    params = [scope.glitch.ext_offset, scope.glitch.width,
                              scope.glitch.offset,
                              successes, partial_successes, sample_size,
                              success_result,
                              repeat]

                    if detected:
                        detected_glitch_results.append(params)
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
    """

    :param ext_offsets:
    :param widths:
    :param offsets:
    :param depth:
    :param repeat:
    :param multi_glitch:
    :param stop_at_optimal:
    :param max_depth:
    :return:
    """
    global partial_successes, failed_glitches, detected_glitches, \
        successful_glitches_count
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
                detected = False
                for i in range(sample_size):

                    # Run firmware
                    response = run_firmware(multi_glitch=multi_glitch)

                    # Was it detected?
                    if "Glitch" in repr(response):
                        print ("GR Detected!!", repr(response))
                        detected = True
                        detected_glitches += 1

                    # Check for glitch success
                    elif SUCCESS_OUTPUT in repr(response):
                        logger.info("Got a success!")
                        success_result = response
                        successes += 1
                        successful_glitches_count += 1

                    # Did we get a partial success?
                    elif "Yes1" in repr(response) and multi_glitch:
                        partials += 1
                        partial_successes += 1
                        if success_result == "":
                            success_result = response

                    else:
                        failed_glitches += 1

                # Save our successful glitches
                params = [scope.glitch.ext_offset, scope.glitch.width,
                          scope.glitch.offset,
                          successes, partials, sample_size,
                          success_result,
                          repeat]

                if detected:
                    detected_glitch_results.append(params)

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
    detected_glitch_results = []
    detected_glitches = 0
    partial_successes = 0
    failed_glitches = 0
    successful_glitches_count = 0

    multi_glitch = False
    repeat = 10
    max_depth = 6
    stop_at_optimal = False
    repeats = []
    gr_build = False

    # Find optimal glitch parameters
    if args.experiment == "single_optimal":

        function_name = "SINGLE"
        sample_size = 10
        ext_offsets = range(0, 10)
        widths = numpy.arange(-49, 50, 1)
        offsets = numpy.arange(-49, 50, 1)
        stop_at_optimal = True
        multi_glitch = False
        repeat = 10
        max_depth = 6
    # Single glitch
    elif args.experiment == "single":

        function_name = "SINGLE"
        sample_size = 1
        ext_offsets = range(0, 10)
        widths = numpy.arange(-49, 50, 1)
        offsets = numpy.arange(-49, 50, 1)
        stop_at_optimal = False
        multi_glitch = False
        repeat = 1

    elif args.experiment == "single_one":

        function_name = "SINGLEONE"
        sample_size = 1
        ext_offsets = range(0, 10)
        widths = numpy.arange(-49, 50, 1)
        offsets = numpy.arange(-49, 50, 1)
        stop_at_optimal = False
        multi_glitch = False
        repeat = 1

    elif args.experiment == "store":

        function_name = "NOZERO"
        sample_size = 1
        ext_offsets = range(0, 10)
        widths = numpy.arange(-49, 50, 1)
        offsets = numpy.arange(-49, 50, 1)
        stop_at_optimal = False
        multi_glitch = False
        repeat = 1

    elif args.experiment == "store_optimal":

        function_name = "NOZERO"
        sample_size = 10
        ext_offsets = [0]
        widths = numpy.arange(-49, 50, 1)
        offsets = numpy.arange(-49, 50, 1)
        stop_at_optimal = True
        multi_glitch = False
        max_depth = 6
        repeat = 10

    # Multi-glitch
    elif args.experiment == "multi":

        function_name = "DOUBLE"
        sample_size = 1
        ext_offsets = range(0, 10)
        widths = numpy.arange(-49, 50, 1)
        offsets = numpy.arange(-49, 50, 1)
        multi_glitch = True
        repeat = 1

    # Multi-glitch
    elif args.experiment == "multi_fixed":

        function_name = "DOUBLEFIXED"
        sample_size = 1
        ext_offsets = range(0, 10)
        widths = numpy.arange(-49, 50, 1)
        offsets = numpy.arange(-49, 50, 1)
        multi_glitch = True
        repeat = 1

    elif args.experiment == "long":
        function_name = "LONG"
        sample_size = 1
        ext_offsets = [0]
        widths = numpy.arange(-49, 50, 1)
        offsets = numpy.arange(-49, 50, 1)
        repeats = range(30, 8, -2)

    elif args.experiment == "scan10":

        function_name = None
        sample_size = 1
        ext_offsets = range(0, 110, 10)
        widths = numpy.arange(-49, 50, 1)
        offsets = numpy.arange(-49, 50, 1)
        stop_at_optimal = False
        multi_glitch = False
        repeat = 10
        max_depth = 6
        fw_dir = "cw_protected"
        fw_path = os.path.join(fw_dir, "build/cw_glitching.hex")
        gr_build = True

    elif args.experiment == "single_real":
        """
        LDR     R1, =0x48000018
        STR     R0, [R1]
        STR     R0, [R1,#0x10]

        LDR     R2, [SP,#0x20+var_18] // 0,1: 2 cycles
        STR     R2, [SP,#0x20+var_14] // 2,3: 2 cycles

        LDR     R0, [SP,#0x20+var_14]
        LDR     R1, =0xE7D25763
        CMP     R2, R1
        BNE     loc_800039A
        """
        function_name = None
        sample_size = 1
        # Offset changed to account for our copying of the variable
        ext_offsets = range(4, 15)
        widths = numpy.arange(-49, 50, 1)
        offsets = numpy.arange(-49, 50, 1)
        stop_at_optimal = False
        multi_glitch = False
        repeat = 1
        fw_dir = "cw_protected"
        fw_path = os.path.join(fw_dir, "build/cw_glitching.hex")
        gr_build = True

    elif args.experiment == "long_real":
        function_name = None
        sample_size = 1
        ext_offsets = [0]
        widths = numpy.arange(-49, 50, 1)
        offsets = numpy.arange(-49, 50, 1)
        repeats = range(10, 110, 10)

        fw_dir = "cw_protected"
        fw_path = os.path.join(fw_dir, "build/cw_glitching.hex")
        gr_build = True

    else:
        logger.error("Invalid experiment name (%s)!" % args.experiment)
        sys.exit(0)

    # Build and flash our firmware
    if not build_firmware(fw_dir, function=function_name,
                          use_glitch_resistor=gr_build):
        logger.error("Could not build firmware")
    cw.program_target(scope, prog, fw_path)

    # Run the firmware once
    benign = run_firmware(arm=False, multi_glitch=False)
    print("Benign result: ", repr(benign))

    time_start = time.time()
    optimal_params = None
    if "long" in args.experiment:
        long_glitch(ext_offsets,  # range(37, 37 + 8, 1),
                    widths,
                    offsets,
                    repeats)
    else:
        optimal_params = optimize_glitch(ext_offsets,
                                         widths,
                                         offsets,
                                         repeat=repeat,
                                         multi_glitch=multi_glitch,
                                         stop_at_optimal=stop_at_optimal)

    pprint.pprint(successful_glitches)
    print("Partials: ", partial_successes)

    scope.dis()
    target.dis()

    print("* Saving output to %s..." % args.output)
    output = {
        'parameters': {
            'function_name': function_name,
            'sample_size': sample_size,
            'ext_offsets': ext_offsets,
            'widths': widths,
            'offsets': offsets,
            'stop_at_optimal': stop_at_optimal,
            'multi_glitch': multi_glitch,
            'repeat': repeat,
            'max_depth': max_depth,
            'repeats': repeats,
            'time_elapsed': time.time() - time_start
        },
        'optimal': optimal_params,
        'failures': failed_glitches,
        'successes': successful_glitches,
        'successful_glitches_count': successful_glitches_count,
        'partial_successes': partial_successes,
        'detected_glitches': detected_glitches,
        'detections': detected_glitch_results}
    pprint.pprint(output)
    pickle.dump(output, open(args.output, "wb"))
