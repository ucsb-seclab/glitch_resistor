import binascii
import itertools
import multiprocessing.pool

import pprint
import sys

from multiprocessing import Pool

from enum import Enum, IntEnum
from keystone import *
from unicorn import *
from unicorn.arm_const import *
from unicorn.mips_const import *
from capstone import *
from capstone.arm import *
from capstone.x86 import *
import logging

# architectures and modes for the assembler, disassembler, and emulator
from unicorn import UcError

logger = logging.getLogger(__name__)

# Globals
# All of our configuration parameters
ks_arch = None
ks_mode = None
cs_arch = None
cs_mode = None
EMU_ARCH = None
EMU_MODE = None
KEYSTONE = None
CAPSTONE = None

# Emulator parameters
text_base = 0x1000
text_size = 0x1000
stack_base = 0xf000
stack_size = 0x1000
# If we are in thumb mode, set the least order bit to 1
START_ADDR = text_base

# Check glitch results
REG_PC = None
REG_SUCCESS = None
REG_SUCCESS_VALUE = None
REG_FAIL = None
REG_FAIL_VALUE = None

code_initial = None
bytes_to_trash = None
flip_operation = None
force_invalid_ins = False


class Architecture(Enum):
    ARM_THUMB = 1
    ARM_32 = 2
    MIPS_BE = 3


class BitFlip(IntEnum):
    # XOR = 0
    AND = 1
    OR = 2


class Result(IntEnum):
    GLITCH_WORKED = 1
    GLITCH_FAILED_BAD_READ = 2
    GLITCH_FAILED_INVALID_INSTRUCTION = 3
    GLITCH_FAILED_BAD_FETCH = 4
    GLITCH_FAILED_UNKNOWN = 5
    GLITCH_FAILED_SIGABRT = 6
    GLITCH_FAILED = 7
    GLITCH_FAILED_NOEFECT = 8


class NoDaemonProcess(multiprocessing.Process):
    # make 'daemon' attribute always return False
    def _get_daemon(self):
        return False

    def _set_daemon(self, value):
        pass

    daemon = property(_get_daemon, _set_daemon)


# We sub-class multiprocessing.pool.Pool instead of multiprocessing.Pool
# because the latter is only a wrapper function, not a proper class.
class MyPool(multiprocessing.pool.Pool):
    Process = NoDaemonProcess


def dumpSimple(mu):
    """
        Print our register values
    :param mu:
    :return:
    """
    if EMU_ARCH == UC_ARCH_ARM:
        sp = mu.reg_read(UC_ARM_REG_SP)
        pc = mu.reg_read(UC_ARM_REG_PC)
        lr = mu.reg_read(UC_ARM_REG_LR)
        r0 = mu.reg_read(UC_ARM_REG_R0)
        r1 = mu.reg_read(UC_ARM_REG_R1)
        r2 = mu.reg_read(UC_ARM_REG_R2)
        r3 = mu.reg_read(UC_ARM_REG_R3)
        print('SP = 0x{:08x}'.format(sp))
        print('PC = 0x{:08x}'.format(pc))
        print('LR = 0x{:08x}'.format(lr))
        print('R0 = 0x{:08x}'.format(r0))
        print('R1 = 0x{:08x}'.format(r1))
        print('R2 = 0x{:08x}'.format(r2))
        print('R3 = 0x{:08x}'.format(r3))
    elif EMU_ARCH == UC_ARCH_MIPS:
        sp = mu.reg_read(UC_MIPS_REG_SP)
        pc = mu.reg_read(UC_MIPS_REG_PC)
        lr = mu.reg_read(UC_MIPS_REG_RA)
        r0 = mu.reg_read(UC_MIPS_REG_T1)
        r1 = mu.reg_read(UC_MIPS_REG_T2)
        r2 = mu.reg_read(UC_MIPS_REG_T3)
        r3 = mu.reg_read(UC_MIPS_REG_T4)
        print('SP = 0x{:08x}'.format(sp))
        print('PC = 0x{:08x}'.format(pc))
        print('RA = 0x{:08x}'.format(lr))
        print('T1 = 0x{:08x}'.format(r0))
        print('T2 = 0x{:08x}'.format(r1))
        print('T3 = 0x{:08x}'.format(r2))
        print('T4 = 0x{:08x}'.format(r3))


