#!/usr/bin/python
# Script to convert benchmark.conf to benchmark.json
# Hopefully we won't have to maintain benchmark.conf files any longer after
# we fully switch to JSON.

from configparser import ConfigParser as config_parser
from collections import defaultdict
import json
import sys
import re

def guess_old_style_used_threads(bench_name, num_threads):
    if bench_name == 'CPU Fibonacci':
        return 1
    if bench_name == 'CPU FFT':
        if num_threads >= 4:
            return 4
        if num_threads >= 2:
            return 2
        return 1
    if bench_name == 'CPU N-Queens':
        if num_threads >= 10:
            return 10
        if num_threads >= 5:
            return 5
        if num_threads >= 2:
            return 2
        return 1
    return num_threads

def first_truthy_or_none(*args):
    for arg in args:
        if arg:
            return arg
    return None

def nice_cpu_name(name,
        trademark_stuff = re.compile(r'\((r|tm|c)\)', re.I),
        vendor_intel = re.compile(r'Intel '),
        vendor_amd = re.compile(r'AMD '),
        integrated_processor = re.compile(r'(Integrated Processor|Processor)', re.I),
        frequency = re.compile(r'(\d+\.?\d+?\+?\s*[GH]z)'),
        core_count = re.compile(r'(Dual|Triple|Quad|Six|Eight|Octal|Twelve|Sixteen|\d+)[ -]Core'),
        radeon = re.compile(r'(with )?Radeon'),
        cpu_apu = re.compile(r' [CA]PU'),
        multiple_spaces = re.compile(r'\s\s+')):
    # Cleaned-up port of nice_name_x86_cpuid_model_string() from
    # deps/sysobj_early/src/nice_name.c

    name = name.strip()

    name = name.replace("@", "")

    # Move vendor to front
    match = first_truthy_or_none(vendor_intel.search(name), vendor_amd.search(name))
    if match:
        span = match.span()
        name = name[span[0]:span[1]] + name[:span[0]] + name[span[1]:]

    # Remove vendor-specific crud
    if name.startswith("AMD"):
        name = name.replace("Mobile Technology", "Mobile")
        name = radeon.sub("", name)
        if "COMPUTE CORES" in name:
            name, _ = name.split(',', 2)
    elif name.startswith("Cyrix"):
        name = name.replace("tm ", "")
    else:
        name = name.replace("Genuine Intel", "Intel")

    for regex in (cpu_apu, integrated_processor, frequency, core_count, trademark_stuff):
        name = regex.sub("", name)
    name = multiple_spaces.sub(" ", name)

    return name.strip()

def cleanup_old_style_cpu_name(name):
    if any(vendor in name for vendor in ('Intel', 'AMD', 'VIA', 'Cyrix')):
        return nice_cpu_name(name)
    return name

def cpu_config_val(config,
        mult_freq_re = re.compile(r'((?P<num_cpu>\d+)x\s+)?(?P<freq>\d+\.?\d*)')):
    # "2x 1400.00 MHz + 2x 800.00 MHz" -> 4400.0
    value = 0.0
    for _, num, val in mult_freq_re.findall(config):
        value += float(val) * (float(num) if num else 1.0)
    return value

def cpu_config_less(config1, config2):
    return cpu_config_val(config1) < cpu_config_val(config2)

def cpu_config_is_close(config1, config2):
    r0 = cpu_config_val(config1)
    r1 = cpu_config_val(config2)
    r1n = r1 * 0.9
    return r0 > r1n and r0 < r1

def guess_old_style_cpu_config(cpu_config, num_logical_cpus,
        frequency_re = re.compile(r'(?P<freq>\d+\.?\d+)\s*(?P<mult>[GH])z')):
    freq_mult = frequency_re.match(cpu_config)
    if not freq_mult:
        if num_logical_cpus > 1:
            return '%dx %s' % (num_logical_cpus, cpu_config)
        return cpu_config

    freq, mult = freq_mult.groups()
    freq = atof(freq)
    if mult[0] == 'G':
        freq *= 1000

    candidate_config = "%dx %.2f MHz" % (num_logical_cpus, freq)
    if cpu_config_less(cpu_config, candidate_config) and \
            not cpu_config_is_close(cpu_config, candidate_config):
        return candidate_config
    
    return cpu_config

