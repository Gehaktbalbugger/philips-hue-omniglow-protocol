"""Compile without Arduino sketch preprocessing; requires native AVR tools and Arduino AVR core1.8.x."""
from pathlib import Path
import argparse,subprocess,concurrent.futures,os
parser=argparse.ArgumentParser()
parser.add_argument('--avr-bin',default=os.environ.get('AVR_TOOLCHAIN'),help='Directory containing avr-gcc,avr-g++,avr-objcopy,avr-objdump,avr-size')
parser.add_argument('--arduino-core',default=os.environ.get('ARDUINO_AVR_CORE'),help='Arduino AVR platform root containing cores/ and variants/')
args=parser.parse_args()
if not args.avr_bin or not args.arduino_core:parser.error('Supply --avr-bin and --arduino-core, or AVR_TOOLCHAIN and ARDUINO_AVR_CORE')
root=Path(__file__).resolve().parents[1]
tool=Path(args.avr_bin).expanduser().resolve();platform=Path(args.arduino_core).expanduser().resolve()
core=platform/'cores/arduino'
build=root/'build';build.mkdir(exist_ok=True)
flags=['-Os','-g','-ffunction-sections','-fdata-sections','-mmcu=atmega328p','-DF_CPU=16000000L','-DARDUINO=10819','-DARDUINO_AVR_UNO','-DARDUINO_ARCH_AVR','-I'+str(core),'-I'+str(platform/'variants/standard')]
sources=sorted([p for p in core.iterdir() if p.suffix in ('.c','.cpp','.S')])+[root/'firmware/omniglow_playground/omniglow_playground.ino']
def compile_one(src):
 obj=build/(src.name+'.o')
 cpp=src.suffix in ('.cpp','.ino')
 extra=['-std=gnu++11','-fpermissive','-fno-exceptions','-fno-threadsafe-statics','-x','c++'] if cpp else (['-x','assembler-with-cpp'] if src.suffix=='.S' else ['-std=gnu11'])
 subprocess.run([str(tool/('avr-g++' if cpp else 'avr-gcc')),*flags,*extra,'-c',str(src),'-o',str(obj)],check=True)
 return obj
with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool: objects=list(pool.map(compile_one,sources))
elf=build/'omniglow_playground.elf'
subprocess.run([str(tool/'avr-gcc'),'-Os','-mmcu=atmega328p','-Wl,--gc-sections','-o',str(elf),*[str(o) for o in objects],'-lm'],check=True)
subprocess.run([str(tool/'avr-objcopy'),'-O','ihex','-R','.eeprom',str(elf),str(build/'omniglow_playground.hex')],check=True)
subprocess.run([str(tool/'avr-size'),str(elf)],check=True)
with (build/'disassembly.txt').open('w') as f:subprocess.run([str(tool/'avr-objdump'),'-dz',str(elf)],stdout=f,check=True)