def dumpMem(mu, addr, size):
    x = mu.mem_read(addr, size)
    x = ''.join(map(chr, x))
    wrap = 16
    group = 4
    for i in xrange(0, len(x), wrap):
        k = i + wrap if i + wrap < len(x) else len(x)
        sys.stdout.write('0x{:x} | '.format(addr + i))
        for j in xrange(i, k):
            sys.stdout.write('{}'.format(x[j].encode('hex')))
            if j % group == group - 1: sys.stdout.write(' ')
        sys.stdout.write('\n')


def init_registers(emu):
    # initialize registers
    if EMU_ARCH == UC_ARCH_ARM:
        emu.reg_write(UC_ARM_REG_SP, stack_base + 0xff0)
        emu.reg_write(UC_ARM_REG_R0, 0x1234)
        emu.reg_write(UC_ARM_REG_R1, 0x5678)
        emu.reg_write(UC_ARM_REG_R2, 0xdead)
        emu.reg_write(UC_ARM_REG_R3, 0xbeef)
        emu.reg_write(UC_ARM_REG_R4, 0xcafe)
        emu.reg_write(UC_ARM_REG_R5, 0x1234)
        emu.reg_write(UC_ARM_REG_R6, 0x1337)
        emu.reg_write(UC_ARM_REG_R7, 0x7fffffff)
        emu.reg_write(UC_ARM_REG_R8, 0x0001)
        emu.reg_write(UC_ARM_REG_R9, 0xbaad)
        emu.reg_write(UC_ARM_REG_R10, 0xd00d)
        emu.reg_write(UC_ARM_REG_R11, 0xcaa7)


def print_code(uc, address, size, user_data):
    # global cs
    # print 'address = 0x{:x}, size = {}'.format(address, size)
    code = uc.mem_read(address, size)
    code = ''.join(map(chr, code))
    asm = list(CAPSTONE.disasm(code, size))

    if len(asm) == 0:
        sys.stdout.write('>>> 0x{:x}\t{}\tdisasm failure'.format(address,
                                                                 code.encode(
                                                                     'hex')))
    for ins in asm:
        sys.stdout.write(
            '>>> 0x{:x}\t{}\t{} {}'.format(address, code.encode('hex').ljust(8),
                                           ins.mnemonic.ljust(8),
                                           ins.op_str.ljust(16)))

    if EMU_ARCH == UC_ARCH_ARM:
        sp = uc.reg_read(UC_ARM_REG_SP)
        pc = uc.reg_read(UC_ARM_REG_PC)
        lr = uc.reg_read(UC_ARM_REG_LR)
        r0 = uc.reg_read(UC_ARM_REG_R0)
        r1 = uc.reg_read(UC_ARM_REG_R1)
        r2 = uc.reg_read(UC_ARM_REG_R2)
        r3 = uc.reg_read(UC_ARM_REG_R3)
        r4 = uc.reg_read(UC_ARM_REG_R4)
        cpsr = uc.reg_read(UC_ARM_REG_CPSR)
        flag_n = 1 if 1 << 31 & cpsr != 0 else 0
        flag_z = 1 if 1 << 30 & cpsr != 0 else 0
        flag_c = 1 if 1 << 29 & cpsr != 0 else 0
        flag_v = 1 if 1 << 28 & cpsr != 0 else 0
        flag_q = 1 if 1 << 27 & cpsr != 0 else 0
        sys.stdout.write('\t|'),
        sys.stdout.write('\tR0 = 0x{:08x}'.format(r0)),
        sys.stdout.write('\tR1 = 0x{:08x}'.format(r1)),
        sys.stdout.write('\tR2 = 0x{:08x}'.format(r2)),
        sys.stdout.write('\tR3 = 0x{:08x}'.format(r3)),
        sys.stdout.write('\tR4 = 0x{:08x}'.format(r4)),
        sys.stdout.write('\tN=%d,Z=%d,C=%d,V=%d,Q=%d' % (
            flag_n, flag_z, flag_c, flag_v, flag_q))
    print()