def parse_old_style_cpu_info(cpu_config, cpu_name, bench_name,
        old_style_cpu_name_re = re.compile('\s*(?P<numcpu>\d+)?(x?\s+)?(?P<cpuname>.*)')):
    num_cpus = old_style_cpu_name_re.match(cpu_name)
    assert num_cpus
    
    groups = num_cpus.groupdict()
    num_logical_cpus = int(groups['numcpu']) if groups['numcpu'] else 1
    return {
        'NumThreads': num_logical_cpus,
        'NumCores': -1,
        'NumCpus': -1,
        'CpuName': cleanup_old_style_cpu_name(groups['cpuname']),
        'CpuConfig': guess_old_style_cpu_config(cpu_config, num_logical_cpus),
        'UsedThreads': guess_old_style_used_threads(bench_name, num_logical_cpus),
    }

def generate_machine_id(bench,
        invalid_chars_re = re.compile(r'[^A-Za-z0-9\(\);]')):
    mid = "%s;%s;%.2f" % (
        bench.get('Board', '(Unknown)'),
        bench['CpuName'],
        cpu_config_val(bench['CpuConfig']),
    )

    return invalid_chars_re.sub("_", mid)

def parse_new_style_bench_value(value):
    values = value.strip().split(';')

    if len(values) >= 3:
        result = float(values[0].replace(",", "."))
        elapsed_time = float(values[1].replace(",", "."))
        threads_used = int(values[2])
    else:
        result = 0
        elapsed_time = 0
        threads_used = 0

    benchmark_version = int(values[3]) if len(values) >= 4 else 1
    extra = values[4] if len(values) >= 5 and not '|' in values[4] else ''
    user_note = values[5] if len(values) >= 6 and not '|' in values[5] else ''

    return {
        'BenchmarkResult': result,
        'ElapsedTime': elapsed_time,
        'UsedThreads': threads_used,
        'BenchmarkVersion': benchmark_version,
        'ExtraInfo': extra,
        'UserNote': user_note,
    }

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: benchmark-conf-to-json.py benchmark.conf")
        sys.exit(1)

    cfg = config_parser(strict=False)
    cfg.optionxform = lambda opt: opt
    cfg.read(sys.argv[1])

    out = defaultdict(lambda: [])

    for section in cfg.sections():
        if section == 'param':
            continue

        for key in cfg[section]:
            values = cfg[section][key].split('|')

            if len(values) >= 10:
                bench = {
                    'MachineId': key,
                    'Board': values[2],
                    'CpuName': values[3],
                    'CpuDesc': values[4],
                    'CpuConfig': values[5].replace(",", "."),
                    'MemoryInKiB': int(values[6]),
                    'NumCpus': int(values[7]),
                    'NumCores': int(values[8]),
                    'NumThreads': int(values[9]),
                    'Legacy': False,
                }
                bench.update(parse_new_style_bench_value(values[0]))
                if len(values) >= 11:
                    bench['OpenGlRenderer'] = values[10]
                if len(values) >= 12:
                    bench['GpuDesc'] = values[11]
                if len(values) >= 13:
                    bench['MachineDataVersion'] = int(values[12])
                if len(values) >= 14:
                    bench['PointerBits'] = int(values[13])
                if len(values) >= 15:
                    bench['DataFromSuperUser'] = int(values[14]) != 0
                if len(values) >= 16:
                    bench['PhysicalMemoryInMiB'] = int(values[15])
                if len(values) >= 17:
                    bench['MemoryTypes'] = values[16]
            elif len(values) >= 2:
                bench = {
                    'BenchmarkResult': float(values[0]),
                    'Legacy': True,
                }

                cpu_info = parse_old_style_cpu_info(values[1].replace(",", "."), key, section)
                bench.update(cpu_info)
                bench['MachineId'] = generate_machine_id(bench)
            else:
                raise SyntaxError("unexpected value length: %d (%s)" % (len(values), values))

            out[section].append(bench)

    print(json.dumps(out))
