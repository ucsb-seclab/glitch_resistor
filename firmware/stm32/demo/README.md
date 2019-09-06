# Demonstrations of GlitchResistor defenses
These are some quick gdb demos to demonstrate the efficacy of our defenses against single glitches.

Before any of the demos, be sure to run `st-util`

WARNING: Recompiling will likely result in different addresses, be sure to update the scripts appropriately

## Constant Diversification (ENUM/Return Value)
The following enums are defined in the firmware:
```C
enum valueRtn {
  GR_SUCCESS,
  GR_FAILURE,
  GR_UNKNOWN
};
```
These are used as return values in a critial function:
```C
int checkValue(int value) {
  if (value == 0) {
    return GR_SUCCESS;
  } else {
    return GR_FAILURE;
  }
}
```

If the return value were to be glitched to 0, it could result in the attacker executing protected code.
Our source re-writer will rewrite these values appropriately:
```C
enum valueRtn {
  GR_SUCCESS = 3889321827,
  GR_FAILURE = 3552161478,
  GR_UNKNOWN = 879491493
};
```
To see this in action run:
```bash
./glitch_return.sh
```

## Memory Integrity
The firmware also reads the tick from the hardware, and will run the success condition if the current tick is 0 (which it will almost never be).
The function below
```C
int checkTick() {
  if (gr_tick == 0) {
    return 1;
  } else {
    return 0;
  }
}
```
is checked during every interation of the loop
```C
if (checkValue(gr_tick) == GR_SUCCESS || checkTick()) 
```
Thus, corrupting the value in memory to be a zero could be a viable attack.
However, our integrity proteciton stores 2 values and will always check them to ensure that `value^integrity_value = 0xFFFFFFFF`.

To see an example of this detecting a glithcing in practice, run
```bash
./glitch_value
```
which will set the value `gr_tick` equal to 0 in memory right before the checks.


## Branch/Loop protection
The firmware will also check the `gr_tick` value during every iteration and will escape the loop if it's zero:
```C
while (last_tick != 0) {
```
However, GlitchResistor adds redudant checks to every branch condition in the firmware. Thus skipping an individual instruction is insufficient.
To see an example of this run
```bash
./glitch_main_loop
```
which will skip the `bne` instruction guarding the loop.
