#!/usr/bin/env python3
"""Run tests/run.sh unchanged in scope, caching identical engine object builds.

Only literal 'engine/*.c' compilations are expanded. Every distinct original
compiler-flag list gets its own temporary object directory. Test code and flags
are not shared across builds. Requires the same tools as tests/run.sh.
"""
from __future__ import annotations
import hashlib,pathlib,shlex,subprocess,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
def main():
 source=(ROOT/'tests/run.sh').read_text();variants={}
 with tempfile.TemporaryDirectory(prefix='vstation-test-objects-') as temp:
  temp=pathlib.Path(temp);lines=[]
  for line in source.splitlines():
   if line.startswith('cd "$(dirname "$0")/.."'):
    line='cd '+shlex.quote(str(ROOT))
   if line.startswith('gcc ') and 'engine/*.c' in line:
    args=shlex.split(line);first=next(i for i,a in enumerate(args) if a.endswith('.c'))
    flags=tuple(args[1:first])
    if flags not in variants:
     target=temp/hashlib.sha256(repr(flags).encode()).hexdigest()[:12];target.mkdir()
     objects=[]
     print('Compile engine variant:',shlex.join(flags),flush=True)
     for path in sorted((ROOT/'engine').glob('*.c')):
      obj=target/(path.stem+'.o')
      subprocess.run(['gcc',*flags,'-c',str(path),'-o',str(obj)],cwd=ROOT,check=True)
      objects.append(str(obj))
     variants[flags]=objects
    line=line.replace('engine/*.c',shlex.join(variants[flags]))
   lines.append(line)
  script=temp/'run.sh';script.write_text('\n'.join(lines)+'\n')
  subprocess.run(['bash',str(script)],cwd=ROOT,check=True)
if __name__=='__main__':main()