def run_emulator(system_code, instruction_count=50, verbose=False):
    """
        This will actually run the emulator with the given code, for the number of instructions specified

    :param system_code: Code to load an run on the emulator
    :param instruction_count: how many instructions to execute
    :return: Result ENUM
    """
    # init emulator
    emu = Uc(EMU_ARCH, EMU_MODE)

    # emulator setup
    # emu.hook_add(UC_HOOK_CODE, hook_code)
    def hook_intr(uc, intno, user_data):
        """ hook our interrupts so that we don't crash """
        logger.debug('Interrupt 0x{:x}'.format(intno))

    emu.hook_add(UC_HOOK_INTR, hook_intr)

    if verbose:
        emu.hook_add(UC_HOOK_CODE, print_code)
    # reset emulator
    emu.mem_map(text_base, text_size)
    emu.mem_map(stack_base, stack_size)

    # Rewrite memory
    emu.mem_write(text_base, system_code)
    emu.mem_write(stack_base, '\x00' * stack_size)
    emu.reg_write(REG_PC, text_base)

    init_registers(emu)

    if verbose:
        print('--- Start context ---')
        dumpSimple(emu)
        dumpMem(emu, stack_base + 0xfd0, 0x20)
    # Start our emulator
    try:
        emu.emu_start(START_ADDR, text_base + len(system_code),
                      count=instruction_count)

        if verbose:
            print('\n--- End context ---')
            dumpSimple(emu)
            print('\n--- Stack View ---')
            dumpMem(emu, stack_base + 0xfd0, 0x20)
        # with threadLock:
        # dumpSimple(emu)
        if emu.reg_read(REG_SUCCESS) == REG_SUCCESS_VALUE:
            # dumpSimple(emu)
            return Result.GLITCH_WORKED
            # print "Win! %s" % hex(emu.reg_read(UC_ARM_REG_R0))
        elif emu.reg_read(REG_FAIL) == REG_FAIL_VALUE:
            return Result.GLITCH_FAILED_NOEFECT
        else:
            # print "Lose. %s" % hex(emu.reg_read(UC_ARM_REG_R0))
            return Result.GLITCH_FAILED

    except UcError as uc:
        # Did we crash the emulator?
        if verbose:
            logger.exception("Emulation failed")

        # Could work in practice if the hardware ignores invalid instructions
        if uc.errno == UC_ERR_INSN_INVALID:
            return Result.GLITCH_FAILED_INVALID_INSTRUCTION

        # This could work in practice if the memory is mapped
        elif uc.errno == UC_ERR_READ_UNMAPPED:
            return Result.GLITCH_FAILED_BAD_READ

        # Maybe?  No clue what happened
        elif uc.errno == UC_ERR_EXCEPTION:
            return Result.GLITCH_FAILED_UNKNOWN

        # This jumped somewhere bad
        elif uc.errno == UC_ERR_FETCH_UNMAPPED:
            return Result.GLITCH_FAILED_BAD_FETCH

        # Something really strange...
        else:
            return Result.GLITCH_FAILED
    except:
        logger.exception("something really bad happened!!")
        return Result.GLITCH_FAILED_UNKNOWN


result_cache = {}


def flip_bits(code, bytes_to_flip, bits_to_flip, flip_op):
    """

    :param flip_op:
    :param bytes_to_flip: index into `code` with the
    :param code: list of bytes that is the code the mutate
    :param bits_to_flip:
    :return:
    """
    # Make a copy of our initial code
    code_input = list(code)

    # Flip all of the appropriate bits
    bit_mask = [0] * len(bytes_to_flip)
    for bit in bits_to_flip:
        bit_to_flip = 1 << (bit % 8)
        bit_mask[bit / 8] |= bit_to_flip
    bit_mask.reverse()
    for (idx, code_idx) in enumerate(bytes_to_flip):
        # if flip_operation == BitFlip.XOR:
        #     code_input[code_idx] ^= bit_mask[idx]
        if flip_op == BitFlip.OR:
            code_input[code_idx] |= bit_mask[idx]
        elif flip_op == BitFlip.AND:
            code_input[code_idx] &= bit_mask[idx]

    return code_input


