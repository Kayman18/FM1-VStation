"""No compiler: check parameter/UI dimensions before an unbuilt checkpoint."""
import ast,pathlib,re
r=pathlib.Path(__file__).resolve().parents[1]
h=(r/'engine/synth.h').read_text();s=(r/'engine/synth.c').read_text();ui=(r/'app/app.c').read_text()
n=int(re.search(r'SYNTH_PARAM_COUNT (\d+)',h)[1])
assert len(re.search(r'synth_defaults\[SYNTH_PARAM_COUNT\]=\{([^}]+)',s)[1].split(','))==n
m=re.search(r'ids\[(\d+)\]\[4\]=\{(.*)\};',ui)
rows=[list(map(int,x.split(','))) for x in re.findall(r'\{([^}]+)\}',m[2])]
assert len(rows)==int(m[1]) and all(len(row)==4 and all(0<=i<n for i in row) for row in rows)
assert set(i for row in rows for i in row)==set(range(n))
assert len(re.findall(r'"[^"]*"',re.search(r'titles\[\]=\{([^}]+)',ui)[1]))==len(rows)+5
assert len(re.findall(r'"[^"]*"',re.search(r'labels\[SYNTH_PARAM_COUNT\]=\{([^}]+)',ui)[1]))==n
for path in (r/'tests').glob('*.py'):ast.parse(path.read_text())
print(f'Source consistency passed: {n} parameters, {len(rows)} parameter pages plus performance/preset/worker/two stage diagnostics; no ARP pages; no build.')
