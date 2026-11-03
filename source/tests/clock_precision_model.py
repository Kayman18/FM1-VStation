"""Independent timestamp arithmetic model; does not execute the C implementation."""
import json, pathlib, random, statistics
rows=[]
for bpm in (40.125,79.999,123.456,180.375,239.25):
    rng=random.Random(123)
    times=[round(60000*i/bpm)+rng.randrange(-1,2) for i in range(101)]
    spans=[b-a for a,b in zip(times,times[1:])]
    raw=[];averaged=[]
    for i in range(3,len(spans)):
        window=spans[i-3:i+1];total=sum(window)
        measured=(240000000+total//2)//total/1000
        # Two endpoint jitters plus rounding bound the total-span error to 3 ms.
        assert abs(measured-bpm)<=bpm*3/(240000/bpm-3)+.001
        raw.append(60000/spans[i]);averaged.append(measured)
    assert statistics.pstdev(averaged)<statistics.pstdev(raw)
    rows.append(dict(bpm=bpm,raw_stddev=statistics.pstdev(raw),
                     averaged_stddev=statistics.pstdev(averaged),
                     max_error=max(abs(x-bpm) for x in averaged)))
result=dict(model_only=True,windows=485,results=rows,
            C_runtime_tests_executed=False,target_build_executed=False)
pathlib.Path(__file__).with_name('clock_precision_results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