def test_program(bits_to_flip):
    """
    Flip the specified bits in the initial code and then execute the code in an emulator

    Note: we do some crazy stuff with multiprocessing because Unicorn sometimes SIGABRTs and we need to catch it.

    :param flip_operation: XOR, AND, or OR
    :param bits_to_flip: the location of the bits to flip
    :return:
    """
    global code_initial, flip_operation, result_cache, force_invalid_ins

    logger.debug(flip_operation)

    code_input = flip_bits(code_initial, bytes_to_trash, bits_to_flip,
                           flip_operation)

    # print code_input

    # Is it cached?
    code_str = int(''.join(map(str, code_input)))
    if result_cache is not None and code_str in result_cache:
        return result_cache[code_str]

    logger.debug(code_input)
    # convert list to str for emulator
    system_code = ''.join(map(chr, code_input))

    if force_invalid_ins:
        asm = list(CAPSTONE.disasm(system_code, len(code_input)))
        # logger.info("compiled: {}".format(system_code.encode('hex')))
        if len(asm) == 0:
            logger.warning('>>> \tdisasm failure'.format(code.encode('hex')))
        for ins in asm:
            # print repr(ins.bytes), len(ins.bytes)
            # logger.info(
            #     '>>> {}\t {} {}'.format(binascii.hexlify(ins.bytes),
            #                             ins.mnemonic,
            #                             ins.op_str))
            if ins.bytes == "\x00" * len(ins.bytes) \
                    or ins.bytes == "\xff" * len(ins.bytes):
                logger.debug("Forcing invalid instruction")
                return Result.GLITCH_FAILED_INVALID_INSTRUCTION

    # Create a 1 process pool to execute our emulator in (effectively a sandbox for SIGABRT)
    pool = MyPool(processes=1)

    # Run the emulator
    t = pool.apply_async(run_emulator, (system_code,))

    # Get the result
    try:
        rtn = t.get(timeout=1)
    except:
        # Sometimes Unicorn will SIGABRT, we need to catch that
        logger.exception("Got a really bad fail!!!")
        rtn = Result.GLITCH_FAILED_SIGABRT
        del t
        # sys.exit(0)

    # Make sure we don't keep making pools
    pool.close()
    pool.terminate()
    pool.join()
    del pool

    if result_cache is not None:
        result_cache[code_str] = rtn
    return rtn


def run_code(code, architecture, nop_bytes=None):
    """
    Initialize and run code in our emulator in verbose mode
    :param code:
    :param architecture:
    :return:
    """
    init_architecture(architecture)

    code_bytes, count = KEYSTONE.asm(code)

    if nop_bytes is not None:
        logger.info("Zeroing out bytes: %s" % nop_bytes)
        code_bytes = flip_bits(code_bytes, nop_bytes, [], BitFlip.AND)

    system_code = ''.join(map(chr, code_bytes))

    asm = list(CAPSTONE.disasm(system_code, len(code_bytes)))
    logger.info("compiled: {}".format(system_code.encode('hex')))
    if len(asm) == 0:
        logger.warning('>>> \tdisasm failure'.format(code.encode('hex')))
    for ins in asm:
        logger.info(
            '>>> {}\t {} {}'.format(binascii.hexlify(ins.bytes), ins.mnemonic,
                                    ins.op_str))

    run_emulator(system_code, verbose=True)


def init_architecture(architecture):
    global START_ADDR, REG_PC, REG_SUCCESS, REG_SUCCESS_VALUE, REG_FAIL, REG_FAIL_VALUE
    global EMU_ARCH, EMU_MODE
    global KEYSTONE, CAPSTONE

    if architecture == Architecture.ARM_THUMB:
        # All of our configuration parameters
        ks_arch = KS_ARCH_ARM
        ks_mode = KS_MODE_THUMB
        cs_arch = CS_ARCH_ARM
        cs_mode = CS_MODE_THUMB
        EMU_ARCH = UC_ARCH_ARM
        EMU_MODE = UC_MODE_THUMB

        REG_PC = UC_ARM_REG_PC
        REG_SUCCESS = UC_ARM_REG_R0
        REG_SUCCESS_VALUE = 0xdead
        REG_FAIL = UC_ARM_REG_R1
        REG_FAIL_VALUE = 0xaaaa

        START_ADDR = text_base | 1

        # declare assembler, disassembler, and emulator objects
        KEYSTONE = Ks(ks_arch, ks_mode)
        CAPSTONE = Cs(cs_arch, cs_mode)

    elif architecture == Architecture.MIPS_BE:
        # All of our configuration parameters
        ks_arch = KS_ARCH_MIPS
        ks_mode = KS_MODE_MIPS32 + KS_MODE_BIG_ENDIAN
        cs_arch = CS_ARCH_MIPS
        cs_mode = CS_MODE_MIPS32 + CS_MODE_BIG_ENDIAN
        EMU_ARCH = UC_ARCH_MIPS
        EMU_MODE = UC_MODE_MIPS32 + UC_MODE_BIG_ENDIAN

        REG_PC = UC_MIPS_REG_PC
        REG_SUCCESS = UC_MIPS_REG_T1
        REG_SUCCESS_VALUE = 0xdead
        REG_FAIL = UC_MIPS_REG_T2
        REG_FAIL_VALUE = 0xaaaa

        START_ADDR = text_base

        # declare assembler, disassembler, and emulator objects
        KEYSTONE = Ks(ks_arch, ks_mode)
        CAPSTONE = Cs(cs_arch, cs_mode)
    elif architecture == Architecture.ARM_32:
        # All of our configuration parameters
        ks_arch = KS_ARCH_ARM
        ks_mode = KS_MODE_ARM
        cs_arch = CS_ARCH_ARM
        cs_mode = CS_MODE_ARM
        EMU_ARCH = UC_ARCH_ARM
        EMU_MODE = UC_MODE_ARM

        REG_PC = UC_ARM_REG_PC
        REG_SUCCESS = UC_ARM_REG_R0
        REG_SUCCESS_VALUE = 0xdead
        REG_FAIL = UC_ARM_REG_R1
        REG_FAIL_VALUE = 0xaaaa

        START_ADDR = text_base

        # declare assembler, disassembler, and emulator objects
        KEYSTONE = Ks(ks_arch, ks_mode)
        CAPSTONE = Cs(cs_arch, cs_mode)
    else:
        logger.error("Architecture (%s) not supported." % architecture)
        return False

    return True


