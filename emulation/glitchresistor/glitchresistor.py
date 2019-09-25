from capstone import *
import ast
import logging

from .emulator import Architecture

logger = logging.getLogger(__name__)


def disassemble_data(data, target):
    """
    Main purpose of the disassembler plugin, it's used to disassemble
    instructions
    :param data:  Data to disassemble
    :param addr:  The address that the dat originated from.
                  If not specified, the current pc is used
    :param arch:  The capstone-architecture to be used.
                  If not specified, it is retrieved from avatar.arch
    :param mode:  The capstone-mode to be used.
                  If not specified, it is retrieved from avatar.arch
    :returns:     A list with capstone instructions
    """

    arch = target._arch.capstone_arch
    mode = target._arch.capstone_mode
    addr = 0

    ret = []
    md = Cs(arch, mode)

    for ins in md.disasm(data, addr):
        ret.append(ins)

    return ret


from keystone import *

# separate assembly instructions by ; or \n
CODE = b"INC ecx; DEC edx"


def assemble_code(code, target):
    arch = target._arch.capstone_arch
    mode = target._arch.capstone_mode
    try:
        # Initialize engine in X86-32bit mode
        ks = Ks(arch, mode)
        encoding, count = ks.asm(code)
        print("%s = %s (number of statements: %u)" % (code, encoding, count))
        return encoding, count
    except KsError as e:
        print("ERROR: %s" % e)
        return False


def parse_config_from_file(file_contents):
    first_line = file_contents.partition("\n")[0]
    if "config" in first_line:
        config_dict = ast.literal_eval(first_line.partition(":")[2])
        return config_dict
    else:
        return {}


def get_opcode_from_results(results, arch=Architecture.ARM_THUMB):
    initial_bytes = results['input']['initial_bytes']
    opcode = "0b"
    if arch == Architecture.ARM_THUMB:
        for b in initial_bytes:
            opcode = format(b, '08b') + opcode
    else:
        logger.error("Architecture not recognized.")
    return opcode