def glitch_code(test_code, architecture, bytes_to_trash_in, flip_type,
                pool_instances, enable_cache=True, force_invalid=False):
    """

    :param code:
    :param bytes_to_trash: the index into the bytes (these must be sequential!)
    :param architecture:
    :return:
    """
    global code_initial, bytes_to_trash, flip_operation, result_cache
    global force_invalid_ins

    force_invalid_ins = force_invalid

    if not enable_cache:
        result_cache = None

    bytes_to_trash = bytes_to_trash_in

    if not init_architecture(architecture):
        return {}

    # assemble the program
    code_initial, count = KEYSTONE.asm(test_code)

    code = ''.join(map(chr, code_initial))
    asm = list(CAPSTONE.disasm(code, len(code_initial)))
    if len(asm) == 0:
        logger.warning('>>> \tdisasm failure'.format(code.encode('hex')))
    for ins in asm:
        logger.info('>>> {}\t{} {}'.format(code.encode('hex'), ins.mnemonic,
                                           ins.op_str))

    flip_operation = flip_type
    flip_type_str = flip_type.name

    # Init our pool
    if pool_instances is None:
        pool = MyPool()
    else:
        pool = MyPool(pool_instances)

    initial_bytes = [code_initial[x] for x in bytes_to_trash]
    results = {'input': {'initial_bytes': initial_bytes,
                         'initial_code': code_initial},
               flip_type_str: {}
               }

    bit_list = range(len(bytes_to_trash) * 8)
    bit_count = range(len(
        bytes_to_trash) * 8 + 1)  # e.g., 0 to 16 bits to flip to include edge cases
    for number_of_bits_to_flip in bit_count:

        # Init our results
        results[flip_type_str][number_of_bits_to_flip] = {}

        # Let's fire off all of our threads in the pool
        logger.info("* Trying %d bit flips (%s)..." % (
            number_of_bits_to_flip, flip_type_str))
        rtn_vals = pool.imap(test_program,
                             itertools.combinations(bit_list,
                                                    number_of_bits_to_flip))

        # Aggregate all of our results
        for rtn in rtn_vals:
            if rtn not in results[flip_type_str][number_of_bits_to_flip]:
                results[flip_type_str][number_of_bits_to_flip][rtn] = 1
            else:
                results[flip_type_str][number_of_bits_to_flip][rtn] += 1

        # Let's print them at each iteration to have some idea of progress
        # pprint.pprint(results[flip_type_str][number_of_bits_to_flip])

        logger.info(
            pprint.pformat(results[flip_type_str][number_of_bits_to_flip]))
        # logger.info(pprint.pformat(results))

        result_cache = {}

    # Close our pool up
    pool.close()
    pool.terminate()
    pool.join()

    # Clear cache, it's unlikely to be useful in the next run
    if result_cache is not None:
        result_cache = {}

    return results
